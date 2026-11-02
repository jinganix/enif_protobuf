%% @doc Fast Google Protobuf (proto2/proto3) encoder and decoder for Erlang/OTP, written in C as a
%% NIF.
%%
%% Drop-in replacement for the `encode_msg/1' and `decode_msg/2' functions that
%% <a href="https://github.com/tomas-abrahamsson/gpb">gpb</a> generates from `.proto' files: same record tuples,
%% same wire format, byte-identical output, no libprotobuf or protoc dependency.
%%
%% Load a schema once with {@link load_cache/1}, then call {@link encode/1} and {@link decode/2}:
%%
%% ```
%% ok = enif_protobuf:load_cache(x:get_msg_defs()),
%% Bin = enif_protobuf:encode(#'Person'{name = "abc", id = 345}),
%% #'Person'{} = enif_protobuf:decode(Bin, 'Person').
%% '''
%%
%% The schema cache is global to the BEAM node and shared by all processes; encode and decode take a
%% read lock and are safe to call concurrently. Calls run on normal schedulers and are not dirty NIFs.
%%
%% Only `{string_as_list, boolean()}' and `{with_utf8, boolean()}' options exist, see
%% {@link set_opts/1}.
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
    %% One state slot per thread that can run a NIF: dirty CPU schedulers
    %% included, so two threads never share a slot.
    Threads = erlang:system_info(schedulers) + erlang:system_info(dirty_cpu_schedulers),
    ok = erlang:load_nif(filename:join(PrivDir, "enif_protobuf"), Threads).

not_loaded(Line) ->
    erlang:nif_error({not_loaded, [{module, ?MODULE}, {line, Line}]}).

%% @doc Set encoder/decoder options. Call before {@link load_cache/1}; the options apply to the
%% schema loaded afterwards.
%%
%% `{string_as_list, true}' makes string fields decode to lists instead of binaries (default
%% `false'). `{with_utf8, true}' allows non-Latin-1 UTF-8 in string fields when encoding (default
%% `false').
%%
%% Raises `badarg' for unknown options, non-boolean values or an improper list.
-spec set_opts([{string_as_list, boolean()} | {with_utf8, boolean()}]) -> ok.
set_opts(_Opts) ->
    ?NOT_LOADED.

%% @doc Load a schema, replacing any schema loaded before. `Defs' is exactly what a gpb generated
%% module returns from `Module:get_msg_defs()'.
%%
%% The cache is global to the node, so all processes and applications on one VM share it; two
%% applications with different schemas need separate nodes or one coordinated `load_cache/1' call.
-spec load_cache(list()) -> ok.
load_cache(_List) ->
    ?NOT_LOADED.

%% @doc Free the loaded schema. Later calls to {@link encode/1} or {@link decode/2} raise
%% `cache_not_exists' until {@link load_cache/1} is called again.
-spec purge_cache() -> ok.
purge_cache() ->
    ?NOT_LOADED.

%% @doc Encode a message. `Msg' is a gpb record (or a map tuple from a `-maps' generated module) that
%% matches a message in the loaded schema; the result is protobuf wire format.
%%
%% Raises `badarg' if `Msg' is not a tuple or does not match the schema, and `cache_not_exists' if no
%% schema is loaded.
-spec encode(tuple()) -> binary().
encode(_Tuple) ->
    ?NOT_LOADED.

%% @doc Decode binary protobuf wire format into a message record or map tuple. `Name' is the message
%% atom as written in the `.proto' file, e.g. `'Person''.
%%
%% Raises `unknown_msg' when the loaded schema has no message called `Name', `cache_not_exists' when
%% no schema is loaded, and `badarg' on malformed wire data.
-spec decode(binary(), atom()) -> tuple().
decode(_Binary, _Name) ->
    ?NOT_LOADED.

%% @doc Print the internal node layout of a loaded schema entry. Debugging aid that returns its
%% argument unchanged.
-spec debug_term(term()) -> term().
debug_term(_Term) ->
    ?NOT_LOADED.

%% @doc {@link load_cache/1} followed by {@link encode/1}, for call sites where loading the schema on
%% every call is acceptable.
-spec encode_msg(tuple(), list()) -> binary().
encode_msg(Msg, Defs) ->
    ok = load_cache(Defs),
    encode(Msg).

%% @doc {@link load_cache/1} followed by {@link decode/2}, for call sites where loading the schema on
%% every call is acceptable.
-spec decode_msg(binary(), atom(), list()) -> tuple().
decode_msg(Bin, Name, Defs) ->
    ok = load_cache(Defs),
    decode(Bin, Name).
