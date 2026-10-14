#!/usr/bin/env bash
# Shared setup helpers; source from setup scripts, not from the game launcher.
require_programs() {
  local program missing=0
  for program in "$@"; do
    if ! command -v "$program" >/dev/null 2>&1; then
      echo "Required command missing: $program. See README.md / Installation." >&2
      missing=1
    fi
  done
  ((missing == 0))
}

validate_build_jobs() {
  BUILD_JOBS=${BUILD_JOBS:-4}
  if [[ ! $BUILD_JOBS =~ ^[0-9]{1,4}$ ]] || ((10#$BUILD_JOBS < 1)); then
    echo 'BUILD_JOBS must be a positive integer (at most 9999).' >&2
    return 2
  fi
  BUILD_JOBS=$((10#$BUILD_JOBS))
  export BUILD_JOBS
}

fetch() {
  local url=$1 output=$2
  [[ ! -s $output ]] || return 0
  echo "Downloading $output..."
  mkdir -p -- "$(dirname -- "$output")"
  curl --fail --location --show-error --retry 3 --retry-all-errors \
    --connect-timeout 20 --output "$output.part" "$url"
  [[ -s $output.part ]] || { echo "Empty download: $url" >&2; return 1; }
  mv -- "$output.part" "$output"
}
