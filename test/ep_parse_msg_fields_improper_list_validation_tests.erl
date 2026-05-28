-module(ep_parse_msg_fields_improper_list_validation_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

load_cache_msg_fields_improper_list_should_fail_test() ->
    ok = enif_protobuf:purge_cache(),
    Defs = [
        {{enum, e}, [{v1, 0}]},
        {{msg, m1}, [
            #field{name = a, fnum = 1, rnum = 2, type = {enum, e}, occurrence = required, opts = []},
            #field{name = b, fnum = 2, rnum = 3, type = int32, occurrence = optional, opts = []}
            | not_a_proper_list
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).
