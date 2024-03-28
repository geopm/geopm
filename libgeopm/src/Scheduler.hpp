/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCHEDULER_HPP_INCLUDE
#define SCHEDULER_HPP_INCLUDE

#include <memory>
#include <functional>
#include <sched.h>

namespace geopm
{
    class Scheduler
    {
        public:
            Scheduler() = default;
            virtual ~Scheduler() = default;
            static std::unique_ptr<Scheduler> make_unique(void);
            virtual int num_cpu(void) const = 0;
            virtual int get_cpu(void) const = 0;
            virtual std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
                proc_cpuset(void) const = 0;
            virtual std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
                proc_cpuset(int pid) const = 0;
            virtual std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
                woomp(int pid) const = 0;
    };

    class SchedulerImp : public Scheduler
    {
        public:
            SchedulerImp();
            virtual ~SchedulerImp() = default;
            virtual int num_cpu(void) const override;
            virtual int get_cpu(void) const override;
            virtual std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
                proc_cpuset(void) const override;
            virtual std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
                proc_cpuset(int pid) const override;
            virtual std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
                woomp(int pid) const override;
        private:
            const int m_num_cpu;
    };
}

#endif
