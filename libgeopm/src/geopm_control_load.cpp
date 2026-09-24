/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "InitControl.hpp"
#include "geopm/Environment.hpp"

namespace geopm
{
    static void __attribute__((constructor)) geopm_init_control_constructor(void)
    {
        if (environment().do_init_control()) {
            auto init_control = InitControl::make_unique();
            init_control->parse_input(environment().init_control());
        }
    }
}
