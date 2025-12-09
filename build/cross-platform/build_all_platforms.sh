#!/bin/bash
# ################################################################
# Copyright (c) Meta Platforms, Inc. and affiliates.
# All rights reserved.
#
# This source code is licensed under both the BSD-style license (found in the
# LICENSE file in the root directory of this source tree) and the GPLv2 (found
# in the COPYING file in the root directory of this source tree).
# You may select, at your option, one of the above-listed licenses.
# ################################################################

# Build script for generating zstd shared libraries for all platforms
# Supports: Linux (.so), macOS (.dylib), Windows (.dll), Android (.so), iOS (.dylib)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build-cross-platform"
OUTPUT_DIR="${BUILD_DIR}/output"

# Parse command line arguments
PLATFORM="${1:-all}"
ARCHITECTURE="${2:-}"

usage() {
    echo "Usage: $0 [platform] [architecture]"
    echo "Platforms: linux, macos, windows, android, ios, all (default)"
    echo "Architecture: x86_64, arm64, armv7, etc. (platform-specific)"
    exit 1
}

create_output_dirs() {
    mkdir -p "${OUTPUT_DIR}/linux"
    mkdir -p "${OUTPUT_DIR}/macos"
    mkdir -p "${OUTPUT_DIR}/windows"
    mkdir -p "${OUTPUT_DIR}/android"
    mkdir -p "${OUTPUT_DIR}/ios"
}

build_linux() {
    echo "Building for Linux..."
    cd "${REPO_ROOT}"
    make -C lib clean
    make -C lib lib -j$(nproc)
    
    # Copy artifacts
    cp lib/libzstd.so.* "${OUTPUT_DIR}/linux/" || true
    cp lib/libzstd.so "${OUTPUT_DIR}/linux/" || true
    cp lib/libzstd.a "${OUTPUT_DIR}/linux/" || true
    
    echo "Linux build complete. Output in ${OUTPUT_DIR}/linux/"
}

build_macos() {
    echo "Building for macOS..."
    cd "${REPO_ROOT}"
    
    if [[ "$OSTYPE" != "darwin"* ]]; then
        echo "Warning: macOS builds should be run on macOS. Skipping..."
        return 0
    fi
    
    make -C lib clean
    make -C lib lib -j$(sysctl -n hw.ncpu)
    
    # Copy artifacts
    cp lib/libzstd.*.dylib "${OUTPUT_DIR}/macos/" || true
    cp lib/libzstd.dylib "${OUTPUT_DIR}/macos/" || true
    cp lib/libzstd.a "${OUTPUT_DIR}/macos/" || true
    
    echo "macOS build complete. Output in ${OUTPUT_DIR}/macos/"
}

build_windows() {
    echo "Building for Windows..."
    cd "${REPO_ROOT}"
    
    # Check if we're on a Windows-compatible system
    case "$OSTYPE" in
        msys*|cygwin*|mingw*)
            make -C lib clean
            make -C lib lib -j$(nproc)
            
            # Copy artifacts
            cp lib/dll/libzstd.dll "${OUTPUT_DIR}/windows/" || true
            cp lib/dll/libzstd.lib "${OUTPUT_DIR}/windows/" || true
            cp lib/libzstd.a "${OUTPUT_DIR}/windows/" || true
            ;;
        *)
            echo "Warning: Windows builds should be run on Windows/MinGW. Skipping..."
            ;;
    esac
    
    echo "Windows build complete. Output in ${OUTPUT_DIR}/windows/"
}

build_android() {
    echo "Building for Android..."
    
    if [ -z "$ANDROID_NDK_HOME" ]; then
        echo "Error: ANDROID_NDK_HOME not set. Please set it to your Android NDK installation."
        return 1
    fi
    
    # Default to arm64-v8a if no architecture specified
    local ABI="${ARCHITECTURE:-arm64-v8a}"
    local API_LEVEL="${ANDROID_API_LEVEL:-21}"
    
    cd "${REPO_ROOT}"
    
    # Use CMake for Android builds
    local ANDROID_BUILD_DIR="${BUILD_DIR}/android-${ABI}"
    mkdir -p "${ANDROID_BUILD_DIR}"
    cd "${ANDROID_BUILD_DIR}"
    
    cmake "${REPO_ROOT}/build/cmake" \
        -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="${ABI}" \
        -DANDROID_PLATFORM="android-${API_LEVEL}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DZSTD_BUILD_SHARED=ON \
        -DZSTD_BUILD_STATIC=ON
    
    cmake --build . --parallel
    
    # Copy artifacts
    find . -name "libzstd*.so*" -exec cp {} "${OUTPUT_DIR}/android/" \;
    find . -name "libzstd*.a" -exec cp {} "${OUTPUT_DIR}/android/" \;
    
    echo "Android build complete. Output in ${OUTPUT_DIR}/android/"
}

build_ios() {
    echo "Building for iOS..."
    
    if [[ "$OSTYPE" != "darwin"* ]]; then
        echo "Warning: iOS builds must be run on macOS. Skipping..."
        return 0
    fi
    
    # Default to arm64 if no architecture specified
    local ARCH="${ARCHITECTURE:-arm64}"
    
    cd "${REPO_ROOT}"
    
    # Use CMake for iOS builds
    local IOS_BUILD_DIR="${BUILD_DIR}/ios-${ARCH}"
    mkdir -p "${IOS_BUILD_DIR}"
    cd "${IOS_BUILD_DIR}"
    
    cmake "${REPO_ROOT}/build/cmake" \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
        -DCMAKE_BUILD_TYPE=Release \
        -DZSTD_BUILD_SHARED=ON \
        -DZSTD_BUILD_STATIC=ON
    
    cmake --build . --parallel
    
    # Copy artifacts
    find . -name "libzstd*.dylib" -exec cp {} "${OUTPUT_DIR}/ios/" \;
    find . -name "libzstd*.a" -exec cp {} "${OUTPUT_DIR}/ios/" \;
    
    echo "iOS build complete. Output in ${OUTPUT_DIR}/ios/"
}

main() {
    echo "=== zstd Cross-Platform Build Script ==="
    echo "Platform: $PLATFORM"
    echo "Repository: $REPO_ROOT"
    echo ""
    
    create_output_dirs
    
    case "$PLATFORM" in
        linux)
            build_linux
            ;;
        macos)
            build_macos
            ;;
        windows)
            build_windows
            ;;
        android)
            build_android
            ;;
        ios)
            build_ios
            ;;
        all)
            build_linux
            build_macos
            build_windows
            build_android
            build_ios
            ;;
        *)
            echo "Unknown platform: $PLATFORM"
            usage
            ;;
    esac
    
    echo ""
    echo "=== Build Summary ==="
    echo "Output directory: ${OUTPUT_DIR}"
    find "${OUTPUT_DIR}" -type f -name "libzstd*" 2>/dev/null | sort
    echo ""
    echo "Build complete!"
}

main
