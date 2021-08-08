
%% Copyright (c) jg_513@163.com, https://github.com/jg513

-module(ep_tests).

-compile(export_all).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-record('Person', {
    name :: iolist(), % = 1
    id :: integer(), % = 2, 32 bits
    email :: iolist() | undefined % = 3
}).

-record(m1, {a}).

load_cache_test() ->
    ok = enif_protobuf:load_cache([
        {{msg, m1}, [
            {field, int32, 1, 1, int32, optional, [packed, {default, ok}]},
            {field, int64, 2, 2, int64, optional, [packed, {default, ok}]},
            {field, uint32, 3, 3, uint32, optional, [packed, {default, ok}]},
            {field, uint64, 4, 4, uint64, optional, [packed, {default, ok}]},
            {field, sint32, 5, 5, sint32, optional, [packed, {default, ok}]},
            {field, sint64, 6, 6, sint64, optional, [packed, {default, ok}]},
            {field, fixed32, 7, 7, fixed32, optional, [packed, {default, ok}]},
            {field, fixed64, 8, 8, fixed64, optional, [packed, {default, ok}]},
            {field, sfixed32, 9, 9, sfixed32, optional, [packed, {default, ok}]},
            {field, sfixed64, 10, 10, sfixed64, optional, [packed, {default, ok}]},
            {field, bool, 11, 11, bool, optional, [packed, {default, ok}]},
            {field, float, 12, 12, float, optional, [packed, {default, ok}]},
            {field, double, 13, 13, double, optional, [packed, {default, ok}]},
            {field, string, 14, 14, string, optional, [packed, {default, ok}]},
            {field, bytes, 15, 15, bytes, optional, [packed, {default, ok}]},
            {field, enum, 16, 16, {enum, e}, optional, [packed, {default, ok}]},
            {field, msg, 17, 17, {msg, m2}, optional, [packed, {default, ok}]},
            {field, map, 18, 18, {map, string, fixed32}, optional, [packed, {default, ok}]},
            {field, required, 19, 19, fixed32, required, [packed, {default, ok}]},
            {field, optional, 20, 20, fixed32, optional, [packed, {default, ok}]},
            {field, repeated, 21, 21, fixed32, repeated, [packed, {default, ok}]},
            {gpb_oneof, oneof, 22, [
                {field, int32, 22, 22, int32, optional, [packed, {default, ok}]},
                {field, int64, 23, 22, int64, optional, [packed, {default, ok}]}
            ]}
        ]},
        {{msg, m2}, [
            {field, int32, 0, 1, int32, optional, [packed, {default, ok}]}
        ]},
        {{msg, m3}, [
            {field, int32, 0, 1, int32, optional, [packed, {default, ok}]},
            {field, int64, 1, 2, int64, optional, [packed, {default, ok}]}
        ]},
        {{enum, e}, [
            {v1, 100},
            {v2, -2},
            {v3, -2},
            {option, allow_alias, true}
        ]}
    ]).

load_oneof_test() ->
    ok = enif_protobuf:load_cache([{{msg, 'Person'}, [
        #gpb_oneof{name = message_body, rnum = 2, fields = [
            #field{name = file_children, fnum = 3, rnum = 2, type = int32, occurrence = optional, opts = []},
            #field{name = xattr, fnum = 4, rnum = 2, type = bytes, occurrence = optional, opts = []}]},
        #field{name = proxy_session_id, fnum = 21, rnum = 3, type = bytes, occurrence = optional, opts = []}
    ]}]).

nested_oneof_test() ->
    ok = enif_protobuf:load_cache([
        {{msg, 'ChildLink'}, [
            #field{name = name, fnum = 2, rnum = 2, type = bytes, occurrence = required, opts = []}
        ]},
        {{msg, 'FileChildren'}, [
            #field{name = child_links, fnum = 1, rnum = 2, type = {msg, 'ChildLink'}, occurrence = repeated, opts = []}
        ]},
        {{msg, 'FuseResponse'}, [
            #gpb_oneof{name = fuse_response, rnum = 2, fields = [
                #field{name = file_children, fnum = 3, rnum = 2, type = {msg, 'FileChildren'}, occurrence = optional, opts = []},
                #field{name = xattr, fnum = 13, rnum = 2, type = bytes, occurrence = optional, opts = []}
            ]}
        ]},
        {{msg, 'ServerMessage'}, [
            #gpb_oneof{name = message_body, rnum = 2, fields = [
                #field{name = fuse_response, fnum = 15, rnum = 2, type = {msg, 'FuseResponse'}, occurrence = optional, opts = []}
            ]}
        ]}
    ]),
    Msg = {'ServerMessage',
        {fuse_response,
            {'FuseResponse',
                {file_children,
                    {'FileChildren', [{'ChildLink', <<"1">>}]}
                }
            }
        }
    },
    Bin = enif_protobuf:encode(Msg),
    Msg = enif_protobuf:decode(Bin, 'ServerMessage').

loading_cache() ->
    ok = enif_protobuf:load_cache([{{msg, 'Person'}, [
        #field{name = name, fnum = 1, rnum = 2, type = string, occurrence = required, opts = []},
        #field{name = id, fnum = 2, rnum = 3, type = int32, occurrence = required, opts = []},
        #field{name = email, fnum = 3, rnum = 4, type = string, occurrence = optional, opts = []}]}
    ]).

encoding() ->
    enif_protobuf:encode(#'Person'{name = "abc def", id = 345, email = "a@example.com"}).

loop_encoding(0) ->
    encoding(),
    ok;
loop_encoding(N) ->
    case rand:uniform() > 0.99 of
        true ->
            loading_cache();
        _ ->
            ignore
    end,
    encoding(),
    loop_encoding(N - 1).

smp_cache_encoding_test_() ->
    rand:uniform(),
    Processors = erlang:system_info(logical_processors),
    N = 500000,
    {spawn, {timeout, 60, ?_test(begin
        [spawn(fun() ->
            loading_cache(),
            loop_encoding(N)
        end) || _N <- lists:seq(1, Processors * 2)],
        loading_cache(),
        loop_encoding(N + 1000000)
    end)}}.

decoding() ->
    Bin = <<10, 7, 97, 98, 99, 32, 100, 101, 102, 16, 217, 2, 26, 13, 97, 64, 101, 120, 97, 109, 112, 108, 101, 46, 99, 111, 109>>,
    enif_protobuf:decode(Bin, 'Person').

loop_decoding(0) ->
    decoding(),
    ok;
loop_decoding(N) ->
    case rand:uniform() > 0.99 of
        true ->
            loading_cache();
        _ ->
            ignore
    end,
    decoding(),
    loop_decoding(N - 1).

smp_cache_decoding_test_() ->
    rand:uniform(),
    Processors = erlang:system_info(logical_processors),
    N = 500000,
    {spawn, {timeout, 60, ?_test(begin
        [spawn(fun() ->
            loading_cache(),
            loop_decoding(N)
        end) || _N <- lists:seq(1, Processors * 2)],
        loading_cache(),
        loop_decoding(N + 1000000)
    end)}}.
