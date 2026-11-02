%% Wire-level behaviour that must match gpb, exercising the real NIF.
-module(ep_wire_parity_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

field(Name, Fnum, Rnum, Type, Occ) ->
    #field{name = Name, fnum = Fnum, rnum = Rnum, type = Type, occurrence = Occ, opts = []}.

varint(N) when N < 128 -> <<N>>;
varint(N) -> <<(16#80 bor (N band 16#7F)), (varint(N bsr 7))/binary>>.

%% Any non-zero varint decodes to true and the whole varint is consumed, so the
%% next tag is not misread (gpb decodes `Value =/= 0`).
bool_varint_parity_test() ->
    Defs = [{{msg, b}, [field(a, 1, 2, bool, optional)]}],
    ok = enif_protobuf:load_cache(Defs),
    Cases = [{<<8, 16#80, 1>>, true}, {<<8, 2>>, true}, {<<8, 0>>, false}],
    lists:foreach(
        fun({Wire, Expected}) ->
            ?assertEqual({b, Expected}, enif_protobuf:decode(Wire, b)),
            ?assertEqual({b, Expected}, gpb:decode_msg(Wire, b, Defs))
        end, Cases),
    ?assertEqual(gpb:encode_msg({b, true}, Defs), enif_protobuf:encode({b, true})),
    ?assertEqual(gpb:encode_msg({b, false}, Defs), enif_protobuf:encode({b, false})).

%% Zigzag values are masked to the declared width before decoding, like gpb.
sint_extremes_and_oversized_parity_test() ->
    Defs = [{{msg, s}, [field(a, 1, 2, sint32, optional), field(c, 2, 3, sint64, optional)]}],
    ok = enif_protobuf:load_cache(Defs),
    lists:foreach(
        fun(V) ->
            Bin = gpb:encode_msg({s, V, 0}, Defs),
            ?assertEqual(Bin, enif_protobuf:encode({s, V, 0})),
            ?assertEqual({s, V, 0}, enif_protobuf:decode(Bin, s))
        end, [-2147483648, -1, 0, 1, 2147483647]),
    %% varint 2^32+1: gpb masks to 32 bits first, giving -1 rather than INT32_MIN
    Oversized = <<8, 16#81, 16#80, 16#80, 16#80, 16#10>>,
    ?assertEqual(gpb:decode_msg(Oversized, s, Defs), enif_protobuf:decode(Oversized, s)).

%% proto3 omits the enum type default (value 0); the other values are sent even
%% when their name sorts first.
proto3_enum_default_parity_test() ->
    Defs = [{syntax, "proto3"},
            {proto3_msgs, [e]},
            {{enum, en}, [{zzz_zero, 0}, {aaa_one, 1}]},
            {{msg, e}, [field(v, 1, 2, {enum, en}, defaulty)]}],
    ok = enif_protobuf:load_cache(Defs),
    ?assertEqual(gpb:encode_msg({e, aaa_one}, Defs), enif_protobuf:encode({e, aaa_one})),
    ?assertEqual(<<>>, enif_protobuf:encode({e, zzz_zero})),
    ?assertEqual({e, zzz_zero}, enif_protobuf:decode(<<>>, e)).

%% Deeply nested messages grow the decoder's spot stack; the result must match
%% gpb instead of crashing or truncating.
deep_recursive_message_parity_test() ->
    Defs = [{{msg, r}, [field(a, 1, 2, {msg, r}, optional)]}],
    ok = enif_protobuf:load_cache(Defs),
    Wire = lists:foldl(fun(_, Acc) -> <<10, (varint(byte_size(Acc)))/binary, Acc/binary>> end,
                       <<>>, lists:seq(1, 200)),
    ?assertEqual(gpb:decode_msg(Wire, r, Defs), enif_protobuf:decode(Wire, r)).

%% A charlist longer than the encoder's initial buffer must grow it safely.
long_utf8_charlist_roundtrip_test() ->
    ok = enif_protobuf:set_opts([{with_utf8, true}, {string_as_list, true}]),
    ok = enif_protobuf:load_cache([{{msg, u}, [field(s, 1, 2, string, optional)]}]),
    Long = lists:duplicate(20000, 16#800),
    Bin = enif_protobuf:encode({u, Long}),
    ?assertEqual({u, Long}, enif_protobuf:decode(Bin, u)).
