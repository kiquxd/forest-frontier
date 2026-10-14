#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mode="${1:-baseline}"
seconds="${2:-120}"
case "$mode" in
  baseline) extra=() ;;
  llm) extra=(--benchmark-llm) ;;
  gameplay) extra=(--benchmark-gameplay) ;;
  full) extra=(--benchmark-gameplay --benchmark-llm) ;;
  expedition) extra=(--benchmark-expedition --benchmark-llm) ;;
  *) echo 'Usage: ./scripts/benchmark.sh [baseline|llm|gameplay|full|expedition] [seconds]' >&2; exit 1 ;;
esac
./build/forest_frontier --benchmark "$seconds" --uncapped --width 1920 --height 1080 "${extra[@]}"
