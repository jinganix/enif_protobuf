
%% Copyright (c) jg_513@163.com, https://github.com/jg513

-module(ep_proper_tests).

-ifdef(PROPER).
-include_lib("proper/include/proper.hrl").
-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-compile(export_all).

-import(enif_protobuf, [load_cache/1, encode_msg/2, encode/1]).
-import(gpb, [decode_msg/3]).

-record(m1, {a}).
-record(m2, {a, b}).
-record(m3, {a, b, c}).
-record(m4, {a, b, c, d}).

ep_proper_test() ->
    Functions = [F || {F, 0} <- ?MODULE:module_info(exports), F > 'prop_', F < 'prop`'],
    lists:foreach(fun(F) ->
        ?assert(proper:quickcheck(?MODULE:F(), [long_result, {to_file, user}]))
    end, Functions).

utf8char() ->
    union([
        integer(0, 36095),
        integer(57344, 65533),
        integer(65536, 1114111)
    ]).

utf8string() -> list(utf8char()).

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

compare_messages(ExpectedMsg, Msg) ->
    lists:foldl(fun({E, D}, Acc) ->
        compare(E, D) andalso Acc
    end, true, lists:zip(tuple_to_list(ExpectedMsg), tuple_to_list(Msg))).

compare(A, A) -> true;
compare([A], B) -> compare(A, B);
compare(A, [B]) -> compare(A, B);
compare(A, B) when is_tuple(A), is_tuple(B) ->
    compare(tuple_to_list(A), tuple_to_list(B));
compare([A | RA], [B | RB]) ->
    compare(A, B) andalso compare(RA, RB);
compare(A, B) when is_float(A), is_float(B) ->
    <<A32:32/little-float>> = <<A:32/little-float>>,
    <<B32:32/little-float>> = <<B:32/little-float>>,
    if A =:= B -> true;
        A32 =:= B32 -> true;
        true -> false
    end;
compare(A, B) ->
    error_logger:error_msg("~p =/= ~p~n", [A, B]), false.

prop_encode_varint() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = int32, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = int32, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = int32, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = int32, occurrence = repeated, opts = [packed]}
        ]}
    ],
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m4{
            a = oneof([integer()]),
            b = oneof([integer(), undefined]),
            c = list(integer()),
            d = list(integer())
        },
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).

prop_encode_varint64() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = sint64, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = sint64, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = sint64, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = sint64, occurrence = repeated, opts = [packed]}
        ]}
    ],
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m4{
            a = oneof([sint64()]),
            b = oneof([sint64(), undefined]),
            c = list(sint64()),
            d = list(sint64())
        },
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).

prop_encode_float() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = float, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = float, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = float, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = float, occurrence = repeated, opts = [packed]}
        ]}
    ],
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m4{
            a = oneof([infinity, '-infinity', nan, float(), integer()]),
            b = oneof([infinity, '-infinity', nan, float(), integer(), undefined]),
            c = list(oneof([infinity, '-infinity', nan, float(), integer()])),
            d = list(oneof([infinity, '-infinity', nan, float(), integer()]))
        },
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).

prop_encode_double() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = double, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = double, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = double, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = double, occurrence = repeated, opts = [packed]}
        ]}
    ],
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m4{
            a = oneof([infinity, '-infinity', nan, float(), integer()]),
            b = oneof([infinity, '-infinity', nan, float(), integer(), undefined]),
            c = list(oneof([infinity, '-infinity', nan, float(), integer()])),
            d = list(oneof([infinity, '-infinity', nan, float(), integer()]))
        },
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).

prop_encode_bool() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = bool, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = bool, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = bool, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = bool, occurrence = repeated, opts = [packed]}
        ]}
    ],
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m4{
            a = oneof([true, false, 1, 0]),
            b = oneof([true, false, 1, 0, undefined]),
            c = list(oneof([true, false, 1, 0])),
            d = list(oneof([true, false, 1, 0]))
        },
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).

prop_encode_map() ->
    Type = {map, string, fixed32},
    Defs = [
        {{msg, m3}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m3.a, type = Type, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m3.b, type = Type, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m3.c, type = Type, occurrence = repeated, opts = []}
        ]}
    ],
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m3{
            a = {string(), integer()},
            b = oneof([{string(), integer()}, undefined]),
            c = list({string(), integer()})
        },
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).

prop_encode_oneof() ->
    Defs = [
        {{msg, m1}, [
            #gpb_oneof{name = a, rnum = #m1.a, fields = [
                #?gpb_field{name = a1, fnum = 1, rnum = #m1.a, type = int32, occurrence = optional, opts = []},
                #?gpb_field{name = a2, fnum = 2, rnum = #m1.a, type = int32, occurrence = optional, opts = []}
            ]}]}
    ],
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m1{a = oneof([undefined, {a1, integer()}, {a2, integer()}])},
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).

prop_encode_msg_with_sub_msg_field() ->
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
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m3{
            a = #m2{a = uint32(), b = uint64()},
            b = oneof([#m2{a = uint32(), b = uint64()}, undefined]),
            c = list(#m2{a = uint32(), b = uint64()})
        },
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).

prop_encode_msg_with_enum_field() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = {enum, e}, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = {enum, e}, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = {enum, e}, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = {enum, e}, occurrence = repeated, opts = [packed]}
        ]},
        {{enum, e}, [{v1, 100}, {v2, 150}]}
    ],
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m4{
            a = oneof([v1, v2, integer()]),
            b = oneof([v1, v2, integer(), undefined]),
            c = list(oneof([v1, v2, integer()])),
            d = list(oneof([v1, v2, integer()]))
        },
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).

prop_encode_msg_with_enum_aliases() ->
    Defs = [
        {{msg, m4}, [
            #?gpb_field{name = a, fnum = 1, rnum = #m4.a, type = {enum, e}, occurrence = required, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = #m4.b, type = {enum, e}, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = #m4.c, type = {enum, e}, occurrence = repeated, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = #m4.d, type = {enum, e}, occurrence = repeated, opts = [packed]}
        ]},
        {{enum, e}, [{option, allow_alias, true}, {v1, 100}, {v2, 150}, {v3, 100}]}
    ],
    ok = load_cache(Defs),
    ?FORALL(Message,
        #m4{
            a = oneof([v1, v2, v3, integer()]),
            b = oneof([v1, v2, v3, integer(), undefined]),
            c = list(oneof([v1, v2, v3, integer()])),
            d = list(oneof([v1, v2, v3, integer()]))
        },
        begin
            encode(Message) =:= gpb:encode_msg(Message, Defs)
        end).
-endif.
