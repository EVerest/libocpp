#!/bin/sh

set -e

cmake \
    -B build \
    -S "$EXT_MOUNT/source" \
    -G Ninja \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="$WORKSPACE_PATH/dist" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ninja -j$(nproc) -C build install

trap "cp build/Testing/Temporary/LastTest.log /ext/ctest-report" EXIT
trap "cp build/lcov_coverage.info /ext/lcov_coverage.info" EXIT

ninja -j$(nproc) -C build test

ninja -j$(nproc) -C build lcov_coverage

# cmake  -B build -G Ninja -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="./dist" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON