/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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
#include "Agg.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    PowerBalancer::PowerBalancer(double ctl_latency)
        : PowerBalancer(ctl_latency, 0.125, 5, 0.25)
    {

    }

    PowerBalancer::PowerBalancer(double ctl_latency, double trial_delta, int num_sample, double measure_duration)
        : M_CONTROL_LATENCY(ctl_latency)
        , M_MIN_TRIAL_DELTA(trial_delta)
        , M_MIN_NUM_SAMPLE(num_sample)
        , M_MIN_DURATION(measure_duration)
        , M_RUNTIME_FRACTION(0.02)
        , m_num_sample(0)
        , m_power_cap(NAN)
        , m_power_limit(NAN)
        , m_power_limit_change_time(GEOPM_TIME_REF)
        , m_target_runtime(NAN)
        , m_trial_delta(8.0)
        , m_runtime_sample(NAN)
        , m_is_target_met(false)
        , m_runtime_buffer(make_unique<CircularBuffer<double> >(0))
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

    void PowerBalancer::power_limit_adjusted(double actual_limit)
    {
        // m_power_limit starts as the requested limit.  actual limit is what the governor returned.
        if (actual_limit > m_power_limit) {
            // we hit the minimum, so stop lowering
            m_is_target_met = true;
        }

        if (m_power_limit != actual_limit) {
            geopm_time(&m_power_limit_change_time);
            m_power_limit = actual_limit;
            m_runtime_buffer->clear();
        }
    }

    double PowerBalancer::power_limit(void) const
    {
        return m_power_limit;
    }

    bool PowerBalancer::is_limit_stable(void)
    {
        return (geopm_time_since(&m_power_limit_change_time) > M_CONTROL_LATENCY);
    }

    bool PowerBalancer::is_runtime_stable(double measured_runtime)
    {
        bool result = false;
        bool is_stable = is_limit_stable() && !std::isnan(measured_runtime);
        if (is_stable && m_runtime_buffer->size() == 0) {
            m_runtime_vec.push_back(measured_runtime);
            if (Agg::sum(m_runtime_vec) > M_MIN_DURATION) {
                m_num_sample = m_runtime_vec.size();
                if (m_num_sample < M_MIN_NUM_SAMPLE) {
                    m_num_sample = M_MIN_NUM_SAMPLE;
                }
                else {
                    result = true;
                }
                m_runtime_buffer->set_capacity(m_num_sample);
                for (auto it : m_runtime_vec) {
                    m_runtime_buffer->insert(it);
                }
                m_runtime_vec.resize(0);
            }
        }
        else if (is_stable) {
            m_runtime_buffer->insert(measured_runtime);
            if (m_runtime_buffer->size() == m_runtime_buffer->capacity()) {
                result = true;
            }
        }
        return result;
    }

    double PowerBalancer::runtime_sample(void) const
    {
        return m_runtime_sample;
    }

    void PowerBalancer::calculate_runtime_sample(void)
    {
        if (m_runtime_buffer->size() != 0) {
            m_runtime_sample = Agg::median(m_runtime_buffer->make_vector());
        }
        else {
            m_runtime_sample = Agg::median(m_runtime_vec);
        }
    }

    void PowerBalancer::target_runtime(double largest_runtime)
    {
        m_target_runtime = largest_runtime * (1 - M_RUNTIME_FRACTION);
        if (m_runtime_sample > m_target_runtime) {
            m_is_target_met = true;
        }
        else {
            m_is_target_met = false;
        }
    }

    bool PowerBalancer::is_target_met(double measured_runtime)
    {
#ifdef GEOPM_DEBUG
        if (std::isnan(measured_runtime)) {
            throw Exception("PowerBalancer::" + std::string(__func__) +
                            "Encountered NAN for sampled epoch runtime.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (!m_is_target_met &&
            is_runtime_stable(measured_runtime)) {
            if (m_runtime_sample > m_target_runtime) {
                if (m_power_limit < m_power_cap) {
                    m_power_limit += m_trial_delta;
                    if (m_power_limit > m_power_cap) {
                        m_power_limit = m_power_cap;
                    }
                }
                m_is_target_met = true;
            }
            else {
                m_power_limit -= m_trial_delta;
            }
            m_runtime_buffer->clear();
        }
        return m_is_target_met;
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
