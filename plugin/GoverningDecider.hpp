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

#ifndef GOVERNING_DECIDER_HPP_INCLUDE
#define GOVERNING_DECIDER_HPP_INCLUDE

#include "Decider.hpp"
#include <map>

namespace geopm
{
    /// @brief Simple implementation of a power governing leaf decider.
    ///
    /// The governing decider uses the RAPL energy readings for each RAPL domain
    /// of control to distribute the given per-node power budget while staying under
    /// a given power cap. Each domain (ex: socket) subtracts the used DRAM power
    /// from the total power budget and gives the remainder to the package (cores).
    class GoverningDecider : public Decider
    {
        public:
            /// @brief GoverningDecider default constructor.
            GoverningDecider();
            GoverningDecider(const GoverningDecider &other) = delete;
            GoverningDecider &operator=(const GoverningDecider &other) = delete;
            /// @brief GoverningDecider destructor, virtual.
            virtual ~GoverningDecider();
            virtual bool update_policy(const struct geopm_policy_message_s &policy_msg, IPolicy &curr_policy) override;
            virtual bool update_policy(IRegion &curr_region, IPolicy &curr_policy) override;
            virtual bool decider_supported(const std::string &descripton) override;
            virtual const std::string& name(void) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IDecider> make_plugin(void);
        protected:
            std::string m_name;
            const unsigned m_min_num_converged;
            double m_last_power_budget;
            double m_last_dram_power;
            const int m_num_sample;
            std::map<uint64_t, unsigned> m_num_converged;
    };
}

#endif
