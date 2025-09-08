/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "ProfileIOGroup.hpp"
#include <geopm/Helper.hpp>

namespace geopm
{
    static void __attribute__((constructor, unused)) profile_iogroup_load(void)
    {
        if (!geopm::has_cap_sys_admin()) {
            iogroup_factory().register_plugin(ProfileIOGroup::plugin_name(),
                                              ProfileIOGroup::make_plugin);
        }
    }
}
