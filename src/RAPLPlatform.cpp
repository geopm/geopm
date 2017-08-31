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

#include "Exception.hpp"
#include "RAPLPlatform.hpp"
#include "PlatformImp.hpp"
#include "Policy.hpp"
#include "geopm_message.h"
#include "geopm_time.h"
#include "config.h"

namespace geopm
{
    RAPLPlatform::RAPLPlatform()
        : Platform(GEOPM_CONTROL_DOMAIN_POWER)
        , m_description("rapl")
        , M_HSX_ID(0x63F)
        , M_IVT_ID(0x63E)
        , M_SNB_ID(0x62D)
        , M_BDX_ID(0x64F)
        , M_KNL_ID(0x657)
    {

    }

    RAPLPlatform::~RAPLPlatform()
    {

    }

    int RAPLPlatform::control_domain()
    {
        return GEOPM_CONTROL_DOMAIN_POWER;
    }

    bool RAPLPlatform::model_supported(int platform_id, const std::string &description) const
    {
        return ((platform_id == M_IVT_ID ||
                 platform_id == M_SNB_ID ||
                 platform_id == M_BDX_ID ||
                 platform_id == M_KNL_ID ||
                 platform_id == M_HSX_ID) &&
                description == m_description);
    }

    void RAPLPlatform::initialize(void)
    {
        m_num_cpu = m_imp->num_hw_cpu();
        m_num_package = m_imp->num_package();
        m_num_tile = m_imp->num_tile();
        m_num_energy_domain = m_imp->num_domain(m_imp->power_control_domain());
        m_num_counter_domain = m_imp->num_domain(m_imp->performance_counter_domain());
        m_batch_desc.resize(m_num_energy_domain * m_imp->num_energy_signal() + m_num_counter_domain * m_imp->num_counter_signal());

        int count = 0;
        int counter_domain_per_energy_domain = m_num_counter_domain / m_num_energy_domain;
        int energy_domain = m_imp->power_control_domain();
        int counter_domain = m_imp->performance_counter_domain();
        for (int i = 0; i < m_num_energy_domain; i++) {
            m_batch_desc[count].device_type = energy_domain;
            m_batch_desc[count].device_index = i;
            m_batch_desc[count].signal_type = GEOPM_TELEMETRY_TYPE_PKG_ENERGY;
            m_batch_desc[count].value = 0;
            ++count;

            m_batch_desc[count].device_type = energy_domain;
            m_batch_desc[count].device_index = i;
            m_batch_desc[count].signal_type = GEOPM_TELEMETRY_TYPE_DRAM_ENERGY;
            m_batch_desc[count].value = 0;
            ++count;

            for (int j = i * counter_domain_per_energy_domain; j < i * counter_domain_per_energy_domain + counter_domain_per_energy_domain; ++j) {
                m_batch_desc[count].device_type = counter_domain;
                m_batch_desc[count].device_index = j;
                m_batch_desc[count].signal_type = GEOPM_TELEMETRY_TYPE_FREQUENCY;
                m_batch_desc[count].value = 0;
                ++count;

                m_batch_desc[count].device_type = counter_domain;
                m_batch_desc[count].device_index = j;
                m_batch_desc[count].signal_type = GEOPM_TELEMETRY_TYPE_INST_RETIRED;
                m_batch_desc[count].value = 0;
                ++count;

                m_batch_desc[count].device_type = counter_domain;
                m_batch_desc[count].device_index = j;
                m_batch_desc[count].signal_type = GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE;
                m_batch_desc[count].value = 0;
                ++count;

                m_batch_desc[count].device_type = counter_domain;
                m_batch_desc[count].device_index = j;
                m_batch_desc[count].signal_type = GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF;
                m_batch_desc[count].value = 0;
                ++count;

                m_batch_desc[count].device_type = counter_domain;
                m_batch_desc[count].device_index = j;
                m_batch_desc[count].signal_type = GEOPM_TELEMETRY_TYPE_READ_BANDWIDTH;
                m_batch_desc[count].value = 0;
                ++count;
            }
        }
        m_imp->batch_read_signal(m_batch_desc, true);
    }

    size_t RAPLPlatform::capacity(void)
    {
        return m_imp->num_domain(m_imp->power_control_domain()) * (m_imp->num_energy_signal() + m_imp->num_counter_signal());
    }

    void RAPLPlatform::bound(double &upper_bound, double &lower_bound)
    {
        double min_pkg;
        double max_pkg;
        double min_dram;
        double max_dram;

        m_imp->bound(GEOPM_TELEMETRY_TYPE_PKG_ENERGY, max_pkg, min_pkg);
        m_imp->bound(GEOPM_TELEMETRY_TYPE_DRAM_ENERGY, max_dram, min_dram);
        upper_bound = max_pkg + max_dram;
        lower_bound = min_pkg + min_dram;
    }

    void RAPLPlatform::sample(std::vector<struct geopm_msr_message_s> &msr_values)
    {
        int count = 0;
        int signal_index = 0;
        double accum_freq;
        double accum_inst;
        double accum_clk_core;
        double accum_clk_ref;
        double accum_read_bw;
        int counter_domain_per_energy_domain = m_num_counter_domain / m_num_energy_domain;
        int energy_domain = m_imp->power_control_domain();
        struct geopm_time_s time;

        m_imp->batch_read_signal(m_batch_desc, false);
        geopm_time(&time);
        //record per package energy readings
        for (int i = 0; i < m_num_energy_domain; i++) {
            msr_values[count].domain_type = energy_domain;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_PKG_ENERGY;
            msr_values[count].signal = m_batch_desc[signal_index++].value;

            count++;
            msr_values[count].domain_type = energy_domain;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_DRAM_ENERGY;
            msr_values[count].signal = m_batch_desc[signal_index++].value;
            count++;

            accum_inst = 0.0;
            accum_clk_core = 0.0;
            accum_clk_ref = 0.0;
            accum_read_bw = 0.0;
            accum_freq = 0.0;

            for (int j = i * counter_domain_per_energy_domain; j < i * counter_domain_per_energy_domain + counter_domain_per_energy_domain; ++j) {
                accum_freq += m_batch_desc[signal_index++].value;
                accum_inst += m_batch_desc[signal_index++].value;
                accum_clk_core += m_batch_desc[signal_index++].value;
                accum_clk_ref += m_batch_desc[signal_index++].value;
                accum_read_bw += m_batch_desc[signal_index++].value;
            }

            msr_values[count].domain_type = energy_domain;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_FREQUENCY;
            msr_values[count].signal = accum_freq / counter_domain_per_energy_domain;
            count++;

            msr_values[count].domain_type = energy_domain;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_INST_RETIRED;
            msr_values[count].signal = accum_inst;
            count++;

            msr_values[count].domain_type = energy_domain;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE;
            msr_values[count].signal = accum_clk_core;
            count++;

            msr_values[count].domain_type = energy_domain;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF;
            msr_values[count].signal = accum_clk_ref;
            count++;

            msr_values[count].domain_type = energy_domain;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_READ_BANDWIDTH;
            msr_values[count].signal = accum_read_bw;
            count++;
        }
    }

    void RAPLPlatform::enforce_policy(uint64_t region_id, IPolicy &policy) const
    {
        int control_type;
        std::vector<double> target(m_num_energy_domain);
        policy.target(region_id, target);

        if ((m_control_domain_type == GEOPM_CONTROL_DOMAIN_POWER) &&
            (m_num_energy_domain == (int)target.size())) {
            control_type = GEOPM_TELEMETRY_TYPE_PKG_ENERGY;
            for (int i = 0; i < m_num_package; ++i) {
                m_imp->write_control(m_imp->power_control_domain(), i, control_type, target[i]);
            }
        }
        else {
            if (m_control_domain_type != GEOPM_CONTROL_DOMAIN_POWER) {
                throw geopm::Exception("RAPLPlatform::enforce_policy: RAPLPlatform Only handles power control domains", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (m_num_energy_domain != (int)target.size()) {
                throw geopm::Exception("RAPLPlatform::enforce_policy: Policy size does not match domains of control", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }
} //geopm
