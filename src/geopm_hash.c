/*
 * Copyright (c) 2015 - 2021, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <smmintrin.h>

#include "geopm_hash.h"
#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif

uint64_t geopm_crc32_u64(uint64_t begin, uint64_t key)
{
    return _mm_crc32_u64(begin, key);
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
