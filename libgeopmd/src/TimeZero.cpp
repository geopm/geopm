/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <string>

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
            struct geopm_time_s time(void) const;
            int error(void) const;
            void reset(const geopm_time_s &zero);
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

    void TimeZero::reset(const geopm_time_s &zero)
    {
        m_time_zero = zero;
        m_err = 0;
    }

    struct geopm_time_s time_zero(void)
    {
        if (TimeZero::time_zero().error()) {
            throw Exception("geopm::time_zero() call to get time failed",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return TimeZero::time_zero().time();
    }

    void time_zero_reset(const geopm_time_s &zero)
    {
        TimeZero::time_zero().reset(zero);
    }

    struct geopm_time_s time_curr(void)
    {
        struct geopm_time_s result;
        geopm_time(&result);
        return result;
    }

    struct geopm_time_s time_curr_real(void)
    {
        struct geopm_time_s result;
        geopm_time_real(&result);
        return result;
    }

    std::string time_curr_string(void)
    {
        char result[GEOPM_TIME_STRING_MAX];
        int err = geopm_time_string(GEOPM_TIME_STRING_MAX, result);
        if (err != 0) {
            throw Exception("geopm_time_to_string() call failed",
                            err, __FILE__, __LINE__);
        }
        return result;
    }
}

extern "C"
{
    int geopm_time_zero(struct geopm_time_s *time)
    {
        *time = geopm::TimeZero::time_zero().time();
        return geopm::TimeZero::time_zero().error();
    }

    int geopm_time_real_to_iso_string(const struct geopm_time_s *time, int buf_size, char *buf)
    {
        if (buf_size == 0) {
            return EINVAL;
        }
        int err = 0;
        size_t buf_size_u = buf_size;
        char *buf_ptr = buf;
        struct tm local;
        localtime_r(&(time->t.tv_sec), &local);
        size_t buf_off = strftime(buf_ptr, buf_size_u, "%FT%T", &local);
        if (buf_off == 0 || buf_off >= buf_size_u) {
            err = EINVAL;
        }
        else {
            buf_size_u -= buf_off;
            buf_ptr += buf_off;
            int buf_off_s;
            int buf_size_s = buf_size_u;
            buf_off_s = snprintf(buf_ptr, buf_size_u, ".%.9ld", time->t.tv_nsec);
            if (buf_off_s < 0 || buf_off_s >= buf_size_s) {
                err = EINVAL;
            }
            buf_off = buf_off_s;
        }
        if (err == 0) {
            buf_size_u -= buf_off;
            buf_ptr += buf_off;
            buf_off = strftime(buf_ptr, buf_size_u, "%z", &local);
            if (buf_off == 0) {
                err = EINVAL;
            }
        }
        return err;
    }
}
