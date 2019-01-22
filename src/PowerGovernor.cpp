/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#include <cmath>
#include <limits.h>
#include <vector>
#include <iostream>
#include <unistd.h>

#include "CircularBuffer.hpp"
#include "Helper.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "PowerGovernor.hpp"
#include "config.h"

namespace geopm
{
    PowerGovernor::PowerGovernor(IPlatformIO &platform_io, IPlatformTopo &platform_topo)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , M_POWER_PACKAGE_TIME_WINDOW(0.015)
        , m_pkg_pwr_domain_type(m_platform_io.control_domain_type("POWER_PACKAGE_LIMIT"))
        , m_num_pkg(m_platform_topo.num_domain(m_pkg_pwr_domain_type))
        , M_MIN_PKG_POWER_SETTING(m_platform_io.read_signal("POWER_PACKAGE_MIN", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , M_MAX_PKG_POWER_SETTING(m_platform_io.read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_min_pkg_power_policy(M_MIN_PKG_POWER_SETTING)
        , m_max_pkg_power_policy(M_MAX_PKG_POWER_SETTING)
        , m_last_pkg_power_setting(NAN)
    {

    }

    PowerGovernor::~PowerGovernor()
    {

    }

    void PowerGovernor::init_platform_io(void)
    {
        for(int domain_idx = 0; domain_idx < m_num_pkg; ++domain_idx) {
            int control_idx = m_platform_io.push_control("POWER_PACKAGE_LIMIT", m_pkg_pwr_domain_type, domain_idx);
            m_control_idx.push_back(control_idx);
            m_platform_io.write_control("POWER_PACKAGE_TIME_WINDOW", m_pkg_pwr_domain_type, domain_idx, M_POWER_PACKAGE_TIME_WINDOW);
        }
    }

    void PowerGovernor::sample_platform(void)
    {

    }

    bool PowerGovernor::adjust_platform(double node_power_request, double &node_power_actual)
    {
#ifdef GEOPM_DEBUG
        if (!m_control_idx.size()) {
            throw Exception("PowerGovernor::" + std::string(__func__) + "(): init_platform_io has not been called.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        if (!std::isnan(node_power_request)) {
            double target_pkg_power = node_power_request / m_num_pkg;
            if (target_pkg_power < m_min_pkg_power_policy) {
                target_pkg_power = m_min_pkg_power_policy;
            }
            else if (target_pkg_power > m_max_pkg_power_policy) {
                target_pkg_power = m_max_pkg_power_policy;
            }
            if (m_last_pkg_power_setting != target_pkg_power) {
                for (auto ctl_idx : m_control_idx) {
                    m_platform_io.adjust(ctl_idx, target_pkg_power);
                }
                m_last_pkg_power_setting = target_pkg_power;
                node_power_actual = m_num_pkg * target_pkg_power;
                result = true;
            }
        }
        return result;
    }

    void PowerGovernor::set_power_bounds(double min_pkg_power, double max_pkg_power)
    {
        if (min_pkg_power < M_MIN_PKG_POWER_SETTING) {
            throw Exception("PowerGovernor::" + std::string(__func__) + " invalid min_pkg_power bound.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (max_pkg_power > M_MAX_PKG_POWER_SETTING) {
            throw Exception("PowerGovernor::" + std::string(__func__) + " invalid max_pkg_power bound.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_min_pkg_power_policy = min_pkg_power;
        m_max_pkg_power_policy = max_pkg_power;
    }

    double PowerGovernor::power_package_time_window(void) const
    {
        return M_POWER_PACKAGE_TIME_WINDOW;
    }
}
