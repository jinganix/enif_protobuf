-module(ep_issue_default_option_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

% Optional fields with a custom {default, _} option must decode as undefined
% when absent on the wire, matching gpb (not the declared default value).
optional_field_custom_default_decode_test() ->
    Defs = [
        {{msg, m1}, [
            #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = [{default, 42}]}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    {m1, undefined} = enif_protobuf:decode(<<>>, m1),
    {m1, undefined} = gpb:decode_msg(<<>>, m1, Defs).

optional_field_custom_default_encode_test() ->
    Defs = [
        {{msg, m1}, [
            #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = optional, opts = [{default, 42}]}
        ]}
    ],
    ok = enif_protobuf:load_cache(Defs),
    <<>> = enif_protobuf:encode({m1, undefined}),
    <<8, 42>> = enif_protobuf:encode({m1, 42}),
    GpbBin = gpb:encode_msg({m1, 43}, Defs),
    GpbBin = enif_protobuf:encode({m1, 43}),
    {m1, 43} = enif_protobuf:decode(GpbBin, m1).
