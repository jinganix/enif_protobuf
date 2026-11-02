-module(ep_improper_list_validation_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

%% encode/1 must reject improper lists for repeated fields (gpb does).
%% ep_encoder.c only walks cells via enif_get_list_cell/3 and never
%% checks that the final tail is [].
repeated_int32_improper_list_should_fail_test() ->
    Defs = [
        {{msg, m1}, [
            #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = repeated, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    Msg = {m1, [1, 2 | not_proper]},
    ?assertError(_, enif_protobuf:encode(Msg)).

%% encode/1 must reject a non-list value for the '$unknown' field instead of
%% silently dropping it (gpb raises too).
unknown_fields_not_a_list_should_fail_test() ->
    Defs = [
        {{msg, m1}, [
            #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = []},
            #field{name = '$unknown', fnum = undefined, rnum = 3, type = unknown, occurrence = repeated, opts = []}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    ?assertError(_, enif_protobuf:encode({m1, 1, not_a_list})),
    ?assertError(_, enif_protobuf:encode({m1, 1, [{varint, 2, 3} | improper_tail]})).
