[![CI](https://github.com/jinganix/enif_protobuf/actions/workflows/ci.yml/badge.svg)](https://github.com/jinganix/enif_protobuf/actions/workflows/ci.yml)

# enif_protobuf — fast Google Protobuf encoder/decoder for Erlang (NIF)

**enif_protobuf** is a Google Protobuf encoder and decoder for **Erlang/OTP**, implemented as a NIF
in C. It is a drop-in replacement for the `encode_msg/1` and `decode_msg/2` functions that
[gpb](https://github.com/tomas-abrahamsson/gpb) generates from `.proto` files: same record tuples,
same wire format, same results — but serialization runs in C instead of Erlang, with no C++
libprotobuf dependency.

Use gpb's `protoc-erl` to generate the Erlang modules as usual, load the schema once with
`enif_protobuf:load_cache(Module:get_msg_defs())`, then call `enif_protobuf:encode/1` and
`enif_protobuf:decode/2`.

## Contents

- [Why use enif_protobuf](#why-use-enif_protobuf)
- [Installation](#installation)
- [Quick start](#quick-start)
- [API reference](#api-reference)
- [Supported protobuf features](#supported-protobuf-features)
- [Options](#options)
- [Performance](#performance)
- [Comparison with alternatives](#comparison-with-alternatives)
- [Using enif_protobuf with AI coding assistants](#using-enif_protobuf-with-ai-coding-assistants)
- [FAQ](#faq)
- [Development](#development)

## Why use enif_protobuf

- **Roughly 1.8–2.6× faster than pure-Erlang gpb**, and roughly 2.6–9× faster than gpb's libprotobuf
  NIF, on the standard Google benchmark messages ([see numbers](#performance)).
- **Byte-compatible with gpb.** The test suite asserts `gpb:encode_msg/1 =:= enif_protobuf:encode/1`
  for the covered messages, so existing consumers keep working unchanged.
- **No runtime dependencies beyond OTP.** One small NIF (`priv/enif_protobuf.so`), no libprotobuf,
  no `protoc` at runtime, no per-module native build.
- **Full protobuf feature set** — proto2 and proto3, every scalar type, enums (including aliases and
  negative values), repeated and packed fields, nested messages, oneofs, maps, groups, unknown-field
  passthrough.
- **No regeneration step for existing gpb users.** The schema comes from `Module:get_msg_defs()`, which
  your generated modules already export.

| | |
|---|---|
| Language | Erlang/OTP + C (NIF) |
| Input/output | gpb record tuples (and gpb `-maps` generated modules) |
| Wire compatibility | Google Protobuf, proto2 + proto3 |
| Requires | Erlang/OTP 28+ (CI matrix: 28, 29), a C compiler, `make` |
| Platforms | Linux, macOS (Windows via `make` is untested in CI) |
| Schema loading | `enif_protobuf:load_cache/1`, one global cache per node |

## Installation

Add it as a git dependency in `rebar.config` (the package is not on Hex):

```erlang
{deps, [
    {enif_protobuf, {git, "https://github.com/jinganix/enif_protobuf.git", {branch, "master"}}}
]}.
```

The NIF is compiled by a `pre_hooks` entry during `rebar3 compile`, so a working C toolchain
(`cc`/`clang`, `make`) is required on the build machine. Erlang/OTP 28 or newer is required; the CI
matrix covers OTP 28 and 29 on Linux and macOS.

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

## API reference

| Function | Returns | Notes |
|---|---|---|
| `enif_protobuf:load_cache(Defs)` | `ok` | `Defs` is exactly `Module:get_msg_defs()`; replaces any previously loaded schema for the whole node |
| `enif_protobuf:purge_cache()` | `ok` | Frees the loaded schema; later `encode/1` / `decode/2` raise `cache_not_exists` |
| `enif_protobuf:set_opts(Opts)` | `ok` | Call before `load_cache/1`; unknown options or non-boolean values raise `badarg` |
| `enif_protobuf:encode(Msg)` | `binary()` | `Msg` is a gpb record or map tuple; raises `badarg` if it is not a tuple |
| `enif_protobuf:decode(Bin, MsgName)` | record/map tuple | `MsgName` is the message atom, e.g. `'Person'`; raises `unknown_msg` if the schema has no such message |
| `enif_protobuf:encode_msg(Msg, Defs)` | `binary()` | `load_cache(Defs)` + `encode(Msg)` |
| `enif_protobuf:decode_msg(Bin, Name, Defs)` | tuple | `load_cache(Defs)` + `decode(Bin, Name)` |
| `enif_protobuf:debug_term(Term)` | term | Prints the internal node layout of the loaded schema (debugging aid) |

Calling `encode/1` or `decode/2` before `load_cache/1` raises `cache_not_exists`. All functions are
safe to call concurrently from many processes: the schema cache is shared and read-locked, and each
scheduler gets its own scratch buffers.

## Supported protobuf features

| Feature | Support |
|---|---|
| Scalar types | `int32/64`, `uint32/64`, `sint32/64`, `fixed32/64`, `sfixed32/64`, `float`, `double`, `bool`, `string`, `bytes` |
| Enums | Yes, including aliases and negative values (encoded as 64-bit varints, as protobuf requires) |
| Occurrence | `required`, `optional`, `repeated` (`packed` and unpacked) |
| Composite | Nested messages, `oneof`, `map<K, V>`, `group` (legacy) |
| Unknown fields | Preserved on decode and re-emitted on encode |
| proto3 | Yes, including default-value and `optional` semantics matching gpb |

`string` fields decode to binaries by default; use `{string_as_list, true}` to get lists instead.
Non-Latin-1 text requires `{with_utf8, true}` when encoding (see below).

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
Last run: 2026-11-02 16:09:07 UTC (standard Google benchmark messages; Erlang 29.0.1 per `.tool-versions`; machine Apple M4 Pro)

| Test case | epb | gpb | gpb nif |
|-----------|-----|-----|---------|
| Small msgs · serialize | 286.91 | 157.14 | 56.79 |
| Small msgs · deserialize | 716.65 | 272.49 | 76.29 |
| Large msgs · serialize | 299.14 | 135.76 | 103.76 |
| Large msgs · deserialize | 516.21 | 255.91 | 196.61 |

Higher is better. Throughput in MB/s on serialized wire size.
<!-- BENCHMARK_RESULTS_END -->

## Comparison with alternatives

| | **enif_protobuf** | **gpb** (pure Erlang) | **gpb NIF** (libprotobuf) |
|---|---|---|---|
| Implementation | Hand-written C NIF | Erlang modules generated from `.proto` | C++ libprotobuf behind a NIF |
| Runtime dependencies | OTP only | OTP only | libprotobuf / protobuf headers |
| Native build per proto module | No | No | Yes (one NIF per generated module) |
| Speed (serialize / deserialize) | fastest in this repo's benchmarks | baseline | slower here, needs C++ toolchain |
| Record shape | gpb records / maps | gpb records / maps | gpb records / maps |
| Best for | high-throughput Erlang services already using gpb | zero build-toolchain portability | projects already standardized on libprotobuf |

The NIF path adds a native build step; if you cannot ship a `.so` (restricted CI, no compiler),
stay with pure gpb — the API is the same, so switching later is a one-line change.

## Using enif_protobuf with AI coding assistants

This repo ships machine-readable context so LLM assistants (Claude Code, Codex, Cursor, Copilot,
pi, …) produce working code on the first try:

- [`AGENTS.md`](AGENTS.md) — repo rules, architecture map, commands, and the API pitfalls that
  cause most wrong answers.
- [`llms.txt`](llms.txt) — compact, [llms.txt-spec](https://llmstxt.org/) summary of the project for
  retrieval-based assistants.

Rules an assistant should follow when writing code against this library:

1. Generate modules with gpb `protoc-erl`; do **not** hand-write record definitions.
2. Call `enif_protobuf:load_cache(Module:get_msg_defs())` once at startup (or use
   `encode_msg/2` / `decode_msg/3`, which load it per call).
3. Preserve the gpb record shape: field order and types must match `Module:get_msg_defs()`.
4. Strings: pass lists on encode, expect binaries on decode unless `{string_as_list, true}` is set;
   non-Latin-1 text needs `{with_utf8, true}` before `load_cache/1`.
5. Decode needs the message name atom, e.g. `enif_protobuf:decode(Bin, 'Person')`.
6. Do not invent options: only `string_as_list` and `with_utf8` exist.

Example prompt that works well:

> Convert this module to use enif_protobuf instead of gpb. Keep the record definitions and generated
> modules, load the schema once in `start/2`, and keep the wire format unchanged.

## FAQ

**Is enif_protobuf faster than gpb?**
Yes, in this repo's benchmarks it is ~1.8–2.6× faster than pure-Erlang gpb and several times faster
than gpb's libprotobuf NIF. Numbers and hardware are in the [benchmark table](#performance); rerun
`make benchmark-update-readme` on your own machine before relying on them.

**Do I need to regenerate my `.proto` files or change my records?**
No. Keep gpb's `protoc-erl` output and record definitions; only the encode/decode calls change.

**Does it produce the same bytes as gpb?**
Yes for the messages covered by the test suite (the tests assert `gpb_bin =:= epb_bin`). Both follow
the protobuf wire format, so any conforming decoder reads either.

**Why does `encode/1` or `decode/2` raise `cache_not_exists`?**
No schema is loaded yet (or `purge_cache/0` was called). Call `enif_protobuf:load_cache/1` first, or
use `encode_msg/2` / `decode_msg/3`.

**Why do I get `badarg` from `set_opts/1`?**
Only `{string_as_list, boolean()}` and `{with_utf8, boolean()}` are accepted, and the argument must be
a proper list of such tuples.

**How do I encode non-ASCII (UTF-8) strings?**
`enif_protobuf:set_opts([{with_utf8, true}])` before `load_cache/1`. Without it, encoding raises on
code points outside Latin-1.

**Are strings returned as binaries or lists?**
Binaries by default. Set `{string_as_list, true}` for lists.

**Can several processes or applications use it at once?**
Encoding and decoding are safe from any number of processes. The schema cache is global to the node,
so two applications with different `.proto` schemas must not share one VM — give each its own node, or
coordinate the single `load_cache/1` call.

**Does it run on dirty schedulers?**
No; calls execute on normal schedulers and hold a NIF read lock for the duration of the call. Keep
individual messages reasonably sized, or shard very large payload work across pools.

**Why does memory stay high after encoding one huge message?**
The per-scheduler encode buffer grows geometrically to fit the largest message encoded and is not
shrunk afterwards; it is released when the NIF is unloaded (VM exit). This is a deliberate trade-off
for the fast path.

**Windows?**
`rebar.config` has a `win32` make hook, but CI only covers Linux and macOS.

## Development

```shell
make              # rebar3 compile + build the NIF
make tests        # EUnit (includes gpb compatibility and PropEr round-trip tests)
make c-tests      # C unit tests
make c-coverage-check
make c-format     # clang-format >= 19 applied to c_src/ and c_src_tests/
```

Layout: `src/enif_protobuf.erl` (public API + NIF stubs) → `c_src/enif_protobuf.c` (NIF entry
points, schema parsing, cache), `c_src/ep_encoder.c` / `c_src/ep_decoder.c` (wire codec),
`test/` (EUnit, gpb parity, PropEr fuzzing), `benchmarks/` (proto-bench suite).

See [`AGENTS.md`](AGENTS.md) for the invariants that must hold in this codebase — an uncaught C
exception or a bad term handled in the NIF kills the whole VM.
