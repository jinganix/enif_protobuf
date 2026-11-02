%% Message-level cases ported from gpb's test/gpb_compile_tests.erl, i.e. tests
%% that only make sense against the gpb compiler. Only the wire behaviour is
%% kept here: every case is checked against gpb itself, so epb and gpb must
%% agree byte for byte.
-module(ep_gpb_compile_parity_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-import(ep_test_helpers, [decode_msg/3, encode_msg/2]).

f(Name, Fnum, Rnum, Type, Occ, Opts) ->
    #?gpb_field{name = Name, fnum = Fnum, rnum = Rnum, type = Type,
        occurrence = Occ, opts = Opts}.

%% gpb accepts non-minimally encoded varints inside a message.
decodes_overly_long_varints_test() ->
    Defs = [{{msg, m1}, [f(a, 1, 2, int32, required, [])]}],
    Canonical = <<8, 54>>,
    OverlyLong = <<8, (128 + 54), 128, 128, 0>>,
    ?assertEqual({m1, 54}, decode_msg(Canonical, m1, Defs)),
    ?assertEqual({m1, 54}, decode_msg(OverlyLong, m1, Defs)),
    ?assertEqual(gpb:decode_msg(OverlyLong, m1, Defs), decode_msg(OverlyLong, m1, Defs)).

%% A group with no fields still encodes as an empty start/end tag pair.
empty_group_test() ->
    Defs = [{{msg, 'Quz'}, [f(e, 11, 2, {group, 'Quz.E'}, optional, [])]},
        {{group, 'Quz.E'}, []}],
    Bin = encode_msg({'Quz', {'Quz.E'}}, Defs),
    ?assertEqual(<<91, 92>>, Bin),
    ?assertEqual({'Quz', {'Quz.E'}}, decode_msg(Bin, 'Quz', Defs)).

%% A bytes field also accepts plain iodata, not only a binary.
list_as_bytes_indata_test() ->
    Defs = [{{msg, m1}, [f(f1, 1, 2, bytes, required, [])]}],
    Bin = encode_msg({m1, [1, 2, 3, 4]}, Defs),
    ?assertEqual(<<10, 4, 1, 2, 3, 4>>, Bin),
    ?assertEqual({m1, <<1, 2, 3, 4>>}, decode_msg(Bin, m1, Defs)).

%% packed is only meaningful for packable types. gpb_compile warns about it and
%% its generated code encodes such a field unpacked (gpb:encode_msg/2 with a raw
%% defs list still packs it, so the two gpb paths disagree); epb follows the
%% generated code instead of failing.
ignores_packed_for_nonpackable_repeated_on_encoding_test() ->
    Defs = [{{msg, m1}, [f(s1, 1, 2, string, repeated, [packed])]}],
    Msg = {m1, ["abc", "def"]},
    Bin = <<10, 3, "abc", 10, 3, "def">>,
    ?assertEqual(Bin, encode_msg(Msg, Defs)),
    ?assertEqual(Msg, decode_msg(Bin, m1, Defs)).

%% Same as above with the defs exactly as gpb_compile emits them for a proto3
%% file (`repeated string strs = 2 [packed]` keeps the packed option).
packed_option_from_gpb_generated_defs_test() ->
    Defs = [{syntax, "proto3"},
        {proto3_msgs, ['P3']},
        {{msg, 'P3'}, [
            f(nums, 1, 2, int32, repeated, [packed]),
            f(strs, 2, 3, string, repeated, [packed]),
            f(plain, 3, 4, int32, optional, [])
        ]}],
    Msg = {'P3', [1, 2], ["a"], 0},
    Bin = encode_msg(Msg, Defs),
    %% repeated int32 keeps the packed block, repeated string falls back to one
    %% tag per element like gpb_compile's generated encoder does.
    ?assertEqual(<<10, 2, 1, 2, 18, 1, "a", 24, 0>>, Bin),
    ?assertEqual(gpb:decode_msg(Bin, 'P3', Defs), decode_msg(Bin, 'P3', Defs)).

%% A declared default is never applied on decode: proto3 type defaults come out
%% as the type default, proto2 leaves the field undefined.
type_default_option_is_ignored_for_proto3_test() ->
    Defs = [{syntax, "proto3"},
        {proto3_msgs, [m]},
        {{msg, m}, [f(f1, 1, 2, uint32, defaulty, [])]}],
    ?assertEqual({m, 0}, decode_msg(<<>>, m, Defs)),
    ?assertEqual(gpb:decode_msg(<<>>, m, Defs), decode_msg(<<>>, m, Defs)).
