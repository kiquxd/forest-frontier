#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if [[ ${1:-} == --help || ${1:-} == -h ]]; then
  echo 'Usage: ./scripts/bootstrap.sh (download dependencies, build Release, run tests)'
  echo 'For optional model setup, use ./scripts/install.sh --with-model.'
  exit 0
fi
if (($#)); then echo 'Unexpected argument; see --help.' >&2; exit 2; fi
source "$project_dir/scripts/setup-common.sh"
validate_build_jobs
require_programs cmake ctest curl tar "${CXX:-c++}" python3
cd -- "$project_dir"
mkdir -p third_party assets models logs saves
if [[ "$(uname -s)" == Linux && "$(uname -m)" == x86_64 ]]; then
  if [[ ! -f third_party/raylib-5.5_linux_amd64/lib/libraylib.a ]]; then
    fetch https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz third_party/raylib.tar.gz
    tar -xzf third_party/raylib.tar.gz -C third_party
  fi
else
  echo 'Install raylib >=5 using your package manager; the bundled binary targets Linux x86_64.'
fi
fetch https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp third_party/json.hpp
fetch https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.20.0/httplib.h third_party/httplib.h
if [[ ! -s assets/NotoSans.ttf ]]; then
  if [[ -f '/usr/share/fonts/google-noto-vf/NotoSans[wght].ttf' ]]; then
    cp '/usr/share/fonts/google-noto-vf/NotoSans[wght].ttf' assets/NotoSans.ttf
  else
    fetch 'https://raw.githubusercontent.com/google/fonts/main/ofl/notosans/NotoSans%5Bwdth,wght%5D.ttf' assets/NotoSans.ttf
  fi
fi
fetch https://raw.githubusercontent.com/google/fonts/main/ofl/notosans/OFL.txt assets/FONT-LICENSE.txt
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j "$BUILD_JOBS"
ctest --test-dir build --output-on-failure
