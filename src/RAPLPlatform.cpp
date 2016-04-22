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

#include "Exception.hpp"
#include "RAPLPlatform.hpp"
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
    {

    }

    RAPLPlatform::~RAPLPlatform()
    {

    }

    bool RAPLPlatform::model_supported(int platform_id, const std::string &description) const
    {
        return ((platform_id == M_IVT_ID || platform_id == M_SNB_ID || platform_id == M_HSX_ID) && description == m_description);
    }

    void RAPLPlatform::set_implementation(PlatformImp* platform_imp)
    {
        m_imp = platform_imp;
        m_imp->initialize();

        m_num_cpu = m_imp->num_hw_cpu();
        m_num_package = m_imp->num_package();
    }

    size_t RAPLPlatform::capacity(void)
    {
        return m_imp->num_package() * (m_imp->num_package_signal() + m_imp->num_cpu_signal());
    }

    void RAPLPlatform::sample(std::vector<struct geopm_msr_message_s> &msr_values)
    {
        int count = 0;
        double accum_freq;
        double accum_inst;
        double accum_clk_core;
        double accum_clk_ref;
        double accum_llc;
        int cpu_per_package = m_num_cpu / m_num_package;
        struct geopm_time_s time;
        //FIXME: Need to deal with counter rollover and unit conversion for energy
        geopm_time(&time);
        //record per package energy readings
        for (int i = 0; i < m_num_package; i++) {
            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_PKG_ENERGY;
            msr_values[count].signal = m_imp->read_signal(GEOPM_DOMAIN_PACKAGE, i, GEOPM_TELEMETRY_TYPE_PKG_ENERGY);
            count++;

            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_PP0_ENERGY;
            msr_values[count].signal = m_imp->read_signal(GEOPM_DOMAIN_PACKAGE, i, GEOPM_TELEMETRY_TYPE_PP0_ENERGY);
            count++;

            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_DRAM_ENERGY;
            msr_values[count].signal = m_imp->read_signal(GEOPM_DOMAIN_PACKAGE, i, GEOPM_TELEMETRY_TYPE_DRAM_ENERGY);
            count++;

            accum_freq = 0.0;
            accum_inst = 0.0;
            accum_clk_core = 0.0;
            accum_clk_ref = 0.0;
            accum_llc = 0.0;
            for (int j = i * cpu_per_package; j < i + cpu_per_package; ++j) {
                accum_freq += m_imp->read_signal(GEOPM_DOMAIN_CPU, j, GEOPM_TELEMETRY_TYPE_FREQUENCY);
                accum_inst += m_imp->read_signal(GEOPM_DOMAIN_CPU, j, GEOPM_TELEMETRY_TYPE_INST_RETIRED);
                accum_clk_core += m_imp->read_signal(GEOPM_DOMAIN_CPU, j, GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE);
                accum_clk_ref += m_imp->read_signal(GEOPM_DOMAIN_CPU, j, GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF);
                accum_llc += m_imp->read_signal(GEOPM_DOMAIN_CPU, j, GEOPM_TELEMETRY_TYPE_LLC_VICTIMS);
            }

            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_FREQUENCY;
            msr_values[count].signal = accum_freq / cpu_per_package;
            count++;

            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_INST_RETIRED;
            msr_values[count].signal = accum_inst;
            count++;

            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE;
            msr_values[count].signal = accum_clk_core;
            count++;

            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF;
            msr_values[count].signal = accum_clk_ref;
            count++;

            msr_values[count].domain_type = GEOPM_DOMAIN_PACKAGE;
            msr_values[count].domain_index = i;
            msr_values[count].timestamp = time;
            msr_values[count].signal_type = GEOPM_TELEMETRY_TYPE_LLC_VICTIMS;
            msr_values[count].signal = accum_llc;;
            count++;
        }
    }

    void RAPLPlatform::enforce_policy(uint64_t region_id, Policy &policy) const
    {
        int control_type;
        std::vector<double> target(m_imp->power_control_domain());
        policy.target(region_id, target);

        if((m_control_domain_type != GEOPM_CONTROL_DOMAIN_POWER) &&
           (m_imp->power_control_domain() == GEOPM_DOMAIN_PACKAGE) &&
           (m_num_package != (int)target.size())) {
            control_type = GEOPM_TELEMETRY_TYPE_PKG_ENERGY;
            for (int i = 0; i < m_num_package; ++i) {
                m_imp->write_control(m_imp->power_control_domain(), i, control_type, target[i]);
            }
        }
        else {
            if (m_control_domain_type != GEOPM_CONTROL_DOMAIN_POWER) {
                throw geopm::Exception("RAPLPlatform::enforce_policy: RAPLPlatform Only handles power control domains", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (m_imp->power_control_domain() != GEOPM_DOMAIN_PACKAGE) {
                throw geopm::Exception("RAPLPlatform::enforce_policy: RAPLPlatform Currently only supports package level power", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
            }
            if (m_num_package != (int)target.size()) {
                throw geopm::Exception("RAPLPlatform::enforce_policy: Policy size does not match domains of control", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }
} //geopm
