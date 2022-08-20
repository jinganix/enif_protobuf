%% Copyright (c) jg_513@163.com, https://github.com/jg513

-module(ep_issue_31_tests).

-compile(export_all).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").


% https://github.com/jg513/enif_protobuf/issues/31
issue_31_test() ->
    Defs = [
        {{enum, very_long}, defs_enum_gen(100000, [])}
    ],

    enif_protobuf:purge_cache(),
    enif_protobuf:load_cache(Defs).

defs_enum_gen(0, OptList) -> OptList;
defs_enum_gen(N, CurOptList) when N > 0 ->
    defs_enum_gen(N-1, [{list_to_atom("opt" ++ integer_to_list(N)), N} | CurOptList]).

