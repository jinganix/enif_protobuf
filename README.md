[![CI](https://github.com/jinganix/enif_protobuf/actions/workflows/ci.yml/badge.svg)](https://github.com/jinganix/enif_protobuf/actions/workflows/ci.yml)

# enif_protobuf
A Google Protobuf implementation with enif (Erlang nif).

Base on gpb, see more info at
https://github.com/tomas-abrahamsson/gpb

Basic example of using epb
--------------------------

Let's say we have a protobuf file, `x.proto`
```protobuf
message Person {
    required string name = 1;
    required int32 id = 2;
    optional string email = 3;
}
```
We can generate code for this definition in a number of different
ways. Here we use the command line tool.
```shell
# export GPB_PATH=/path/to/gpb
# ${GPB_PATH}/bin/protoc-erl -I. x.proto
```
Now we've got `x.erl` and `x.hrl`. First we compile it.
```shell
# erlc -I${GPB_PATH}/include x.erl
```

Then we can try it out in the Erlang shell. When use rebar3, the `EPB_EBIN_PATH`
is in `_build` directory.
```erlang
# export EPB_EBIN_PATH=/path/to/enif_protobuf/ebin
# erl -pa ${EPB_EBIN_PATH}
Erlang/OTP 18 [erts-7.3] [source] [64-bit] [smp:4:4] [async-threads:10] [kernel-poll:false]

Eshell V7.3  (abort with ^G)
1> rr("x.hrl").
['Person']
2> Bin=x:encode_msg(#'Person'{name="abc def", id=345, email="a@example.com"}).
<<10,7,97,98,99,32,100,101,102,16,217,2,26,13,97,64,101,
  120,97,109,112,108,101,46,99,111,109>>
3> enif_protobuf:load_cache(x:get_msg_defs()).
ok
4> Bin=enif_protobuf:encode(#'Person'{name="abc def", id=345, email="a@example.com"}).
<<10,7,97,98,99,32,100,101,102,16,217,2,26,13,97,64,101,
  120,97,109,112,108,101,46,99,111,109>>
5> enif_protobuf:decode(Bin,'Person').
#'Person'{name = <<"abc def">>,id = 345,
          email = <<"a@example.com">>}
6> enif_protobuf:set_opts([{string_as_list, true}]).
ok
7> enif_protobuf:decode(Bin,'Person').
#'Person'{name = "abc def",id = 345,email = "a@example.com"}
8> enif_protobuf:encode(#'Person'{name="你好", id=345, email="a@example.com"}).
{error,[20320,22909]}
9> enif_protobuf:set_opts([{with_utf8, true}]).
ok
10> enif_protobuf:encode(#'Person'{name="你好", id=345, email="a@example.com"}).
<<10,6,228,189,160,229,165,189,16,217,2,26,13,97,64,101,
  120,97,109,112,108,101,46,99,111,109>>
```

Performance
-----------

Comparison of **enif_protobuf** (NIF), **gpb** (pure Erlang), and **gpb NIF** (libprotobuf) on the standard Google benchmark messages (228 and 84584 bytes on the wire). Regenerate on your machine:

```shell
asdf install   # uses .tool-versions (erlang 29.0.1)
make benchmark-update-readme
```

Benchmarks always use the Erlang version from [`.tool-versions`](.tool-versions). See [benchmarks/README.md](benchmarks/README.md) for all targets (maps, `d.proto`, gpb NIF, etc.).

<!-- BENCHMARK_RESULTS_START -->
Last run: 2026-05-28 06:01:24 UTC (standard Google benchmark messages; Erlang 29.0.1 per `.tool-versions`; machine Apple M4 Pro)

```
    [MB/s]        | epb    | gpb   | gpb nif |
    --------------+--------+-------+---------+
    small msgs    |        |       |         |
      serialize   | 307.88 | 165.33 |  62.18 |
      deserialize | 274.72 | 282.08 |  92.00 |
    --------------+--------+-------+---------+
    large msgs    |        |       |         |
      serialize   | 314.95 | 144.16 | 101.93 |
      deserialize | 241.86 | 269.09 | 193.95 |
    --------------+--------+-------+---------+
```

Higher is better. Throughput is measured on serialized wire size.
<!-- BENCHMARK_RESULTS_END -->
