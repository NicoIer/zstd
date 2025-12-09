/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/*
 * Test program for ZSTD diff API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ZSTD_STATIC_LINKING_ONLY
#include "zstd.h"

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(cond, msg) do { \
    if (cond) { \
        printf("PASS: %s\n", msg); \
        g_testsPassed++; \
    } else { \
        printf("FAIL: %s\n", msg); \
        g_testsFailed++; \
    } \
} while(0)

static void test_basic_diff(void)
{
    const char* source = "Hello, this is the source text.";
    const char* target = "Hello, this is the target text!";
    size_t sourceSize = strlen(source);
    size_t targetSize = strlen(target);
    
    /* Create diff */
    size_t diffBound = ZSTD_getDiffBound(targetSize, sourceSize);
    void* diffBuff = malloc(diffBound);
    assert(diffBuff != NULL);
    
    size_t diffSize = ZSTD_createDiff(
        diffBuff, diffBound,
        target, targetSize,
        source, sourceSize,
        3  /* compression level */
    );
    
    TEST(!ZSTD_isError(diffSize), "Create diff");
    TEST(diffSize <= diffBound, "Diff size within bounds");
    
    /* Apply diff */
    void* reconstructed = malloc(targetSize);
    assert(reconstructed != NULL);
    
    size_t reconstructedSize = ZSTD_applyDiff(
        reconstructed, targetSize,
        diffBuff, diffSize,
        source, sourceSize
    );
    
    TEST(!ZSTD_isError(reconstructedSize), "Apply diff");
    TEST(reconstructedSize == targetSize, "Reconstructed size matches");
    TEST(memcmp(reconstructed, target, targetSize) == 0, "Reconstructed content matches");
    
    free(diffBuff);
    free(reconstructed);
}

static void test_empty_files(void)
{
    const char* source = "";
    const char* target = "";
    size_t sourceSize = 0;
    size_t targetSize = 0;
    
    size_t diffBound = ZSTD_getDiffBound(targetSize, sourceSize);
    void* diffBuff = malloc(diffBound);
    assert(diffBuff != NULL);
    
    size_t diffSize = ZSTD_createDiff(
        diffBuff, diffBound,
        target, targetSize,
        source, sourceSize,
        1
    );
    
    TEST(!ZSTD_isError(diffSize), "Create diff for empty files");
    
    void* reconstructed = malloc(1);  /* at least 1 byte */
    assert(reconstructed != NULL);
    
    size_t reconstructedSize = ZSTD_applyDiff(
        reconstructed, 1,
        diffBuff, diffSize,
        source, sourceSize
    );
    
    TEST(!ZSTD_isError(reconstructedSize), "Apply diff for empty files");
    TEST(reconstructedSize == 0, "Reconstructed empty file");
    
    free(diffBuff);
    free(reconstructed);
}

static void test_identical_files(void)
{
    const char* source = "Same content";
    const char* target = "Same content";
    size_t sourceSize = strlen(source);
    size_t targetSize = strlen(target);
    
    size_t diffBound = ZSTD_getDiffBound(targetSize, sourceSize);
    void* diffBuff = malloc(diffBound);
    assert(diffBuff != NULL);
    
    size_t diffSize = ZSTD_createDiff(
        diffBuff, diffBound,
        target, targetSize,
        source, sourceSize,
        1
    );
    
    TEST(!ZSTD_isError(diffSize), "Create diff for identical files");
    
    void* reconstructed = malloc(targetSize);
    assert(reconstructed != NULL);
    
    size_t reconstructedSize = ZSTD_applyDiff(
        reconstructed, targetSize,
        diffBuff, diffSize,
        source, sourceSize
    );
    
    TEST(!ZSTD_isError(reconstructedSize), "Apply diff for identical files");
    TEST(reconstructedSize == targetSize, "Reconstructed size matches for identical files");
    TEST(memcmp(reconstructed, target, targetSize) == 0, "Reconstructed content matches for identical files");
    
    free(diffBuff);
    free(reconstructed);
}

static void test_large_data(void)
{
    /* Test with larger data */
    size_t sourceSize = 100000;
    size_t targetSize = 100500;
    
    char* source = malloc(sourceSize);
    char* target = malloc(targetSize);
    assert(source != NULL && target != NULL);
    
    /* Fill with pseudo-random but deterministic data */
    for (size_t i = 0; i < sourceSize; i++) {
        source[i] = (char)(i * 13 + 7);
    }
    for (size_t i = 0; i < targetSize; i++) {
        target[i] = (char)(i * 17 + 11);
    }
    
    size_t diffBound = ZSTD_getDiffBound(targetSize, sourceSize);
    void* diffBuff = malloc(diffBound);
    assert(diffBuff != NULL);
    
    size_t diffSize = ZSTD_createDiff(
        diffBuff, diffBound,
        target, targetSize,
        source, sourceSize,
        5
    );
    
    TEST(!ZSTD_isError(diffSize), "Create diff for large data");
    
    void* reconstructed = malloc(targetSize);
    assert(reconstructed != NULL);
    
    size_t reconstructedSize = ZSTD_applyDiff(
        reconstructed, targetSize,
        diffBuff, diffSize,
        source, sourceSize
    );
    
    TEST(!ZSTD_isError(reconstructedSize), "Apply diff for large data");
    TEST(reconstructedSize == targetSize, "Reconstructed size matches for large data");
    TEST(memcmp(reconstructed, target, targetSize) == 0, "Reconstructed content matches for large data");
    
    free(source);
    free(target);
    free(diffBuff);
    free(reconstructed);
}

static void test_compression_levels(void)
{
    const char* source = "Source data for testing compression levels.";
    const char* target = "Target data for testing compression levels!";
    size_t sourceSize = strlen(source);
    size_t targetSize = strlen(target);
    
    /* Test different compression levels */
    int levels[] = {1, 3, 9, 19, 22};
    for (int i = 0; i < 5; i++) {
        int level = levels[i];
        
        size_t diffBound = ZSTD_getDiffBound(targetSize, sourceSize);
        void* diffBuff = malloc(diffBound);
        assert(diffBuff != NULL);
        
        size_t diffSize = ZSTD_createDiff(
            diffBuff, diffBound,
            target, targetSize,
            source, sourceSize,
            level
        );
        
        char msg[100];
        snprintf(msg, sizeof(msg), "Create diff with compression level %d", level);
        TEST(!ZSTD_isError(diffSize), msg);
        
        void* reconstructed = malloc(targetSize);
        assert(reconstructed != NULL);
        
        size_t reconstructedSize = ZSTD_applyDiff(
            reconstructed, targetSize,
            diffBuff, diffSize,
            source, sourceSize
        );
        
        snprintf(msg, sizeof(msg), "Apply diff with compression level %d", level);
        TEST(!ZSTD_isError(reconstructedSize) && 
             reconstructedSize == targetSize &&
             memcmp(reconstructed, target, targetSize) == 0, msg);
        
        free(diffBuff);
        free(reconstructed);
    }
}

int main(void)
{
    printf("=== ZSTD Diff API Tests ===\n\n");
    
    test_basic_diff();
    test_empty_files();
    test_identical_files();
    test_large_data();
    test_compression_levels();
    
    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", g_testsPassed);
    printf("Failed: %d\n", g_testsFailed);
    
    return g_testsFailed > 0 ? 1 : 0;
}
