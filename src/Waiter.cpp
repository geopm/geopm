/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "Waiter.hpp"

#include <time.h>

#include "geopm/Exception.hpp"
#include "geopm_time.h"


namespace geopm
{
    std::unique_ptr<Waiter> Waiter::make_unique(double period)
    {
        return Waiter::make_unique(period, "sleep");
    }

    std::unique_ptr<Waiter> Waiter::make_unique(double period,
                                                std::string strategy)
    {
        if (strategy == "sleep") {
            return std::make_unique<SleepWaiter>(period);
        }
        else {
            throw Exception("Waiter::make_unique(): Unknown strategy: " + strategy,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    SleepWaiter::SleepWaiter(double period)
        : m_period(period)
    {
        reset();
    }

    void SleepWaiter::reset(void)
    {
        geopm_time_real(&m_time_target);
        geopm_time_add(&m_time_target, m_period, &m_time_target);
    }

    void SleepWaiter::reset(double period)
    {
        m_period = period;
        reset();
    }

    void SleepWaiter::wait(void)
    {
        int err = 0;
        do {
            err = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                                  &(m_time_target.t), nullptr);
        } while(err == EINTR);

        if (err != 0) {
            throw Exception("Waiter::wait(): Failed with error: ",
                            err, __FILE__, __LINE__);
        }
        geopm_time_add(&m_time_target, m_period, &m_time_target);
    }

    double SleepWaiter::period(void) const
    {
        return m_period;
    }
}
