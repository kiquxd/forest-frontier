#!/usr/bin/env bash
# Build and test the native Apple Silicon executable without opening the game.
set -euo pipefail
if [[ ${1:-} == --help || ${1:-} == -h ]]; then
  echo 'Usage: ./scripts/build-macos-arm64.sh (native macOS arm64 build and tests)'
  exit 0
fi
if (($#)); then echo 'Unexpected argument; see --help.' >&2; exit 2; fi
if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 ]]; then
  echo 'Run this script in a native arm64 terminal on Apple Silicon macOS.' >&2
  exit 1
fi
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
export FRONTIER_BUILD_DIR="$project_dir/build-macos-arm64"
exec "$project_dir/scripts/bootstrap.sh"
