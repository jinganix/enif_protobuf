%% Wire-format edge cases exercised through the real NIF: malformed tags,
%% groups on non-group fields, reserved wire types and group nesting depth.
%% These back the hardening in ep_decoder.c; the C stub tests cover the same
%% paths without the Erlang API in front of them.
-module(ep_wire_edge_case_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-define(GROUP_DEPTH_LIMIT, 100).

optional_int32_defs() ->
    [{{msg, m1}, [
        #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []}
    ]}].

unknown_defs() ->
    [{{msg, m1}, [
        #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []},
        #field{name = '$unknown', fnum = undefined, rnum = 3, type = unknown, occurrence = repeated, opts = []}
    ]}].

two_int32_defs() ->
    [{{msg, m1}, [
        #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []},
        #field{name = b, fnum = 2, rnum = 3, type = int32, occurrence = optional, opts = []}
    ]}].

bytes_defs() ->
    [{{msg, m1}, [
        #field{name = a, fnum = 1, rnum = 2, type = bytes, occurrence = optional, opts = []}
    ]}].

nested_defs() ->
    [{{msg, m1}, [
        #field{name = a, fnum = 1, rnum = 2, type = {msg, m2}, occurrence = optional, opts = []}
    ]},
     {{msg, m2}, [
        #field{name = b, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []}
    ]}].

packed_defs() ->
    [{{msg, m1}, [
        #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = repeated, opts = [{packed, true}]}
    ]}].

start_group(N) ->
    <<((N bsl 3) bor 3)>>.

end_group(N) ->
    <<((N bsl 3) bor 4)>>.

nested_groups(Depth) ->
    iolist_to_binary([binary:copy(start_group(1), Depth), binary:copy(end_group(1), Depth)]).

decode(Defs, Bin) ->
    ok = ep_test_helpers:load_cache(Defs),
    enif_protobuf:decode(Bin, m1).

%% A group where a scalar is expected is skipped, the field keeps its default.
group_wire_type_on_scalar_field_is_skipped_test() ->
    %% start group 1, field 1 varint = 1, end group 1
    ?assertEqual({m1, undefined}, decode(optional_int32_defs(), <<16#0b, 16#08, 16#01, 16#0c>>)).

nested_group_on_scalar_field_is_skipped_test() ->
    %% start group 1, start+end group 2, end group 1
    ?assertEqual({m1, undefined}, decode(optional_int32_defs(), <<16#0b, 16#13, 16#14, 16#0c>>)).

unterminated_group_raises_test() ->
    ok = ep_test_helpers:load_cache(optional_int32_defs()),
    ?assertError(_, enif_protobuf:decode(<<16#0b, 16#08, 16#01>>, m1)).

group_nesting_at_limit_decodes_test() ->
    ?assertEqual({m1, undefined}, decode(optional_int32_defs(), nested_groups(?GROUP_DEPTH_LIMIT))).

group_nesting_over_limit_raises_instead_of_crashing_test() ->
    ok = ep_test_helpers:load_cache(optional_int32_defs()),
    ?assertError(_, enif_protobuf:decode(nested_groups(?GROUP_DEPTH_LIMIT + 1), m1)),
    %% unclosed deep nesting must fail too, not overflow the C stack
    ?assertError(_, enif_protobuf:decode(binary:copy(start_group(1), 40000), m1)).

field_number_over_32_bits_raises_test() ->
    ok = ep_test_helpers:load_cache(optional_int32_defs()),
    %% field number 2^32, wire type 0
    ?assertError(_, enif_protobuf:decode(<<16#80, 16#80, 16#80, 16#80, 16#80, 16#01, 16#01>>, m1)).

reserved_wire_types_raise_test() ->
    ok = ep_test_helpers:load_cache(optional_int32_defs()),
    ?assertError(_, enif_protobuf:decode(<<16#0e>>, m1)), % field 1, wire type 6
    ?assertError(_, enif_protobuf:decode(<<16#0f>>, m1)). % field 1, wire type 7

end_group_at_top_level_raises_test() ->
    ok = ep_test_helpers:load_cache(optional_int32_defs()),
    ?assertError(_, enif_protobuf:decode(<<16#0c>>, m1)).

%% {default, V} is accepted in field options but never applied on decode: epb
%% leaves missing optionals undefined, like gpb without defaults-for-omitted.
default_field_option_is_ignored_on_decode_test() ->
    Defs = [{{msg, m1}, [
        #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = [{default, 7}]}
    ]}],
    ?assertEqual({m1, undefined}, decode(Defs, <<>>)),
    ?assertEqual({m1, 3}, decode(Defs, <<16#08, 16#03>>)).

unknown_group_is_collected_test() ->
    %% start group 2, field 1 varint = 5, end group 2
    Bin = <<16#13, 16#08, 16#05, 16#14>>,
    ?assertEqual({m1, undefined, [{group, 2, [{varint, 1, 5}]}]}, decode(unknown_defs(), Bin)).

unknown_deep_group_nesting_over_limit_raises_test() ->
    ok = ep_test_helpers:load_cache(unknown_defs()),
    Deep = binary:copy(start_group(2), ?GROUP_DEPTH_LIMIT + 1),
    ?assertError(_, enif_protobuf:decode(Deep, m1)).

%% A tag is a varint32: it is read in at most five bytes and bits above bit 31
%% are dropped before the field number is derived. Length prefixes are a strict
%% varint32 instead: overflow and a sixth byte are rejected. Both rules match
%% the reference parsers (the same behaviour pbwire's conformance run pins).

tag_truncates_to_32_bits_test() ->
    %% Fifth byte 0x70 sets bit 34; it is dropped, so this is field 2's tag and
    %% 0x01 is that field's value.
    ?assertEqual(
        {m1, undefined, 1},
        decode(two_int32_defs(), <<16#90, 16#80, 16#80, 16#80, 16#70, 16#01>>)).

tag_wider_than_32_bits_raises_test() ->
    %% A sixth byte means the tag is not a varint32, even when it adds nothing.
    ?assertError(
        _,
        decode(
            optional_int32_defs(),
            <<16#88, 16#80, 16#80, 16#80, 16#80, 16#00, 16#01>>
        )).

tag_high_bits_become_an_unknown_field_test() ->
    %% Four bits in the fifth byte still fit in 32 bits: a valid tag for an
    %% undeclared field, so it is skipped.
    ?assertEqual(
        {m1, undefined, undefined},
        decode(two_int32_defs(), <<16#90, 16#80, 16#80, 16#80, 16#0f, 16#01>>)).

tag_non_minimal_bytes_are_accepted_test() ->
    ?assertEqual(
        {m1, undefined, 1},
        decode(two_int32_defs(), <<16#90, 16#80, 16#80, 16#80, 16#00, 16#01>>)).

length_prefix_wider_than_32_bits_raises_test() ->
    %% Unlike a tag, a fifth byte above 0x0f is rejected, not truncated.
    ?assertError(
        _, decode(bytes_defs(), <<16#0a, 16#80, 16#80, 16#80, 16#80, 16#70>>)
    ).

length_prefix_six_bytes_raises_test() ->
    ?assertError(
        _, decode(bytes_defs(), <<16#0a, 16#80, 16#80, 16#80, 16#80, 16#80, 16#00>>)
    ).

length_prefix_ten_bytes_raises_test() ->
    ?assertError(
        _,
        decode(
            bytes_defs(),
            <<16#0a, 16#80, 16#80, 16#80, 16#80, 16#80, 16#80, 16#80, 16#80, 16#80, 16#00>>
        )).

length_prefix_non_minimal_bytes_decode_test() ->
    %% 0x81 80 80 80 00 is five bytes that encode length 1: in range, accepted.
    ?assertEqual(
        {m1, <<"x">>},
        decode(bytes_defs(), <<16#0a, 16#81, 16#80, 16#80, 16#80, 16#00, $x>>)
    ).

unknown_length_prefix_wider_than_32_bits_raises_test() ->
    %% The same rule while skipping an undeclared length-delimited field.
    ?assertError(
        _, decode(optional_int32_defs(), <<16#1a, 16#80, 16#80, 16#80, 16#80, 16#70>>)
    ).

nested_length_prefix_wider_than_32_bits_raises_test() ->
    ?assertError(
        _, decode(nested_defs(), <<16#0a, 16#80, 16#80, 16#80, 16#80, 16#70>>)
    ).

packed_length_prefix_wider_than_32_bits_raises_test() ->
    ?assertError(
        _, decode(packed_defs(), <<16#0a, 16#80, 16#80, 16#80, 16#80, 16#70>>)
    ).

group_length_prefix_wider_than_32_bits_raises_test() ->
    %% Unknown group 3 holding a length-delimited field with an overflowing prefix.
    ?assertError(
        _,
        decode(
            optional_int32_defs(),
            <<16#1b, 16#0a, 16#80, 16#80, 16#80, 16#80, 16#70, 16#1c>>
        )
    ).

group_field_number_zero_raises_test() ->
    %% Field number 0 is invalid inside a skipped group too.
    ?assertError(_, decode(optional_int32_defs(), <<16#1b, 16#00, 16#1c>>)).
