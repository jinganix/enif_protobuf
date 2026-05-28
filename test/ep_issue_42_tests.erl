-module(ep_issue_42_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

% https://github.com/jinganix/enif_protobuf/issues/42
issue_42_test_() ->
    {timeout, 5, fun issue_42_self_recursive_load_cache_test/0}.

issue_42_self_recursive_load_cache_test() ->
    Defs = [
        {{msg, p_v}, [
            #field{name = int_v, fnum = 1, rnum = 2, type = int64, occurrence = optional, opts = []},
            #field{name = str_v, fnum = 2, rnum = 3, type = string, occurrence = optional, opts = []},
            #field{name = float_v, fnum = 3, rnum = 4, type = float, occurrence = optional, opts = []},
            #field{name = list, fnum = 4, rnum = 5, type = {msg, p_v}, occurrence = repeated, opts = []}
        ]}
    ],
    Msg = {p_v, 1001, "root", 1.5, [
        {p_v, 1002, "child_1", 2.5, []},
        {p_v, 1003, "child_2", 3.5, [
            {p_v, 1004, "leaf", 4.5, []}
        ]}
    ]},
    ok = enif_protobuf:set_opts([{string_as_list, true}, {with_utf8, true}]),
    ok = enif_protobuf:load_cache(Defs),
    Bin = gpb:encode_msg(Msg, Defs),
    Bin = enif_protobuf:encode_msg(Msg, Defs),
    Msg = enif_protobuf:decode_msg(Bin, p_v, Defs),
    Msg = gpb:decode_msg(Bin, p_v, Defs).
