/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <vector>
#include <cmath>

#include "PowerBalancer.hpp"
#include "CircularBuffer.hpp"
#include "PlatformIO.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    PowerBalancer::PowerBalancer()
        : M_MIN_TRIAL_DELTA(0.125)
        , M_NUM_SAMPLE(5)
        , m_power_cap(NAN)
        , m_power_limit(NAN)
        , m_target_runtime(NAN)
        , m_trial_delta(8.0)
        , m_runtime_sample(NAN)
        , m_is_target_met(false)
        , m_runtime_buffer(make_unique<CircularBuffer<double> >(M_NUM_SAMPLE))
    {

    }

    void PowerBalancer::power_cap(double cap)
    {
        m_power_limit = cap;
        m_power_cap = cap;
        m_runtime_buffer->clear();
        m_target_runtime = NAN;
    }

    double PowerBalancer::power_cap(void) const
    {
        return m_power_cap;
    }

    double PowerBalancer::power_limit(void) const
    {
        return m_power_limit;
    }

    bool PowerBalancer::is_runtime_stable(double measured_runtime)
    {
        bool result = false;
        m_runtime_buffer->insert(measured_runtime);
        if (m_runtime_buffer->size() == m_runtime_buffer->capacity()) {
            result = true;
        }
        return result;
    }

    double PowerBalancer::runtime_sample(void)
    {
        m_runtime_sample = IPlatformIO::agg_median(m_runtime_buffer->make_vector());
        return m_runtime_sample;
    }

    void PowerBalancer::target_runtime(double largest_runtime)
    {
        m_target_runtime = largest_runtime;
        if (m_runtime_sample == largest_runtime) {
            m_is_target_met = true;
        }
        else {
            m_is_target_met = false;
        }
    }

    bool PowerBalancer::is_target_met(double measured_runtime)
    {
        if (!m_is_target_met) {
            if (is_runtime_stable(measured_runtime)) {
                if (runtime_sample() > m_target_runtime) {
                    if (m_power_limit != m_power_cap) {
                        m_power_limit += m_trial_delta;
                    }
                    m_is_target_met = true;
                }
                else {
                    m_power_limit -= m_trial_delta;
                }
                m_runtime_buffer->clear();
            }
        }
        return m_is_target_met;
    }

    void PowerBalancer::achieved_limit(double achieved)
    {
        if (!std::isnan(m_target_runtime) &&
            achieved > m_power_limit + m_trial_delta) {
            int num_delta = (achieved - m_power_limit) / m_trial_delta;
            m_power_limit += num_delta * m_trial_delta;
            if (m_power_limit > m_power_cap) {
                m_power_limit = m_power_cap;
            }
            m_runtime_buffer->clear();
            m_is_target_met = true;
        }
    }

    double PowerBalancer::power_slack(void)
    {
        double result = m_power_cap - m_power_limit;
        if (result == 0.0) {
            m_trial_delta /= 2.0;
            if (m_trial_delta < M_MIN_TRIAL_DELTA) {
                m_trial_delta = M_MIN_TRIAL_DELTA;
            }
        }
        return result;
    }
}
