/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "FrequencyGovernorImp.hpp"

#include <cmath>
#include <unistd.h>

#include <vector>

#include "Exception.hpp"
#include "Helper.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "config.h"

namespace geopm
{
    std::unique_ptr<FrequencyGovernor> FrequencyGovernor::make_unique(PlatformIO &platform_io, PlatformTopo &platform_topo)
    {
        return geopm::make_unique<FrequencyGovernorImp>(platform_io, platform_topo);
    }

    std::shared_ptr<FrequencyGovernor> FrequencyGovernor::make_shared(PlatformIO &platform_io, PlatformTopo &platform_topo)
    {
        return std::make_shared<FrequencyGovernorImp>(platform_io, platform_topo);
    }

    FrequencyGovernorImp::FrequencyGovernorImp(PlatformIO &platform_io, PlatformTopo &platform_topo)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , M_FREQ_STEP(get_limit("CPUINFO::FREQ_STEP"))
        , M_PLAT_FREQ_MIN(get_limit("CPUINFO::FREQ_MIN"))
        , M_PLAT_FREQ_MAX(get_limit("CPUINFO::FREQ_MAX"))
        , m_freq_min(M_PLAT_FREQ_MIN)
        , m_freq_max(M_PLAT_FREQ_MAX)
    {

    }

    FrequencyGovernorImp::~FrequencyGovernorImp()
    {

    }

    double FrequencyGovernorImp::get_limit(const std::string &sig_name) const
    {
        const int domain_type = m_platform_io.signal_domain_type(sig_name);
        double result = NAN;
        if (sig_name == "CPUINFO::FREQ_MIN") {
            result = m_platform_io.read_signal(sig_name, domain_type, 0);
        }
        else if (sig_name == "CPUINFO::FREQ_MAX") {
            result = m_platform_io.read_signal(sig_name, domain_type, 0);
        }
        else if (sig_name == "CPUINFO::FREQ_STEP") {
            result = m_platform_io.read_signal(sig_name, domain_type, 0);
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("FrequencyGovernorImp::" + std::string(__func__) + "(): requested invalid signal name.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    void FrequencyGovernorImp::init_platform_io(void)
    {
        init_platform_io(m_platform_io.control_domain_type("FREQUENCY"));
    }

    void FrequencyGovernorImp::init_platform_io(int freq_ctl_domain_type)
    {
        m_freq_ctl_domain_type = freq_ctl_domain_type;
        const int num_freq_ctl_domain = m_platform_topo.num_domain(m_freq_ctl_domain_type);
        for (int ctl_dom_idx = 0; ctl_dom_idx != num_freq_ctl_domain; ++ctl_dom_idx) {
            m_control_idx.push_back(m_platform_io.push_control("FREQUENCY",
                                                               m_freq_ctl_domain_type,
                                                               ctl_dom_idx));
        }
    }

    int FrequencyGovernorImp::frequency_domain_type(void) const
    {
        return m_freq_ctl_domain_type;
    }

    bool FrequencyGovernorImp::adjust_platform(const std::vector<double> &frequency_request,
                                               std::vector<double> &frequency_actual)
    {
        if (frequency_request.size() != m_control_idx.size()) {
            throw Exception("FrequencyGovernorImp::" + std::string(__func__) +
                            "(): size of request vector does not match size of control domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (frequency_actual.size() != m_control_idx.size()) {
            throw Exception("FrequencyGovernorImp::" + std::string(__func__) +
                            "(): size of actual vector does not match size of control domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        for (size_t idx = 0; idx < m_control_idx.size(); ++idx) {
            if (frequency_request[idx] > m_freq_max) {
                frequency_actual[idx] = m_freq_max;
            }
            else if (frequency_request[idx] < m_freq_min) {
                frequency_actual[idx] = m_freq_min;
            }
            else {
                frequency_actual[idx] = frequency_request[idx];
            }
            m_platform_io.adjust(m_control_idx[idx], frequency_actual[idx]);
        }
        return true;
    }

    bool FrequencyGovernorImp::set_frequency_bounds(double freq_min, double freq_max)
    {
        if (freq_min < M_PLAT_FREQ_MIN || freq_max > M_PLAT_FREQ_MAX ||
            freq_min > freq_max) {
            throw Exception("FrequencyGovernorImp::" + std::string(__func__) + "(): invalid frequency bounds.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);

        }
        bool result = false;
        if (m_freq_min != freq_min ||
            m_freq_max != freq_max) {
            m_freq_min = freq_min;
            m_freq_max = freq_max;
            result = true;
        }
        return result;
    }

    void FrequencyGovernorImp::get_frequency_bounds(double &freq_min, double &freq_max, double &freq_step) const
    {
        freq_min = m_freq_min;
        freq_max = m_freq_max;
        freq_step = M_FREQ_STEP;
    }

    void FrequencyGovernorImp::validate_policy(double &freq_min, double &freq_max) const
    {
        double target_freq_min = std::isnan(freq_min) ? get_limit("CPUINFO::FREQ_MIN") : freq_min;
        double target_freq_max = std::isnan(freq_max) ? get_limit("CPUINFO::FREQ_MAX") : freq_max;
        freq_min = target_freq_min;
        freq_max = target_freq_max;
    }
}
