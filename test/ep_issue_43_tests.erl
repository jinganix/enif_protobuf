-module(ep_issue_43_tests).

-compile(export_all).

-include_lib("eunit/include/eunit.hrl").
-include_lib("gpb/include/gpb.hrl").

-define(DEFAULT_BENCH_SECONDS, 5).

% https://github.com/jinganix/enif_protobuf/issues/43
issue_43_smoke_test_() ->
    case os:getenv("CI") of
        false ->
            [{timeout, 5, ?_test(run_issue_43_smoke())}];
        _ ->
            []
    end.

run_issue_43_smoke() ->
    Defs = issue_43_defs(),
    Bin = issue_43_bin(),
    ok = enif_protobuf:set_opts([{with_utf8, true}]),
    ok = enif_protobuf:load_cache(Defs),
    _ = verify_codec_consistency_once(Bin, Defs),
    ok.

benchmark_issue_43() ->
    benchmark_issue_43(?DEFAULT_BENCH_SECONDS).

benchmark_issue_43(Seconds) when is_integer(Seconds), Seconds > 0 ->
    Defs = issue_43_defs(),
    Bin = issue_43_bin(),
    ok = enif_protobuf:set_opts([{with_utf8, true}]),
    ok = enif_protobuf:load_cache(Defs),
    {GpbMsg, EpbMsg} = verify_codec_consistency_once(Bin, Defs),
    warmup(GpbMsg, Bin, Defs),

    EpbEncodeResult = bench_for_seconds(Seconds, fun() -> enif_protobuf:encode(EpbMsg) end),
    GpbEncodeResult = bench_for_seconds(Seconds, fun() -> gpb:encode_msg(GpbMsg, Defs) end),
    EpbDecodeResult = bench_for_seconds(Seconds, fun() -> enif_protobuf:decode(Bin, auth2s) end),
    GpbDecodeResult = bench_for_seconds(Seconds, fun() -> gpb:decode_msg(Bin, auth2s, Defs) end),

    EpbEncodeOps = maps:get(ops, EpbEncodeResult),
    GpbEncodeOps = maps:get(ops, GpbEncodeResult),
    EpbDecodeOps = maps:get(ops, EpbDecodeResult),
    GpbDecodeOps = maps:get(ops, GpbDecodeResult),
    EncodeSpeedup = speedup(EpbEncodeOps, GpbEncodeOps),
    DecodeSpeedup = speedup(EpbDecodeOps, GpbDecodeOps),
    io:format(
        "issue_43 benchmark (~p seconds each)~n"
        "encode(count): epb=~p gpb=~p speedup=~.2fx~n"
        "encode(ops/s): epb=~.2f gpb=~.2f~n"
        "decode(count): epb=~p gpb=~p speedup=~.2fx~n"
        "decode(ops/s): epb=~.2f gpb=~.2f~n",
        [
            Seconds,
            maps:get(count, EpbEncodeResult),
            maps:get(count, GpbEncodeResult),
            EncodeSpeedup,
            EpbEncodeOps,
            GpbEncodeOps,
            maps:get(count, EpbDecodeResult),
            maps:get(count, GpbDecodeResult),
            DecodeSpeedup,
            EpbDecodeOps,
            GpbDecodeOps
        ]
    ),
    #{
        seconds => Seconds,
        encode => #{
            epb => EpbEncodeResult,
            gpb => GpbEncodeResult,
            epb_speedup => EncodeSpeedup
        },
        decode => #{
            epb => EpbDecodeResult,
            gpb => GpbDecodeResult,
            epb_speedup => DecodeSpeedup
        }
    }.

verify_codec_consistency_once(Bin, Defs) ->
    GpbMsg = gpb:decode_msg(Bin, auth2s, Defs),
    EpbMsg = enif_protobuf:decode(Bin, auth2s),
    GpbBin = gpb:encode_msg(GpbMsg, Defs),
    EpbBin = enif_protobuf:encode(EpbMsg),
    ?assertEqual(GpbBin, EpbBin),
    ?assertEqual(GpbMsg, gpb:decode_msg(EpbBin, auth2s, Defs)),
    ?assertEqual(EpbMsg, enif_protobuf:decode(GpbBin, auth2s)),
    {GpbMsg, EpbMsg}.

warmup(Msg, Bin, Defs) ->
    bench_encode_epb(1000, Msg),
    bench_encode_gpb(1000, Msg, Defs),
    bench_decode_epb(1000, Bin),
    bench_decode_gpb(1000, Bin, Defs),
    ok.

bench_encode_epb(0, _Msg) ->
    ok;
bench_encode_epb(N, Msg) ->
    _ = enif_protobuf:encode(Msg),
    bench_encode_epb(N - 1, Msg).

bench_encode_gpb(0, _Msg, _Defs) ->
    ok;
bench_encode_gpb(N, Msg, Defs) ->
    _ = gpb:encode_msg(Msg, Defs),
    bench_encode_gpb(N - 1, Msg, Defs).

bench_decode_epb(0, _Bin) ->
    ok;
bench_decode_epb(N, Bin) ->
    _ = enif_protobuf:decode(Bin, auth2s),
    bench_decode_epb(N - 1, Bin).

bench_decode_gpb(0, _Bin, _Defs) ->
    ok;
bench_decode_gpb(N, Bin, Defs) ->
    _ = gpb:decode_msg(Bin, auth2s, Defs),
    bench_decode_gpb(N - 1, Bin, Defs).

bench_for_seconds(Seconds, Fun) ->
    StartUs = erlang:monotonic_time(microsecond),
    TargetUs = StartUs + Seconds * 1000000,
    Count = bench_until(Fun, TargetUs, 0),
    ElapsedUs = erlang:monotonic_time(microsecond) - StartUs,
    #{
        count => Count,
        elapsed_us => ElapsedUs,
        ops => Count * 1000000 / ElapsedUs
    }.

bench_until(Fun, TargetUs, Count) ->
    case erlang:monotonic_time(microsecond) >= TargetUs of
        true ->
            Count;
        false ->
            _ = Fun(),
            bench_until(Fun, TargetUs, Count + 1)
    end.

speedup(_FasterOps, 0) ->
    0.0;
speedup(FasterOps, SlowerOps) ->
    FasterOps / SlowerOps.

issue_43_defs() ->
    [
        {{msg, p_ks}, [
            #field{name = id, fnum = 1, rnum = 2, type = int64, occurrence = optional, opts = []},
            #field{name = str, fnum = 2, rnum = 3, type = string, occurrence = optional, opts = []}
        ]},
        {{msg, auth2s}, [
            #field{name = uid, fnum = 1, rnum = 2, type = string, occurrence = optional, opts = []},
            #field{name = uname, fnum = 2, rnum = 3, type = string, occurrence = optional, opts = []},
            #field{name = s0, fnum = 3, rnum = 4, type = string, occurrence = optional, opts = []},
            #field{name = time, fnum = 4, rnum = 5, type = int64, occurrence = optional, opts = []},
            #field{name = id1, fnum = 5, rnum = 6, type = int64, occurrence = optional, opts = []},
            #field{name = id2, fnum = 6, rnum = 7, type = int64, occurrence = optional, opts = []},
            #field{name = id3, fnum = 7, rnum = 8, type = int64, occurrence = optional, opts = []},
            #field{name = id4, fnum = 8, rnum = 9, type = int64, occurrence = optional, opts = []},
            #field{name = id5, fnum = 9, rnum = 10, type = int64, occurrence = optional, opts = []},
            #field{name = s1, fnum = 10, rnum = 11, type = string, occurrence = optional, opts = []},
            #field{name = s2, fnum = 11, rnum = 12, type = string, occurrence = optional, opts = []},
            #field{name = id6, fnum = 12, rnum = 13, type = int64, occurrence = optional, opts = []},
            #field{name = s3, fnum = 13, rnum = 14, type = string, occurrence = repeated, opts = []},
            #field{name = ks1, fnum = 14, rnum = 15, type = {msg, p_ks}, occurrence = repeated, opts = []},
            #field{name = ks2, fnum = 15, rnum = 16, type = {msg, p_ks}, occurrence = repeated, opts = []},
            #field{name = id7, fnum = 16, rnum = 17, type = int64, occurrence = optional, opts = []}
        ]}
    ].

issue_43_bin() ->
    <<48,1,82,10,55,48,49,95,49,57,48,51,95,48,32,201,220,202,190,6,90,16,49,46,48,46,48,46,57,54,54,46,49,51,49,50,56,48,114,2,8,1,114,2,8,2,114,7,8,3,18,3,110,105,108,114,23,8,4,18,19,55,48,49,45,49,57,48,51,45,49,55,52,49,55,54,50,49,50,49,114,18,8,21,18,14,87,105,110,100,111,119,115,32,49,48,32,120,54,52,114,2,8,6,114,12,8,10,18,8,57,54,50,42,49,54,56,48,114,2,8,13,114,5,8,14,18,1,48,114,2,8,15,114,10,8,16,18,6,229,144,155,228,184,187,56,239,14,106,24,105,87,88,116,78,80,51,104,54,73,51,35,80,67,76,106,116,69,75,90,67,98,111,88,106,17,95,49,55,48,52,56,56,48,51,48,57,54,54,53,49,55,48,64,239,14,10,9,53,56,50,57,48,51,50,52,57,40,189,5,122,5,8,7,18,1,48,122,8,8,8,18,4,49,48,48,48,122,5,8,4,18,1,48,18,9,53,56,50,57,48,51,50,52,57>>.
