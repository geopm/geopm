/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef GEOPM_HASH_H_INCLUDE
#define GEOPM_HASH_H_INCLUDE

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
#include <string>

extern "C"
{
#endif


/***************************/
/* APPLICATION REGION HASH */
/***************************/
enum geopm_region_hash_e {
    GEOPM_REGION_HASH_INVALID  = 0x0ULL,
    GEOPM_REGION_HASH_UNMARKED = 0x725e8066ULL,
    GEOPM_U64_SENTINEL_REGION_HASH = UINT64_MAX, /* Force enum type to uint64_t */
};

/*!
 * @brief Enum for internally defined region hashes.
 */
enum geopm_region_hash_epoch_e {
    GEOPM_REGION_HASH_EPOCH = 0x66c91423ULL,
    GEOPM_REGION_HASH_APP = 0x9d540c53ULL,
};


/// @brief This function is used to produce unique region IDs for
///        named regions.
///
/// @details An Agent implementation with specialized behavior for
///          specific region names can use this function to figure out
///          the region ID to expect for the desired region.  Only the
///          bottom 32 bits will be filled in, reserving the top 32
///          bits for hints and other information.
///
/// @param [in] key This string is hashed to produce a 64-bit value.
///
/// @return uint64_t The result is returned as a 64-bit integer.
uint64_t geopm_hash_str(const char *key);

/// @brief **DEPRECATED** Implements a hashing algorithm (not crc32)
///
/// @param [in] begin Algorithm starts with this value
///
/// @param [in] key This value is hashed to produce a 32-bit result.
///
/// @return uint64_t The result is returned as a 64-bit integer.
uint64_t geopm_crc32_u64(uint64_t begin, uint64_t key);

/// @brief **DEPRECATED** This function is used to produce unique region IDs for
///        named regions (not crc32).
///
/// @details An Agent implementation with specialized behavior for
///          specific region names can use this function to figure out
///          the region ID to expect for the desired region.  Only the
///          bottom 32 bits will be filled in, reserving the top 32
///          bits for hints and other information.
///
/// @param [in] key This string is hashed to produce a 64-bit value.
///
/// @return uint64_t The result is returned as a 64-bit integer.
uint64_t geopm_crc32_str(const char *key);

#ifdef __cplusplus
}

namespace geopm
{
    uint64_t hash(const std::string &key);
}

#endif

#endif
