
%% Copyright (c) jg_513@163.com, https://github.com/jg513

-module(ep_cache_tests).

-compile(export_all).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

load_all_test() ->
    ok = enif_protobuf:load_cache([
        {{msg, m1}, [
            {field, int32, 1, 1, int32, optional, [packed, {default, ok}]},
            {field, int64, 2, 2, int64, optional, [packed, {default, ok}]},
            {field, uint32, 3, 3, uint32, optional, [packed, {default, ok}]},
            {field, uint64, 4, 4, uint64, optional, [packed, {default, ok}]},
            {field, sint32, 5, 5, sint32, optional, [packed, {default, ok}]},
            {field, sint64, 6, 6, sint64, optional, [packed, {default, ok}]},
            {field, fixed32, 7, 7, fixed32, optional, [packed, {default, ok}]},
            {field, fixed64, 8, 8, fixed64, optional, [packed, {default, ok}]},
            {field, sfixed32, 9, 9, sfixed32, optional, [packed, {default, ok}]},
            {field, sfixed64, 10, 10, sfixed64, optional, [packed, {default, ok}]},
            {field, bool, 11, 11, bool, optional, [packed, {default, ok}]},
            {field, float, 12, 12, float, optional, [packed, {default, ok}]},
            {field, double, 13, 13, double, optional, [packed, {default, ok}]},
            {field, string, 14, 14, string, optional, [packed, {default, ok}]},
            {field, bytes, 15, 15, bytes, optional, [packed, {default, ok}]},
            {field, enum, 16, 16, {enum, e}, optional, [packed, {default, ok}]},
            {field, msg, 17, 17, {msg, m2}, optional, [packed, {default, ok}]},
            {field, map, 18, 18, {map, string, fixed32}, optional, [packed, {default, ok}]},
            {field, required, 19, 19, fixed32, required, [packed, {default, ok}]},
            {field, optional, 20, 20, fixed32, optional, [packed, {default, ok}]},
            {field, repeated, 21, 21, fixed32, repeated, [packed, {default, ok}]},
            {gpb_oneof, oneof, 22, [
                {field, int32, 22, 22, int32, optional, [packed, {default, ok}]},
                {field, int64, 23, 22, int64, optional, [packed, {default, ok}]}
            ]}
        ]},
        {{msg, m2}, [
            {field, int32, 0, 1, int32, optional, [packed, {default, ok}]}
        ]},
        {{msg, m3}, [
            {field, int32, 0, 1, int32, optional, [packed, {default, ok}]},
            {field, int64, 1, 2, int64, optional, [packed, {default, ok}]}
        ]},
        {{enum, e}, [
            {v1, 100},
            {v2, -2},
            {v3, -2},
            {option, allow_alias, true}
        ]}
    ]).
