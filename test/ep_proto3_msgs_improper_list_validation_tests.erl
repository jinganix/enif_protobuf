-module(ep_proto3_msgs_improper_list_validation_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

%% parse_node/5 marks proto3 messages by walking proto3_msgs with
%% enif_get_list_cell/3 but never checks the final tail is [].
load_cache_proto3_msgs_improper_list_should_fail_test() ->
    Defs = [
        {syntax, "proto2"},
        {proto3_msgs, [m1, m2 | not_a_proper_list]},
        {{msg, m1}, [
            #field{name = a, fnum = 1, rnum = 2, type = uint32, occurrence = defaulty, opts = []}
        ]},
        {{msg, m2}, [
            #field{name = b, fnum = 1, rnum = 2, type = uint32, occurrence = defaulty, opts = []}
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).
