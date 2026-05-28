-module(ep_field_opts_improper_list_validation_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

%% parse_opts/3 walks field opts with enif_get_list_cell/3 but never checks
%% the final tail is []. load_cache/1 should reject defs with improper opts.
load_cache_field_opts_improper_list_should_fail_test() ->
    Defs = [
        {{msg, m1}, [
            #field{
                name = a,
                fnum = 1,
                rnum = 2,
                type = int32,
                occurrence = repeated,
                opts = [packed | not_a_proper_list]
            }
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).
