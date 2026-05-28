-module(ep_repeated_msg_arity_validation_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

repeated_msg_element_extra_field_should_fail_test() ->
    Defs = [
        {{msg, child}, [
            #field{name = v, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []}
        ]},
        {{msg, parent}, [
            #field{name = children, fnum = 1, rnum = 2, type = {msg, child}, occurrence = repeated, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    Msg = {parent, [{child, 1, ignored_extra_field}]},
    ?assertError(_, enif_protobuf:encode(Msg)).

repeated_msg_element_missing_field_should_fail_test() ->
    Defs = [
        {{msg, child}, [
            #field{name = v, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []}
        ]},
        {{msg, parent}, [
            #field{name = children, fnum = 1, rnum = 2, type = {msg, child}, occurrence = repeated, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    Msg = {parent, [{child}]},
    ?assertError(_, enif_protobuf:encode(Msg)).
