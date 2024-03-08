/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"
#include "geopm_hash.h"

#include <string.h>
#include <zlib.h>

#include "geopm/Helper.hpp"


static uint64_t _crc32(uint64_t begin, uint64_t key)
{
    uint32_t key_32 = key;
    return ::crc32(begin, reinterpret_cast<unsigned char*>(&key_32), sizeof(key_32));
}

static uint64_t _crc32(const char *key)
{
    return ::crc32(0, reinterpret_cast<const unsigned char*>(key), strlen(key));
}

namespace geopm
{
    uint64_t hash(const std::string &key)
    {
        return _crc32(key.c_str());
    }
}

uint64_t geopm_crc32_str(const char *key)
{
    static geopm::DeprecationWarning warn(__func__, "use geopm_hash_str() instead");
    return _crc32(key);
}


uint64_t geopm_crc32_u64(uint64_t begin, uint64_t key)
{
    static geopm::DeprecationWarning warn(__func__, "consider crc32() defined in zlib.h provided by libz");
    return _crc32(begin, key);
}

uint64_t geopm_hash_str(const char *key)
{
    return _crc32(key);
}
