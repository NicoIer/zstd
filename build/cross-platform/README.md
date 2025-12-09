# Cross-Platform Build Instructions for zstd

This directory contains scripts and workflows for building zstd shared libraries for multiple platforms.

## Supported Platforms

- **Linux**: `.so` files (shared objects)
- **macOS**: `.dylib` files (dynamic libraries)
- **Windows**: `.dll` files (dynamic link libraries)
- **Android**: `.so` files (shared objects for ARM/x86)
- **iOS**: `.dylib` files (dynamic libraries for ARM)

## Quick Start

### Using the Build Script

```bash
# Build for all platforms (requires appropriate toolchains)
./build_all_platforms.sh all

# Build for specific platform
./build_all_platforms.sh linux
./build_all_platforms.sh macos
./build_all_platforms.sh windows
./build_all_platforms.sh android
./build_all_platforms.sh ios

# Build for specific architecture (Android example)
./build_all_platforms.sh android arm64-v8a
```

### Using GitHub Actions

The repository includes GitHub Actions workflows that automatically build libraries for each platform:

- `.github/workflows/android-ndk-build.yml` - Android builds
- `.github/workflows/windows-artifacts.yml` - Windows builds
- `.github/workflows/macos-ios-build.yml` - macOS and iOS builds

These workflows run on pull requests and releases, generating build artifacts.

## Platform-Specific Instructions

### Linux

**Prerequisites:**
- GCC or Clang
- Make

**Build:**
```bash
cd lib
make lib
```

**Output:**
- `lib/libzstd.so` - Shared library
- `lib/libzstd.a` - Static library

### macOS

**Prerequisites:**
- Xcode Command Line Tools
- Make

**Build:**
```bash
cd lib
make lib
```

**Output:**
- `lib/libzstd.dylib` - Dynamic library
- `lib/libzstd.a` - Static library

### Windows

**Prerequisites:**
- MinGW-w64 or Visual Studio
- Make (for MinGW) or MSBuild (for Visual Studio)

**Build with MinGW:**
```bash
cd lib
make lib
```

**Build with CMake:**
```bash
cd build/cmake
mkdir build && cd build
cmake .. -DZSTD_BUILD_SHARED=ON
cmake --build . --config Release
```

**Output:**
- `lib/dll/libzstd.dll` - Dynamic library
- `lib/dll/libzstd.lib` - Import library (for Visual Studio)
- `lib/libzstd.a` - Static library

### Android

**Prerequisites:**
- Android NDK (r21 or later)
- CMake

**Build:**
```bash
export ANDROID_NDK_HOME=/path/to/android-ndk
./build_all_platforms.sh android arm64-v8a
```

**Supported ABIs:**
- `arm64-v8a` (ARM 64-bit)
- `armeabi-v7a` (ARM 32-bit)
- `x86_64` (Intel 64-bit)
- `x86` (Intel 32-bit)

**Output:**
- `libzstd.so` - Shared library for the specified ABI
- `libzstd.a` - Static library for the specified ABI

### iOS

**Prerequisites:**
- macOS with Xcode
- CMake

**Build:**
```bash
./build_all_platforms.sh ios arm64
```

**Supported Architectures:**
- `arm64` (iPhone 5s and later)
- `x86_64` (iOS Simulator on Intel Macs)
- `arm64` (iOS Simulator on Apple Silicon Macs)

**Output:**
- `libzstd.dylib` - Dynamic library
- `libzstd.a` - Static library

## Output Directory Structure

After building, artifacts are organized in `build-cross-platform/output/`:

```
build-cross-platform/output/
├── linux/
│   ├── libzstd.so
│   ├── libzstd.so.1
│   ├── libzstd.so.1.5.8
│   └── libzstd.a
├── macos/
│   ├── libzstd.dylib
│   ├── libzstd.1.dylib
│   ├── libzstd.1.5.8.dylib
│   └── libzstd.a
├── windows/
│   ├── libzstd.dll
│   ├── libzstd.lib
│   └── libzstd.a
├── android/
│   ├── libzstd.so
│   └── libzstd.a
└── ios/
    ├── libzstd.dylib
    └── libzstd.a
```

## CMake Build Options

For more control over the build, use CMake directly:

```bash
cmake -S build/cmake -B build-output \
  -DCMAKE_BUILD_TYPE=Release \
  -DZSTD_BUILD_SHARED=ON \
  -DZSTD_BUILD_STATIC=ON \
  -DZSTD_BUILD_PROGRAMS=OFF
cmake --build build-output --parallel
```

### Common CMake Options

- `ZSTD_BUILD_SHARED` - Build shared library (default: ON)
- `ZSTD_BUILD_STATIC` - Build static library (default: ON)
- `ZSTD_BUILD_PROGRAMS` - Build command-line programs (default: ON)
- `ZSTD_MULTITHREAD` - Enable multithreading support (default: ON for shared, OFF for static)

## Troubleshooting

### Android NDK Not Found
```bash
export ANDROID_NDK_HOME=/path/to/android-ndk
# Or install via Android Studio SDK Manager
```

### iOS Build Fails
- Ensure you're on macOS
- Install Xcode Command Line Tools: `xcode-select --install`
- Verify CMake version: `cmake --version` (3.10 or later required)

### Windows DLL Not Created
- For MinGW, ensure you're in an MSYS2/MinGW shell
- For Visual Studio, use the CMake approach or VS solution files in `build/VS2010/`

## Contributing

When adding new platforms or architectures:

1. Update `build_all_platforms.sh` with the new build logic
2. Add corresponding GitHub Actions workflow if needed
3. Update this README with platform-specific instructions
4. Test the build on the target platform

## License

This build infrastructure is part of the zstd project and follows the same dual BSD/GPLv2 license.
