/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCHEDULER_HPP_INCLUDE
#define SCHEDULER_HPP_INCLUDE

#include <memory>
#include <set>
#include <functional>
#include <sched.h>

namespace geopm
{
    std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
        make_cpu_set(int num_cpu, const std::set<int> &cpu_enabled);

    // TODO: Add a "Scheduler" class that provides a mockable
    // abstraction to the Linux sched_* interfaces in place of
    // geopm_sched_*().
}

#endif
