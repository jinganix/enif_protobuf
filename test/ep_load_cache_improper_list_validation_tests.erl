-module(ep_load_cache_improper_list_validation_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

%% load_cache/1 walks defs with enif_get_list_cell/3 but never verifies
%% the final tail is []. An improper list silently drops defs after the
%% improper tail (gpb callers expect the full schema to load or fail).
load_cache_improper_defs_list_should_fail_test() ->
    Defs = [
        {{msg, m1}, [
            #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = required, opts = []}
        ]},
        {{msg, m2}, [
            #field{name = b, fnum = 1, rnum = 2, type = int32, occurrence = required, opts = []}
        ]}
        | not_a_proper_list
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).
