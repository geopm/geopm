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
            static const TimeZero &time_zero(void);
            struct geopm_time_s time(void) const;
            int error(void) const;
        private:
            struct geopm_time_s m_time_zero;
            int m_err;
    };

    TimeZero::TimeZero()
    {
        m_err = geopm_time(&m_time_zero);
    }

    const TimeZero &TimeZero::time_zero(void)
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

    struct geopm_time_s time_zero(void)
    {
        if (TimeZero::time_zero().error()) {
            throw Exception("geopm::time_zero() call to get time failed",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return TimeZero::time_zero().time();
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
