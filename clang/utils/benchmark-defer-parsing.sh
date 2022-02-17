#!/usr/bin/env bash

set -ex

UTILS_DIR="$( cd "$( dirname "$0" )" && pwd )"
BENCH_DIR=/tmp/bench-defer-parse

# Output some info about the environment:
xcrun clang --version

rm -rf $BENCH_DIR /tmp/cas-test
mkdir $BENCH_DIR
pushd $BENCH_DIR

mkdir -p clang-to-build
mkdir -p clang-to-test

pushd clang-to-build

git clone -q git@github.com:akyrtzi/llvm-project.git
pushd llvm-project
git checkout -q deferred-parsing-to-bench
git rev-parse HEAD
popd #llvm-project
mkdir -p build
pushd build
xcrun cmake -G Ninja ../llvm-project/llvm \
  -DLLVM_TARGETS_TO_BUILD="X86;ARM;AArch64" \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -DLLVM_ENABLE_PROJECTS=clang \
  > /dev/null
xcrun ninja clang > /dev/null
popd #build

pushd llvm-project
git worktree add -q ../../clang-to-test/llvm-project sources/test-deferred-parsing
git rev-parse sources/test-deferred-parsing
popd #llvm-project

popd #clang-to-build

pushd clang-to-test
mkdir -p build
pushd build
xcrun cmake -G Ninja ../llvm-project/llvm \
  -DCMAKE_BUILD_TYPE:STRING=Debug \
  -DCMAKE_C_COMPILER="$BENCH_DIR/clang-to-build/build/bin/clang" \
  -DCMAKE_CXX_COMPILER="$BENCH_DIR/clang-to-build/build/bin/clang++" \
  -DLLVM_ENABLE_PROJECTS=clang \
  > /dev/null
xcrun ninja clang > /dev/null
xcrun llbuild ninja load-manifest --json build.ninja > build.json

echo === Measure debug - normal
time "$UTILS_DIR/process-ninja.py" -manifest build.json -target bin/clang-14 -quiet -extra-compiler-args=-w
echo === Measure debug - lazy body parsing
time "$UTILS_DIR/process-ninja.py" -manifest build.json -target bin/clang-14 -quiet -extra-compiler-args="-w -Xclang -fdefer-body-parsing"
echo === Measure debug - lazy body and codegen once
time "$UTILS_DIR/process-ninja.py" -manifest build.json -target bin/clang-14 -quiet -extra-compiler-args="-w -Xclang -fdefer-body-parsing -Xclang -fcodegen-body-once"

echo === Measure release - normal
time "$UTILS_DIR/process-ninja.py" -manifest build.json -target bin/clang-14 -quiet -extra-compiler-args="-w -O3 -DNDEBUG -U_DEBUG -g0"
echo === Measure release - lazy body parsing
time "$UTILS_DIR/process-ninja.py" -manifest build.json -target bin/clang-14 -quiet -extra-compiler-args="-w -O3 -DNDEBUG -U_DEBUG -g0 -Xclang -fdefer-body-parsing"

popd #build

popd #clang-to-test

popd #$BENCH_DIR
