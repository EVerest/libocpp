#!/bin/sh

set -e

copy_ctest_report() {
    cp build/Testing/Temporary/LastTest.log /ext/ctest-report
}

copy_lcov_coverage() {
    cp build/lcov_coverage.info /ext/lcov_coverage.info
}

cmake \
    -B build \
    -S "$EXT_MOUNT/source" \
    -G Ninja \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="$WORKSPACE_PATH/dist" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ninja -j$(nproc) -C build install

trap copy_ctest_report EXIT
ninja -j$(nproc) -C build test
trap EXIT
copy_ctest_report

trap copy_lcov_coverage EXIT
ninja -j$(nproc) -C build lcov_coverage
trap EXIT
copy_lcov_coverage
