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
