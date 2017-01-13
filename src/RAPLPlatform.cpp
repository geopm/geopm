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
#include "geopm_message.h"
#include "geopm_time.h"
#include "config.h"

namespace geopm
{
    RAPLPlatform::RAPLPlatform()
        : Platform()
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

    bool RAPLPlatform::is_model_supported(int platform_id, const std::string &description) const
    {
        return ((platform_id == M_IVT_ID ||
                 platform_id == M_SNB_ID ||
                 platform_id == M_BDX_ID ||
                 platform_id == M_KNL_ID ||
                 platform_id == M_HSX_ID) &&
                description == m_description);
    }

    void RAPLPlatform::sample(struct geopm_time_s &sample_time, std::vector<double> &msr_values)
    {
        m_imp->batch_read_signal(msr_values);
        geopm_time(&sample_time);
    }

    void RAPLPlatform::enforce_policy(uint64_t region_id, IPolicy &policy) const
    {
        std::vector<double> pwr_target(m_imp->num_domain(GEOPM_DOMAIN_CONTROL_POWER));
        std::vector<double> freq_target(m_imp->num_domain(GEOPM_DOMAIN_CONTROL_FREQUENCY));
        policy.target(region_id, GEOPM_DOMAIN_CONTROL_POWER, pwr_target);
        policy.target(region_id, GEOPM_DOMAIN_CONTROL_FREQUENCY, freq_target);

        if ((m_imp->num_domain(GEOPM_DOMAIN_CONTROL_POWER) == (int)pwr_target.size()) &&
            (m_imp->num_domain(GEOPM_DOMAIN_CONTROL_FREQUENCY) == (int)freq_target.size())) {
            if (pwr_target[0] != GEOPM_VALUE_INVALID) {
                int i = 0;
                for (auto it = pwr_target.begin(); it != pwr_target.end(); ++it) {
                    m_imp->write_control(GEOPM_DOMAIN_CONTROL_POWER, i, (*it));
                    ++i;
                }
            }
            if (freq_target[0] != GEOPM_VALUE_INVALID) {
                int i = 0;
                for (auto it = freq_target.begin(); it != freq_target.end(); ++it) {
                    m_imp->write_control(GEOPM_DOMAIN_CONTROL_FREQUENCY, i, (*it));
                    ++i;
                }
            }
        }
        else {
            throw geopm::Exception("RAPLPlatform::enforce_policy: Policy size does not match domains of control", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }
} //geopm
