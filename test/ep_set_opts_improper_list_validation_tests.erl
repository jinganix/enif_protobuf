-module(ep_set_opts_improper_list_validation_tests).

-include_lib("eunit/include/eunit.hrl").

%% set_opts/1 walks the option list with enif_get_list_cell/3 but never
%% checks that the final tail is [] (same class of bug as load_cache/1).
set_opts_improper_list_should_fail_test() ->
    ?assertError(badarg, enif_protobuf:set_opts([{with_utf8, true} | not_proper])).
