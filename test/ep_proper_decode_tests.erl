-module(ep_proper_decode_tests).

-ifdef(PROPER).
-include_lib("proper/include/proper.hrl").
-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-compile(export_all).

-record(m1, {a}).
-record(m2, {a, b}).
-record(m3, {a, b, c}).
-record(m4, {a, b, c, d}).

ep_proper_test() ->
    Functions = [F || {F, 0} <- ?MODULE:module_info(exports), F > 'prop_', F < 'prop`'],
    lists:foreach(fun(F) ->
        ?debugFmt("-> ~p", [F]),
        ?assert(proper:quickcheck(?MODULE:F(), [long_result, {to_file, user}]))
    end, Functions).

utf8char() ->
    union([
        integer(0, 36095),
        integer(57344, 65533),
        integer(65536, 1114111)
    ]).

utf8string() -> list(utf8char()).

ascii_string() -> list(choose(0, 127)).

uint32() -> choose(0, 4294967295).

sint32() -> choose(-2147483648, 2147483647).

uint64() -> choose(0, 18446744073709551615).

sint64() ->
    choose(-9223372036854775808, 9223372036854775807).

value() ->
    oneof([{real(), double}, {real(), float}, {nan, float},
        {infinity, float}, {'-infinity', float}, {nan, double},
        {infinity, double}, {'-infinity', double},
        {uint32(), uint32}, {uint64(), uint64},
        {sint32(), sint32}, {sint64(), sint64},
        {uint32(), fixed32}, {uint64(), fixed64},
        {sint32(), sfixed32}, {sint64(), sfixed64},
        {sint32(), int32}, {sint64(), int64}, {bool(), bool},
        {sint32(), enum}, {utf8string(), string},
        {binary(), bytes}]).

prop_decode_int32() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = int32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = int32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = int32, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = int32, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = int32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = int32, occurrence = optional, opts = []},
            #?gpb_field{name = d, fnum = 3, rnum = #m4.d, type = int32, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = c, fnum = 4, rnum = #m4.c, type = int32, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = sint32(),
            b = oneof([sint32(), undefined]),
            c = list(sint32()),
            d = list(sint32())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_int64() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = int64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = int64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = int64, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = int64, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = int64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = int64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = int64, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = int64, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = sint64(),
            b = oneof([sint64(), undefined]),
            c = list(sint64()),
            d = list(sint64())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_uint32() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = uint32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = uint32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = uint32, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = uint32, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = uint32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = uint32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = uint32, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = uint32, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = uint32(),
            b = oneof([uint32(), undefined]),
            c = list(uint32()),
            d = list(uint32())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_uint64() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = uint64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = uint64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = uint64, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = uint64, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = uint64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = uint64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = uint64, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = uint64, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = uint64(),
            b = oneof([uint64(), undefined]),
            c = list(uint64()),
            d = list(uint64())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_sint32() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = sint32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = sint32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = sint32, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = sint32, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = sint32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = sint32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = sint32, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = sint32, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = sint32(),
            b = oneof([sint32(), undefined]),
            c = list(sint32()),
            d = list(sint32())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_sint64() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = sint64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = sint64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = sint64, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = sint64, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = sint64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = sint64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = sint64, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = sint64, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = sint64(),
            b = oneof([sint64(), undefined]),
            c = list(sint64()),
            d = list(sint64())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_fixed32() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = fixed32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = fixed32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = fixed32, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = fixed32, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = fixed32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = fixed32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = fixed32, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = fixed32, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = uint32(),
            b = oneof([uint32(), undefined]),
            c = list(uint32()),
            d = list(uint32())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_fixed64() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = fixed64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = fixed64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = fixed64, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = fixed64, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = fixed64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = fixed64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = fixed64, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = fixed64, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = uint64(),
            b = oneof([uint64(), undefined]),
            c = list(uint64()),
            d = list(uint64())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_sfixed32() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = sfixed32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = sfixed32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = sfixed32, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = sfixed32, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = sfixed32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = sfixed32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = sfixed32, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = sfixed32, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = sint32(),
            b = oneof([sint32(), undefined]),
            c = list(sint32()),
            d = list(sint32())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_sfixed64() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = sfixed64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = sfixed64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = sfixed64, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = sfixed64, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = sfixed64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = sfixed64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = sfixed64, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = sfixed64, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = sint64(),
            b = oneof([sint64(), undefined]),
            c = list(sint64()),
            d = list(sint64())
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_float() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = float, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = float, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = float, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = float, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = float, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = float, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = float, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = float, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = oneof([infinity, '-infinity', nan, float(), integer()]),
            b = oneof([infinity, '-infinity', nan, float(), integer(), undefined]),
            c = list(oneof([infinity, '-infinity', nan, float(), integer()])),
            d = list(oneof([infinity, '-infinity', nan, float(), integer()]))
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_double() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = double, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = double, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = double, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = double, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = double, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = double, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = double, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = double, occurrence = repeated, opts = []}
        ]}
    ],
    ?FORALL(Message,
        #m4{
            a = oneof([infinity, '-infinity', nan, float(), integer()]),
            b = oneof([infinity, '-infinity', nan, float(), integer(), undefined]),
            c = list(oneof([infinity, '-infinity', nan, float(), integer()])),
            d = list(oneof([infinity, '-infinity', nan, float(), integer()]))
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_bool() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = bool, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = bool, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = bool, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = bool, occurrence = repeated, opts = [packed]}
        ]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = bool, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = bool, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = bool, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = bool, occurrence = repeated, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    ?FORALL(Message,
        #m4{
            a = oneof([true, false, 1, 0]),
            b = oneof([true, false, 1, 0, undefined]),
            c = list(oneof([true, false, 1, 0])),
            d = list(oneof([true, false, 1, 0]))
        },
        begin
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_ascii_string() ->
    Defs = [
        {{msg, m3}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m3.a, type = string, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m3.b, type = string, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m3.c, type = string, occurrence = repeated, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    ok = enif_protobuf:set_opts([{string_as_list, true}]),
    ?FORALL(Message,
        #m3{
            a = ascii_string(),
            b = oneof([ascii_string(), undefined]),
            c = list(ascii_string())
        },
        begin
            Bin = gpb:encode_msg(Message, Defs),
            enif_protobuf:decode(Bin, m3) =:= gpb:decode_msg(Bin, m3, Defs)
        end).

prop_decode_utf8_string() ->
    Defs = [
        {{msg, m3}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m3.a, type = string, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m3.b, type = string, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m3.c, type = string, occurrence = repeated, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    ok = enif_protobuf:set_opts([{with_utf8, true}, {string_as_list, true}]),
    ?FORALL(Message,
        #m3{
            a = utf8string(),
            b = oneof([utf8string(), undefined]),
            c = list(utf8string())
        },
        begin
            Bin = gpb:encode_msg(Message, Defs),
            enif_protobuf:decode(Bin, m3) =:= gpb:decode_msg(Bin, m3, Defs)
        end).

prop_encode_bytes() ->
    Defs = [
        {{msg, m3}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m3.a, type = bytes, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m3.b, type = bytes, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m3.c, type = bytes, occurrence = repeated, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    ?FORALL(Message,
        #m3{
            a = binary(),
            b = oneof([binary(), undefined]),
            c = list(binary())
        },
        begin
            Bin = gpb:encode_msg(Message, Defs),
            enif_protobuf:decode(Bin, m3) =:= gpb:decode_msg(Bin, m3, Defs)
        end).

prop_decode_map() ->
    Type = {map, string, fixed32},
    Defs = [
        {{msg, m3}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m3.a, type = Type, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m3.b, type = Type, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m3.c, type = Type, occurrence = repeated, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    ok = enif_protobuf:set_opts([{string_as_list, true}, {with_utf8, false}]),
    ?FORALL(Message,
        #m3{
            a = {ascii_string(), integer()},
            b = oneof([{ascii_string(), integer()}, undefined]),
            c = list({ascii_string(), integer()})
        },
        begin
            Bin = gpb:encode_msg(Message, Defs),
            Decoded = enif_protobuf:decode(Bin, m3),
            Decoded#m3{c = lists:reverse(Decoded#m3.c)} =:= gpb:decode_msg(Bin, m3, Defs)
        end).

prop_decode_enum() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = {enum, e}, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = {enum, e}, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = {enum, e}, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = {enum, e}, occurrence = repeated, opts = [packed]}
        ]},
        {{enum, e}, [{v1, 100}, {v2, 150}]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = {enum, e}, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = {enum, e}, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = {enum, e}, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = {enum, e}, occurrence = repeated, opts = []}
        ]},
        {{enum, e}, [{v1, 100}, {v2, 150}]}
    ],
    ?FORALL(Message,
        #m4{
            a = oneof([v1, v2, sint32()]),
            b = oneof([v1, v2, sint32(), undefined]),
            c = list(oneof([v1, v2, sint32()])),
            d = list(oneof([v1, v2, sint32()]))
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_enum_aliases() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = {enum, e}, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = {enum, e}, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = {enum, e}, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = {enum, e}, occurrence = repeated, opts = [packed]}
        ]},
        {{enum, e}, [{option, allow_alias, true}, {v1, 100}, {v2, 150}, {v3, 100}]}
    ],
    Defs2 = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = {enum, e}, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = {enum, e}, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = {enum, e}, occurrence = repeated, opts = [packed]},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = {enum, e}, occurrence = repeated, opts = []}
        ]},
        {{enum, e}, [{option, allow_alias, true}, {v1, 100}, {v2, 150}, {v3, 100}]}
    ],
    ?FORALL(Message,
        #m4{
            a = oneof([v1, v2, v3, sint32()]),
            b = oneof([v1, v2, v3, sint32(), undefined]),
            c = list(oneof([v1, v2, v3, sint32()])),
            d = list(oneof([v1, v2, v3, sint32()]))
        },
        begin
            ok = enif_protobuf:load_cache(Defs),
            Bin = gpb:encode_msg(Message, Defs),
            Ret = enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs),
            ok = enif_protobuf:load_cache(Defs2),
            Ret and (enif_protobuf:decode(Bin, m4) =:= gpb:decode_msg(Bin, m4, Defs2))
        end).

prop_decode_oneof() ->
    Defs = [
        {{msg, m1}, [
            #gpb_oneof{name = a, rnum = #m1.a, fields = [
                #?gpb_field{name = a1, fnum = 1, rnum = #m1.a, type = int32, occurrence = optional, opts = []},
                #?gpb_field{name = a2, fnum = 2, rnum = #m1.a, type = int32, occurrence = optional, opts = []}
            ]}]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    ?FORALL(Message,
        #m1{a = oneof([undefined, {a1, integer()}, {a2, integer()}])},
        begin
            Bin = gpb:encode_msg(Message, Defs),
            enif_protobuf:decode(Bin, m1) =:= gpb:decode_msg(Bin, m1, Defs)
        end).

prop_decode_sub_msg() ->
    Defs = [
        {{msg, m3}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m3.a, type = {msg, m2}, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m3.b, type = {msg, m2}, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m3.c, type = {msg, m2}, occurrence = repeated, opts = []}
        ]},
        {{msg, m2}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m2.a, type = uint32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m2.b, type = uint64, occurrence = required, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    ?FORALL(Message,
        #m3{
            a = #m2{a = uint32(), b = uint64()},
            b = oneof([#m2{a = uint32(), b = uint64()}, undefined]),
            c = list(#m2{a = uint32(), b = uint64()})
        },
        begin
            Bin = gpb:encode_msg(Message, Defs),
            enif_protobuf:decode(Bin, m3) =:= gpb:decode_msg(Bin, m3, Defs)
        end).
-endif.
