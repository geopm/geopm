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

namespace geopm
{
    static const int hsx_id = 0x63F;
    static const int ivb_id = 0x63E;
    static const int snb_id = 0x62D;

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

        m_num_cpu = m_imp->get_num_cpu();
        int hyper = m_imp->get_num_hyperthreads();

        //Set up our indexes into the circular buffers for all the observables
        int cpu_offset = hyper ? (m_num_cpu / hyper) : m_num_cpu;
        m_buffer_index.package0_pkg_energy = 0;
        m_buffer_index.package1_pkg_energy = 1;
        m_buffer_index.package0_pp0_energy = 2;
        m_buffer_index.package1_pp0_energy = 3;
        m_buffer_index.package0_dram_energy = 4;
        m_buffer_index.package1_dram_energy = 5;
        m_buffer_index.inst_retired_any_base  = 6;
        m_buffer_index.clk_unhalted_core_base = m_buffer_index.inst_retired_any_base + cpu_offset;
        m_buffer_index.clk_unhalted_ref_base = m_buffer_index.clk_unhalted_core_base + cpu_offset;
        m_buffer_index.llc_victims_base = m_buffer_index.clk_unhalted_ref_base + cpu_offset;
        m_buffer_index.num_slot = m_buffer_index.llc_victims_base + cpu_offset;

        //Save off the msr offsets for the things we want to observe to avoid a map lookup
        m_observe_msr_offsets.push_back(m_imp->get_msr_offset("PKG_ENERGY_STATUS"));
        m_observe_msr_offsets.push_back(m_imp->get_msr_offset("PP0_ENERGY_STATUS"));
        m_observe_msr_offsets.push_back(m_imp->get_msr_offset("DRAM_ENERGY_STATUS"));
        m_observe_msr_offsets.push_back(m_imp->get_msr_offset("PERF_FIXED_CTR0"));
        m_observe_msr_offsets.push_back(m_imp->get_msr_offset("PERF_FIXED_CTR1"));
        m_observe_msr_offsets.push_back(m_imp->get_msr_offset("PERF_FIXED_CTR2"));
        for (int i = 0; i < m_num_cpu; i++) {
            std::string msr_name("_MSR_PMON_CTR1");
            msr_name.insert(0, std::to_string(i));
            msr_name.insert(0, "C");
            m_observe_msr_offsets.push_back(m_imp->get_msr_offset(msr_name));
        }

        //Save off the msr offsets for the things we want to enforce to avoid a map lookup
        m_enforce_msr_offsets.push_back(m_imp->get_msr_offset("PKG_ENERGY_LIMIT"));
        m_enforce_msr_offsets.push_back(m_imp->get_msr_offset("PKG_DRAM_LIMIT"));
        m_enforce_msr_offsets.push_back(m_imp->get_msr_offset("PKG_PP0_LIMIT"));

    }

    void RAPLPlatform::observe(void)
    {
        //record per package energy readings
        m_curr_phase->observation_insert(m_buffer_index.package0_pkg_energy, (double)m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, 0, m_observe_msr_offsets[0]));
        m_curr_phase->observation_insert(m_buffer_index.package0_pp0_energy, (double)m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, 0, m_observe_msr_offsets[1]));
        m_curr_phase->observation_insert(m_buffer_index.package0_dram_energy, (double)m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, 0, m_observe_msr_offsets[2]));
        m_curr_phase->observation_insert(m_buffer_index.package1_pkg_energy, (double)m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, 1, m_observe_msr_offsets[0]));
        m_curr_phase->observation_insert(m_buffer_index.package1_pp0_energy, (double)m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, 1, m_observe_msr_offsets[1]));
        m_curr_phase->observation_insert(m_buffer_index.package1_dram_energy, (double)m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, 1, m_observe_msr_offsets[2]));

        //record per cpu metrics
        for (int i = 0; i < m_num_cpu; i++) {
            m_curr_phase->observation_insert((m_buffer_index.inst_retired_any_base + i),
                                             (double)m_imp->read_msr(GEOPM_DOMAIN_CPU, i, m_observe_msr_offsets[3]));
            m_curr_phase->observation_insert((m_buffer_index.clk_unhalted_core_base + i),
                                             (double)m_imp->read_msr(GEOPM_DOMAIN_CPU, i, m_observe_msr_offsets[4]));
            m_curr_phase->observation_insert((m_buffer_index.clk_unhalted_ref_base + i),
                                             (double)m_imp->read_msr(GEOPM_DOMAIN_CPU, i, m_observe_msr_offsets[5]));
            m_curr_phase->observation_insert((m_buffer_index.llc_victims_base + i),
                                             (double)m_imp->read_msr(GEOPM_DOMAIN_CPU, i, m_observe_msr_offsets[6 + i]));
        }
    }

    void RAPLPlatform::sample(struct geopm_sample_message_s &sample) const
    {
        sample.phase_id = m_curr_phase->identifier();
        sample.runtime = m_curr_phase->observation_mean(0);
        sample.progress = m_curr_phase->observation_mean(0);
        sample.energy = m_curr_phase->observation_mean(0);
        sample.frequency = m_curr_phase->observation_mean(0);
    }

    void RAPLPlatform::enforce_policy(const Policy &policy) const
    {

    }
} //geopm
