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

#ifndef GLOBALPOLICY_HPP_INCLUDE
#define GLOBALPOLICY_HPP_INCLUDE

#include <string>
#include <fstream>
#include "geopm_message.h"

namespace geopm
{
   /// Encapsulates the power policy that is applied across the entire MPI job
   /// allocation being managed by geopm.
    class GlobalPolicy
    {
        public:
            /// GlobalPolicy constructor. Only one of in_config or out_config can be non-empty.
            /// @param in_config Name of a policy configuration file to be read in. The object state
            /// will be initialized from the policy in the file.
            /// @param out_config Name of a policy configuration file to be written. The contents of
            /// the file will be obtained from the object's state.
            GlobalPolicy(const std::string in_config, const std::string out_config);
            // GlobalPolicy destructor
            virtual ~GlobalPolicy();
            /// Get the policy power mode
            /// @return geopm_policy_mode_e power mode
            int mode(void) const;
            /// Get the policy frequency
            /// @return frequency in MHz
            int frequency_mhz(void) const;
            /// Get the policy tdp percentage
            /// @return tdp percentage between 0-100
            int tdp_percent(void) const;
            /// Get the policy power budget
            /// @return power budget in watts
            int budget_watts(void) const;
            /// Get the policy affinity. This is the cores that we
            /// will dynamically control. One of
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT or
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT.
            /// @return enum power affinity
            int affinity(void) const;
            /// Get the policy power goal, One of
            /// GEOPM_FLAGS_GOAL_CPU_EFFICIENCY,
            /// GEOPM_FLAGS_GOAL_NETWORK_EFFICIENCY, or
            /// GEOPM_FLAGS_GOAL_MEMORY_EFFICIENCY
            /// @return enum power goal
            int goal(void) const;
            /// Get the number of 'big' cores
            /// @return number of cores where we will run unconstrained power.
            int num_max_perf(void) const;
            /// Get the policy flags
            /// @return 32-bit flags
            long int flags(void) const;
            /// Get a policy message from th policy object
            /// @param policy_message structure to be filled in
            void policy_message(struct geopm_policy_message_s &policy_message) const;
            /// Set the policy power mode
            /// @param mode geopm_policy_mode_e power mode
            void mode(int mode);
            /// Set the policy frequency
            /// @param frequency frequency in MHz
            void frequency_mhz(int frequency);
            /// Set the policy tdp percentage
            /// @param percentage tdp percentage between 0-100
            void tdp_percent(int percentage);
            /// Set the policy power budget
            /// @param budget power budget in watts
            void budget_watts(int budget);
            /// Set the policy affinity. This is the cores that we
            /// will dynamically control. One of
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT or
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT.
            /// @param cpu_affinity enum power affinity
            void affinity(int cpu_affinity);
            /// Set the policy power goal. One of
            /// GEOPM_FLAGS_GOAL_CPU_EFFICIENCY,
            /// GEOPM_FLAGS_GOAL_NETWORK_EFFICIENCY, or
            /// GEOPM_FLAGS_GOAL_MEMORY_EFFICIENCY
            /// @param geo_goal enum power goal
            void goal(int geo_goal);
            /// Set the number of 'big' cores
            /// @param num_big_cores of cores where we will run unconstrained power.
            void num_max_perf(int num_big_cores);
            /// Write out a policy file from the current state
            void write(void);
            /// Read in a policy file and set our current state
            void read(void);
            /// Enforce one of the static modes:
            /// GEOPM_MODE_TDP_BALANCE_STATIC,
            /// GEOPM_MODE_FREQ_UNIFORM_STATIC, or
            /// GEOPM_MODE_FREQ_HYBRID_STATIC
            void enforce_static_mode();
        protected:
            /// Take in an affinity enum and print out its string representation
            void affinity_string(int value, std::string &name);
            /// input file name
            std::string m_in_config;
            /// output file name
            std::string m_out_config;
            /// power mode
            int m_mode;
            /// power budget in watts
            int m_power_budget_watts;
            /// flags encapsulates frequency, number of 'big'cpus,
            /// 'small' core affinity, tdp percentage, and power goal
            long int m_flags;
            /// true if reading policy from shared memory, else false
            bool m_is_shm_in;
            /// true if outputting policy to shared memory, else false
            bool m_is_shm_out;
            /// true if the input file is valid, else false
            bool m_do_read;
            /// true if the output file is valid, else false
            bool m_do_write;
            /// structure to use if reading in from shared memory
            struct geopm_policy_shmem_s *m_policy_shmem_in;
            /// structure to use if writing out to shared memory
            struct geopm_policy_shmem_s *m_policy_shmem_out;
            /// file stream for input file
            std::ifstream m_config_file_in;
            /// file stream for output file
            std::ofstream m_config_file_out;
    };

}
#endif
