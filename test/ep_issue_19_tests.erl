-module(ep_issue_19_tests).

-compile(export_all).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-record(m1, {a}).

% https://github.com/jinganix/enif_protobuf/issues/19
issue_19_test() ->
    Defs = [
        {{msg, m1}, [
            #field{name = a, fnum = 1, rnum = #m1.a, type = uint64, occurrence = required, opts = []}
        ]}
    ],
    Bin = <<8, 181, 207, 209, 168, 154, 47>>,
    enif_protobuf:load_cache(Defs),
    {m1, 1621972248501} = enif_protobuf:decode(Bin, m1),
    {m1, 1621972248501} = gpb:decode_msg(Bin, m1, Defs).
