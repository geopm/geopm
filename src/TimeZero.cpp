/*
 * Copyright (c) 2015 - 2021, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "geopm_time.h"
#include "Exception.hpp"

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
