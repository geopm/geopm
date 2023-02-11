/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "Scheduler.hpp"

namespace geopm
{
    std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
        make_cpu_set(int num_cpu, const std::set<int> &cpu_enabled)
    {
        if (num_cpu < 128) {
            num_cpu = 128;
        }
        std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> > result(
            CPU_ALLOC(num_cpu),
            [](cpu_set_t *ptr)
            {
                CPU_FREE(ptr);
            });

        auto enabled_it = cpu_enabled.cbegin();
        for (int cpu_idx = 0; cpu_idx != num_cpu; ++cpu_idx) {
            if (enabled_it != cpu_enabled.cend() &&
                *enabled_it == cpu_idx) {
                CPU_SET(cpu_idx, result.get());
                ++enabled_it;
            }
            else {
                CPU_CLR(cpu_idx, result.get());
            }
        }
        return result;
    }
}
