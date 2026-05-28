-module(ep_parse_oneof_fields_improper_list_validation_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

%% parse_oneof_fields/3 walks gpb_oneof member lists with enif_get_list_cell/3;
%% the final tail must be [] (see fill_oneof_field/3 and require_proper_list/2).
load_cache_oneof_fields_improper_list_should_fail_test() ->
    ok = enif_protobuf:purge_cache(),
    Defs = [
        {{msg, m1}, [
            #gpb_oneof{
                name = a,
                rnum = 2,
                fields = [
                    #field{name = a1, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []}
                    | not_a_proper_list
                ]
            }
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).

load_cache_oneof_tuple_with_wrong_arity_should_fail_test() ->
    ok = enif_protobuf:purge_cache(),
    Defs = [
        {{msg, m1}, [
            {gpb_oneof, a, 2}
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).

load_cache_oneof_non_atom_name_should_fail_test() ->
    ok = enif_protobuf:purge_cache(),
    Defs = [
        {{msg, m1}, [
            #gpb_oneof{
                name = 123,
                rnum = 2,
                fields = [
                    #field{name = a1, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []}
                ]
            }
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).

load_cache_oneof_tuple_with_extra_elements_should_fail_test() ->
    ok = enif_protobuf:purge_cache(),
    Defs = [
        {{msg, m1}, [
            {gpb_oneof, a, 2, [
                #field{name = a1, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []}
            ], [], unexpected_extra}
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).

load_cache_oneof_invalid_opts_should_fail_test() ->
    ok = enif_protobuf:purge_cache(),
    Defs = [
        {{msg, m1}, [
            {gpb_oneof, a, 2, [
                #field{name = a1, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []}
            ], invalid_opts_type}
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).
