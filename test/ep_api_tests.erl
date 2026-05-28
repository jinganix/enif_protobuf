-module(ep_api_tests).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

encode_without_cache_test() ->
    ?assertError(cache_not_exists, enif_protobuf:encode({m1, 1})).

decode_without_cache_test() ->
    ?assertError(cache_not_exists, enif_protobuf:decode(<<>>, m1)).

encode_badarg_test() ->
    ok = ep_test_helpers:load_cache(ep_test_helpers:simple_int32_defs()),
    ?assertError(badarg, enif_protobuf:encode(not_a_tuple)).

decode_badarg_test() ->
    ok = ep_test_helpers:load_cache(ep_test_helpers:simple_int32_defs()),
    ?assertError(badarg, enif_protobuf:decode(not_a_binary, m1)),
    ?assertError(not_an_atom, enif_protobuf:decode(<<>>, not_an_atom)).

unknown_message_name_test() ->
    ok = ep_test_helpers:load_cache(ep_test_helpers:simple_int32_defs()),
    ?assertError(unknown_msg, enif_protobuf:decode(<<8, 1>>, unknown_msg)).

encode_msg_test() ->
    Defs = ep_test_helpers:person_defs(),
    Msg = {'Person', "abc def", 345, "a@example.com"},
    Bin = enif_protobuf:encode_msg(Msg, Defs),
    ?assert(is_binary(Bin)),
    Msg2 = enif_protobuf:decode_msg(Bin, 'Person', Defs),
    ?assertEqual(Msg, Msg2).

decode_msg_test() ->
    Defs = ep_test_helpers:person_defs(),
    Bin = <<10, 7, 97, 98, 99, 32, 100, 101, 102, 16, 217, 2, 26, 13, 97, 64,
        101, 120, 97, 109, 112, 108, 101, 46, 99, 111, 109>>,
    {'Person', Name, Id, Email} = enif_protobuf:decode_msg(Bin, 'Person', Defs),
    ?assertEqual("abc def", Name),
    ?assertEqual(345, Id),
    ?assertEqual("a@example.com", Email).

purge_cache_test() ->
    Defs = ep_test_helpers:simple_int32_defs(),
    ok = ep_test_helpers:load_cache(Defs),
    _ = enif_protobuf:encode({m1, 1}),
    ok = enif_protobuf:purge_cache(),
    ?assertError(cache_not_exists, enif_protobuf:encode({m1, 1})).

reload_cache_after_purge_test() ->
    Defs = ep_test_helpers:simple_int32_defs(),
    ok = ep_test_helpers:load_cache(Defs),
    Bin1 = enif_protobuf:encode({m1, 1}),
    ok = enif_protobuf:purge_cache(),
    ok = ep_test_helpers:load_cache(Defs),
    Bin2 = enif_protobuf:encode({m1, 1}),
    ?assertEqual(Bin1, Bin2).

set_opts_test() ->
    Defs = [{{msg, m1}, [
        #field{name = a, fnum = 1, rnum = 2, type = string, occurrence = required, opts = []}
    ]}],
    Bin = <<10, 3, 97, 98, 99>>,
    ok = enif_protobuf:set_opts([{string_as_list, true}]),
    ok = enif_protobuf:load_cache(Defs),
    {m1, "abc"} = enif_protobuf:decode(Bin, m1),
    ok = enif_protobuf:set_opts([{string_as_list, false}]),
    {m1, <<"abc">>} = enif_protobuf:decode(Bin, m1).

set_opts_utf8_test() ->
    Defs = ep_test_helpers:person_defs(),
    Msg = {'Person', "你好", 1, undefined},
    NoUtf8Opts = [{with_utf8, false}, {string_as_list, true}],
    ?assertError(_, ep_test_helpers:encode_msg(Msg, Defs, NoUtf8Opts)),
    Utf8Opts = [{with_utf8, true}, {string_as_list, true}],
    Bin = ep_test_helpers:encode_msg(Msg, Defs, Utf8Opts),
    {'Person', "你好", 1, undefined} = ep_test_helpers:decode_msg(Bin, 'Person', Defs, Utf8Opts).

set_opts_invalid_option_test() ->
    ?assertError(badarg, enif_protobuf:set_opts([{unknown_opt, true}])),
    ?assertError(badarg, enif_protobuf:set_opts([{with_utf8, not_bool}])).

load_cache_badarg_test() ->
    ?assertError(not_a_list, enif_protobuf:load_cache(not_a_list)).

roundtrip_test() ->
    Defs = ep_test_helpers:person_defs(),
    Msg = {'Person', "abc def", 345, "a@example.com"},
    ?assertEqual(Msg, ep_test_helpers:roundtrip(Msg, Defs)).
