#!/usr/bin/env bash
# Generate line coverage for c_src/ and enforce a minimum threshold.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
C_SRC="$ROOT/c_src"
BUILD="$ROOT/c_src_tests/build"
OUT="$ROOT/c_src_tests/coverage"
MIN_RAW="${COVERAGE_MIN:-0.8}"
MIN_PCT="$(awk -v m="$MIN_RAW" 'BEGIN {
    if (m + 0 > 0 && m + 0 <= 1) print m * 100;
    else print m;
}')"

mkdir -p "$OUT"

write_summary_and_check() {
    local pct="$1"
    local covered="${2:-}"
    local total="${3:-}"

    {
        if [[ -n "$covered" && -n "$total" ]]; then
            echo "lines: ${pct}% (${covered} of ${total} lines in c_src)"
        else
            echo "lines: ${pct}%"
        fi
        echo "TOTAL: ${pct}"
    } > "$OUT/summary.txt"
    echo "Line coverage: ${pct}% (minimum ${MIN_PCT}%)"
    awk -v p="$pct" -v m="$MIN_PCT" 'BEGIN { exit !(p+0 >= m+0) }' \
        || { echo "Coverage ${pct}% is below ${MIN_PCT}%"; exit 1; }
}

report_gcovr() {
    mkdir -p "$OUT/html"
    gcovr --root "$ROOT" \
        --filter "$C_SRC/" \
        --exclude '.*c_src_tests/.*' \
        --txt "$OUT/gcovr.txt" \
        --html-details "$OUT/html/index.html"
    pct="$(awk '/^TOTAL/ {gsub(/%/,"",$4); print $4; exit}' "$OUT/gcovr.txt")"
    covered="$(awk '/^TOTAL/ {print $3; exit}' "$OUT/gcovr.txt")"
    total="$(awk '/^TOTAL/ {print $2; exit}' "$OUT/gcovr.txt")"
    cp "$OUT/gcovr.txt" "$OUT/summary.txt"
    write_summary_and_check "$pct" "$covered" "$total"
}

report_lcov() {
    lcov --capture \
        --directory "$BUILD" \
        --quiet \
        --output-file "$OUT/coverage.info"
    lcov --remove "$OUT/coverage.info" \
        '*/c_src_tests/*' \
        --output-file "$OUT/coverage.info" \
        --quiet
    genhtml "$OUT/coverage.info" \
        --output-directory "$OUT/html" \
        --quiet
    lcov --summary "$OUT/coverage.info" | tee "$OUT/lcov_summary.txt"
    pct="$(awk '/lines/{print $2}' "$OUT/lcov_summary.txt" | tr -d '%')"
    write_summary_and_check "$pct"
}

gcov_adjusted_counts() {
    local gcov_file="$1"
    local exclude="${COVERAGE_EXCLUDE_PATTERN:-raise_exception|return_error}"
    local total=0 uncovered=0 covered=0
    local mark linenum text

    [[ -f "$gcov_file" ]] || return 1

    while IFS= read -r line; do
        [[ "$line" =~ ^[[:space:]]*([^:]+):[[:space:]]*([0-9]+):(.*)$ ]] || continue
        mark="${BASH_REMATCH[1]}"
        linenum="${BASH_REMATCH[2]}"
        text="${BASH_REMATCH[3]}"
        [[ "$linenum" -gt 0 ]] || continue
        [[ "$text" =~ $exclude ]] && continue
        total=$((total + 1))
        if [[ "$mark" == "#####" ]]; then
            uncovered=$((uncovered + 1))
        else
            covered=$((covered + 1))
        fi
    done < "$gcov_file"
    echo "$covered $total"
}

report_gcov() {
    local total_covered=0 total_lines=0
    local pct_line file_path covered total file_pct gcov_name

    {
        echo "file,line_pct,lines_covered,lines_total"
        for gcda in "$BUILD"/c_src_*.gcda; do
            [[ -f "$gcda" ]] || continue
            base="$(basename "$gcda" .gcda)"
            gcov_name="${base#c_src_}.c.gcov"
            (cd "$BUILD" && gcov -b -o . "$base" >/dev/null 2>&1)
            file_path=""
            while IFS= read -r line; do
                case "$line" in
                    File\ \'"$C_SRC"/*)
                        file_path="${line#File \'}"
                        file_path="${file_path%\'}"
                        ;;
                esac
            done < <(cd "$BUILD" && gcov -b -o . "$base" 2>&1)

            if [[ -n "$file_path" ]] && read -r covered total < <(gcov_adjusted_counts "$BUILD/$gcov_name"); then
                if [[ "$total" -gt 0 ]]; then
                    file_pct="$(awk -v c="$covered" -v t="$total" 'BEGIN { printf "%.2f", (c * 100) / t }')"
                    echo "$file_path,$file_pct,$covered,$total"
                    total_covered=$((total_covered + covered))
                    total_lines=$((total_lines + total))
                fi
            fi
        done
        if [[ "$total_lines" -gt 0 ]]; then
            pct="$(awk -v c="$total_covered" -v t="$total_lines" 'BEGIN { printf "%.1f", (c * 100) / t }')"
            echo "TOTAL,$pct,$total_covered,$total_lines"
        else
            echo "TOTAL,0,0,0"
        fi
    } > "$OUT/summary.csv"

    pct="$(awk -F, '$1=="TOTAL"{print $2}' "$OUT/summary.csv")"
    tee "$OUT/summary.txt" <<EOF
lines: ${pct}% ($total_covered of $total_lines lines in c_src)
TOTAL: ${pct}
details: $OUT/summary.csv
EOF
    write_summary_and_check "$pct" "$total_covered" "$total_lines"
}

if ! find "$BUILD" -name 'c_src_*.gcda' -print -quit | grep -q .; then
    echo "No coverage data found. Build with COVERAGE=1 and run tests first." >&2
    exit 1
fi

if command -v gcovr >/dev/null 2>&1; then
    report_gcovr
elif command -v lcov >/dev/null 2>&1; then
    report_lcov
elif command -v gcov >/dev/null 2>&1; then
    report_gcov
else
    echo "Install gcovr or lcov, or ensure gcov is on PATH." >&2
    exit 1
fi

echo "Coverage report: $OUT"
