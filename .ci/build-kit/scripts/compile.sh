#!/bin/sh

cmake \
    -B "$EXT_MOUNT/build" \
    -S "$EXT_MOUNT/source" \
    -G Ninja \
    -DBUILD_TESTING=ON \
    -DLIBOCPP16_BUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="$EXT_MOUNT/dist" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ninja -j$(nproc) -C build
