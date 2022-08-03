/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "PowerBalancerImp.hpp"

#include <vector>
#include <cmath>

#include "geopm/CircularBuffer.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "config.h"

namespace geopm
{
    std::unique_ptr<PowerBalancer> PowerBalancer::make_unique(double ctl_latency)
    {
        return geopm::make_unique<PowerBalancerImp>(ctl_latency);
    }

    std::shared_ptr<PowerBalancer> PowerBalancer::make_shared(double ctl_latency)
    {
        return std::make_shared<PowerBalancerImp>(ctl_latency);
    }

    PowerBalancerImp::PowerBalancerImp(double ctl_latency)
        : PowerBalancerImp(ctl_latency, 0.125, 9, 0.25)
    {

    }

    PowerBalancerImp::PowerBalancerImp(double ctl_latency, double trial_delta,
                                       int num_sample, double measure_duration)
        : M_CONTROL_LATENCY(ctl_latency)
        , M_MIN_TRIAL_DELTA(trial_delta)
        , M_MIN_NUM_SAMPLE(num_sample)
        , M_MIN_DURATION(measure_duration)
        , M_RUNTIME_FRACTION(0.02)
        , m_num_sample(0)
        , m_power_cap(NAN)
        , m_power_limit(NAN)
        , m_power_limit_change_time({{0,0}})
    , m_target_runtime(NAN)
    , m_trial_delta(8.0)
    , m_runtime_sample(NAN)
    , m_is_target_met(false)
    , m_runtime_buffer(geopm::make_unique<CircularBuffer<double> >(0))
    {

    }

    void PowerBalancerImp::power_cap(double cap)
    {
        m_power_limit = cap;
        m_power_cap = cap;
        m_runtime_buffer->clear();
        m_target_runtime = NAN;
    }

    double PowerBalancerImp::power_cap(void) const
    {
        return m_power_cap;
    }

    void PowerBalancerImp::power_limit_adjusted(double actual_limit)
    {
        // m_power_limit starts as the requested limit.  actual limit
        // is the clipped value.
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

    double PowerBalancerImp::power_limit(void) const
    {
        return m_power_limit;
    }

    bool PowerBalancerImp::is_limit_stable(void)
    {
        return (geopm_time_since(&m_power_limit_change_time) > M_CONTROL_LATENCY);
    }

    bool PowerBalancerImp::is_runtime_stable(double measured_runtime)
    {
        bool result = false;
        if (!is_limit_stable() ||
            std::isnan(measured_runtime)) {
            return result;
        }
        /// m_runtime_vec used as a temporary holder until enough time
        /// has passed to determine how many samples are required in
        /// the circular buffer.
        if (m_runtime_buffer->size() == 0) {
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
        else {
            m_runtime_buffer->insert(measured_runtime);
            if (m_runtime_buffer->size() == m_runtime_buffer->capacity()) {
                result = true;
            }
        }
        calculate_runtime_sample();
        return result;
    }

    double PowerBalancerImp::runtime_sample(void) const
    {
        return m_runtime_sample;
    }

    void PowerBalancerImp::calculate_runtime_sample(void)
    {
        if (m_runtime_buffer->size() != 0) {
            m_runtime_sample = Agg::median(m_runtime_buffer->make_vector());
        }
        else {
            m_runtime_sample = Agg::median(m_runtime_vec);
        }
    }

    void PowerBalancerImp::target_runtime(double largest_runtime)
    {
        m_target_runtime = largest_runtime * (1 - M_RUNTIME_FRACTION);
        if (m_runtime_sample > m_target_runtime) {
            m_is_target_met = true;
        }
        else {
            m_is_target_met = false;
        }
    }

    bool PowerBalancerImp::is_target_met(double measured_runtime)
    {
#ifdef GEOPM_DEBUG
        if (std::isnan(measured_runtime)) {
            throw Exception("PowerBalancerImp::" + std::string(__func__) +
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
                m_runtime_buffer->clear();
            }
        }
        return m_is_target_met;
    }

    double PowerBalancerImp::power_slack(void)
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
