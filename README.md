# enif_protobuf
A Google Protobuf implementation with enif (Erlang nif).

Base on gpb, see more info at
https://github.com/tomas-abrahamsson/gpb

Basic example of using gpb
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
```
# export GPB_PATH=/path/to/gpb
# ${GPB_PATH}/bin/protoc-erl -I. x.proto
```
Now we've got `x.erl` and `x.hrl`. First we compile it and then we can
try it out in the Erlang shell:
```erlang
# erlc -I${GPB_PATH}/include x.erl
# export EPB_PATH=/path/to/enif_protobuf
# erl -pa ${EPB_PATH}/ebin
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

Here is a comparison between enif_protobuf(epb) and gpb

    [MB/s]        | epb    | gpb   | gpb nif |
    --------------+--------+-------+---------+
    small msgs    |        |       |         |
      serialize   | 116.09 | 32.17 |  38.02  |
      deserialize | 111.88 | 35.57 |  61.87  |
    --------------+--------+-------+---------+
    large msgs    |        |       |         |
      serialize   | 122.86 | 20.90 |  38.64  |
      deserialize | 114.72 | 32.53 |  59.29  |
    --------------+--------+-------+---------+

HW info

    CPU info
    model name	: Intel(R) Core(TM) i3-3220 CPU @ 3.30GHz
    cache size	: 3072 KB
    cores/threads   : 4
    bogomips	: 6585.04

    Erlang (SMP,ASYNC_THREADS) (BEAM) emulator version 7.3

The performances are measured as number of processed MB/s, serialized form.  Higher values means better performance.

The benchmarks are run with small and large messages (228 and 84584 bytes, respectively, in serialized form)
