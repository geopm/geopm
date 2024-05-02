/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_PLUGIN_HPP_INCLUDE
#define GEOPM_PLUGIN_HPP_INCLUDE

namespace geopm
{
    void __attribute__((visibility("default"))) plugin_load(const std::string &plugin_prefix);
    void __attribute__((visibility("default"))) plugin_reset(void);
}

#endif
