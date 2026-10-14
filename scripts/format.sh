#!/usr/bin/env bash
# Format only first-party C++ sources and tests.
set -euo pipefail
case "${1:-}" in
  --check) options=(--dry-run --Werror) ;;
  '') options=(-i) ;;
  --help|-h)
    echo 'Usage: ./scripts/format.sh [--check]'
    echo 'Google-format src/*.cpp, src/*.hpp and tests/*.cpp; --check makes no edits.'
    echo 'Requires clang-format >=15 (migration validated with 23.1.3).'
    echo 'CLANG_FORMAT may specify an executable path.'
    exit 0 ;;
  *) echo 'Usage: ./scripts/format.sh [--check]' >&2; exit 2 ;;
esac
if (($# > 1)); then echo 'Too many arguments; see --help.' >&2; exit 2; fi
formatter=${CLANG_FORMAT:-clang-format}
if ! command -v "$formatter" >/dev/null 2>&1; then
  echo 'clang-format is required for development formatting; see README.md.' >&2
  exit 1
fi
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
"$formatter" --style=file "${options[@]}" src/*.cpp src/*.hpp tests/*.cpp
