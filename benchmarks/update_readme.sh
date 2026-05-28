#!/bin/sh
# Run standard benchmarks and refresh README result sections.
set -e

cd "$(dirname "$0")"
TOOL_SH="$(cd .. && pwd)/scripts/with_tool_versions.sh"

MARKER_START='<!-- BENCHMARK_RESULTS_START -->'
MARKER_END='<!-- BENCHMARK_RESULTS_END -->'

if [ -n "$BENCH_TARGET_TIME" ]; then
  echo "Using BENCH_TARGET_TIME=${BENCH_TARGET_TIME} (shorter run)"
fi

echo "[update-readme] preparing benchmark artifacts..."
make build >/dev/null
make benchmarks-nif-code >/dev/null

HW_INFO=$(make show-hw-sw-info 2>&1)
RUN_DATE=$(date -u '+%Y-%m-%d %H:%M:%S UTC')
MACHINE_MODEL=$(echo "$HW_INFO" | awk '
  /^CPU info$/ { in_cpu=1; next }
  in_cpu && /^$/ { exit }
  in_cpu && /^model name[[:space:]]*:/ {
    sub(/^model name[[:space:]]*:[[:space:]]*/, "", $0)
    print
    exit
  }
  in_cpu && $0 !~ /^(physical cpus|logical cpus|cache size|cores\/threads|bogomips)[[:space:]]*:/ {
    print
    exit
  }
')
[ -n "$MACHINE_MODEL" ] || MACHINE_MODEL="unknown"

GPB_ARGS="msg_r Message1 google_message1.dat msg_r Message2 google_message2.dat"
GPB_NIF_ARGS="msg Message1 google_message1.dat msg Message2 google_message2.dat"
EPB_ARGS="msg_r Message1 google_message1.dat msg_r Message2 google_message2.dat"
GPB_LOG=$(mktemp)
GPB_NIF_LOG=$(mktemp)
EPB_LOG=$(mktemp)
trap 'rm -f "$GPB_LOG" "$GPB_NIF_LOG" "$EPB_LOG"' EXIT INT TERM

echo "[update-readme] running gpb benchmarks..."
"$TOOL_SH" ./proto-bench --backend gpb $GPB_ARGS 2>&1 | tee "$GPB_LOG"
GPB_OUT=$(cat "$GPB_LOG")

echo "[update-readme] running gpb nif benchmarks..."
"$TOOL_SH" ./proto-bench --backend gpb $GPB_NIF_ARGS 2>&1 | tee "$GPB_NIF_LOG"
GPB_NIF_OUT=$(cat "$GPB_NIF_LOG")

echo "[update-readme] running epb benchmarks..."
"$TOOL_SH" ./proto-bench --backend epb $EPB_ARGS 2>&1 | tee "$EPB_LOG"
EPB_OUT=$(cat "$EPB_LOG")

ERLANG_VERSION=$("$TOOL_SH" --erlang-version)

extract_mbps() {
  # $1=output  $2=Message1|Message2  $3=Serialize|Deserialize
  echo "$1" | awk -v msg="$2" -v op="$3" '
    $0 ~ ("\\(" msg "\\)") { inblock=1 }
    inblock && $0 ~ op {
      if (match($0, /; [0-9.]+MB\/s/)) {
        print substr($0, RSTART + 2, RLENGTH - 6)
      }
      exit
    }
  '
}

format_mbps() {
  # Keep table cells aligned while tolerating empty values.
  printf "%6s" "$1"
}

gpb_s1=$(extract_mbps "$GPB_OUT" Message1 "Serialize")
gpb_d1=$(extract_mbps "$GPB_OUT" Message1 "Deserialize")
gpb_s2=$(extract_mbps "$GPB_OUT" Message2 "Serialize")
gpb_d2=$(extract_mbps "$GPB_OUT" Message2 "Deserialize")
epb_s1=$(extract_mbps "$EPB_OUT" Message1 "Serialize")
epb_d1=$(extract_mbps "$EPB_OUT" Message1 "Deserialize")
epb_s2=$(extract_mbps "$EPB_OUT" Message2 "Serialize")
epb_d2=$(extract_mbps "$EPB_OUT" Message2 "Deserialize")
gpb_nif_s1=$(extract_mbps "$GPB_NIF_OUT" Message1 "Serialize")
gpb_nif_d1=$(extract_mbps "$GPB_NIF_OUT" Message1 "Deserialize")
gpb_nif_s2=$(extract_mbps "$GPB_NIF_OUT" Message2 "Serialize")
gpb_nif_d2=$(extract_mbps "$GPB_NIF_OUT" Message2 "Deserialize")

gpb_s1_f=$(format_mbps "$gpb_s1")
gpb_d1_f=$(format_mbps "$gpb_d1")
gpb_s2_f=$(format_mbps "$gpb_s2")
gpb_d2_f=$(format_mbps "$gpb_d2")
epb_s1_f=$(format_mbps "$epb_s1")
epb_d1_f=$(format_mbps "$epb_d1")
epb_s2_f=$(format_mbps "$epb_s2")
epb_d2_f=$(format_mbps "$epb_d2")
gpb_nif_s1_f=$(format_mbps "$gpb_nif_s1")
gpb_nif_d1_f=$(format_mbps "$gpb_nif_d1")
gpb_nif_s2_f=$(format_mbps "$gpb_nif_s2")
gpb_nif_d2_f=$(format_mbps "$gpb_nif_d2")

TABLE_BLOCK=$(cat <<EOF
Last run: ${RUN_DATE} (standard Google benchmark messages; Erlang ${ERLANG_VERSION} per \`.tool-versions\`; machine ${MACHINE_MODEL})

\`\`\`
    [MB/s]        | epb    | gpb   | gpb nif |
    --------------+--------+-------+---------+
    small msgs    |        |       |         |
      serialize   | ${epb_s1_f} | ${gpb_s1_f} | ${gpb_nif_s1_f} |
      deserialize | ${epb_d1_f} | ${gpb_d1_f} | ${gpb_nif_d1_f} |
    --------------+--------+-------+---------+
    large msgs    |        |       |         |
      serialize   | ${epb_s2_f} | ${gpb_s2_f} | ${gpb_nif_s2_f} |
      deserialize | ${epb_d2_f} | ${gpb_d2_f} | ${gpb_nif_d2_f} |
    --------------+--------+-------+---------+
\`\`\`

Higher is better. Throughput is measured on serialized wire size.
EOF
)

RESULTS_FILE=$(mktemp)
SUMMARY_FILE=$(mktemp)
cat > "$RESULTS_FILE" <<EOF
$MARKER_START
${TABLE_BLOCK}

<details>
<summary>Hardware / OTP (last run)</summary>

\`\`\`
${HW_INFO}
\`\`\`
</details>

<details>
<summary>Full benchmark log (last run)</summary>

\`\`\`
=== gpb ===
${GPB_OUT}

=== gpb nif ===
${GPB_NIF_OUT}

=== enif_protobuf ===
${EPB_OUT}
\`\`\`
</details>
$MARKER_END
EOF

cat > "$SUMMARY_FILE" <<EOF
$MARKER_START
${TABLE_BLOCK}
$MARKER_END
EOF

patch_file() {
  target=$1
  block_file=$2
  if [ ! -f "$target" ]; then
    echo "skip missing $target"
    return 0
  fi
  tmp=$(mktemp)
  if grep -q "$MARKER_START" "$target"; then
    before=$(grep -n "$MARKER_START" "$target" | head -1 | cut -d: -f1)
    after=$(grep -n "$MARKER_END" "$target" | head -1 | cut -d: -f1)
    head -n $((before - 1)) "$target" > "$tmp"
    cat "$block_file" >> "$tmp"
    tail -n +$((after + 1)) "$target" >> "$tmp"
  else
    cp "$target" "$tmp"
    printf '\n' >> "$tmp"
    cat "$block_file" >> "$tmp"
  fi
  mv "$tmp" "$target"
  echo "updated $target"
}

patch_file README.md "$RESULTS_FILE"
patch_file ../README.md "$SUMMARY_FILE"

rm -f "$RESULTS_FILE" "$SUMMARY_FILE"

echo "Benchmark summary:"
echo "  gpb  small: serialize=${gpb_s1} deserialize=${gpb_d1}"
echo "  gpb nif small: serialize=${gpb_nif_s1} deserialize=${gpb_nif_d1}"
echo "  epb  small: serialize=${epb_s1} deserialize=${epb_d1}"
echo "  gpb  large: serialize=${gpb_s2} deserialize=${gpb_d2}"
echo "  gpb nif large: serialize=${gpb_nif_s2} deserialize=${gpb_nif_d2}"
echo "  epb  large: serialize=${epb_s2} deserialize=${epb_d2}"
