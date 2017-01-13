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

#ifndef PLATFORM_HPP_INCLUDE
#define PLATFORM_HPP_INCLUDE

#include "Region.hpp"
#include "TelemetryConfig.hpp"
#include "Policy.hpp"
#include "PlatformImp.hpp"
#include "geopm_message.h"

namespace geopm
{
    /// @brief This class provides an abstraction of specific functionality
    /// and attributes of classes of hardware implementations. It holds
    /// the implementation of the specific hardware platform and knows how
    /// to interact with it.
    class IPlatform
    {
        public:
            IPlatform() {}
            virtual ~IPlatform() {}
            /// @brief Set our member variable pointing to a PlatformImp object.
            /// @param [in] platform_imp A PlatformImp object that is compatible
            ///        with this platform and the underlying hardware.
            /// @param [in] do_initialize Choose whether or not to initialize the Platform.
            ///        This also initialized the underlying PlatformImp.
            virtual void set_implementation(PlatformImp* platform_imp, bool do_initialize) = 0;
            /// @brief Retrieve the string name of the hw platform.
            /// @return The hw platform name.
            virtual void name(std::string &plat_name) const = 0;
            /// @brief Write to a file the current state of RAPL, per-CPU counters,
            /// and free running counters.
            /// @param [in] path The path of the file to write.
            virtual void save_msr_state(const char *path) const = 0;
            /// @brief Read in MSR state for RAPL, per-CPU counters,
            /// and free running counters and set them to that
            /// state.
            /// @param [in] path The path of the file to read in.
            virtual void restore_msr_state(const char *path) const = 0;
            /// @brief Output a MSR whitelist for use with the Linux MSR driver.
            /// @param [in] file_desc File descriptor for output.
            virtual void write_msr_whitelist(FILE *file_desc) const = 0;
            /// @brief Revert the MSR values to their initial state.
            virtual void revert_msr_state(void) const = 0;
            /// @brief Record telemetry from counters and RAPL MSRs.
            /// @param [in] msr_values MSR structures in which to save values.
            virtual void sample(struct geopm_time_s &sample_time, std::vector<double> &msr_values) = 0;
            /// @brief Does this Platform support a specific platform.
            /// @param [in] platform_id Platform identifier specific to the
            ///        underlying hardware. On x86 platforms this can be obtained by
            ///        the cpuid instruction.
            /// @param [in] A description string that can be used to determine if the
            ///        platform can fulfill the requirements of the request.
            /// @return true if this Platform supports platform_id,
            ///         else false.
            virtual bool is_model_supported(int platform_id, const std::string &description) const = 0;
            /// @brief Enforce a static power management mode including
            /// tdp_balance_static, freq_uniform_static, and
            /// freq_hybrid_static.
            /// @param [in] policy A Policy object containing the policy information
            ///        to be enforced.
            virtual void enforce_policy(uint64_t region_id, IPolicy &policy) const = 0;
            virtual void provides(TelemetryConfig &config) const = 0;
            virtual void init_telemetry(const TelemetryConfig &config) = 0;
            ////////////////////////////////////////
            /// signals are expected as follows: ///
            /// per socket signals               ///
            ///     PKG_ENERGY                   ///
            ///     DRAM_ENERGY                  ///
            /// followed by per cpu signals      ///
            ///     FREQUENCY                    ///
            ///     INST_RETIRED                 ///
            ///     CLK_UNHALTED_CORE            ///
            ///     CLK_UNHALTED_REF             ///
            ///     LLC_VICTIMS                  ///
            /// followed by per rank signals     ///
            ///     PROGRESS                     ///
            ///     RUNTIME                      ///
            ////////////////////////////////////////
            virtual double control_latency_ms(int control_type) const = 0;
            /// @brief Return the frequency limit where throttling occurs.
            ///
            /// @return frequency limit where anything <= is considered throttling.
            virtual double throttle_limit_mhz(void) const = 0;
            /// @brief Has the trigger msr of the platform changed value since last call.
            virtual bool is_updated(void) = 0;
            virtual double energy(void) const = 0;
    };

    class Platform : public IPlatform
    {
        public:
            /// @brief Default constructor.
            Platform();
            /// @param [in] control_domain_type enum geopm_domain_type_e
            ///        describing the finest grain domain of control.
            Platform(int control_domain_type);
            /// @brief Default destructor.
            virtual ~Platform();
            void set_implementation(PlatformImp* platform_imp, bool do_initialize);
            int num_domain(void) const;
            void name(std::string &plat_name) const;
            void save_msr_state(const char *path) const;
            void restore_msr_state(const char *path) const;
            void write_msr_whitelist(FILE *file_desc) const;
            void revert_msr_state(void) const;
            virtual void sample(struct geopm_time_s &sample_time, std::vector<double> &msr_values) = 0;
            virtual bool is_model_supported(int platform_id, const std::string &description) const = 0;
            virtual void enforce_policy(uint64_t region_id, IPolicy &policy) const = 0;
            void provides(TelemetryConfig &config) const;
            void init_telemetry(const TelemetryConfig &config);
            double control_latency_ms(int control_type) const;
            double throttle_limit_mhz(void) const;
            virtual bool is_updated(void);
            double energy(void) const;
        protected:
            /// @brief Pointer to a PlatformImp object that supports the target
            /// hardware platform.
            PlatformImp *m_imp;
            /// @brief The matrix that transforms the per package,
            /// per-cpu, and per-rank signals into the domain of control.
            std::vector<std::vector<int> > m_rank_cpu;
            int m_num_rank;
    };
}

#endif
