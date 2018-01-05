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

#include "PolicyFlags.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    PolicyFlags::PolicyFlags(long int flags)
        : m_flags(flags)
    {

    }

    PolicyFlags::~PolicyFlags()
    {

    }

    unsigned long PolicyFlags::flags(void) const
    {
        return m_flags;
    }

    int PolicyFlags::frequency_mhz(void) const
    {
        long int freq = m_flags & 0x00000000000000FFUL;
        return (int)freq*100;
    }

    int PolicyFlags::tdp_percent(void) const
    {
        long int perc = (m_flags & 0x0000000001FC0000UL) >> 18;
        return (int)perc;;
    }

    int PolicyFlags::affinity(void) const
    {
        int result = 0;
        long int affinity_flag = (m_flags & 0x0000000000030000UL);
        switch (affinity_flag) {
            case M_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT:
                result = GEOPM_POLICY_AFFINITY_COMPACT;
                break;
            case M_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER:
                result = GEOPM_POLICY_AFFINITY_SCATTER;
                break;
            default:
                result = GEOPM_POLICY_AFFINITY_INVALID;
                break;
        }

        return result;
    }

    int PolicyFlags::goal(void) const
    {
        int result = 0;
        long int goal_flag = (m_flags & 0x000000000E000000UL);
        switch (goal_flag) {
            case M_FLAGS_GOAL_CPU_EFFICIENCY:
                result = GEOPM_POLICY_GOAL_CPU_EFFICIENCY;
                break;
            case M_FLAGS_GOAL_NETWORK_EFFICIENCY:
                result = GEOPM_POLICY_GOAL_NETWORK_EFFICIENCY;
                break;
            case M_FLAGS_GOAL_MEMORY_EFFICIENCY:
                result = GEOPM_POLICY_GOAL_MEMORY_EFFICIENCY;
            default:
                throw Exception("PolicyFlags::goal(): input does not match any geopm_policy_goal_e values.", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }

        return result;
    }

    int PolicyFlags::num_max_perf(void) const
    {
        long int max_num_perf = (m_flags & 0x000000000000FF00UL) >> 8;
        return max_num_perf;
    }

    void PolicyFlags::flags(unsigned long flags)
    {
        m_flags = flags;
    }

    void PolicyFlags::frequency_mhz(int frequency)
    {
        m_flags = m_flags & 0xFFFFFFFFFFFFFF00UL;
        //Convert to 100s of MHZ
        //Rounds frequency precisionto a 10th of a GHz
        m_flags = m_flags | (frequency/100);
    }

    void PolicyFlags::tdp_percent(int percentage)
    {
        m_flags = m_flags & 0xFFFFFFFFFE03FFFFUL;
        long int perc = percentage << 18;
        m_flags = m_flags | perc;
    }

    void PolicyFlags::affinity(int affinity)
    {
        long int affinity_flag = 0;
        switch (affinity) {
            case GEOPM_POLICY_AFFINITY_COMPACT:
                affinity_flag = M_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT;
                break;
            case GEOPM_POLICY_AFFINITY_SCATTER:
                affinity_flag = M_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER;
                break;
            default:
                throw Exception("PolicyFlags::affinity(): input does not match any geopm_policy_affinity_e values.", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
        m_flags = m_flags & 0xFFFFFFFFFFFCFFFFUL;
        m_flags = m_flags | affinity_flag;
    }

    void PolicyFlags::goal(int geo_goal)
    {
        long int goal_flag = 0;
        switch (geo_goal) {
            case GEOPM_POLICY_GOAL_CPU_EFFICIENCY:
                goal_flag = M_FLAGS_GOAL_CPU_EFFICIENCY;
                break;
            case GEOPM_POLICY_GOAL_NETWORK_EFFICIENCY:
                goal_flag = M_FLAGS_GOAL_NETWORK_EFFICIENCY;
                break;
            case GEOPM_POLICY_GOAL_MEMORY_EFFICIENCY:
                goal_flag = M_FLAGS_GOAL_MEMORY_EFFICIENCY;
            default:
                throw Exception("PolicyFlags::goal(): input does not match any geopm_policy_goal_e values.", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
        m_flags = m_flags & 0xFFFFFFFFF1FFFFFFUL;
        m_flags = m_flags | goal_flag;
    }

    void PolicyFlags::num_max_perf(int num_big_cores)
    {
        m_flags = m_flags & 0xFFFFFFFFFFFF00FFUL;
        long int num_cpu = num_big_cores << 8;
        m_flags = m_flags | num_cpu;
    }
}
