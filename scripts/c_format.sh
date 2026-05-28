#!/usr/bin/env bash
# Check or apply clang-format for project C sources.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"

usage() {
    cat <<'EOF'
Usage: scripts/c_format.sh [check|fix]

  check  Verify formatting (default). Exits non-zero if any file differs.
  fix    Rewrite files in place with clang-format.

Environment:
  CLANG_FORMAT  Path to clang-format (default: clang-format)
EOF
}

find_c_sources() {
    find "$ROOT/c_src" "$ROOT/c_src_tests" \
        \( -name '*.c' -o -name '*.h' \) \
        ! -path '*/build/*' \
        | sort
}

require_clang_format() {
    if ! command -v "$CLANG_FORMAT" >/dev/null 2>&1; then
        echo "error: $CLANG_FORMAT not found (install clang-format)" >&2
        exit 1
    fi
}

cmd_check() {
    require_clang_format
    local failed=0
    local f

    while IFS= read -r f; do
        if ! "$CLANG_FORMAT" --dry-run --Werror "$f" 2>/dev/null; then
            echo "not formatted: $f" >&2
            failed=1
        fi
    done < <(find_c_sources)

    if [[ "$failed" -ne 0 ]]; then
        echo "Run 'make c-format' to fix." >&2
        exit 1
    fi

    echo "C format check passed."
}

cmd_fix() {
    require_clang_format
    local f

    while IFS= read -r f; do
        "$CLANG_FORMAT" -i "$f"
    done < <(find_c_sources)

    echo "Formatted C sources under c_src/ and c_src_tests/."
}

main() {
    local action="${1:-check}"

    case "$action" in
        check) cmd_check ;;
        fix) cmd_fix ;;
        -h | --help | help) usage ;;
        *)
            echo "error: unknown action: $action" >&2
            usage >&2
            exit 1
            ;;
    esac
}

main "$@"
