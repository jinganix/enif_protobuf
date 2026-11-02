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
Last run: 2026-11-02 16:09:07 UTC (standard Google benchmark messages; Erlang 29.0.1 per `.tool-versions`; machine Apple M4 Pro)

| Test case | epb | gpb | gpb nif |
|-----------|-----|-----|---------|
| Small msgs · serialize | 286.91 | 157.14 | 56.79 |
| Small msgs · deserialize | 716.65 | 272.49 | 76.29 |
| Large msgs · serialize | 299.14 | 135.76 | 103.76 |
| Large msgs · deserialize | 516.21 | 255.91 | 196.61 |

Higher is better. Throughput in MB/s on serialized wire size.

<details>
<summary>Hardware / OTP (last run)</summary>

```
tool-versions erlang: 29.0.1
erl: /Users/jingan/.asdf/installs/erlang/29.0.1/bin/erl
System
Darwin 27.0.0 Darwin Kernel Version 27.0.0: Tue Aug 11 21:05:48 PDT 2026; root:xnu-13432.1.9~1/RELEASE_ARM64_T6041 arm64
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
Serialize to binary: 21025345 iterations in 29.093s; 157.14MB/s
Deserialize from binary: 38342080 iterations in 30.596s; 272.49MB/s

Benchmarking gpb msg_r (Message2) with file google_message2.dat
Serialize to binary: 51724 iterations in 30.733s; 135.76MB/s
Deserialize from binary: 76002 iterations in 23.956s; 255.91MB/s

=== gpb nif ===
Benchmarking gpb msg (Message1) with file google_message1.dat
Serialize to binary: 7761921 iterations in 29.717s; 56.79MB/s
Deserialize from binary: 10498134 iterations in 29.920s; 76.29MB/s

Benchmarking gpb msg (Message2) with file google_message2.dat
Serialize to binary: 37157 iterations in 28.887s; 103.76MB/s
Deserialize from binary: 75670 iterations in 31.046s; 196.61MB/s

=== enif_protobuf ===
Benchmarking enif_protobuf msg_r (Message1) with file google_message1.dat
Serialize to binary: 40812520 iterations in 30.931s; 286.91MB/s
Deserialize from binary: 99976895 iterations in 30.334s; 716.65MB/s

Benchmarking enif_protobuf msg_r (Message2) with file google_message2.dat
Serialize to binary: 109803 iterations in 29.609s; 299.14MB/s
Deserialize from binary: 192489 iterations in 30.080s; 516.21MB/s
```
</details>
<!-- BENCHMARK_RESULTS_END -->

## Notes

- Small/large messages are 228 and 84584 bytes on the wire (`google_message1.dat`, `google_message2.dat`).
- Compared to upstream gpb benchmarks, groups were converted to repeated sub-messages; see `msg.proto` comments.
- Artifacts go under `tmp/` (gitignored).
