-module(ep_issue_enum_decoding_tests).
-compile(export_all).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").


issue_enum_decoding_test() ->
    PAtom = pre_existing_atom,
    Defs = [{syntax, "proto3"},
        {proto3_msgs, [enum_with_default]},
        {{enum, en}, [{zzz, 0}, {PAtom, 1}, {nval, -1}]},
        {{msg, enum_with_default}, [#?gpb_field{name = a, fnum = 1, rnum = 2, type = {enum, en},
            occurrence = defaulty, opts = []}]}],
    ok = enif_protobuf:load_cache(Defs),
    {enum_with_default, zzz} = enif_protobuf:decode(<<>>, enum_with_default).

