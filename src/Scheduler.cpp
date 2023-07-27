/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "Scheduler.hpp"
#include "geopm/Exception.hpp"
#include "geopm_sched.h"

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
