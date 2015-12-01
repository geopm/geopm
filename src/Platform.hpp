/*
 * Copyright (c) 2015, Intel Corporation
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

#include <hwloc.h>

#include "Region.hpp"
#include "Policy.hpp"
#include "PlatformImp.hpp"
#include "PowerModel.hpp"
#include "geopm_message.h"

namespace geopm
{
    /// This class provides an abstraction of specific functionality
    /// and attributes of classes of hardware implementations. It holds
    /// the implementation of the specific hardware platform and knows how
    /// to interact with it.
    class Platform
    {
        public:
            /// Default constructor.
            Platform();
            /// Default destructor.
            virtual ~Platform();
            /// Set our member variable pointing to a PlatformImp object.
            /// @param platform_imp A PlatformImp object that is compatable
            ///        with this platform and the underlying hardware.
            virtual void set_implementation(PlatformImp* platform_imp);
            /// Sets the current region to add telemerty to.
            /// @param region A pointer to the current Region object.
            void region_begin(Region *region);
            /// Signal that the current region has ended.
            void region_end(void);
            /// Retrieve the current region object.
            /// @return A pointer to the current region object.
            Region *cur_region(void) const;
            /// Retrieve the number of power domains.
            /// @return Number of power domains.
            int num_domain(void) const;
            void domain_index(int domain_type, std::vector <int> &domain_index) const;
            int level(void) const;
            std::string name(void) const;
            void buffer_index(hwloc_obj_t domain,
                              const std::vector <std::string> &signal_names,
                              std::vector <int> &buffer_index) const;
            void observe(struct geopm_sample_message_s &sample) const;
            void observe(const std::vector <struct geopm_sample_message_s> &sample) const;
            PowerModel *power_model(int domain_type) const;
            /// Set the power limit of the cpus to a percentage of
            /// Thermal Design Power (TDP).
            /// @param percentage The percentage of TDP.
            void tdp_limit(int percentage) const;
            /// Set the frequency to a fixed value for cpus within an
            /// affinity set.
            /// @param frequency Frequency in MHz to set the cpus to.
            /// @param num_cpu_max_perf The number of cores to leave
            ///        unconstrained.
            /// @param affinity The affinity of the cores for which the
            ///        frequency will be set. 
            void manual_frequency(int frequency, int num_cpu_max_perf, int affinity) const;
            /// Write to a file the current state of RAPL, per-cpu counters,
            /// and free running counters.
            /// @param path The path of the file to write.
            void save_msr_state(const char *path) const;
            /// Read in MSR state for RAPL, per-cpu counters,
            /// and free running counters and set them to that
            /// state.
            /// @param path The path of the file to read in.
            void restore_msr_state(const char *path) const;
            /// Output a MSR whitelist for use with the Linux MSR driver.
            /// @param file_desc File descriptor for output.
            void write_msr_whitelist(FILE *file_desc) const;
            /// Record telemetry from counters and RAPL MSRs.
            virtual void observe(void) = 0;
            /// Does this Platform support a specific platform.
            /// @param platform_id Platform identifier specific to the
            ///        underlying hradware. On x86 plaforms this can be obtained by
            ///        the cpuid instruction.
            /// @return true if this Platform supports platform_id,
            ///         else false.
            virtual bool model_supported(int platform_id) const = 0;
            /// Retrieve a telemetry sample.
            /// @param sample Sample message in which to store the sample.
            virtual void sample(struct geopm_sample_message_s &sample) const = 0;
            /// Enforce a static power management mode including
            /// tdp_balance_static, freq_uniform_static, and 
            /// freq_hybrid_static.
            /// @param policy A Policy object containing the policy information
            ///        to be enforced.
            virtual void enforce_policy(const Policy &policy) const = 0;
            /// Retrieve the topology of the current platform.
            /// @return PlatformTopology object containing the current
            ///         topology information.
            const PlatformTopology topology() const;
        protected:
            /// Pointer to a PlatformImp object that supports the target
            /// hardware platform.
            PlatformImp *m_imp;
            /// Pointer to the application region object that is
            /// currently executing.
            Region *m_curr_region;
            /// Map from a power domain type to its corresponding
            /// PowerModel object.
            std::map <int, PowerModel *> m_power_model;
            /// The number of power domains
            int m_num_domains;
            int m_window_size;
            int m_level;
    };
}

#endif
