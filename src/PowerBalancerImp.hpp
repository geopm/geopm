/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef POWERBALANCERIMP_HPP_INCLUDE
#define POWERBALANCERIMP_HPP_INCLUDE

#include <memory>
#include <vector>

#include "geopm_time.h"
#include "PowerBalancer.hpp"

namespace geopm
{
    template <typename T> class CircularBuffer;

    class PowerBalancerImp : public PowerBalancer
    {
        public:
            /// @brief Construct a testable PowerBalancerImp object.
            PowerBalancerImp(double ctl_latency, double trial_delta, int num_sample, double measure_duration);
            /// @brief Construct a PowerBalancerImp object.
            PowerBalancerImp(double ctl_latency);
            /// @brief Destroy a PowerBalancerImp object.
            virtual ~PowerBalancerImp() = default;
            void power_cap(double cap) override;
            double power_cap(void) const override;
            double power_limit(void) const override;
            void power_limit_adjusted(double limit) override;
            bool is_runtime_stable(double measured_runtime) override;
            double runtime_sample(void) const override;
            void calculate_runtime_sample(void) override;
            void target_runtime(double largest_runtime) override;
            bool is_target_met(double measured_runtime) override;
            double power_slack(void) override;
        private:
            bool is_limit_stable(void);

            const double M_CONTROL_LATENCY;
            const double M_MIN_TRIAL_DELTA;
            const int M_MIN_NUM_SAMPLE;
            const double M_MIN_DURATION;
            const double M_RUNTIME_FRACTION;
            int m_num_sample;
            // @brief Maximum power as set in last global budget
            //        increase.
            double m_power_cap;
            // @brief Current power limit to get to target runtime
            //        which may be lower than the cap.
            double m_power_limit;
            struct geopm_time_s m_power_limit_change_time;
            double m_target_runtime;
            double m_trial_delta;
            double m_runtime_sample;
            bool m_is_target_met;
            std::unique_ptr<CircularBuffer<double> > m_runtime_buffer;
            std::vector<double> m_runtime_vec;
    };
}

#endif
