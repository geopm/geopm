/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_PLUGIN_HPP_INCLUDE
#define GEOPM_PLUGIN_HPP_INCLUDE

#include <string>

#include "geopm_public.h"

namespace geopm
{
    void GEOPM_PUBLIC
        plugin_load(const std::string &plugin_prefix);
    void GEOPM_PUBLIC
        plugin_reset(void);
}

#endif
