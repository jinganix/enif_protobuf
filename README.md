[![CI](https://github.com/jinganix/enif_protobuf/actions/workflows/ci.yml/badge.svg)](https://github.com/jinganix/enif_protobuf/actions/workflows/ci.yml)

# enif_protobuf

A fast Google Protobuf encoder/decoder for Erlang, implemented as a NIF.

Use [gpb](https://github.com/tomas-abrahamsson/gpb) to generate Erlang modules from `.proto` files,
then call `enif_protobuf` instead of the generated `encode_msg/1` and `decode_msg/2` for the same
record tuples and wire format.

## Build

```shell
make            # compile (rebar3 + C NIF)
make tests      # EUnit
make c-tests    # C unit tests
make c-format-check  # verify C style (clang-format >= 19)
make c-format        # apply clang-format to c_src/ and c_src_tests/
```

With rebar3 directly:

```shell
rebar3 compile
rebar3 eunit
```

## Quick start

Given `x.proto`:

```protobuf
message Person {
  required string name = 1;
  required int32 id = 2;
  optional string email = 3;
}
```

Generate and compile Erlang with gpb:

```shell
export GPB_PATH=/path/to/gpb
${GPB_PATH}/bin/protoc-erl -I. x.proto
erlc -I${GPB_PATH}/include x.erl
```

With rebar3, compiled beams live under `_build/default/lib/.../ebin`.

Load the schema cache once, then encode and decode:

```shell
export EPB_EBIN_PATH=/path/to/enif_protobuf/_build/default/lib/enif_protobuf/ebin
erl -pa ${EPB_EBIN_PATH} -pa .
```

```erlang
1> rr("x.hrl").
['Person']
2> Msg = #'Person'{name = "abc def", id = 345, email = "a@example.com"}.
3> gpb_bin = x:encode_msg(Msg).
4> enif_protobuf:load_cache(x:get_msg_defs()).
ok
5> epb_bin = enif_protobuf:encode(Msg).
6> gpb_bin =:= epb_bin.
true
7> enif_protobuf:decode(epb_bin, 'Person').
#'Person'{name = <<"abc def">>, id = 345, email = <<"a@example.com">>}
```

Convenience wrappers that load the cache on each call:

```erlang
enif_protobuf:encode_msg(Msg, x:get_msg_defs()).
enif_protobuf:decode_msg(Bin, 'Person', x:get_msg_defs()).
```

## Options

Call `enif_protobuf:set_opts/1` before `load_cache/1`:

| Option                   | Default | Effect                                                 |
|--------------------------|---------|--------------------------------------------------------|
| `{string_as_list, true}` | `false` | Decode string fields as lists instead of binaries      |
| `{with_utf8, true}`      | `false` | Allow non-Latin-1 UTF-8 in string fields when encoding |

```erlang
%% UTF-8 strings require with_utf8
enif_protobuf:set_opts([{with_utf8, true}]).
enif_protobuf:encode(#'Person'{name = "你好", id = 345, email = "a@example.com"}).
```

Other API: `purge_cache/0` clears the loaded schema; `debug_term/1` prints internal term layout.

## Performance

Comparison of **enif_protobuf** (NIF), **gpb** (pure Erlang), and **gpb NIF** (libprotobuf) on the
standard Google benchmark messages (228 and 84584 bytes on the wire). Regenerate on your machine:

```shell
asdf install   # uses .tool-versions (erlang 29.0.1)
make benchmark-update-readme
```

Benchmarks always use the Erlang version from [`.tool-versions`](.tool-versions).
See [benchmarks/README.md](benchmarks/README.md) for all targets (maps, `d.proto`, gpb NIF, etc.).

<!-- BENCHMARK_RESULTS_START -->
Last run: 2026-05-28 12:28:41 UTC (standard Google benchmark messages; Erlang 29.0.1 per `.tool-versions`; machine Apple M4 Pro)

| Test case | epb | gpb | gpb nif |
|-----------|-----|-----|---------|
| Small msgs · serialize | 314.54 | 171.77 | 67.63 |
| Small msgs · deserialize | 573.06 | 300.92 | 92.97 |
| Large msgs · serialize | 313.59 | 148.20 | 105.56 |
| Large msgs · deserialize | 443.93 | 282.71 | 202.56 |

Higher is better. Throughput in MB/s on serialized wire size.
<!-- BENCHMARK_RESULTS_END -->
