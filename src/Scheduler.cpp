/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "Scheduler.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_sched.h"

namespace geopm
{
    std::unique_ptr<Scheduler> Scheduler::make_unique(void)
    {
        return std::make_unique<SchedulerImp>();
    }

    SchedulerImp::SchedulerImp()
        : m_num_cpu(num_cpu())
    {

    }

    int SchedulerImp::num_cpu(void) const
    {
        return geopm_sched_num_cpu();
    }

    int SchedulerImp::get_cpu(void) const
    {
        return geopm_sched_get_cpu();
    }

    std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
        SchedulerImp::proc_cpuset(void) const
    {
        auto result = make_cpu_set(m_num_cpu, {});
        int err = geopm_sched_proc_cpuset(m_num_cpu, result.get());
        if (err != 0) {
            throw Exception("geopm_sched_proc_cpuset() failed",
                            err, __FILE__, __LINE__);
        }
        return result;
    }

    std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
        SchedulerImp::proc_cpuset(int pid) const
    {
        auto result = make_cpu_set(m_num_cpu, {});
        int err = geopm_sched_proc_cpuset_pid(pid, m_num_cpu, result.get());
        if (err != 0) {
            throw Exception("geopm_sched_proc_cpuset() failed",
                            err, __FILE__, __LINE__);
        }
        return result;
    }

    std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
        SchedulerImp::woomp(int pid) const
    {
        auto result = make_cpu_set(m_num_cpu, {});
        int err = geopm_sched_woomp(m_num_cpu, result.get());
        if (err != 0) {
            throw Exception("geopm_sched_proc_cpuset() failed",
                            err, __FILE__, __LINE__);
        }
        return result;
    }
}
