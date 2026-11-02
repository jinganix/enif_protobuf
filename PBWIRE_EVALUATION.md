# Replacing the codec with pbwire: prototype and measurements

Branch: `feat/pbwire-backend`. Not merged. Master (v1.1.0) still uses the native
term-driven codec.

## What was built

`c_src/pbwire/` vendors the pbwire core (commit `0d7f616`), and `c_src/ep_pbw.c`
is an adapter that:

1. At `load_cache/1`, mirrors the already-parsed `ep_node` schema into a
   `pbw_schema` (messages, enums, oneofs, maps synthesized as key/value
   entries). No second parse of `get_msg_defs/0`.
2. At `encode/1`, walks the gpb record and builds a `pbw_message` tree, then
   calls pbwire's encoder.
3. At `decode/2`, calls pbwire's decoder, then converts the `pbw_message` tree
   back to the gpb record shape (defaults, oneof tuples, map entries,
   repeated lists).

The switch is compile-time: `-DEPB_USE_PBW=1` in `c_src/Makefile`. Remove that
line (and the vendored tree) to get the native codec back.

Supported: records, every scalar type, repeated (packed and unpacked), nested
messages, enums, oneofs, maps in gpb's default list-of-`{K,V}` shape, proto2 and
proto3 defaulty fields, `string_as_list`/`with_utf8` on decode.

Not supported (hence the test failures below): legacy group fields, unknown-field
collection in the gpb `$unknown` shape, the `maps` record option, and gpb's
charlist input for string fields.

## Correctness

`make tests` on this branch: **155 passed, 44 failed** (master: 199 passed).

The failures cluster as:

| Area | Rough count | Why |
|---|---|---|
| Legacy groups | 6 | pbwire has no group field codec (encode writes them as varints, decode skips them) |
| Unknown-field collection | 5 | pbwire keeps unknown wire bytes internally but does not expose them; the adapter cannot fill `$unknown` |
| `maps` record option | 4 | adapter only builds the default list-of-`{K,V}` maps shape |
| charlists / list strings | 4 | adapter accepts binaries only for `string`/`bytes` |
| error reason / arity validation | 6 | adapter raises different reasons and does not reproduce gpb's exact arity errors |
| misc. (`encode_msg/2`, options) | rest | follow-ups of the above |

The benchmark message (`google_message1.dat`, `Message1`) round-trips and its
re-encoded bytes are identical to the input (`enc_match=true`).

## Performance

Same harness for both builds: one long-lived VM, `msg_r` records,
`enif_protobuf:decode/2` and `enif_protobuf:encode/1`, 1,000,000 iterations for
the 228-byte Google message and 200,000 for a 9,228-byte variant (the same
record with `field5` repeated 1,000 times). Apple M4 Pro, OTP 29.0.1, `-O3`.
Median of three runs.

| Case | Native decode | pbwire decode | Native encode | pbwire encode |
|---|---|---|---|---|
| small, 228 B | 262 ns (867 MB/s) | 624 ns (366 MB/s) — **2.4x slower** | 269 ns (846 MB/s) | 428 ns (533 MB/s) — **1.6x slower** |
| large, 9,228 B | 11.37 us (812 MB/s) | 14.73 us (626 MB/s) — **1.3x slower** | 5.78 us (1597 MB/s) | 13.06 us (707 MB/s) — **2.3x slower** |

Optimization history for the adapter (small message, medians):

| Step | decode ns | encode ns |
|---|---|---|
| adapter, plain pbwire API (`malloc` per value, per-call output buffer) | 1106 | 884 |
| + decode iterates the known-field table (no O(n^2) reflection) | 1160 | 900 |
| + `pbw_decode_ctx` / `pbw_encode_ctx` (arena + reusable buffer) | 810 | 800 |
| + field-index fast path (`pbw_message_get_index` / `set_index`) | 665 | 720 |
| + encode tree built in the context's build arena | 665 | 425 |
| + stack scratch arrays in decode (no per-message calloc) | 624 | 428 |

## Changes made to pbwire (general, not adapter-specific)

These are in the vendored copy under `c_src/pbwire/` and are candidates for
upstreaming to pbwire itself:

- `pbw_message_get_index` / `pbw_message_set_index`: direct field access by
  linked-descriptor index, skipping the field-number lookup on every value.
- `pbw_descriptor_field_index`: resolve a field number to that index once.
- `pbw_message_get_field` / `pbw_message_set_field`: same fast path by field
  pointer (kept for API symmetry; the index variant is used here).
- `pbw_message_new_ctx` and `pbw_value_new_*_ctx`: build a message tree in the
  context's bump arena, so `pbw_ctx_reset()` releases the tree at once. A
  `readonly` flag on `pbw_arena` keeps decoded trees immutable while build
  arenas stay writable.
- `pbw_arena` gained the `readonly` flag; `pbw_ctx` gained the `build` arena.

These are additive and do not change wire behaviour.

## Verdict

Do not replace the native codec with pbwire as-is.

- A dynamic-message runtime needs an intermediate tree, and adapting gpb records
  to it adds a second traversal and a term-by-term conversion. Even after the
  optimizations above, decode is 1.3x-2.4x and encode 1.6x-2.3x slower.
- pbwire lacks three things a drop-in gpb replacement needs: group fields,
  unknown-field reflection, and map descriptors (synthesizable, but not part of
  its schema model).
- The gap is architectural, not a constant: to win, pbwire would need a
  streaming API that encodes/decodes a field at a time against caller callbacks,
  so no tree is built. That is a different library surface, not a tuning pass.

Keep the native codec. The two wire-format fixes found by pbwire's conformance
and differential testing (tag truncation and strict length prefixes) are already
in master as v1.1.0.
