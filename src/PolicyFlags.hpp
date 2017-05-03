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

#ifndef POLICYFLAGS_HPP_INCLUDE
#define POLICYFLAGS_HPP_INCLUDE

#include "geopm_policy.h"

namespace geopm
{
    /// @brief PolicyFlags class encapsulated functioality around packing and unpacking
    /// policy settings into/from a 64 bit integer.
    class IPolicyFlags
    {
        public:
            IPolicyFlags() {}
            virtual ~IPolicyFlags() {}
            /// @brief Get the encoded flags
            /// @return Integer representation of flags
            virtual unsigned long flags(void) const = 0;
            /// @brief Get the policy frequency
            /// @return frequency in MHz
            virtual int frequency_mhz(void) const = 0;
            /// @brief Get the policy TDP percentage
            /// @return TDP (thermal design power) percentage between 0-100
            virtual int tdp_percent(void) const = 0;
            /// @brief Get the policy affinity. This is the cores that we
            /// will dynamically control. One of
            /// GEOPM_AFFINITY_SCATTER or
            /// GEOPM_AFFINITY_COMPACT.
            /// @return enum power affinity
            virtual int affinity(void) const = 0;
            /// @brief Get the policy power goal, One of
            /// GEOPM_GOAL_CPU_EFFICIENCY,
            /// GEOPM_GOAL_NETWORK_EFFICIENCY, or
            /// GEOPM_GOAL_MEMORY_EFFICIENCY
            /// @return enum power goal
            virtual int goal(void) const = 0;
            /// @brief Get the number of 'big' cores
            /// @return number of cores where we will run
            ///         unconstrained power.
            virtual int num_max_perf(void) const = 0;
            /// @brief Set the encodoed flags
            /// @param [in] flags Integer representation of flags
            virtual void flags(unsigned long flags) = 0;
            /// @brief Set the policy frequency
            /// @param [in] frequency frequency in MHz
            virtual void frequency_mhz(int frequency) = 0;
            /// @brief Set the policy TDP percentage
            /// @param [in] percentage TDP percentage between 0-100
            virtual void tdp_percent(int percentage) = 0;
            /// @brief Set the policy affinity. This is the cores that we
            /// will dynamically control. One of
            /// GEOPM_AFFINITY_SCATTER or
            /// GEOPM_AFFINITY_COMPACT.
            /// @param [in] cpu_affinity enum power affinity
            virtual void affinity(int cpu_affinity) = 0;
            /// @brief Set the policy power goal. One of
            /// GEOPM_GOAL_CPU_EFFICIENCY,
            /// GEOPM_GOAL_NETWORK_EFFICIENCY, or
            /// GEOPM_GOAL_MEMORY_EFFICIENCY
            /// @param [in] geo_goal enum power goal
            virtual void goal(int geo_goal) = 0;
            /// @brief Set the number of 'big' cores
            /// @param [in] num_big_cores of cores where we will run
            ///        unconstrained power.
            virtual void num_max_perf(int num_big_cores) = 0;
    };

    class PolicyFlags : public IPolicyFlags
    {
        public:
            PolicyFlags(long int flags);
            /// @brief GlobalPolicy destructor
            virtual ~PolicyFlags();
            unsigned long flags(void) const;
            int frequency_mhz(void) const;
            int tdp_percent(void) const;
            int affinity(void) const;
            int goal(void) const;
            int num_max_perf(void) const;
            void flags(unsigned long flags);
            void frequency_mhz(int frequency);
            void tdp_percent(int percentage);
            void affinity(int cpu_affinity);
            void goal(int geo_goal);
            void num_max_perf(int num_big_cores);
        protected:
            /// @brief Encapsulates power policy information as a
            /// 32-bit bitmask.
            enum m_policy_flags_e {
                M_FLAGS_SMALL_CPU_FREQ_100MHZ_1 = 1ULL << 0,
                M_FLAGS_SMALL_CPU_FREQ_100MHZ_2 = 1ULL << 1,
                M_FLAGS_SMALL_CPU_FREQ_100MHZ_4 = 1ULL << 2,
                M_FLAGS_SMALL_CPU_FREQ_100MHZ_8 = 1ULL << 3,
                M_FLAGS_SMALL_CPU_FREQ_100MHZ_16 = 1ULL << 4,
                M_FLAGS_SMALL_CPU_FREQ_100MHZ_32 = 1ULL << 5,
                M_FLAGS_SMALL_CPU_FREQ_100MHZ_64 = 1ULL << 6,
                M_FLAGS_SMALL_CPU_FREQ_100MHZ_128 = 1ULL << 7,
                M_FLAGS_BIG_CPU_NUM_1 = 1ULL << 8,
                M_FLAGS_BIG_CPU_NUM_2 = 1ULL << 9,
                M_FLAGS_BIG_CPU_NUM_4 = 1ULL << 10,
                M_FLAGS_BIG_CPU_NUM_8 = 1ULL << 11,
                M_FLAGS_BIG_CPU_NUM_16 = 1ULL << 12,
                M_FLAGS_BIG_CPU_NUM_32 = 1ULL << 13,
                M_FLAGS_BIG_CPU_NUM_64 = 1ULL << 14,
                M_FLAGS_BIG_CPU_NUM_128 = 1ULL << 15,
                M_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT = 1ULL << 16,
                M_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER = 1ULL << 17,
                M_FLAGS_TDP_PERCENT_1 = 1ULL << 18,
                M_FLAGS_TDP_PERCENT_2 = 1ULL << 19,
                M_FLAGS_TDP_PERCENT_4 = 1ULL << 20,
                M_FLAGS_TDP_PERCENT_8 = 1ULL << 21,
                M_FLAGS_TDP_PERCENT_16 = 1ULL << 22,
                M_FLAGS_TDP_PERCENT_32 = 1ULL << 23,
                M_FLAGS_TDP_PERCENT_64 = 1ULL << 24,
                M_FLAGS_GOAL_CPU_EFFICIENCY = 1ULL << 25,
                M_FLAGS_GOAL_NETWORK_EFFICIENCY = 1ULL << 26,
                M_FLAGS_GOAL_MEMORY_EFFICIENCY = 1ULL << 27,
            };
            unsigned long m_flags;
    };

}
#endif
