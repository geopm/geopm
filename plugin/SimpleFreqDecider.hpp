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

#ifndef SIMPLE_FREQ_DECIDER_HPP_INCLUDE
#define SIMPLE_FREQ_DECIDER_HPP_INCLUDE

#include <memory>

#include "Decider.hpp"
#include "geopm_plugin.h"
#include "GoverningDecider.hpp"

namespace geopm
{
    class AdaptiveFreqRegion;

    /// @brief Simple implementation of a binary frequency decider.
    ///
    /// This frequency decider uses the geopm_hint interface to determine
    /// wether we are in a compute or memory bound region and choose
    /// the maximum frequency and a fraction of the minimal possible frequency
    /// repsectively.
    ///
    /// This is a leaf decider.

    class SimpleFreqDecider : public GoverningDecider
    {
        public:
            /// @ brief SimpleFreqDecider default constructor.
            SimpleFreqDecider();
            SimpleFreqDecider(const SimpleFreqDecider &other);
            /// @brief SimpleFreqDecider destructor, virtual.
            virtual ~SimpleFreqDecider();
            virtual IDecider *clone(void) const;
            /// @brief Actual method altering GoverningDecider behavior
            virtual bool update_policy(IRegion &curr_region, IPolicy &curr_policy);
        private:
            double cpu_freq_sticker(void);
            double cpu_freq_min(void);
            double cpu_freq_max(void);
            void parse_env_map(void);
            const std::string m_cpu_info_path;
            const std::string m_cpu_freq_min_path;
            const std::string m_cpu_freq_max_path;
            const double m_freq_min;
            const double m_freq_max;
            const double m_freq_step;
            const unsigned int m_num_cores;
            double m_last_freq;
            std::map<uint64_t, double> m_rid_freq_map;
            // for adaptive decider
            bool m_is_adaptive = false;
            IRegion *m_region_last = nullptr;
            std::map<uint64_t, std::unique_ptr<AdaptiveFreqRegion>> m_region_map;
    };
}

#endif
