/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "geopm_time.h"
#include "geopm/Exception.hpp"

namespace geopm
{
    class TimeZero
    {
        public:
            TimeZero();
            virtual ~TimeZero() = default;
            static TimeZero &time_zero(void);
            geopm_time_s time(void) const;
            int error(void) const;
            void reset(const geopm_time_s &time_zero);
        private:
            struct geopm_time_s m_time_zero;
            int m_err;
    };

    TimeZero::TimeZero()
    {
        m_err = geopm_time(&m_time_zero);
    }

    TimeZero &TimeZero::time_zero(void)
    {
        static geopm::TimeZero instance;
        return instance;
    }

    struct geopm_time_s TimeZero::time(void) const
    {
        return m_time_zero;
    }

    int TimeZero::error(void) const
    {
        return m_err;
    }

    void TimeZero::reset(const geopm_time_s &time_zero)
    {
        m_time_zero = time_zero;
    }

    struct geopm_time_s time_zero(void)
    {
        if (TimeZero::time_zero().error()) {
            throw Exception("geopm::time_zero() call to get time failed",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return TimeZero::time_zero().time();
    }
    void time_zero_reset(const geopm_time_s &time_zero)
    {
        TimeZero::time_zero().reset(time_zero);
    }
}

extern "C"
{
    int geopm_time_zero(struct geopm_time_s *time)
    {
        *time = geopm::TimeZero::time_zero().time();
        return geopm::TimeZero::time_zero().error();
    }
}
