#! /usr/bin/env escript
%%! -pa ../_build/test/lib/gpb/ebin -pa ../_build/default/lib/enif_protobuf/ebin -pa tmp/
%% -*- erlang -*-
%% Controlled A/B benchmark: same data, same iteration count, median of N rounds.
%% Usage: ./ab-bench.escript [--iters N] [--rounds N] [--backend both|gpb|epb]
-mode(compile).
-export([main/1]).

main(Args) ->
    {Iters, Rounds, Backends} = parse(Args, {200, 7, [gpb, epb]}),
    Cases = [
        {'Message1', "google_message1.dat"},
        {'Message2', "google_message2.dat"}
    ],
    [begin
         Bin = read(File),
         Size = byte_size(Bin),
         [begin
              Samples = bench(B, Bin, Msg, Iters, Rounds),
              {EncMed, DecMed} = median_pair(Samples),
              io:format("~-4s ~-9s ~7wB  enc ~8.2f MB/s  dec ~8.2f MB/s~n",
                        [B, Msg, Size, mbps(EncMed, Size * Iters), mbps(DecMed, Size * Iters)])
          end || B <- Backends],
         io:format("~n")
     end || {Msg, File} <- Cases],
    ok.

parse([], Acc) -> Acc;
parse(["--iters", N | Rest], {_, R, B}) -> parse(Rest, {list_to_integer(N), R, B});
parse(["--rounds", N | Rest], {I, _, B}) -> parse(Rest, {I, list_to_integer(N), B});
parse(["--backend", "both" | Rest], {I, R, _}) -> parse(Rest, {I, R, [gpb, epb]});
parse(["--backend", "gpb" | Rest], {I, R, _}) -> parse(Rest, {I, R, [gpb]});
parse(["--backend", "epb" | Rest], {I, R, _}) -> parse(Rest, {I, R, [epb]});
parse([Other | _], _) -> io:format(standard_error, "bad arg ~s~n", [Other]), halt(2).

read(File) -> {ok, B} = file:read_file(File), B.

bench(gpb, Bin, Msg, Iters, Rounds) ->
    setup(gpb),
    M = msg_r:decode_msg(Bin, Msg),
    [{time(Iters, fun() -> msg_r:encode_msg(M) end),
      time(Iters, fun() -> msg_r:decode_msg(Bin, Msg) end)} || _ <- lists:seq(1, Rounds)];
bench(epb, Bin, Msg, Iters, Rounds) ->
    setup(epb),
    M = enif_protobuf:decode(Bin, Msg),
    [{time(Iters, fun() -> enif_protobuf:encode(M) end),
      time(Iters, fun() -> enif_protobuf:decode(Bin, Msg) end)} || _ <- lists:seq(1, Rounds)].

setup(epb) -> ok = enif_protobuf:load_cache(msg_r:get_msg_defs()), ok;
setup(gpb) -> ok.

time(N, Fun) ->
    erlang:garbage_collect(),
    T0 = erlang:monotonic_time(microsecond),
    iterate(N, Fun),
    T1 = erlang:monotonic_time(microsecond),
    (T1 - T0) / 1000000.

iterate(N, _) when N =< 0 -> ok;
iterate(N, Fun) -> Fun(), iterate(N - 1, Fun).

mbps(Sec, Size) -> Size / (Sec * 1024 * 1024).

median_pair(Samples) ->
    Enc = median([E || {E, _} <- Samples]),
    Dec = median([D || {_, D} <- Samples]),
    {Enc, Dec}.

median(List) ->
    L = lists:sort(List),
    N = length(L),
    case N rem 2 of
        0 -> (lists:nth(N div 2, L) + lists:nth(N div 2 + 1, L)) / 2;
        1 -> lists:nth(N div 2 + 1, L)
    end.
