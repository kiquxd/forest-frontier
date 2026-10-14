#!/usr/bin/env bash
# Explicit setup only: no sudo, no changes to user saves or settings.
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
with_model=false
while (($#)); do
  case "$1" in
    --with-model) with_model=true ;;
    --help|-h)
      cat <<'EOF'
Usage: ./scripts/install.sh [--with-model]

Download game dependencies, build Release, and run tests.
--with-model also builds the CPU llama-server and downloads Qwen3-1.7B
Q4_K_M (approximately 1.11 GB). Internet access is needed for uncached files.
Existing saves/settings and completed downloads are preserved.
Prerequisites and Linux packages: README.md / Installation.
BUILD_JOBS controls build parallelism (default: 4).
EOF
      exit 0 ;;
    *) echo "Unknown setup option: $1 (see --help)." >&2; exit 2 ;;
  esac
  shift
done
source "$project_dir/scripts/setup-common.sh"
validate_build_jobs
require_programs cmake ctest curl tar "${CXX:-c++}" python3
if $with_model; then require_programs git "${CC:-cc}"; fi
"$project_dir/scripts/bootstrap.sh"
if $with_model; then
  "$project_dir/scripts/build-llama.sh"
  "$project_dir/scripts/download-model.sh"
  echo 'Setup complete. Start the game and model: ./scripts/run.sh'
else
  echo 'Setup complete. Start the game: ./scripts/run.sh --no-model'
  echo 'Optional local model: ./scripts/install.sh --with-model'
fi
