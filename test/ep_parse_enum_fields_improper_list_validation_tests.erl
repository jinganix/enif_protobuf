-module(ep_parse_enum_fields_improper_list_validation_tests).

-include_lib("eunit/include/eunit.hrl").

load_cache_enum_values_improper_list_should_fail_test() ->
    ok = enif_protobuf:purge_cache(),
    Defs = [
        {{enum, e}, [
            {v1, 0},
            {v2, 1}
            | not_a_proper_list
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).

load_cache_enum_invalid_tuple_after_allow_alias_should_fail_test() ->
    ok = enif_protobuf:purge_cache(),
    Defs = [
        {{enum, e}, [
            {option, allow_alias, true},
            {v1, 0},
            {unexpected, tuple, shape}
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).

load_cache_enum_duplicate_name_should_fail_test() ->
    ok = enif_protobuf:purge_cache(),
    Defs = [
        {{enum, e}, [
            {v1, 0},
            {v1, 1}
        ]}
    ],
    ?assertError(_, enif_protobuf:load_cache(Defs)).
