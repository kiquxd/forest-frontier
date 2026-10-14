#!/usr/bin/env bash
set -euo pipefail
if [[ ${1:-} == --help || ${1:-} == -h ]]; then
  echo 'Usage: ./scripts/run-model.sh [MODEL_FILE]'
  echo 'Run CPU-only llama-server on localhost. LLM_PORT=8080; LLM_THREADS=4.'
  echo 'Relative model paths are resolved from the project directory. Logs go to logs/.'
  exit 0
fi
if (($# > 1)); then echo 'Usage: ./scripts/run-model.sh [MODEL_FILE]' >&2; exit 2; fi
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
model="${1:-models/Qwen3-1.7B-Q4_K_M.gguf}"
server="${LLAMA_SERVER:-third_party/llama-build/bin/llama-server}"
port=${LLM_PORT:-8080}
threads=${LLM_THREADS:-4}
if [[ ! $port =~ ^[0-9]{1,5}$ ]] || ((10#$port < 1 || 10#$port > 65535)); then
  echo 'LLM_PORT must be an integer between 1 and 65535.' >&2; exit 2
fi
if [[ ! $threads =~ ^[0-9]{1,4}$ ]] || ((10#$threads < 1)); then
  echo 'LLM_THREADS must be a positive integer (at most 9999).' >&2; exit 2
fi
port=$((10#$port)); threads=$((10#$threads))
if [[ ! -f "$model" ]]; then
  echo "Model missing: $model" >&2
  echo 'Download the default model: ./scripts/download-model.sh' >&2
  exit 1
fi
if [[ ! -x "$server" ]]; then
  if [[ -n ${LLAMA_SERVER:-} ]]; then
    if [[ $server != */* ]] && command -v "$server" >/dev/null 2>&1; then server=$(command -v "$server");
    else echo "Configured server executable missing: $server" >&2; exit 1; fi
  elif command -v llama-server >/dev/null 2>&1; then server="$(command -v llama-server)";
  else echo 'Run ./scripts/build-llama.sh first.' >&2; exit 1; fi
fi
mkdir -p logs
log="${LLM_LOG_FILE:-logs/model-$(date +%Y%m%d-%H%M%S)-$$.log}"
mkdir -p "$(dirname -- "$log")"
echo "Model server log: $log"
exec "$server" -m "$model" --alias forest-npc --host 127.0.0.1 --port "$port" \
 -ngl 0 -t "$threads" -tb "$threads" -c 2048 -np 1 \
  --jinja --reasoning off --reasoning-budget 0 \
  --log-verbosity 0 >"$log" 2>&1
