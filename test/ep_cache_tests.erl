
%% Copyright (c) jg_513@163.com, https://github.com/jg513

-ifndef(gpb_compile_common_tests). %% non-shared code below vvvvvvvvv
-module(ep_cache_tests).

%-compile(export_all).
-import(enif_protobuf, [load_cache/1]).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").
-endif. %% gpb_compile_common_tests end of non-shared code ^^^^^^^^^^

load_all_test() ->
    ok = load_cache([
        {{msg, m1}, [
            {field, int32, 0, 1, int32, optional, [packed, {default, ok}]},
            {field, int64, 0, 1, int64, optional, [packed, {default, ok}]},
            {field, uint32, 0, 1, uint32, optional, [packed, {default, ok}]},
            {field, uint64, 0, 1, uint64, optional, [packed, {default, ok}]},
            {field, sint32, 0, 1, sint32, optional, [packed, {default, ok}]},
            {field, sint64, 0, 1, sint64, optional, [packed, {default, ok}]},
            {field, fixed32, 0, 1, fixed32, optional, [packed, {default, ok}]},
            {field, fixed64, 0, 1, fixed64, optional, [packed, {default, ok}]},
            {field, sfixed32, 0, 1, sfixed32, optional, [packed, {default, ok}]},
            {field, sfixed64, 0, 1, sfixed64, optional, [packed, {default, ok}]},
            {field, bool, 0, 1, bool, optional, [packed, {default, ok}]},
            {field, float, 0, 1, float, optional, [packed, {default, ok}]},
            {field, double, 0, 1, double, optional, [packed, {default, ok}]},
            {field, string, 0, 1, string, optional, [packed, {default, ok}]},
            {field, bytes, 0, 1, bytes, optional, [packed, {default, ok}]},
            {field, enum, 0, 1, {enum, e}, optional, [packed, {default, ok}]},
            {field, msg, 0, 1, {msg, m2}, optional, [packed, {default, ok}]},
            {field, map, 0, 1, {map, string, fixed32}, optional, [packed, {default, ok}]},
            {field, required, 0, 1, fixed32, required, [packed, {default, ok}]},
            {field, optional, 0, 1, fixed32, optional, [packed, {default, ok}]},
            {field, repeated, 0, 1, fixed32, repeated, [packed, {default, ok}]}
        ]},
        {{msg, m2}, [
            {field, int32, 0, 1, int32, optional, [packed, {default, ok}]}
        ]},
        {{enum, e}, [
            {v1, 100},
            {v2, -2},
            {v3, -2},
            {option, allow_alias, true}
        ]}
    ]).
