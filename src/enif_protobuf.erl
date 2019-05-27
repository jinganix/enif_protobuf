
%% Copyright (c) jg_513@163.com, https://github.com/jg513

-module(enif_protobuf).

-export([
    set_opts/1,
    load_cache/1,
    purge_cache/0,
    encode/1,
    encode_msg/2,
    decode/2,
    decode_msg/3,
    debug_term/1
]).

-define(NOT_LOADED, not_loaded(?LINE)).

-compile([no_native]).

-on_load(init/0).

init() ->
    PrivDir = case code:priv_dir(?MODULE) of
        {error, _} ->
            BeamDir = filename:dirname(code:which(?MODULE)),
            AppPath = filename:dirname(BeamDir),
            filename:join(AppPath, "priv");
        Path ->
            Path
    end,
    Threads = erlang:system_info(schedulers),
    ok = erlang:load_nif(filename:join(PrivDir, "enif_protobuf"), Threads).

not_loaded(Line) ->
    erlang:nif_error({not_loaded, [{module, ?MODULE}, {line, Line}]}).

set_opts(_Opts) ->
    ?NOT_LOADED.

load_cache(_List) ->
    ?NOT_LOADED.

purge_cache() ->
    ?NOT_LOADED.

encode(_Tuple) ->
    ?NOT_LOADED.

decode(_Binary, _Name) ->
    ?NOT_LOADED.

debug_term(_Term) ->
    ?NOT_LOADED.

encode_msg(Msg, Defs) ->
    ok = load_cache(Defs),
    encode(Msg).

decode_msg(Bin, Name, Defs) ->
    ok = load_cache(Defs),
    decode(Bin, Name).
