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

#ifndef FREQUENCYGOVERNORIMP_HPP_INCLUDE
#define FREQUENCYGOVERNORIMP_HPP_INCLUDE

#include <memory>
#include <vector>
#include <string>

#include "FrequencyGovernor.hpp"

namespace geopm
{
    class FrequencyGovernorImp : public FrequencyGovernor
    {
        public:
            FrequencyGovernorImp();
            FrequencyGovernorImp(PlatformIO &platform_io, const PlatformTopo &platform_topo);
            virtual ~FrequencyGovernorImp();
            void init_platform_io(void) override;
            int frequency_domain_type(void) const override;
            void adjust_platform(const std::vector<double> &frequency_request) override;
            bool do_write_batch(void) const override;
            bool set_frequency_bounds(double freq_min, double freq_max) override;
            double get_frequency_min() const override;
            double get_frequency_max() const override;
            double get_frequency_step() const override;
            void validate_policy(double &freq_min, double &freq_max) const override;
        private:
            double get_limit(const std::string &sig_name) const;
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            const double M_FREQ_STEP;
            const double M_PLAT_FREQ_MIN;
            const double M_PLAT_FREQ_MAX;
            double m_freq_min;
            double m_freq_max;
            bool m_do_write_batch;
            int m_freq_ctl_domain_type;
            std::vector<int> m_control_idx;
            std::vector<double> m_last_freq;
    };
}

#endif
