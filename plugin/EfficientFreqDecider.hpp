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

#ifndef EFFICIENT_FREQ_DECIDER_HPP_INCLUDE
#define EFFICIENT_FREQ_DECIDER_HPP_INCLUDE

#include <memory>
#include <vector>

#include "GoverningDecider.hpp"

namespace geopm
{
    class EfficientFreqRegion;
    class IDecider;
    class IPlatformIO;
    class IPlatformTopo;

    /// @brief Energy efficient implementation of a binary frequency decider.
    ///
    /// This frequency decider uses the geopm_hint interface or feedback from
    /// region runtime obtained offline or online to determine
    /// whether we are in a compute or memory bound region and choose
    /// the maximum frequency and a fraction of the minimal possible frequency
    /// repsectively.
    ///
    /// This is a leaf decider.
    class EfficientFreqDecider : public GoverningDecider
    {
        public:
            /// @brief EfficientFreqDecider default constructor.
            EfficientFreqDecider();
            EfficientFreqDecider(const std::string &cpu_info_path,
                                 const std::string &cpu_freq_min_path,
                                 const std::string &cpu_freq_max_path,
                                 IPlatformIO &platform_io,
                                 IPlatformTopo &platform_topo);
            EfficientFreqDecider(const EfficientFreqDecider &other) = delete;
            EfficientFreqDecider &operator=(const EfficientFreqDecider &other) = delete;
            /// @brief EfficientFreqDecider destructor, virtual.
            virtual ~EfficientFreqDecider();
            /// @brief Actual method altering GoverningDecider behavior.
            virtual bool update_policy(IRegion &curr_region, IPolicy &curr_policy) override;
            static std::string plugin_name(void);
            static std::unique_ptr<IDecider> make_plugin(void);

            // TODO: needs doc strings
            double cpu_freq_sticker(void);
            double cpu_freq_min(void);
            double cpu_freq_max(void);
        protected:
            void init_platform_io(void);
            void parse_env_map(void);
            const std::string m_cpu_info_path;
            const std::string m_cpu_freq_min_path;
            const std::string m_cpu_freq_max_path;
            const double m_freq_min;
            const double m_freq_max;
            const double m_freq_step;
            const unsigned int m_num_cpu;
            std::vector<int> m_control_idx;
            double m_last_freq;
            std::map<uint64_t, double> m_rid_freq_map;
            // for online adaptive mode
            bool m_is_adaptive = false;
            IRegion *m_region_last = nullptr;
            std::map<uint64_t, std::unique_ptr<EfficientFreqRegion>> m_region_map;
            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
    };
}

#endif
