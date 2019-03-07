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

#ifndef FREQUENCYGOVERNOR_HPP_INCLUDE
#define FREQUENCYGOVERNOR_HPP_INCLUDE

#include <memory>
#include <vector>

namespace geopm
{
    class IPlatformIO;
    class IPlatformTopo;

    class IFrequencyGovernor
    {
        public:
            IFrequencyGovernor() = default;
            virtual ~IFrequencyGovernor() = default;
            /// @brief Registsters signals and controls with PlatformIO.
            /// @return The domain with which frequency will be governed.
            virtual int init_platform_io(void) = 0;
            /// @brief Write frequency control, may be clamped if request cannot be
            ///        satisfied.
            /// @param [in] frequency_request Desired per domain frequency.
            /// @param [out] frequency_actual Actual per domain frequency.  Should equal
            //              frequency_request unless clamped by bounds.
            /// @return True if platform adjustments have been made, false otherwise.
            virtual bool adjust_platform(std::vector<double> frequency_request, std::vector<double> &frequency_actual) = 0;
            /// @brief Sets min and max package bounds.
            /// @param [in] min_freq Minimum frequency control domain value.
            /// @param [in] max_freq Maximum frequency control domain value.
            /// @return Returns true if internal state updated, otherwise false.
            virtual bool set_frequency_bounds(double min_freq, double max_freq) = 0;
    };

    class FrequencyGovernor : public IFrequencyGovernor
    {
        public:
            FrequencyGovernor(IPlatformIO &platform_io, IPlatformTopo &platform_topo);
            virtual ~FrequencyGovernor();
            int init_platform_io(void) override;
            bool adjust_platform(std::vector<double> frequency_request, std::vector<double> &frequency_actual) override;
            bool set_frequency_bounds(double min_freq, double max_freq) override;
        private:
            double get_limit(const std::string &sig_name) const;
            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
            double m_freq_min;
            double m_freq_max;
            const double M_FREQ_STEP;
            std::vector<int> m_control_idx;
            double m_last_freq;
    };
}

#endif
