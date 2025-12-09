/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#include <stdio.h>     // printf
#include <stdlib.h>    // free, malloc
#include <string.h>    // memcmp
#include <zstd.h>      // ZSTD_createDiff, ZSTD_applyDiff, etc.
#include "common.h"    // Helper functions, CHECK(), and CHECK_ZSTD()

/* Example demonstrating diff/patch functionality:
 * 1. Load source and target files
 * 2. Create a compressed diff
 * 3. Apply the diff to reconstruct target
 * 4. Verify the result matches original target
 */

static void diff_example(const char* sourceFile, const char* targetFile, const char* diffFile)
{
    size_t sourceSize, targetSize;
    void* const sourceBuff = mallocAndLoadFile_orDie(sourceFile, &sourceSize);
    void* const targetBuff = mallocAndLoadFile_orDie(targetFile, &targetSize);
    
    printf("Source file: %s (%zu bytes)\n", sourceFile, sourceSize);
    printf("Target file: %s (%zu bytes)\n", targetFile, targetSize);
    
    /* Create diff */
    size_t const diffBound = ZSTD_getDiffBound(targetSize, sourceSize);
    void* const diffBuff = malloc_orDie(diffBound);
    
    printf("Creating diff (max size: %zu bytes)...\n", diffBound);
    size_t const diffSize = ZSTD_createDiff(
        diffBuff, diffBound,
        targetBuff, targetSize,
        sourceBuff, sourceSize,
        3  /* compression level */
    );
    CHECK_ZSTD(diffSize);
    
    printf("Diff created: %zu bytes (%.2f%% of target size)\n", 
           diffSize, (diffSize * 100.0) / targetSize);
    
    /* Save diff to file */
    saveFile_orDie(diffFile, diffBuff, diffSize);
    printf("Diff saved to: %s\n", diffFile);
    
    /* Apply diff to reconstruct target */
    void* const reconstructedBuff = malloc_orDie(targetSize);
    
    printf("Applying diff to reconstruct target...\n");
    size_t const reconstructedSize = ZSTD_applyDiff(
        reconstructedBuff, targetSize,
        diffBuff, diffSize,
        sourceBuff, sourceSize
    );
    CHECK_ZSTD(reconstructedSize);
    
    printf("Reconstructed: %zu bytes\n", reconstructedSize);
    
    /* Verify reconstruction */
    if (reconstructedSize != targetSize) {
        printf("ERROR: Size mismatch! Reconstructed %zu bytes, expected %zu bytes\n",
               reconstructedSize, targetSize);
        exit(1);
    }
    
    if (memcmp(reconstructedBuff, targetBuff, targetSize) != 0) {
        printf("ERROR: Content mismatch! Reconstructed data differs from target\n");
        exit(1);
    }
    
    printf("SUCCESS: Reconstructed target matches original!\n");
    
    /* Cleanup */
    free(sourceBuff);
    free(targetBuff);
    free(diffBuff);
    free(reconstructedBuff);
}

int main(int argc, const char** argv)
{
    const char* const exeName = argv[0];
    
    if (argc != 4) {
        printf("Diff example: create and apply compressed diff between two files\n");
        printf("Usage:\n");
        printf("  %s SOURCE_FILE TARGET_FILE DIFF_FILE\n", exeName);
        printf("\n");
        printf("Creates a compressed diff from SOURCE_FILE to TARGET_FILE,\n");
        printf("saves it to DIFF_FILE, then applies it to verify correctness.\n");
        return 1;
    }
    
    const char* const sourceFile = argv[1];
    const char* const targetFile = argv[2];
    const char* const diffFile = argv[3];
    
    diff_example(sourceFile, targetFile, diffFile);
    
    return 0;
}
