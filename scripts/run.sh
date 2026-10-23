#!/usr/bin/env bash
# Start the local model and game, owning only processes launched here.
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: ./scripts/run.sh [--no-model] [--model FILE] [--startup-timeout SECONDS] [-- GAME_ARGS...]

Starts Qwen, waits for readiness, then opens Forest Frontier's main menu.
Game arguments are forwarded unchanged. Close the game to stop a model started
by this launcher; an already running forest-npc server is left running.
--no-model opens the game directly, without a server or readiness checks.

Examples:
  ./scripts/run.sh
  ./scripts/run.sh --no-model
  ./scripts/run.sh --model /path/to/model.gguf -- --fullscreen
  ./scripts/run.sh -- --benchmark 55 --benchmark-community --benchmark-llm

Environment:
  LLM_PORT             Local server port (default: 8080).
  LLM_THREADS          CPU threads (default: 4).
  LLAMA_SERVER         llama-server executable (see run-model.sh).
  LLM_MODEL            Model file (default: models/Qwen3-1.7B-Q4_K_M.gguf).
  LLM_STARTUP_TIMEOUT  Readiness timeout in seconds (default: 120).
  GAME_BINARY          Game executable (default: build/forest_frontier;
                       build-macos-arm64/forest_frontier on Apple Silicon).

Relative model/game paths are resolved from the calling directory. No automatic
downloads or builds. Normal gameplay uses authored NPC interactions; model
generation is currently exercised only by explicit --benchmark-llm runs.
EOF
}

caller_dir=$PWD
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
model=${LLM_MODEL:-$project_dir/models/Qwen3-1.7B-Q4_K_M.gguf}
default_build=build
if [[ "$(uname -s)" == Darwin && "$(uname -m)" == arm64 ]]; then
  default_build=build-macos-arm64
fi
game=${GAME_BINARY:-$project_dir/$default_build/forest_frontier}
startup_timeout=${LLM_STARTUP_TIMEOUT:-120}
port=${LLM_PORT:-8080}
no_model=false
while (($#)); do
  case "$1" in
    --no-model) no_model=true; shift ;;
    --model|--startup-timeout)
      if (($# < 2)); then echo "Missing value for $1" >&2; exit 2; fi
      if [[ $1 == --model ]]; then model=$2; else startup_timeout=$2; fi
      shift 2 ;;
    --help|-h) usage; exit 0 ;;
    --) shift; break ;;
    *) break ;;
  esac
done
for arg in "$@"; do
  if [[ $arg == --llm-port || $arg == --llm-port=* ]]; then
    echo 'Set LLM_PORT instead of passing --llm-port, so model and game agree.' >&2
    exit 2
  fi
done
if [[ ! $port =~ ^[0-9]{1,5}$ ]] || ((10#$port < 1 || 10#$port > 65535)); then
  echo 'LLM_PORT must be an integer between 1 and 65535.' >&2; exit 2
fi
if [[ ! $startup_timeout =~ ^[0-9]{1,5}$ ]] || ((10#$startup_timeout < 1)); then
  echo 'Startup timeout must be a positive integer (at most 99999 seconds).' >&2; exit 2
fi
port=$((10#$port)); startup_timeout=$((10#$startup_timeout))
[[ $model == /* ]] || model=$caller_dir/$model
[[ $game == /* ]] || game=$caller_dir/$game
if [[ -n ${LLAMA_SERVER:-} && $LLAMA_SERVER == */* && $LLAMA_SERVER != /* ]]; then
  export LLAMA_SERVER=$caller_dir/$LLAMA_SERVER
fi
if [[ ! -x $game ]]; then
  echo "Game executable missing: $game" >&2
  echo 'Build the game first: ./scripts/install.sh' >&2
  exit 1
fi
if $no_model; then
  cd -- "$project_dir"
  echo 'Starting Forest Frontier without a model server.'
  exec "$game" "$@"
fi
command -v curl >/dev/null || { echo 'curl is required for server readiness checks.' >&2; exit 1; }
cd -- "$project_dir"
mkdir -p logs
export LLM_PORT=$port
export LLM_LOG_FILE=${LLM_LOG_FILE:-$project_dir/logs/model-$(date +%Y%m%d-%H%M%S)-$$.log}
model_pid=''
game_pid=''

stop_child() {
  local pid=$1 deadline
  [[ -n $pid ]] || return 0
  if kill -0 "$pid" 2>/dev/null; then
    kill -TERM "$pid" 2>/dev/null || true
    deadline=$((SECONDS + 5))
    while kill -0 "$pid" 2>/dev/null && ((SECONDS < deadline)); do sleep .1; done
    if kill -0 "$pid" 2>/dev/null; then kill -KILL "$pid" 2>/dev/null || true; fi
  fi
  wait "$pid" 2>/dev/null || true
}
cleanup() {
  local result=$?
  trap - EXIT INT TERM HUP
  stop_child "$game_pid"
  stop_child "$model_pid"
  exit "$result"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

url=http://127.0.0.1:$port
ready() {
  curl --noproxy '*' --silent --fail --connect-timeout 1 --max-time 2 "$url/health" >/dev/null 2>&1 &&
    curl --noproxy '*' --silent --fail --connect-timeout 1 --max-time 2 "$url/v1/models" 2>/dev/null |
      grep -Eq '"id"[[:space:]]*:[[:space:]]*"forest-npc"'
}
if ready; then
  echo "Reusing ready forest-npc server at $url (it will be left running)."
else
  # Any HTTP responder on this port belongs to somebody else; do not replace it.
  if curl --noproxy '*' --silent --connect-timeout 1 --max-time 2 "$url/health" >/dev/null 2>&1; then
    echo "Port $port already serves HTTP but is not a ready forest-npc server." >&2
    echo 'Wait for it to finish loading or choose another LLM_PORT.' >&2
    exit 1
  fi
  if [[ ! -f $model ]]; then echo "Model missing: $model (see README.md)." >&2; exit 1; fi
  "$project_dir/scripts/run-model.sh" "$model" &
  model_pid=$!
  echo "Waiting for model readiness at $url (timeout: ${startup_timeout}s)..."
  deadline=$((SECONDS + startup_timeout))
  while ! ready; do
    if ! kill -0 "$model_pid" 2>/dev/null; then
      echo "Model server exited before becoming ready. Log: $LLM_LOG_FILE" >&2
      exit 1
    fi
    if ((SECONDS >= deadline)); then
      echo "Model startup timed out. Log: $LLM_LOG_FILE" >&2
      exit 1
    fi
    sleep .25
  done
  # A failed bind must never accidentally adopt another server that appeared.
  if ! kill -0 "$model_pid" 2>/dev/null; then
    echo "Model server exited during startup. Log: $LLM_LOG_FILE" >&2; exit 1
  fi
  echo "Model ready. Log: $LLM_LOG_FILE"
fi

echo 'Starting Forest Frontier. Close the game to finish this session.'
"$game" --llm-port "$port" "$@" &
game_pid=$!
while kill -0 "$game_pid" 2>/dev/null; do
  if [[ -n $model_pid ]] && ! kill -0 "$model_pid" 2>/dev/null; then
    echo "Model server stopped unexpectedly. Log: $LLM_LOG_FILE" >&2
    echo 'The game can continue with authored NPC interactions.' >&2
    wait "$model_pid" 2>/dev/null || true
    model_pid=''
  fi
  sleep .25
done
result=0
wait "$game_pid" || result=$?
game_pid=''
exit "$result"
