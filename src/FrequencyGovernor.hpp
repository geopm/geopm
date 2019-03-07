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
    class PlatformIO;
    class PlatformTopo;

    class FrequencyGovernor
    {
        public:
            FrequencyGovernor() = default;
            virtual ~FrequencyGovernor() = default;
            /// @brief Registers signals and controls with PlatformIO using the
            ///        default control domain.
            virtual void init_platform_io(void) = 0;
            /// @brief Registers signals and controls with PlatformIO using the
            ///        default control domain.
            virtual void init_platform_io(int freq_ctl_domain_type) = 0;
            /// @brief Get the domain type of frequency control on the
            ///        platform.  Users of the FrequencyGovernor can
            ///        use this information to determine the size of
            ///        the vector to pass to adjust_platform().
            /// @return The domain with which frequency will be governed.
            virtual int frequency_domain_type(void) const = 0;
            /// @brief Write frequency control, may be clamped between
            ///        min and max frequency if request cannot be
            ///        satisfied.
            /// @param [in] frequency_request Desired per domain frequency.
            /// @param [out] frequency_actual Actual per domain frequency.  Should equal
            //              frequency_request unless clamped by bounds.
            /// @return True if platform adjustments have been made, false otherwise.
            virtual bool adjust_platform(const std::vector<double> &frequency_request, std::vector<double> &frequency_actual) = 0;
            /// @brief Sets min and max package bounds.  The defaults before calling
            ///        this method are the min and max frequency for the platform.
            /// @param [in] freq_min Minimum frequency value for the control domain.
            /// @param [in] freq_max Maximum frequency value for the control domain.
            /// @return Returns true if internal state updated, otherwise false.
            virtual bool set_frequency_bounds(double freq_min, double freq_max) = 0;
            virtual void get_frequency_bounds(double &freq_min, double &freq_max) const = 0;
            virtual void validate_policy(double &freq_min, double &freq_max) const = 0;
            virtual double get_frequency_step() const = 0;
    };

    class FrequencyGovernorImp : public FrequencyGovernor
    {
        public:
            FrequencyGovernorImp(PlatformIO &platform_io, PlatformTopo &platform_topo);
            virtual ~FrequencyGovernorImp();
            void init_platform_io(void) override;
            void init_platform_io(int control_domain) override;
            int frequency_domain_type(void) const override;
            bool adjust_platform(const std::vector<double> &frequency_request, std::vector<double> &frequency_actual) override;
            bool set_frequency_bounds(double freq_min, double freq_max) override;
            void get_frequency_bounds(double &freq_min, double &freq_max) const override;
            void validate_policy(double &freq_min, double &freq_max) const override;
            double get_frequency_step() const override;
        private:
            double get_limit(const std::string &sig_name) const;
            PlatformIO &m_platform_io;
            PlatformTopo &m_platform_topo;
            double m_freq_min;
            double m_freq_max;
            const double M_FREQ_STEP;
            const double M_PLAT_FREQ_MIN;
            const double M_PLAT_FREQ_MAX;
            int m_freq_ctl_domain_type;
            std::vector<int> m_control_idx;
            double m_last_freq;
    };
}

#endif
