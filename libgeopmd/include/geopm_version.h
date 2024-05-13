/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_VERSION_H_INCLUDE
#define GEOPM_VERSION_H_INCLUDE

#include "geopm_public.h"

#ifdef __cplusplus
extern "C" {
#endif

const char GEOPM_PUBLIC *
    geopm_version(void);

#ifdef __cplusplus
} // End extern "C"

#include <vector>
#include <string>

namespace geopm {
    std::string GEOPM_PUBLIC
        version(void);
    std::vector<int> GEOPM_PUBLIC
        shared_object_version(void);
}

#endif // End C++
#endif // Include guard
