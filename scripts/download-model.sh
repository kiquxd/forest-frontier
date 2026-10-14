#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
case "${1:-}" in
  --help|-h)
    echo 'Usage: ./scripts/download-model.sh'
    echo 'Download Qwen3-1.7B-Q4_K_M.gguf (~1.11 GB) into models/; resume partial downloads.'
    exit 0 ;;
  '') ;;
  *) echo 'Usage: ./scripts/download-model.sh' >&2; exit 2 ;;
esac
if (($# > 1)); then echo 'No positional arguments supported.' >&2; exit 2; fi
source "$project_dir/scripts/setup-common.sh"
require_programs curl head
cd -- "$project_dir"
model=models/Qwen3-1.7B-Q4_K_M.gguf
if [[ -s $model ]]; then echo "Model already available: $model"; exit 0; fi
mkdir -p models
echo 'Downloading Qwen3-1.7B Q4_K_M (~1.11 GB); interrupted downloads can be resumed.'
curl --fail --location --show-error --retry 3 --retry-all-errors --connect-timeout 20 \
  --continue-at - --output "$model.part" \
  'https://huggingface.co/unsloth/Qwen3-1.7B-GGUF/resolve/main/Qwen3-1.7B-Q4_K_M.gguf'
if [[ $(head -c 4 "$model.part") != GGUF ]]; then
  echo "Download is not a GGUF file: $model.part. Remove it before retrying." >&2
  exit 1
fi
mv -- "$model.part" "$model"
echo "Model ready: $model"
