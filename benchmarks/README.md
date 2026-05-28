# Benchmarks

Benchmark setup follows [gpb/benchmarks](https://github.com/tomas-abrahamsson/gpb/tree/master/benchmarks): Google’s standard `msg.proto` wire messages (`google_message1.dat`, `google_message2.dat`) plus optional `d.proto` stress cases.

## Prerequisites

1. Install the Erlang version from the repo [`.tool-versions`](../.tool-versions) (currently via [asdf](https://asdf-vm.com/) or [mise](https://mise.jdx.dev/)):

   ```shell
   asdf install   # from repo root
   ```

   All benchmark `make` targets run Erlang through `scripts/with_tool_versions.sh` and fail if the active OTP major version does not match `.tool-versions`.

2. Build the parent project (NIF + `enif_protobuf`):

   ```shell
   cd ..
   ../scripts/with_tool_versions.sh ./rebar3 compile
   ../scripts/with_tool_versions.sh ./rebar3 as test compile   # gpb for code generation
   ```

   (`make build` in this directory does this automatically with the correct Erlang.)

3. Optional: point `GPB_PATH` at a local gpb checkout if you do not use the test profile deps:

   ```shell
   make GPB_PATH=/path/to/gpb build
   ```

## Running

From this directory:

| Target | Description |
|--------|-------------|
| `make build` | Compile benchmark modules (`tmp/msg_r.beam`, …) |
| `make gpb-benchmarks` | Pure Erlang **gpb** (records, `msg_r`) |
| `make gpb-nif-benchmarks` | **gpb NIF** (libprotobuf, `msg`) |
| `make epb-benchmarks` | **enif_protobuf** NIF (same generated modules) |
| `make benchmarks` | Run all three backends: `gpb`, `gpb nif`, `epb` |
| `make benchmarks-quick` | Same suite with `BENCH_TARGET_TIME=3` (debug/dev) |
| `make update-readme` | Run standard suite and refresh result tables in this file and `../README.md` |
| `make erl-benchmarks` | Records + maps variants for both backends |
| `make d-benchmarks` | `d.proto` merge stress (both backends) |
| `make nif-benchmarks` | gpb’s libprotobuf NIF (requires C++ protobuf) |

Shorter runs while developing:

```shell
BENCH_TARGET_TIME=3 make benchmarks
```

Or use the shortcut target:

```shell
make benchmarks-quick
```

HiPE (if supported on your OTP):

```shell
make HIPE=1 gpb-benchmarks
```

### Manual invocation

`proto-bench` takes `--backend gpb` or `--backend epb`, then triples: `MODULE MESSAGE FILE`:

```shell
make build
../scripts/with_tool_versions.sh ./proto-bench \
  --backend gpb msg_r Message1 google_message1.dat \
  --backend gpb msg Message1 google_message1.dat \
  --backend epb msg_r Message1 google_message1.dat
```

Each case runs serialize and deserialize for ~30 seconds (configurable via `BENCH_TARGET_TIME`).

## Results

<!-- BENCHMARK_RESULTS_START -->
Last run: 2026-05-28 12:28:41 UTC (standard Google benchmark messages; Erlang 29.0.1 per `.tool-versions`; machine Apple M4 Pro)

| Test case | epb | gpb | gpb nif |
|-----------|-----|-----|---------|
| Small msgs · serialize | 314.54 | 171.77 | 67.63 |
| Small msgs · deserialize | 573.06 | 300.92 | 92.97 |
| Large msgs · serialize | 313.59 | 148.20 | 105.56 |
| Large msgs · deserialize | 443.93 | 282.71 | 202.56 |

Higher is better. Throughput in MB/s on serialized wire size.

<details>
<summary>Hardware / OTP (last run)</summary>

```
tool-versions erlang: 29.0.1
System
Darwin 25.5.0 Darwin Kernel Version 25.5.0: Mon Apr 27 20:41:15 PDT 2026; root:xnu-12377.121.6~2/RELEASE_ARM64_T6041 arm64
CPU info
Apple M4 Pro
physical cpus  : 14
logical cpus   : 14

OTP release 29
Erlang/OTP 29 [erts-17.0.1] [source] [64-bit] [smp:14:14] [ds:14:14:10] [async-threads:1] [jit]
```
</details>

<details>
<summary>Full benchmark log (last run)</summary>

```
=== gpb ===
Benchmarking gpb msg_r (Message1) with file google_message1.dat
Serialize to binary: 23795246 iterations in 30.121s; 171.77MB/s
Deserialize from binary: 41744282 iterations in 30.163s; 300.92MB/s

Benchmarking gpb msg_r (Message2) with file google_message2.dat
Serialize to binary: 52259 iterations in 28.445s; 148.20MB/s
Deserialize from binary: 81486 iterations in 23.250s; 282.71MB/s

=== gpb nif ===
Benchmarking gpb msg (Message1) with file google_message1.dat
Serialize to binary: 9601190 iterations in 30.867s; 67.63MB/s
Deserialize from binary: 14943686 iterations in 34.952s; 92.97MB/s

Benchmarking gpb msg (Message2) with file google_message2.dat
Serialize to binary: 39562 iterations in 30.231s; 105.56MB/s
Deserialize from binary: 77044 iterations in 30.681s; 202.56MB/s

=== enif_protobuf ===
Benchmarking enif_protobuf msg_r (Message1) with file google_message1.dat
Serialize to binary: 43363214 iterations in 29.976s; 314.54MB/s
Deserialize from binary: 79128903 iterations in 30.024s; 573.06MB/s

Benchmarking enif_protobuf msg_r (Message2) with file google_message2.dat
Serialize to binary: 117020 iterations in 30.102s; 313.59MB/s
Deserialize from binary: 162919 iterations in 29.604s; 443.93MB/s
```
</details>
<!-- BENCHMARK_RESULTS_END -->

## Notes

- Small/large messages are 228 and 84584 bytes on the wire (`google_message1.dat`, `google_message2.dat`).
- Compared to upstream gpb benchmarks, groups were converted to repeated sub-messages; see `msg.proto` comments.
- Artifacts go under `tmp/` (gitignored).
