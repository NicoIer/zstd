/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#include <string.h>     /* memcpy, memset */
#include "zstd_diff.h"
#include "../zstd.h"
#include "error_private.h"
#include "mem.h"

/* Diff format:
 * [Header: 16 bytes]
 *   - Magic: 4 bytes (0x5A535444 = "ZSTD" in hex, but 0x44494646 = "DIFF")
 *   - Target size: 8 bytes (little endian)
 *   - Source size: 8 bytes (little endian, removed - not needed)
 * [Compressed data]
 *   - Copy operations and new data compressed with zstd
 *
 * Simple diff format (for initial implementation):
 * - Just store target size and compressed target data
 * - Future optimization: use source as dictionary for better compression
 */

#define ZSTD_DIFF_MAGIC 0x44494646  /* "DIFF" in little-endian */
#define ZSTD_DIFF_HEADER_SIZE 12

typedef struct {
    U32 magic;
    U64 targetSize;
} ZSTD_diffHeader;

size_t ZSTD_getDiffBound(size_t targetSize, size_t sourceSize)
{
    /* Worst case: header + compressed target
     * In worst case, target is incompressible */
    (void)sourceSize;  /* Currently unused, reserved for future optimization */
    return ZSTD_DIFF_HEADER_SIZE + ZSTD_compressBound(targetSize);
}

size_t ZSTD_createDiff(void* dst, size_t dstCapacity,
                       const void* target, size_t targetSize,
                       const void* source, size_t sourceSize,
                       int compressionLevel)
{
    char* const dstStart = (char*)dst;
    size_t pos = 0;
    
    (void)source;      /* Currently unused, reserved for future optimization */
    (void)sourceSize;  /* Currently unused, reserved for future optimization */
    
    /* Check parameters */
    if (dst == NULL || target == NULL) 
        return ERROR(GENERIC);
    if (dstCapacity < ZSTD_getDiffBound(targetSize, sourceSize))
        return ERROR(dstSize_tooSmall);
    
    /* Write header */
    if (dstCapacity < ZSTD_DIFF_HEADER_SIZE)
        return ERROR(dstSize_tooSmall);
    
    MEM_writeLE32(dstStart + pos, ZSTD_DIFF_MAGIC);
    pos += 4;
    MEM_writeLE64(dstStart + pos, targetSize);
    pos += 8;
    
    /* Compress target data
     * Future optimization: use source as dictionary for better compression ratio */
    {
        size_t const compressedSize = ZSTD_compress(
            dstStart + pos, 
            dstCapacity - pos,
            target, 
            targetSize, 
            compressionLevel
        );
        
        if (ZSTD_isError(compressedSize))
            return compressedSize;
        
        pos += compressedSize;
    }
    
    return pos;
}

size_t ZSTD_applyDiff(void* dst, size_t dstCapacity,
                      const void* diff, size_t diffSize,
                      const void* source, size_t sourceSize)
{
    const char* const diffStart = (const char*)diff;
    size_t pos = 0;
    U32 magic;
    U64 targetSize;
    
    (void)source;      /* Currently unused, reserved for future optimization */
    (void)sourceSize;  /* Currently unused, reserved for future optimization */
    
    /* Check parameters */
    if (dst == NULL || diff == NULL)
        return ERROR(GENERIC);
    if (diffSize < ZSTD_DIFF_HEADER_SIZE)
        return ERROR(srcSize_wrong);
    
    /* Read and verify header */
    magic = MEM_readLE32(diffStart + pos);
    pos += 4;
    if (magic != ZSTD_DIFF_MAGIC)
        return ERROR(prefix_unknown);
    
    targetSize = MEM_readLE64(diffStart + pos);
    pos += 8;
    
    if (dstCapacity < targetSize)
        return ERROR(dstSize_tooSmall);
    
    /* Decompress target data */
    {
        size_t const decompressedSize = ZSTD_decompress(
            dst,
            dstCapacity,
            diffStart + pos,
            diffSize - pos
        );
        
        if (ZSTD_isError(decompressedSize))
            return decompressedSize;
        
        if (decompressedSize != targetSize)
            return ERROR(corruption_detected);
        
        return decompressedSize;
    }
}
