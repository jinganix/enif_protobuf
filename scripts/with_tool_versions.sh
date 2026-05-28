#!/bin/sh
# Run commands with tools from the repo root .tool-versions (asdf or mise).
set -e

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
TOOL_VERSIONS=$ROOT/.tool-versions

erlang_version() {
    if [ -f "$TOOL_VERSIONS" ]; then
        awk '/^erlang / {print $2; exit}' "$TOOL_VERSIONS"
    fi
}

otp_release() {
    "$1" -noshell -eval 'io:format("~s", [erlang:system_info(otp_release)]), halt().' 2>/dev/null
}

otp_release_major() {
    otp_release "$1" | sed 's/[^0-9].*//'
}

tool_versions_otp_major() {
    ver=$(erlang_version)
    [ -n "$ver" ] || return 0
    echo "$ver" | sed 's/[^0-9].*//'
}

run_via_version_manager() {
    if command -v asdf >/dev/null 2>&1; then
        ver=$(erlang_version)
        if [ -n "$ver" ]; then
            export ASDF_ERLANG_VERSION=$ver
        fi
        (cd "$ROOT" && exec asdf exec "$@")
    fi
    if command -v mise >/dev/null 2>&1; then
        (cd "$ROOT" && exec mise exec -- "$@")
    fi
    return 1
}

resolve_tool() {
    tool=$1
    if command -v asdf >/dev/null 2>&1; then
        ver=$(erlang_version)
        if [ -n "$ver" ]; then
            ASDF_ERLANG_VERSION=$ver asdf which "$tool" 2>/dev/null && return 0
        fi
        (cd "$ROOT" && asdf which "$tool" 2>/dev/null) && return 0
    fi
    if command -v mise >/dev/null 2>&1; then
        (cd "$ROOT" && mise which "$tool" 2>/dev/null) && return 0
    fi
    command -v "$tool"
}

resolve_erl() {
    resolve_tool erl
}

check_erlang_version() {
    want=$(erlang_version)
    [ -n "$want" ] || return 0
    erl_bin=$(resolve_erl) || {
        echo "with_tool_versions: erl not found (want erlang $want from .tool-versions)" >&2
        return 1
    }
    want_major=$(tool_versions_otp_major)
    got_major=$(otp_release_major "$erl_bin")
    if [ -n "$want_major" ] && [ -n "$got_major" ] && [ "$want_major" != "$got_major" ]; then
        echo "with_tool_versions: active OTP $got_major does not match .tool-versions erlang $want" >&2
        echo "  Install/select: asdf install erlang $want && asdf local erlang $want" >&2
        echo "  Or run via: $0 <command>" >&2
        return 1
    fi
}

case "${1:-}" in
    --erl)
        resolve_erl
        ;;
    --erlc)
        resolve_tool erlc
        ;;
    --escript)
        resolve_tool escript
        ;;
    --erlang-version)
        erlang_version
        ;;
    --check)
        check_erlang_version
        ;;
    "")
        echo "usage: $0 [--erl|--erlc|--escript|--erlang-version|--check] <command> [args...]" >&2
        exit 2
        ;;
    *)
        check_erlang_version
        run_via_version_manager "$@" || exec "$@"
        ;;
esac
