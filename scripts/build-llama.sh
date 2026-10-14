#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if [[ ${1:-} == --help || ${1:-} == -h ]]; then
  echo 'Usage: ./scripts/build-llama.sh (build CPU-only llama-server, pinned release b9982)'
  echo 'BUILD_JOBS controls parallelism (default: 4). Existing source checkouts are preserved.'
  exit 0
fi
if (($#)); then echo 'Unexpected argument; see --help.' >&2; exit 2; fi
source "$project_dir/scripts/setup-common.sh"
validate_build_jobs
require_programs git cmake "${CXX:-c++}" "${CC:-cc}"
cd -- "$project_dir"
mkdir -p third_party
# Pinned release; build only the server, CPU-only, without libcurl/OpenSSL development packages.
if [[ ! -d third_party/llama.cpp ]]; then
  git clone --depth 1 --branch b9982 https://github.com/ggml-org/llama.cpp.git third_party/llama.cpp
fi
cmake -S third_party/llama.cpp -B third_party/llama-build \
  -DCMAKE_BUILD_TYPE=Release -DGGML_CUDA=OFF -DGGML_VULKAN=OFF \
  -DLLAMA_CURL=OFF -DLLAMA_OPENSSL=OFF -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_SERVER=ON \
 -DLLAMA_BUILD_UI=OFF -DLLAMA_USE_PREBUILT_UI=OFF -DLLAMA_BUILD_APP=OFF
cmake --build third_party/llama-build --target llama-server -j "$BUILD_JOBS"
