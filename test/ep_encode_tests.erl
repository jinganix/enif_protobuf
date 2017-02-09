
%% Copyright (c) jg_513@163.com, https://github.com/jg513

-module(ep_encode_tests).

-import(enif_protobuf, [encode_msg/2]).
-import(gpb, [decode_msg/3]).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

proto3_type_default_values_never_serialized_test() ->
    Defs = [
        {syntax, "proto3"},
        {proto3_msgs, [m, s]},
        {{enum, e}, [{e0, 0}, {e1, 1}]},
        {{msg, s}, [
            #?gpb_field{name = a, fnum = 1, rnum = 2, type = string, occurrence = optional, opts = []}
        ]},
        {{msg, m}, [
            #?gpb_field{name = a, fnum = 1, rnum = 2, type = string, occurrence = optional, opts = []},
            #?gpb_field{name = b, fnum = 2, rnum = 3, type = bytes, occurrence = optional, opts = []},
            #?gpb_field{name = c, fnum = 3, rnum = 4, type = bool, occurrence = optional, opts = []},
            #?gpb_field{name = d, fnum = 4, rnum = 5, type = uint32, occurrence = optional, opts = []},
            #?gpb_field{name = e, fnum = 5, rnum = 6, type = float, occurrence = optional, opts = []},
            #?gpb_field{name = f, fnum = 6, rnum = 7, type = double, occurrence = optional, opts = []},
            #?gpb_field{name = g, fnum = 7, rnum = 8, type = {msg, s}, occurrence = optional, opts = []},
            #gpb_oneof{name = h, rnum = 9, fields = [
                #?gpb_field{name = ha, fnum = 8, rnum = 9, type = uint32, occurrence = optional, opts = []}
            ]}
        ]}
    ],
    Msg = {
        m,
        "", <<>>, false, % string, bytes, bool
        0,               % numeric types
        0.0, 0.0,        % float, double
        undefined,       % submsg
        undefined        % oneof
    },
    <<>> = encode_msg(Msg, Defs),
    Msg = decode_msg(<<>>, m, Defs).

