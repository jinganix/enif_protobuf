-module(ep_issue_11_tests).

-compile(export_all).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

% https://github.com/jinganix/enif_protobuf/issues/11
issue_11_test() ->
    Defs = [
        {{msg, pro_180_items_list}, [
            #field{name = id, fnum = 1, rnum = 2, type = uint32, occurrence = defaulty, opts = []},
            #field{name = num, fnum = 2, rnum = 3, type = uint32, occurrence = defaulty, opts = []},
            #field{name = text, fnum = 3, rnum = 4, type = string, occurrence = defaulty, opts = []}
        ]},
        {{msg, pro_180_all_prop}, [
            #field{name = key, fnum = 1, rnum = 2, type = uint32, occurrence = defaulty, opts = []},
            #field{name = value, fnum = 2, rnum = 3, type = uint32, occurrence = defaulty, opts = []},
            #field{name = incval, fnum = 3, rnum = 4, type = uint32, occurrence = defaulty, opts = []}
        ]},
        {proto3_msgs, [pro_180_all_prop, pro_180_items_list]}
    ],
    ok = enif_protobuf:load_cache(Defs).
