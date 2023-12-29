-module(ep_issue_29_tests).

-compile(export_all).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-record(m1, {a}).

% https://github.com/jinganix/enif_protobuf/issues/29
issue_29_test() ->
    Defs = [
        {{msg, a_message}, [
            {field, non_trivial_map, 1, 2, {map, string, {msg, non_trivial_item}}, repeated, []}
        ]},
        {{msg, non_trivial_item}, [
            {field, item, 1, 2, int64, optional, []}
        ]}
    ],
    Bin = <<10,7,10,1,97,18,2,8,1>>,
    Bin = gpb:encode_msg({a_message, [{"a", {non_trivial_item, 1}}]}, Defs),
    Bin = enif_protobuf:encode_msg({a_message, [{"a", {non_trivial_item, 1}}]}, Defs).
