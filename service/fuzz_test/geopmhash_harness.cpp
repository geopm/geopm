/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdint>
#include <iostream>
#include <vector>

#include "geopm_hash.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    std::vector<char> input(data, data + size);
    input.push_back('\0');
    (void)!geopm_hash_str(input.data());
    return 0;
}
