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
 * zstd-diff: command line tool for creating and applying zstd compressed diffs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include "timefn.h"

#define ZSTD_STATIC_LINKING_ONLY
#include "zstd.h"

static const char* g_programName = "zstd-diff";

static void usage(void)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  Create diff:\n");
    fprintf(stderr, "    %s create SOURCE TARGET DIFF [-l LEVEL]\n", g_programName);
    fprintf(stderr, "      Creates compressed diff from SOURCE to TARGET\n");
    fprintf(stderr, "      -l : compression level (1-22, default: 3)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Apply diff:\n");
    fprintf(stderr, "    %s apply SOURCE DIFF OUTPUT\n", g_programName);
    fprintf(stderr, "      Applies DIFF to SOURCE to create OUTPUT\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Help:\n");
    fprintf(stderr, "    %s -h | --help\n", g_programName);
}

static int createDiff(const char* sourcePath, const char* targetPath, 
                      const char* diffPath, int compressionLevel)
{
    size_t sourceSize, targetSize;
    void *sourceBuff = NULL, *targetBuff = NULL, *diffBuff = NULL;
    FILE* diffFile = NULL;
    int result = 1;
    UTIL_time_t const timeStart = UTIL_getTime();
    
    /* Load source file */
    {
        U64 const size64 = UTIL_getFileSize(sourcePath);
        if (size64 == UTIL_FILESIZE_UNKNOWN) {
            fprintf(stderr, "Error: cannot determine size of source file '%s'\n", sourcePath);
            goto cleanup;
        }
        sourceSize = (size_t)size64;
        sourceBuff = malloc(sourceSize);
        if (!sourceBuff) {
            fprintf(stderr, "Error: not enough memory for source file\n");
            goto cleanup;
        }
        {
            FILE* f = fopen(sourcePath, "rb");
            if (!f) {
                fprintf(stderr, "Error: cannot open source file '%s': %s\n", 
                        sourcePath, strerror(errno));
                goto cleanup;
            }
            if (fread(sourceBuff, 1, sourceSize, f) != sourceSize) {
                fprintf(stderr, "Error: cannot read source file\n");
                fclose(f);
                goto cleanup;
            }
            fclose(f);
        }
    }
    
    /* Load target file */
    {
        U64 const size64 = UTIL_getFileSize(targetPath);
        if (size64 == UTIL_FILESIZE_UNKNOWN) {
            fprintf(stderr, "Error: cannot determine size of target file '%s'\n", targetPath);
            goto cleanup;
        }
        targetSize = (size_t)size64;
        targetBuff = malloc(targetSize);
        if (!targetBuff) {
            fprintf(stderr, "Error: not enough memory for target file\n");
            goto cleanup;
        }
        {
            FILE* f = fopen(targetPath, "rb");
            if (!f) {
                fprintf(stderr, "Error: cannot open target file '%s': %s\n", 
                        targetPath, strerror(errno));
                goto cleanup;
            }
            if (fread(targetBuff, 1, targetSize, f) != targetSize) {
                fprintf(stderr, "Error: cannot read target file\n");
                fclose(f);
                goto cleanup;
            }
            fclose(f);
        }
    }
    
    /* Create diff */
    {
        size_t const diffBound = ZSTD_getDiffBound(targetSize, sourceSize);
        diffBuff = malloc(diffBound);
        if (!diffBuff) {
            fprintf(stderr, "Error: not enough memory for diff buffer\n");
            goto cleanup;
        }
        
        size_t const diffSize = ZSTD_createDiff(
            diffBuff, diffBound,
            targetBuff, targetSize,
            sourceBuff, sourceSize,
            compressionLevel
        );
        
        if (ZSTD_isError(diffSize)) {
            fprintf(stderr, "Error creating diff: %s\n", ZSTD_getErrorName(diffSize));
            goto cleanup;
        }
        
        /* Save diff to file */
        diffFile = fopen(diffPath, "wb");
        if (!diffFile) {
            fprintf(stderr, "Error: cannot create diff file '%s': %s\n", 
                    diffPath, strerror(errno));
            goto cleanup;
        }
        
        if (fwrite(diffBuff, 1, diffSize, diffFile) != diffSize) {
            fprintf(stderr, "Error: cannot write diff file\n");
            goto cleanup;
        }
        
        {
            double const elapsed = UTIL_clockSpanMicro(timeStart) / 1000000.0;
            double const ratio = (sourceSize > 0) 
                ? (double)diffSize / sourceSize * 100.0 
                : 0.0;
            
            fprintf(stderr, "Created diff: %s (%zu bytes, %.2f%% of source, %.2fs)\n",
                    diffPath, diffSize, ratio, elapsed);
            fprintf(stderr, "  Source: %zu bytes\n", sourceSize);
            fprintf(stderr, "  Target: %zu bytes\n", targetSize);
        }
        
        result = 0;
    }
    
cleanup:
    if (diffFile) fclose(diffFile);
    free(sourceBuff);
    free(targetBuff);
    free(diffBuff);
    return result;
}

static int applyDiff(const char* sourcePath, const char* diffPath, 
                     const char* outputPath)
{
    size_t sourceSize, diffSize;
    void *sourceBuff = NULL, *diffBuff = NULL, *outputBuff = NULL;
    FILE* outputFile = NULL;
    int result = 1;
    UTIL_time_t const timeStart = UTIL_getTime();
    
    /* Load source file */
    {
        U64 const size64 = UTIL_getFileSize(sourcePath);
        if (size64 == UTIL_FILESIZE_UNKNOWN) {
            fprintf(stderr, "Error: cannot determine size of source file '%s'\n", sourcePath);
            goto cleanup;
        }
        sourceSize = (size_t)size64;
        sourceBuff = malloc(sourceSize);
        if (!sourceBuff) {
            fprintf(stderr, "Error: not enough memory for source file\n");
            goto cleanup;
        }
        {
            FILE* f = fopen(sourcePath, "rb");
            if (!f) {
                fprintf(stderr, "Error: cannot open source file '%s': %s\n", 
                        sourcePath, strerror(errno));
                goto cleanup;
            }
            if (fread(sourceBuff, 1, sourceSize, f) != sourceSize) {
                fprintf(stderr, "Error: cannot read source file\n");
                fclose(f);
                goto cleanup;
            }
            fclose(f);
        }
    }
    
    /* Load diff file */
    {
        U64 const size64 = UTIL_getFileSize(diffPath);
        if (size64 == UTIL_FILESIZE_UNKNOWN) {
            fprintf(stderr, "Error: cannot determine size of diff file '%s'\n", diffPath);
            goto cleanup;
        }
        diffSize = (size_t)size64;
        diffBuff = malloc(diffSize);
        if (!diffBuff) {
            fprintf(stderr, "Error: not enough memory for diff file\n");
            goto cleanup;
        }
        {
            FILE* f = fopen(diffPath, "rb");
            if (!f) {
                fprintf(stderr, "Error: cannot open diff file '%s': %s\n", 
                        diffPath, strerror(errno));
                goto cleanup;
            }
            if (fread(diffBuff, 1, diffSize, f) != diffSize) {
                fprintf(stderr, "Error: cannot read diff file\n");
                fclose(f);
                goto cleanup;
            }
            fclose(f);
        }
    }
    
    /* Apply diff - need to determine output size first */
    /* For now, use a heuristic buffer size. In a future enhancement,
     * the target size could be read from the diff header. */
    {
        /* Heuristic: assume target is no more than 10x source size + 1MB overhead */
        const size_t SIZE_MULTIPLIER = 10;
        const size_t SIZE_OVERHEAD = 1024*1024;
        size_t const maxOutputSize = sourceSize * SIZE_MULTIPLIER + SIZE_OVERHEAD;
        outputBuff = malloc(maxOutputSize);
        if (!outputBuff) {
            fprintf(stderr, "Error: not enough memory for output buffer\n");
            goto cleanup;
        }
        
        size_t const outputSize = ZSTD_applyDiff(
            outputBuff, maxOutputSize,
            diffBuff, diffSize,
            sourceBuff, sourceSize
        );
        
        if (ZSTD_isError(outputSize)) {
            fprintf(stderr, "Error applying diff: %s\n", ZSTD_getErrorName(outputSize));
            goto cleanup;
        }
        
        /* Save output to file */
        outputFile = fopen(outputPath, "wb");
        if (!outputFile) {
            fprintf(stderr, "Error: cannot create output file '%s': %s\n", 
                    outputPath, strerror(errno));
            goto cleanup;
        }
        
        if (fwrite(outputBuff, 1, outputSize, outputFile) != outputSize) {
            fprintf(stderr, "Error: cannot write output file\n");
            goto cleanup;
        }
        
        {
            double const elapsed = UTIL_clockSpanMicro(timeStart) / 1000000.0;
            fprintf(stderr, "Applied diff: %s (%zu bytes, %.2fs)\n",
                    outputPath, outputSize, elapsed);
            fprintf(stderr, "  Source: %zu bytes\n", sourceSize);
            fprintf(stderr, "  Diff:   %zu bytes\n", diffSize);
        }
        
        result = 0;
    }
    
cleanup:
    if (outputFile) fclose(outputFile);
    free(sourceBuff);
    free(diffBuff);
    free(outputBuff);
    return result;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        usage();
        return 1;
    }
    
    g_programName = argv[0];
    
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        usage();
        return 0;
    }
    
    if (strcmp(argv[1], "create") == 0) {
        int compressionLevel = 3;  /* default */
        
        if (argc < 5) {
            fprintf(stderr, "Error: 'create' requires SOURCE, TARGET, and DIFF arguments\n");
            usage();
            return 1;
        }
        
        /* Parse optional compression level */
        if (argc >= 7 && strcmp(argv[5], "-l") == 0) {
            char* endptr;
            long level = strtol(argv[6], &endptr, 10);
            if (*endptr != '\0' || level < 1 || level > 22) {
                fprintf(stderr, "Error: compression level must be between 1 and 22\n");
                return 1;
            }
            compressionLevel = (int)level;
        }
        
        return createDiff(argv[2], argv[3], argv[4], compressionLevel);
    }
    
    if (strcmp(argv[1], "apply") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Error: 'apply' requires SOURCE, DIFF, and OUTPUT arguments\n");
            usage();
            return 1;
        }
        
        return applyDiff(argv[2], argv[3], argv[4]);
    }
    
    fprintf(stderr, "Error: unknown command '%s'\n", argv[1]);
    usage();
    return 1;
}
