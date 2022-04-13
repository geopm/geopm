/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include <string.h>
#ifdef GEOPM_HAS_SSE42
#include <smmintrin.h>
#else
#include <stdio.h>
#endif

#include "geopm_hash.h"

#ifdef __cplusplus
extern "C"
{
#endif

uint64_t geopm_crc32_u64(uint64_t begin, uint64_t key)
{
    uint64_t result = 0;
#ifdef GEOPM_HAS_SSE42
    result = _mm_crc32_u64(begin, key);
#else
#ifdef GEOPM_DEBUG
    static int is_once = 1;
    if (is_once) {
        fprintf(stderr, "Warning: <geopm> geopm_crc32_u64() called, but was compiled without SSE4.2 support, returning 0.\n");
        is_once = 0;
    }
#endif
#endif
    return result;
}

uint64_t geopm_crc32_str(const char *key)
{
    uint64_t result = 0;
    const uint64_t *ptr = (const uint64_t *)key;
    size_t num_word = strlen(key) / 8;
    for (size_t i = 0; i < num_word; ++i) {
        result = geopm_crc32_u64(result, ptr[i]);
    }
    size_t extra = strlen(key) - 8 * num_word;
    if (extra) {
        uint64_t last_word = 0;
        for (size_t i = 0; i < extra; ++i) {
            ((char *)(&last_word))[i] = ((char *)(ptr + num_word))[i];
        }
        result = geopm_crc32_u64(result, last_word);
    }
    return result;
}

#ifdef __cplusplus
}
#endif
