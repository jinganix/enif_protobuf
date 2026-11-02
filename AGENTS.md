# AGENTS.md

Guidance for AI coding agents (Claude Code, Codex, Cursor, Copilot, pi, …) working in this repo or
writing code against the library.

## What this repo is

`enif_protobuf` is a Google Protobuf encoder/decoder for Erlang/OTP implemented as a C NIF. Users
generate Erlang modules with [gpb](https://github.com/tomas-abrahamsson/gpb) (`protoc-erl`), then call
`enif_protobuf` instead of the generated `encode_msg/1` / `decode_msg/2`.

## Commands

```shell
make                    # rebar3 compile + build the NIF
make tests              # EUnit (gpb parity, PropEr round-trips, per-issue regressions)
make c-tests            # C unit tests
make c-coverage-check   # gcovr coverage gate
make c-format           # clang-format >= 19 on c_src/ and c_src_tests/
make benchmark-update-readme   # regenerate benchmark tables in README.md
```

Run `make tests` and `make c-tests` after any C or Erlang change; run `make c-format` after touching
`c_src/`.

## Layout

| Path | Contents |
|---|---|
| `src/enif_protobuf.erl` | Public API; each function is an `erlang:nif_error` stub replaced by the NIF |
| `c_src/enif_protobuf.c` | NIF entry points, schema (`get_msg_defs/0`) parsing, global cache |
| `c_src/ep_cache.c` | Schema node cache (lookup by message id / name) |
| `c_src/ep_encoder.c` / `c_src/ep_decoder.c` | Wire format codec |
| `c_src/ep_node.c` | Schema node tree, reference counting |
| `test/` | EUnit tests; `ep_gpb_tests.erl` asserts byte parity with gpb |
| `c_src_tests/` | C unit tests (built with `EPB_UNIT_TEST`) |
| `benchmarks/` | `proto-bench` suite against gpb and gpb NIF |

## Hard invariants

A mistake here crashes or corrupts the entire BEAM VM, not just one process. Anything is better than a
segfault; raising an Erlang exception is always acceptable.

1. **No uncaught C errors.** Every failure path must use `raise_exception/2` (returns a term) or return
   `enif_make_badarg/1`. Never `abort`, never dereference a NULL after a failed `_alloc`/`_realloc`.
2. **Validate at the boundary.** All arguments crossing from Erlang are untrusted: malformed
   `get_msg_defs()` lists, improper lists, wrong tuple arities, wrong types. Bad input → exception,
   never memory corruption.
3. **Resource lifetime.** Schema nodes are owned by the global cache (`ep_cache.c`), not reference
   counted: they live from `load_cache/1` until `purge_cache/0` or the next `load_cache/1`, and are
   guarded by the cache read/write lock (`enif_rwlock`) — read lock for encode/decode, write lock for
   `load_cache/1` / `purge_cache/0`. Never free or mutate a node while a read lock is released.
4. **Buffers are per scheduler slot** (`ep_get_tdata/1`) and reused; reset `p`/`end` before each
   encode, and never shrink them without reallocation.
5. **Depths and sizes are bounded.** Recursion over nested messages/groups and buffer growth must stay
   bounded by input size — no unbounded recursion on attacker-controlled bytes.

## Conventions

- C: clang-format with `.clang-format` (requires clang-format >= 19); 4-space indent, `snake_case`,
  static helpers for file-local functions, `ep_` / `EP_` prefixes.
- Erlang: keep the public surface exactly as in `src/enif_protobuf.erl` plus `encode_msg/2`,
  `decode_msg/3`; do not add options without updating README and tests.
- Tests: add a regression test named after the issue (`ep_issue_42_tests.erl`) for bug fixes; C-side
  tests go in `c_src_tests/`.
- Never hand-edit the `<!-- BENCHMARK_RESULTS_START -->` … `<!-- BENCHMARK_RESULTS_END -->` block in
  `README.md`; regenerate it with `make benchmark-update-readme`.

## Writing code that uses this library

```erlang
ok = enif_protobuf:set_opts([{with_utf8, true}]),   %% optional, before load_cache
ok = enif_protobuf:load_cache(x:get_msg_defs()),    %% once per node
Bin = enif_protobuf:encode(Msg),                    %% Msg is a gpb record/map tuple
Msg = enif_protobuf:decode(Bin, 'Person').
```

Pitfalls to avoid:

- `encode/1` / `decode/2` before `load_cache/1` raises `cache_not_exists`; use `encode_msg/2` /
  `decode_msg/3` if per-call loading is fine.
- The schema cache is global to the VM and is replaced by the next `load_cache/1` call. Two apps with
  different schemas need separate nodes.
- Non-Latin-1 strings need `{with_utf8, true}`; without it encoding raises.
- `decode/2` needs the message atom (`'Person'`, not `"Person"`) and raises `unknown_msg` otherwise.
- Only `string_as_list` and `with_utf8` options exist. There is no maps/records switch — the shape is
  dictated by the gpb-generated module's definitions.
- Do not subclass or wrap the NIF API in generated code; call it directly from the module that owns
  the schema.

## Extra context for agents

`llms.txt` (repo root) is the short machine-readable summary; `README.md` has the API reference, FAQ
and benchmark methodology.
