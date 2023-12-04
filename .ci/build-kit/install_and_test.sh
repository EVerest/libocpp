#!/bin/sh

set -e

cmake \
    -B build \
    -S "$EXT_MOUNT/source" \
    -DBUILD_TESTING_LIBOCPP=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="$WORKSPACE_PATH/dist"

make -j$(nproc) -C build install

cd ./build/tests/ 

./database_tests
./utils_tests
