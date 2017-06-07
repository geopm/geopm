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

#ifndef GLOBALPOLICY_HPP_INCLUDE
#define GLOBALPOLICY_HPP_INCLUDE

#include <string>
#include <fstream>
#include <json-c/json.h>

#include "geopm_plugin.h"
#include "PolicyFlags.hpp"

namespace geopm
{
    /// @brief Encapsulates the power policy that is applied across the
    /// entire MPI job allocation being managed by geopm.
    ///
    /// The C++ implementation of the resource manager interface to
    /// the GEOPM policy.  The class methods support the C interface
    /// defined for use with the geopm_policy_c structure and are
    /// named accordingly.  The geopm_policy_c structure is an opaque
    /// reference to the geopm::GlobalPolicy class.
    class IGlobalPolicy
    {
        public:
            IGlobalPolicy() {}
            /// @brief GlobalPolicy destructor
            virtual ~IGlobalPolicy() {}
            /// @brief Get the policy power mode
            /// @return geopm_policy_mode_e power mode
            virtual int mode(void) const = 0;
            /// @brief Get the policy frequency
            /// @return frequency in MHz
            virtual int frequency_mhz(void) const = 0;
            /// @brief Get the policy TDP percentage
            /// @return TDP (thermal design power) percentage between 0-100
            virtual int tdp_percent(void) const = 0;
            /// @brief Get the policy power budget
            /// @return power budget in watts
            virtual int budget_watts(void) const = 0;
            /// @brief Get the policy affinity. This is the cores that we
            /// will dynamically control. One of
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT or
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT.
            /// @return enum power affinity
            virtual int affinity(void) const = 0;
            /// @brief Get the policy power goal, One of
            /// GEOPM_FLAGS_GOAL_CPU_EFFICIENCY,
            /// GEOPM_FLAGS_GOAL_NETWORK_EFFICIENCY, or
            /// GEOPM_FLAGS_GOAL_MEMORY_EFFICIENCY
            /// @return enum power goal
            virtual int goal(void) const = 0;
            /// @brief Get the number of 'big' cores
            /// @return number of cores where we will run
            ///         unconstrained power.
            virtual int num_max_perf(void) const = 0;
            /// @brief Get the TreeDecider description
            /// @return String reference containing the description
            ///         of the requested tree decider
            virtual const std::string &tree_decider() const = 0;
            /// @brief Get the LeafDecider description
            /// @return String reference containing the description
            ///         of the requested leaf decider
            virtual const std::string &leaf_decider() const = 0;
            /// @brief Get the Platform description
            /// @return String reference containing the description
            ///         of the requested platform
            virtual const std::string &platform() const = 0;
            /// @brief Get the string representation of the mode
            /// @return String containing the description of the mode
            virtual std::string mode_string() const = 0;
            /// @brief Get a policy message from the policy object
            /// @param [out] policy_message structure to be filled in
            virtual void policy_message(struct geopm_policy_message_s &policy_message) = 0;
            /// @brief Set the policy power mode
            /// @param [in] mode geopm_policy_mode_e power mode
            virtual void mode(int mode) = 0;
            /// @brief Set the policy frequency
            /// @param [in] frequency frequency in MHz
            virtual void frequency_mhz(int frequency) = 0;
            /// @brief Set the policy TDP percentage
            /// @param [in] percentage TDP percentage between 0-100
            virtual void tdp_percent(int percentage) = 0;
            /// @brief Set the policy power budget
            /// @param [in] budget power budget in watts
            virtual void budget_watts(int budget) = 0;
            /// @brief Set the policy affinity. This is the cores that we
            /// will dynamically control. One of
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT or
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT.
            /// @param [in] cpu_affinity enum power affinity
            virtual void affinity(int cpu_affinity) = 0;
            /// @brief Set the policy power goal. One of
            /// GEOPM_FLAGS_GOAL_CPU_EFFICIENCY,
            /// GEOPM_FLAGS_GOAL_NETWORK_EFFICIENCY, or
            /// GEOPM_FLAGS_GOAL_MEMORY_EFFICIENCY
            /// @param [in] geo_goal enum power goal
            virtual void goal(int geo_goal) = 0;
            /// @brief Set the number of 'big' cores
            /// @param [in] num_big_cores of cores where we will run
            ///        unconstrained power.
            virtual void num_max_perf(int num_big_cores) = 0;
            /// @brief Set the requested TreeDecider
            /// @param [in] Set the description string for the
            ///             requested tree decider.
            virtual void tree_decider(const std::string &description) = 0;
            /// @brief Set the requested LeafDecider
            /// @param [in] Set the description string for the
            ///             requested leaf decider.
            virtual void leaf_decider(const std::string &description) = 0;
            /// @brief Set the requested Platform
            /// @param [in] Set the description string for the
            ///             requested platform.
            virtual void platform(const std::string &description) = 0;
            /// @brief Write out a policy file from the current state
            virtual void write(void) = 0;
            /// @brief Read in a policy file and set our current state
            virtual void read(void) = 0;
            /// @brief Enforce one of the static modes:
            /// GEOPM_MODE_TDP_BALANCE_STATIC,
            /// GEOPM_MODE_FREQ_UNIFORM_STATIC, or
            /// GEOPM_MODE_FREQ_HYBRID_STATIC
            virtual void enforce_static_mode() = 0;
            /// @brief Return the header describing the current policy
            virtual std::string header(void) const = 0;
    };

    class GlobalPolicy : public IGlobalPolicy
    {
        public:
            /// @brief GlobalPolicy constructor. Only one of in_config or
            /// out_config can be non-empty.
            /// @param [in] in_config Name of a policy configuration file
            ///        to be read in. The object state will be
            ///        initialized from the policy in the file.
            /// @param [in] out_config Name of a policy configuration file
            ///        to be written. The contents of the file will be
            ///        obtained from the object's state.
            GlobalPolicy(const std::string in_config, const std::string out_config);
            /// @brief GlobalPolicy destructor
            virtual ~GlobalPolicy();
            int mode(void) const;
            int frequency_mhz(void) const;
            int tdp_percent(void) const;
            int budget_watts(void) const;
            int affinity(void) const;
            int goal(void) const;
            int num_max_perf(void) const;
            const std::string &tree_decider() const;
            const std::string &leaf_decider() const;
            const std::string &platform() const;
            std::string mode_string() const;
            void policy_message(struct geopm_policy_message_s &policy_message);
            void mode(int mode);
            void frequency_mhz(int frequency);
            void tdp_percent(int percentage);
            void budget_watts(int budget);
            void affinity(int cpu_affinity);
            void goal(int geo_goal);
            void num_max_perf(int num_big_cores);
            void tree_decider(const std::string &description);
            void leaf_decider(const std::string &description);
            void platform(const std::string &description);
            void write(void);
            void read(void);
            void enforce_static_mode();
            std::string header(void) const;
        protected:
            /// @brief Structure intended to be shared between
            /// the resource manager and the geopm
            /// runtime in order to convey job wide
            /// power policy changes to the geopm runtime.
            struct m_policy_shmem_s {
                /// @brief Enables the geopm runtime to know when the resource
                ///        manager has initialized the power policy.
                int is_init;
                /// @brief Lock to ensure read/write consistency between the
                ///        resource manager and the geopm runtime.
                pthread_mutex_t lock;
                /// @brief Holds the job power policy as given by the resource
                ///        manager.
                struct geopm_policy_message_s policy;
                /// @brief plugin selection strings
                struct geopm_plugin_description_s plugin;
            };
            /// @brief Take in an affinity enum and fill in its string representation.
            /// @param [in] value the enum value of the affinity.
            /// @param [out] name the string representation of the affinity.
            void affinity_string(int value, std::string &name);
            void read_shm(void);
            void read_json(void);
            void read_json_mode(json_object *mode_obj);
            void read_json_options(json_object *option_obj);
            void write_shm(void);
            void write_json(void);
            void check_valid(void);
            /// @brief input file name
            std::string m_in_config;
            /// @brief output file name
            std::string m_out_config;
            /// @brief power mode
            int m_mode;
            /// @brief power budget in watts
            int m_power_budget_watts;
            /// @brief flags encapsulates frequency, number of 'big'cpus,
            /// 'small' core affinity, TDP percentage, and power goal
            IPolicyFlags *m_flags;
            /// @brief description string for selecting the tree decider
            std::string m_tree_decider;
            /// @brief description string for selecting the leaf decider
            std::string m_leaf_decider;
            /// @brief description string for selecting the platform
            std::string m_platform;
            /// @brief true if reading policy from shared memory, else false
            bool m_is_shm_in;
            /// @brief true if outputting policy to shared memory, else false
            bool m_is_shm_out;
            /// @brief true if the input file is valid, else false
            bool m_do_read;
            /// @brief true if the output file is valid, else false
            bool m_do_write;
            /// @brief structure to use if reading in from shared memory
            struct m_policy_shmem_s *m_policy_shmem_in;
            /// @brief structure to use if writing out to shared memory
            struct m_policy_shmem_s *m_policy_shmem_out;
    };

}

std::ostream& operator<<(std::ostream &os, const geopm::IGlobalPolicy *obj);

#endif
