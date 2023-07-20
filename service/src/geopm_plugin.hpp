/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_PLUGIN_HPP_INCLUDE
#define GEOPM_PLUGIN_HPP_INCLUDE

namespace geopm
{
    void plugin_load(const std::string &plugin_prefix);
    void plugin_reset(void);
}

#endif
