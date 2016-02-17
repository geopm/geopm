/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#ifndef POLICYFLAGS_HPP_INCLUDE
#define POLICYFLAGS_HPP_INCLUDE

#include "geopm_message.h"

namespace geopm
{
    class PolicyFlags
    {
        public:
            PolicyFlags(long int flags);
            /// @brief GlobalPolicy destructor
            virtual ~PolicyFlags();
            /// @brief Get the encoded flags
            /// @return Integer representation of flags
            unsigned long flags(void) const;
            /// @brief Get the policy frequency
            /// @return frequency in MHz
            int frequency_mhz(void) const;
            /// @brief Get the policy TDP percentage
            /// @return TDP (thermal design power) percentage between 0-100
            int tdp_percent(void) const;
            /// @brief Get the policy affinity. This is the cores that we
            /// will dynamically control. One of
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT or
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT.
            /// @return enum power affinity
            int affinity(void) const;
            /// @brief Get the policy power goal, One of
            /// GEOPM_FLAGS_GOAL_CPU_EFFICIENCY,
            /// GEOPM_FLAGS_GOAL_NETWORK_EFFICIENCY, or
            /// GEOPM_FLAGS_GOAL_MEMORY_EFFICIENCY
            /// @return enum power goal
            int goal(void) const;
            /// @brief Get the number of 'big' cores
            /// @return number of cores where we will run
            ///         unconstrained power.
            int num_max_perf(void) const;
            /// @brief Set the encodoed flags
            /// @param [in] flags Integer representation of flags
            void flags(unsigned long flags);
            /// @brief Set the policy frequency
            /// @param [in] frequency frequency in MHz
            void frequency_mhz(int frequency);
            /// @brief Set the policy TDP percentage
            /// @param [in] percentage TDP percentage between 0-100
            void tdp_percent(int percentage);
            /// @brief Set the policy affinity. This is the cores that we
            /// will dynamically control. One of
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER or
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT.
            /// @param [in] cpu_affinity enum power affinity
            void affinity(int cpu_affinity);
            /// @brief Set the policy power goal. One of
            /// GEOPM_FLAGS_GOAL_CPU_EFFICIENCY,
            /// GEOPM_FLAGS_GOAL_NETWORK_EFFICIENCY, or
            /// GEOPM_FLAGS_GOAL_MEMORY_EFFICIENCY
            /// @param [in] geo_goal enum power goal
            void goal(int geo_goal);
            /// @brief Set the number of 'big' cores
            /// @param [in] num_big_cores of cores where we will run
            ///        unconstrained power.
            void num_max_perf(int num_big_cores);
        protected:
            unsigned long m_flags;
    };

}
#endif
