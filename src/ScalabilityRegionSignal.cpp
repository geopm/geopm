/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "ScalabilityRegionSignal.hpp"

#include "Exception.hpp"
#include "Helper.hpp"
#include "geopm_debug.hpp"

#include <math.h>
#include <unistd.h>

namespace geopm
{
    ScalabilityRegionSignal::ScalabilityRegionSignal(std::shared_ptr<Signal> scalability_sig, std::shared_ptr<Signal> time_sig,
                                         double range_upper, double range_lower,
                                         double sleep_time)
        : m_scalability(scalability_sig)
        , m_time(time_sig)
        , m_range_upper(range_upper)
        , m_range_lower(range_lower)
        , m_sleep_time(sleep_time)
        , m_is_batch_ready(false)
    {
        GEOPM_DEBUG_ASSERT(m_scalability && m_time,
                           "Signal pointers for scalability and time cannot be null.");
    }

    void ScalabilityRegionSignal::setup_batch(void)
    {
        if (!m_is_batch_ready) {
            m_scalability->setup_batch();
            m_time->setup_batch();
            m_is_batch_ready = true;
        }
    }

    double ScalabilityRegionSignal::compute_region_time(double scalability,
                               double curr_time,
                               double prev_time,
                               double upper,
                               double lower) {
        double result = 0;
        if (scalability < upper
            && scalability >= lower
            && !isnan(scalability)) {
            result = curr_time - prev_time;
        }

        return result;
    }

    double ScalabilityRegionSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        double scalability = m_scalability->sample();
        double curr_time = m_time->sample();

        m_region_time += compute_region_time(scalability,
                                             curr_time,
                                             m_prev_time,
                                             m_range_upper,
                                             m_range_lower);
        m_prev_time = curr_time;

        return m_region_time;
    }

    double ScalabilityRegionSignal::read(void) const
    {
        double result = 0;
        double prev_time = m_time->read();
        usleep(m_sleep_time * 1e6);
        double scalability = m_scalability->read();
        double curr_time = m_time->read();

        result = compute_region_time(scalability,
                                     curr_time,
                                     prev_time,
                                     m_range_upper,
                                     m_range_lower);

        return result;
    }
}
