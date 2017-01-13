/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#ifndef BALANCING_DECIDER_HPP_INCLUDE
#define BALANCING_DECIDER_HPP_INCLUDE

#include "Decider.hpp"
#include "geopm_plugin.h"

namespace geopm
{

    /// @brief Simple implementation of a power balancing tree decider.
    ///
    /// The balancing decider uses the runtimes of each child node to calculate
    /// ratios of power to give to each node. Nodes that are slower will be given
    /// more power than nodes that are ahead. The sum of the individual node budgets
    /// will sum to the budget allocated to the level of the heirarchy the decider
    /// instance is running at.
    class BalancingDecider : public Decider
    {
        public:
            /// @ brief BalancingDecider default constructor.
            BalancingDecider();
            BalancingDecider(const BalancingDecider &other);
            /// @brief BalancinDecider destructor, virtual.
            virtual ~BalancingDecider();
            virtual IDecider *clone(void) const;
            virtual void bound(double upper_bound, double lower_bound);
            virtual bool update_policy(const struct geopm_policy_message_s &policy_msg, IPolicy &curr_policy);
            virtual bool update_policy(IRegion &curr_region, IPolicy &curr_policy);
            virtual bool decider_supported(const std::string &descripton);
            virtual const std::string& name(void) const;
            virtual void requires(int level, TelemetryConfig &config);
        private:
            const std::string m_name;
            const double m_convergence_target;
            const unsigned m_min_num_converged;
            unsigned m_num_converged;
            double m_last_power_budget;
            int m_num_sample;
            double m_lower_bound;
            double m_upper_bound;
            unsigned m_num_out_of_range;
            double m_slope_modifier;
            const double M_GUARD_BAND;
    };
}

#endif
