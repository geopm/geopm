/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"
#include "geopm_hash.h"

#include <string.h>
#include <iostream>

namespace geopm
{
    uint64_t hash(const std::string &key)
    {
        static std::hash<std::string> hasher;
        return hasher(key) & 0xFFFFFFFF;
    }

    class DeprecationWarning
    {
        public:
            DeprecationWarning() = delete;
            DeprecationWarning(const std::string &function, const std::string &message) {
                std::cerr << "Warning: " << function << " has been deprecated: " << message << ".\n";
            }
            virtual ~DeprecationWarning() = default;
    };
}

uint64_t geopm_crc32_str(const char *key)
{
    static geopm::DeprecationWarning warn(__func__, "use geopm_hash_str() instead");
    return geopm_hash_str(key);
}


uint64_t geopm_crc32_u64(uint64_t begin, uint64_t key)
{
    static geopm::DeprecationWarning warn(__func__, "consider std::hash<uint64_t> or https://github.com/memcached/memcached/blob/master/crc32c.h as a possible replacement");
    static std::hash<uint64_t> hasher;
    return hasher(key) & 0xFFFFFFFF;
}

uint64_t geopm_hash_str(const char *key)
{
    return geopm::hash(key);
}
