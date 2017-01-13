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

#ifndef KNLPLATFORMIMP_HPP_INCLUDE
#define KNLPLATFORMIMP_HPP_INCLUDE

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

#include <sys/types.h>
#include <stdint.h>
#include <vector>
#include <map>
#include <string>

#include "PlatformImp.hpp"

namespace geopm
{
    /// @brief This class provides a concrete platform implementation of
    /// Haswell E processors.
    /// (cpuid 0x63F).
    class KNLPlatformImp : public PlatformImp
    {
        public:
            /// @brief Default constructor.
            KNLPlatformImp();
            KNLPlatformImp(const KNLPlatformImp &other);
            /// @brief Default destructor.
            virtual ~KNLPlatformImp();

            //////////////////////////////////////////////
            // KNLPlatformImp dependent implementations //
            //////////////////////////////////////////////
            virtual bool is_model_supported(int platform_id);
            virtual std::string platform_name(void);
            virtual int num_domain(int domain_type) const;
            virtual void create_domain_map(int domain, std::vector<std::vector<int> > &domain_map) const;
            virtual double throttle_limit_mhz(void) const;
            virtual void batch_read_signal(std::vector<double> &signal_value);
            virtual void write_control(int control_domain, int domain_index, double value);
            virtual void msr_initialize(void);
            virtual void init_telemetry(const TelemetryConfig &config);
            virtual void msr_reset(void);
            static int platform_id(void);
            void provides(TelemetryConfig &config) const;
            double energy(void) const;
        protected:
            /// @brief Initialize Running Average Power Limiting (RAPL) controls.
            void rapl_init(void);
            /// @brief Initialize per-CPU counters.
            void cbo_counters_init(int counter_idx, uint32_t event);
            /// @brief Initialize free running counters.
            void fixed_counters_init(void);
            /// @brief Reset per-CPU counters to default state.
            void cbo_counters_reset(void);
            /// @brief Reset free running counters to default state.
            void fixed_counters_reset(void);

            /// @brief Frequency where anything <= is considered throttling.
            double m_throttle_limit_mhz;
            /// @brief Store the units of energy read from RAPL.
            double m_energy_units;
            /// @brief Store the inverse units of power read from RAPL.
            double m_power_units_inv;
            /// @brief Store the units of DRAM energy read from RAPL.
            double m_dram_energy_units;
            /// @brief Minimum value for package (CPU) power read from RAPL.
            double m_min_pkg_watts;
            /// @brief Maximum value for package (CPU) power read from RAPL.
            double m_max_pkg_watts;
            /// @brief Minimum value for DRAM power read from RAPL.
            double m_min_dram_watts;
            /// @brief Maximum value for DRAM power read from RAPL.
            double m_max_dram_watts;
            /// @brief Minimum supported p-state.
            double m_min_freq_mhz;
            /// @brief Maximum supported p-state.
            double m_max_freq_mhz;
            /// @brief Step size in between supported p-states.
            double m_freq_step_mhz;
            ///@brief Vector of MSR data containing pairs of offsets and write masks.
            std::vector<std::pair<off_t, unsigned long> > m_control_msr_pair;
            uint64_t m_pkg_power_limit_static;

            ///Constants
            const unsigned int M_BOX_FRZ_EN;
            const unsigned int M_BOX_FRZ;
            const unsigned int M_CTR_EN;
            const unsigned int M_RST_CTRS;
            const uint64_t M_DRAM_POWER_LIMIT_MASK;
            const int M_PLATFORM_ID;
            const std::string M_MODEL_NAME;
            const std::string M_TRIGGER_NAME;

            enum {
                M_RAPL_PKG_LIMIT,
                M_RAPL_DRAM_LIMIT,
                M_IA32_PERF_CTL,
                M_NUM_CONTROL
            } m_control_e;
    };
}
#endif
