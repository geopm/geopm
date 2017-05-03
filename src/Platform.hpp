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
#include "Policy.hpp"
#include "PlatformImp.hpp"
#include "geopm_message.h"

namespace geopm
{
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
            /// @brief Retrieve the number of power domains.
            /// @return Number of power domains.
            virtual int num_domain(void) const = 0;
            /// @brief Retrieve the string name of the hw platform.
            /// @return The hw platform name.
            virtual void name(std::string &plat_name) const = 0;
            /// @brief Set the power limit of the CPUs to a percentage of
            /// Thermal Design Power (TDP).
            /// @param [in] percentage The percentage of TDP.
            virtual void tdp_limit(double percentage) const = 0;
            /// @brief Set the frequency to a fixed value for CPUs within an
            /// affinity set.
            /// @param [in] frequency Frequency in MHz to set the CPUs to.
            /// @param [in] num_cpu_max_perf The number of cores to leave
            ///        unconstrained.
            /// @param [in] affinity The affinity of the cores for which the
            ///        frequency will be set.
            virtual void manual_frequency(int frequency, int num_cpu_max_perf, int affinity) const = 0;
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
            /// @brief Return the domain of control;
            virtual int control_domain(void) = 0;
            /// @brief Number of MSR values returned from sample().
            virtual size_t capacity(void) = 0;
            /// @brief Record telemetry from counters and RAPL MSRs.
            /// @param [in] msr_values MSR structures in which to save values.
            virtual void sample(std::vector<struct geopm_msr_message_s> &msr_values) = 0;
            /// @brief Does this Platform support a specific platform.
            /// @param [in] platform_id Platform identifier specific to the
            ///        underlying hardware. On x86 platforms this can be obtained by
            ///        the cpuid instruction.
            /// @param [in] A description string that can be used to determine if the
            ///        platform can fulfill the requirements of the request.
            /// @return true if this Platform supports platform_id,
            ///         else false.
            virtual bool model_supported(int platform_id, const std::string &description) const = 0;
            /// @brief Enforce a static power management mode including
            /// tdp_balance_static, freq_uniform_static, and
            /// freq_hybrid_static.
            /// @param [in] policy A Policy object containing the policy information
            ///        to be enforced.
            virtual void enforce_policy(uint64_t region_id, IPolicy &policy) const = 0;
            /// @brief Return the upper and lower bounds of the control.
            ///
            /// For a RAPL platform this would be the package power limit,
            /// for a frequency platform this would be the p-state bounds.
            ///
            /// @param [out] upper_bound The upper control bound.
            ///
            /// @param [out] lower_bound The lower control bound.
            ///
            virtual void bound(double &upper_bound, double &lower_bound) = 0;
            /// @brief Retrieve the topology of the current platform.
            /// @return PlatformTopology object containing the current
            ///         topology information.
            virtual const PlatformTopology *topology(void) const = 0;
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
            /// @brief initialize the signal transformation matrix.
            /// Initialize the matrix that transforms the per package,
            /// per-cpu, and per-rank signals into the domain of control.
            /// @param [in] cpu_rank The mapping from cpu index to rank id.
            virtual void init_transform(const std::vector<int> &cpu_rank) = 0;
            /// @brief Retrieve the number of control domains
            /// @return The number of control domains on the hw platform.
            virtual int num_control_domain(void) const = 0;
            virtual double control_latency_ms(void) const = 0;
            /// @brief Return the frequency limit where throttling occurs.
            ///
            /// @return frequency limit where anything <= is considered throttling.
            virtual double throttle_limit_mhz(void) const = 0;
            /// @brief Has the trigger msr of the platform changed value since last call.
            virtual bool is_updated(void) = 0;
            virtual void transform_rank_data(uint64_t region_id, const struct geopm_time_s &aligned_time,
                                             const std::vector<double> &aligned_data,
                                             std::vector<struct geopm_telemetry_message_s> &telemetry) = 0;
    };



    /// @brief This class provides an abstraction of specific functionality
    /// and attributes of classes of hardware implementations. It holds
    /// the implementation of the specific hardware platform and knows how
    /// to interact with it.
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
            void tdp_limit(double percentage) const;
            void manual_frequency(int frequency, int num_cpu_max_perf, int affinity) const;
            void save_msr_state(const char *path) const;
            void restore_msr_state(const char *path) const;
            void write_msr_whitelist(FILE *file_desc) const;
            void revert_msr_state(void) const;
            virtual int control_domain(void) = 0;
            virtual size_t capacity(void) = 0;
            virtual void sample(std::vector<struct geopm_msr_message_s> &msr_values) = 0;
            virtual bool model_supported(int platform_id, const std::string &description) const = 0;
            virtual void enforce_policy(uint64_t region_id, IPolicy &policy) const = 0;
            virtual void bound(double &upper_bound, double &lower_bound) = 0;
            const PlatformTopology *topology(void) const;
            void init_transform(const std::vector<int> &cpu_rank);
            int num_control_domain(void) const;
            double control_latency_ms(void) const;
            double throttle_limit_mhz(void) const;
            virtual bool is_updated(void);
            void transform_rank_data(uint64_t region_id, const struct geopm_time_s &aligned_time,
                                     const std::vector<double> &aligned_data,
                                     std::vector<struct geopm_telemetry_message_s> &telemetry);
        protected:
            /// @brief Platform specific initialization code.
            virtual void initialize(void) = 0;
            /// @brief Pointer to a PlatformImp object that supports the target
            /// hardware platform.
            PlatformImp *m_imp;
            /// @brief The number of power domains
            int m_num_domain;
            /// @brief The geopm_domain_type_e of the finest domain of control
            /// on the hw platform.
            int m_control_domain_type;
            int m_num_energy_domain;
            int m_num_counter_domain;
            /// @brief The matrix that transforms the per package,
            /// per-cpu, and per-rank signals into the domain of control.
            std::vector<std::vector<int> > m_rank_cpu;
            int m_num_rank;
    };
}

#endif
