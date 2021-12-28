
%% Copyright (c) jg_513@163.com, https://github.com/jg513

-module(ep_issue_27_tests).

-compile(export_all).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-record(default_uint32_msg, {
    count_num = 0 :: non_neg_integer() | undefined % = 1, optional, 32 bits
}).

-record(default_string_mix_msg, {
    count_num = 0 :: non_neg_integer() | undefined, % = 1, optional, 32 bits
    rname = [] :: unicode:chardata() | undefined, % = 2, optional
    py_id = 0 :: non_neg_integer() | undefined, % = 3, optional, 32 bits
    status_arr = [] :: [non_neg_integer()] | undefined, % = 4, repeated, 32 bits
    py_num = 0 :: non_neg_integer() | undefined, % = 5, optional, 32 bits
    gname = [] :: unicode:chardata() | undefined, % = 6, optional
    gnum = 0 :: non_neg_integer() | undefined % = 7, optional, 32 bits
}).

% https://github.com/jg513/enif_protobuf/issues/27
get_proto_defs() ->
    [
        {syntax, "proto3"},
        {{msg, default_uint32_msg}, [
            #field{name = count_num, fnum = 1, rnum = 2, type = uint32, occurrence = optional, opts = []}
        ]},
        {proto3_msgs, [default_uint32_msg]}
    ].

get_proto_defs_02() ->
    [
        {syntax, "proto3"},
        {{msg, default_string_mix_msg}, [
            #field{name = count_num, fnum = 1, rnum = 2, type = uint32, occurrence = optional, opts = []},
            #field{name = rname, fnum = 2, rnum = 3, type = string, occurrence = optional, opts = []},
            #field{name = py_id, fnum = 3, rnum = 4, type = uint32, occurrence = optional, opts = []},
            #field{name = status_arr, fnum = 4, rnum = 5, type = uint32, occurrence = repeated, opts = [packed]},
            #field{name = py_num, fnum = 5, rnum = 6, type = uint32, occurrence = optional, opts = []},
            #field{name = gname, fnum = 11, rnum = 7, type = string, occurrence = optional, opts = []},
            #field{name = gnum, fnum = 12, rnum = 8, type = uint32, occurrence = optional, opts = []}
        ]},
        {proto3_msgs, [default_string_mix_msg]}
    ].

issue_27_test() ->
    Defs = get_proto_defs_02() ++ get_proto_defs(),
    enif_protobuf:load_cache(Defs),
    Msg1 = #default_uint32_msg{},
    Bin1 = <<8, 0>>,
    Bin1 = gpb:encode_msg(Msg1, Defs),
    Bin1 = enif_protobuf:encode_msg(Msg1, Defs),

    Bin2 = <<8,99,18,4,116,101,115,116,24,0,40,0,90,0,96,0>>,
    Msg2 = #default_string_mix_msg{count_num = 99, rname = <<"test">>, py_id = 0, status_arr = [], gname = <<>>},
    Bin2 = gpb:encode_msg(Msg2, Defs),
    Bin2 = enif_protobuf:encode_msg(Msg2, Defs).
