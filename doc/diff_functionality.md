# ZSTD Diff/Patch Functionality

## Overview

The zstd diff functionality provides a way to create and apply compressed binary patches between files. This is useful for distributing software updates, synchronizing file versions, or any scenario where you need to transmit the difference between two files efficiently.

## Features

- **Simple API**: Three main functions for diff operations
- **Compression**: Diff data is automatically compressed with zstd
- **Flexible**: Works with any binary data
- **Efficient**: Smaller than transmitting full files when changes are localized

## API Functions

### ZSTD_getDiffBound()
```c
size_t ZSTD_getDiffBound(size_t targetSize, size_t sourceSize);
```
Calculate the maximum size needed for a diff buffer.

**Parameters:**
- `targetSize`: Size of the target (newer) file
- `sourceSize`: Size of the source (older) file

**Returns:** Maximum size needed for the diff buffer

### ZSTD_createDiff()
```c
size_t ZSTD_createDiff(void* dst, size_t dstCapacity,
                       const void* target, size_t targetSize,
                       const void* source, size_t sourceSize,
                       int compressionLevel);
```
Create a compressed diff from source to target.

**Parameters:**
- `dst`: Output buffer for the compressed diff
- `dstCapacity`: Size of the output buffer
- `target`: The target (newer) data
- `targetSize`: Size of target data
- `source`: The source (older) data
- `sourceSize`: Size of source data
- `compressionLevel`: Compression level (1-22, default 3)

**Returns:** Size of the compressed diff, or an error code

### ZSTD_applyDiff()
```c
size_t ZSTD_applyDiff(void* dst, size_t dstCapacity,
                      const void* diff, size_t diffSize,
                      const void* source, size_t sourceSize);
```
Apply a compressed diff to source to reconstruct target.

**Parameters:**
- `dst`: Output buffer for reconstructed target
- `dstCapacity`: Size of output buffer
- `diff`: The compressed diff data
- `diffSize`: Size of diff data
- `source`: The source (older) data
- `sourceSize`: Size of source data

**Returns:** Size of reconstructed target, or an error code

## Usage Examples

### C API Example

```c
#include <zstd.h>

// Create a diff
size_t sourceSize, targetSize;
void* source = loadFile("old.bin", &sourceSize);
void* target = loadFile("new.bin", &targetSize);

size_t diffBound = ZSTD_getDiffBound(targetSize, sourceSize);
void* diffBuff = malloc(diffBound);

size_t diffSize = ZSTD_createDiff(
    diffBuff, diffBound,
    target, targetSize,
    source, sourceSize,
    3  // compression level
);

if (ZSTD_isError(diffSize)) {
    // Handle error
}

saveFile("patch.diff", diffBuff, diffSize);

// Apply the diff
void* reconstructed = malloc(targetSize);

size_t reconstructedSize = ZSTD_applyDiff(
    reconstructed, targetSize,
    diffBuff, diffSize,
    source, sourceSize
);

if (ZSTD_isError(reconstructedSize)) {
    // Handle error
}

// reconstructed now contains the target data
```

### Command Line Tool

The `zstd-diff` command-line tool provides easy access to the diff functionality:

#### Create a diff:
```bash
zstd-diff create old.bin new.bin patch.diff
```

With custom compression level:
```bash
zstd-diff create old.bin new.bin patch.diff -l 9
```

#### Apply a diff:
```bash
zstd-diff apply old.bin patch.diff new.bin
```

## Current Implementation

The current implementation uses a simple approach where:
1. The target file is compressed using zstd
2. A header is added containing magic number and metadata
3. The compressed data is stored in the diff file

This provides immediate benefits through zstd's compression while leaving room for future optimizations such as:
- Using the source file as a dictionary for better compression ratio
- Implementing more sophisticated binary diff algorithms (like bsdiff or xdelta)
- Block-level differencing for large files

## Performance Characteristics

- **Compression**: Diff creation time is dominated by zstd compression of the target
- **Decompression**: Diff application time is dominated by zstd decompression
- **Memory**: Requires memory for entire source, target, and diff buffers
- **Size**: Diff size depends on how compressible the target data is

## Use Cases

1. **Software Updates**: Distribute patches instead of full applications
2. **File Synchronization**: Sync changes between systems efficiently
3. **Version Control**: Store deltas between file versions
4. **Backup Systems**: Incremental backups with compressed diffs
5. **Network Transfer**: Reduce bandwidth by sending diffs

## Building

The diff functionality is automatically built as part of libzstd:

```bash
cd lib
make
```

To build the command-line tool:

```bash
cd programs
make zstd-diff
```

To build the example:

```bash
cd examples
make diff_usage
```

To run tests:

```bash
cd tests
make test-diff
```

## Future Enhancements

Potential future improvements include:

- **Dictionary-based compression**: Use source as dictionary for better compression
- **Streaming API**: Support for large files that don't fit in memory
- **Block-based diffs**: More efficient diffs for large files with localized changes
- **Bidirectional diffs**: Create reverse diffs that can undo changes
- **Multi-threaded compression**: Faster diff creation for large files

## Error Handling

All functions return error codes that can be checked with `ZSTD_isError()`:

```c
size_t result = ZSTD_createDiff(...);
if (ZSTD_isError(result)) {
    const char* errorMsg = ZSTD_getErrorName(result);
    fprintf(stderr, "Error: %s\n", errorMsg);
}
```

Common errors:
- `ZSTD_error_dstSize_tooSmall`: Output buffer too small
- `ZSTD_error_srcSize_wrong`: Invalid diff data
- `ZSTD_error_corruption_detected`: Diff data is corrupted

## License

The diff functionality is part of zstd and follows the same dual BSD/GPLv2 license.
