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

#ifndef POWERGOVERNOR_HPP_INCLUDE
#define POWERGOVERNOR_HPP_INCLUDE

#include <memory>
#include <vector>

namespace geopm
{
    class IPlatformIO;
    class IPlatformTopo;
    template <class type>
    class ICircularBuffer;

    class IPowerGovernor
    {
        public:
            IPowerGovernor() = default;
            virtual ~IPowerGovernor() = default;
            /// @brief Registsters signals and controls with PlatformIO.
            virtual void init_platform_io(void) = 0;
            /// @brief Samples DRAM power, storing history of past values.
            virtual void sample_platform(void) = 0;
            /// @brief Calculates metric of DRAM power history, subtracting that value
            ///        from the provided target node power.
            /// @param [in] node_power_setting Total expected node power consumption.
            /// @return True if platform adjustments have been made, false otherwise.
            virtual bool adjust_platform(double node_power_setting) = 0;
            /// @brief Sets min and max node power target bounds.
            /// @param min_node_power Minimum node power.
            /// @param max_node_power Maximum node power.
            virtual void set_power_bounds(double min_node_power, double max_node_power) = 0;
    };

    class PowerGovernor : public IPowerGovernor
    {
        public:
            PowerGovernor(IPlatformIO &platform_io, IPlatformTopo &platform_topo);
            PowerGovernor(IPlatformIO &platform_io, IPlatformTopo &platform_topo, int samples_per_control);
            virtual ~PowerGovernor();
            void init_platform_io(void) override;
            void sample_platform(void) override;
            bool adjust_platform(double node_power_setting) override;
            void set_power_bounds(double min_node_power, double max_node_power) override;
        private:
            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
            const int M_SAMPLES_PER_CONTROL;
            int m_sample_count;
            int m_pkg_pwr_domain_type;
            int m_num_pkg;
            double M_MIN_PKG_POWER_SETTING;
            double M_MAX_PKG_POWER_SETTING;
            double m_min_pkg_power_policy;
            double m_max_pkg_power_policy;
            int m_dram_sig_idx;
            std::vector<int> m_control_idx;
            std::unique_ptr<ICircularBuffer<double> > m_dram_power_buf;
            mutable double m_last_node_power_setting;
    };
}

#endif
