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

#include "geopm_error.h"
#include "Exception.hpp"
#include "RAPLPlatform.hpp"
#include "geopm_message.h"
#include "geopm_time.h"

namespace geopm
{
    static const int hsx_id = 0x63F;
    static const int ivb_id = 0x63E;
    static const int snb_id = 0x62D;
    static const int NUM_RAPL_DOMAIN = 3;
    static const int NUM_COUNTER = 4;

    RAPLPlatform::RAPLPlatform()
    {

    }

    RAPLPlatform::~RAPLPlatform()
    {

    }

    bool RAPLPlatform::model_supported(int platform_id) const
    {
        return (platform_id == ivb_id || platform_id == snb_id || platform_id == hsx_id);
    }

    void RAPLPlatform::set_implementation(PlatformImp* platform_imp)
    {
        PowerModel *power_model = new PowerModel();
        m_imp = platform_imp;
        m_imp->initialize();
        m_power_model.insert(std::pair <int, PowerModel*>(GEOPM_DOMAIN_PACKAGE, power_model));
        m_power_model.insert(std::pair <int, PowerModel*>(GEOPM_DOMAIN_PACKAGE_UNCORE, power_model));
        m_power_model.insert(std::pair <int, PowerModel*>(GEOPM_DOMAIN_BOARD_MEMORY, power_model));

        m_num_cpu = m_imp->hw_cpu();
        m_num_package = m_imp->package();
        m_num_domain = NUM_RAPL_DOMAIN;
        m_num_counter = NUM_COUNTER;

        int cpu_offset_divisor = m_num_package * m_imp->logical_cpu();

        //Save off the msr offsets for the things we want to observe to avoid a map lookup
        m_observe_msr_offsets.push_back(m_imp->msr_offset("PKG_ENERGY_STATUS"));
        m_observe_msr_offsets.push_back(m_imp->msr_offset("PP0_ENERGY_STATUS"));
        m_observe_msr_offsets.push_back(m_imp->msr_offset("DRAM_ENERGY_STATUS"));
        m_observe_msr_offsets.push_back(m_imp->msr_offset("PERF_FIXED_CTR0"));
        m_observe_msr_offsets.push_back(m_imp->msr_offset("PERF_FIXED_CTR1"));
        m_observe_msr_offsets.push_back(m_imp->msr_offset("PERF_FIXED_CTR2"));
        for (int i = 0; i < m_num_cpu; i++) {
            std::string msr_name("_MSR_PMON_CTR1");
            msr_name.insert(0, std::to_string(i/cpu_offset_divisor));
            msr_name.insert(0, "C");
            m_observe_msr_offsets.push_back(m_imp->msr_offset(msr_name));
        }

        //Save off the msr offsets for the things we want to enforce to avoid a map lookup
        m_enforce_msr_offsets.push_back(m_imp->msr_offset("PKG_ENERGY_LIMIT"));
        m_enforce_msr_offsets.push_back(m_imp->msr_offset("PKG_DRAM_LIMIT"));
        m_enforce_msr_offsets.push_back(m_imp->msr_offset("PKG_PP0_LIMIT"));

    }

    size_t RAPLPlatform::capacity(void)
    {
        return m_imp->package() * NUM_RAPL_DOMAIN + m_imp->hw_cpu() * NUM_COUNTER;
    }

    void RAPLPlatform::sample(std::vector<struct geopm_msr_message_s> &msr_values)
    {
        int count = 0;
        struct geopm_time_s time;

        geopm_time(&time);
        //record per package energy readings
        for (int i = 0; i < m_num_package; i++) {
            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_SIGNAL_TYPE_PKG_ENERGY;
            msr_values[count++].signal = (double)m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, i, m_observe_msr_offsets[0]);
            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_SIGNAL_TYPE_PP0_ENERGY;
            msr_values[count++].signal = (double)m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, i, m_observe_msr_offsets[1]);
            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_SIGNAL_TYPE_DRAM_ENERGY;
            msr_values[count++].signal = (double)m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, i, m_observe_msr_offsets[2]);
        }

        //record per cpu metrics
        for (int i = 0; i < m_num_cpu; i++) {
            msr_values[count].domain_type = GEOPM_DOMAIN_CPU;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_SIGNAL_TYPE_INST_RETIRED;
            msr_values[count++].signal = (double)m_imp->read_msr(GEOPM_DOMAIN_CPU, i, m_observe_msr_offsets[3]);
            msr_values[count].domain_type = GEOPM_DOMAIN_CPU;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_SIGNAL_TYPE_CLK_UNHALTED_CORE;
            msr_values[count++].signal = (double)m_imp->read_msr(GEOPM_DOMAIN_CPU, i, m_observe_msr_offsets[4]);
            msr_values[count].domain_type = GEOPM_DOMAIN_CPU;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_SIGNAL_TYPE_CLK_UNHALTED_REF;
            msr_values[count++].signal = (double)m_imp->read_msr(GEOPM_DOMAIN_CPU, i, m_observe_msr_offsets[5]);
            msr_values[count].domain_type = GEOPM_DOMAIN_CPU;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_SIGNAL_TYPE_LLC_VICTIMS;
            msr_values[count++].signal = (double)m_imp->read_msr(GEOPM_DOMAIN_CPU, i, m_observe_msr_offsets[6 + i]);
        }
    }

    void RAPLPlatform::enforce_policy(const Policy &policy) const
    {

    }
} //geopm
