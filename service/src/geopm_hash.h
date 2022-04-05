/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
#ifndef GEOPM_HASH_H_INCLUDE
#define GEOPM_HASH_H_INCLUDE

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif


/***************************/
/* APPLICATION REGION HASH */
/***************************/
enum geopm_region_hash_e {
    GEOPM_REGION_HASH_INVALID  = 0x0ULL,
    GEOPM_REGION_HASH_UNMARKED = 0x725e8066ULL, /* Note the value is the geopm_crc32_str() of the stringified enum */
    GEOPM_U64_SENTINEL_REGION_HASH = UINT64_MAX, /* Force enum type to uint64_t */
};

/*!
 * @brief Enum for internally defined region hashes.
 * Note the value is the geopm_crc32_str() of the stringified enum
 */
enum geopm_region_hash_epoch_e {
    GEOPM_REGION_HASH_EPOCH = 0x66c91423ULL,
    GEOPM_REGION_HASH_APP = 0x9d540c53ULL,
};


/// @brief Implements the CRC32 hashing algorithm
///
/// @param [in] begin Algorithm starts with this value
///
/// @param [in] key This value is hashed to produce a 32-bit result.
///
/// @return uint64_t The result is returned as a 64-bit integer.
uint64_t geopm_crc32_u64(uint64_t begin, uint64_t key);

/// @brief This function is used to produce unique region IDs for
///        named regions.
///
/// @details An Agent implementation with specialized behavior for
///          specific region names can use this function to figure out
///          the region ID to expect for the desired region.  As this
///          uses the CRC32 algorithm, only the bottom 32 bits will be
///          filled in, reserving the top 32 bits for hints and other
///          information.
///
/// @param [in] key This string is hashed to produce a 64-bit value.
///
/// @return uint64_t The result is returned as a 64-bit integer.
uint64_t geopm_crc32_str(const char *key);

#ifdef __cplusplus
}
#endif

#endif
