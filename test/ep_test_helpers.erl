-module(ep_test_helpers).

-export([
    default_opts/0,
    set_opts/1,
    load_cache/1,
    load_cache/2,
    purge_cache/0,
    encode_msg/2,
    encode_msg/3,
    decode_msg/3,
    decode_msg/4,
    roundtrip/2,
    roundtrip/3,
    person_defs/0,
    simple_int32_defs/0
]).

-include_lib("gpb/include/gpb.hrl").

default_opts() ->
    [{string_as_list, true}, {with_utf8, true}].

set_opts(Opts) ->
    enif_protobuf:set_opts(Opts).

load_cache(Defs) ->
    load_cache(Defs, default_opts()).

load_cache(Defs, Opts) ->
    ok = enif_protobuf:set_opts(Opts),
    enif_protobuf:load_cache(Defs).

purge_cache() ->
    enif_protobuf:purge_cache().

encode_msg(Msg, Defs) ->
    encode_msg(Msg, Defs, default_opts()).

encode_msg(Msg, Defs, Opts) ->
    ok = load_cache(Defs, Opts),
    enif_protobuf:encode(Msg).

decode_msg(Bin, Name, Defs) ->
    decode_msg(Bin, Name, Defs, default_opts()).

decode_msg(Bin, Name, Defs, Opts) ->
    ok = load_cache(Defs, Opts),
    enif_protobuf:decode(Bin, Name).

roundtrip(Msg, Defs) ->
    roundtrip(Msg, Defs, default_opts()).

roundtrip(Msg, Defs, Opts) ->
    Name = element(1, Msg),
    Bin = encode_msg(Msg, Defs, Opts),
    decode_msg(Bin, Name, Defs, Opts).

person_defs() ->
    [{{msg, 'Person'}, [
        #field{name = name, fnum = 1, rnum = 2, type = string, occurrence = required, opts = []},
        #field{name = id, fnum = 2, rnum = 3, type = int32, occurrence = required, opts = []},
        #field{name = email, fnum = 3, rnum = 4, type = string, occurrence = optional, opts = []}
    ]}].

simple_int32_defs() ->
    [{{msg, m1}, [
        #field{name = a, fnum = 1, rnum = 2, type = int32, occurrence = required, opts = []}
    ]}].
