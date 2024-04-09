/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include "PowerGovernorImp.hpp"

#include <cmath>
#include <limits.h>
#include <vector>
#include <iostream>
#include <unistd.h>

#include "geopm/PlatformIOProf.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"


namespace geopm
{
    std::unique_ptr<PowerGovernor> PowerGovernor::make_unique(void)
    {
        return geopm::make_unique<PowerGovernorImp>();
    }

    std::shared_ptr<PowerGovernor> PowerGovernor::make_shared(void)
    {
        return std::make_shared<PowerGovernorImp>();
    }

    PowerGovernorImp::PowerGovernorImp()
        : PowerGovernorImp(PlatformIOProf::platform_io(), platform_topo())
    {

    }

    PowerGovernorImp::PowerGovernorImp(PlatformIO &platform_io, const PlatformTopo &platform_topo)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , M_CPU_POWER_TIME_WINDOW(0.015)
        , m_pkg_pwr_domain_type(m_platform_io.control_domain_type("CPU_POWER_LIMIT_CONTROL"))
        , m_num_pkg(m_platform_topo.num_domain(m_pkg_pwr_domain_type))
        , M_MIN_PKG_POWER_SETTING(m_platform_io.read_signal("CPU_POWER_MIN_AVAIL", GEOPM_DOMAIN_PACKAGE, 0))
        , M_MAX_PKG_POWER_SETTING(m_platform_io.read_signal("CPU_POWER_MAX_AVAIL", GEOPM_DOMAIN_PACKAGE, 0))
        , m_min_pkg_power_policy(M_MIN_PKG_POWER_SETTING)
        , m_max_pkg_power_policy(M_MAX_PKG_POWER_SETTING)
        , m_last_pkg_power_setting(NAN)
        , m_do_write_batch(false)
    {

    }

    PowerGovernorImp::~PowerGovernorImp()
    {

    }

    void PowerGovernorImp::init_platform_io(void)
    {
        for(int domain_idx = 0; domain_idx < m_num_pkg; ++domain_idx) {
            int control_idx = m_platform_io.push_control("CPU_POWER_LIMIT_CONTROL", m_pkg_pwr_domain_type, domain_idx);
            m_control_idx.push_back(control_idx);
            m_platform_io.write_control("CPU_POWER_TIME_WINDOW_CONTROL", m_pkg_pwr_domain_type, domain_idx, M_CPU_POWER_TIME_WINDOW);
        }
    }

    void PowerGovernorImp::sample_platform(void)
    {

    }

    void PowerGovernorImp::adjust_platform(double node_power_request, double &node_power_actual)
    {
#ifdef GEOPM_DEBUG
        if (!m_control_idx.size()) {
            throw Exception("PowerGovernorImp::" + std::string(__func__) + "(): init_platform_io has not been called.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_do_write_batch = false;
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
                m_do_write_batch = true;
            }
        }
    }

    bool PowerGovernorImp::do_write_batch() const
    {
        return m_do_write_batch;
    }

    void PowerGovernorImp::set_power_bounds(double min_pkg_power, double max_pkg_power)
    {
        if (min_pkg_power < M_MIN_PKG_POWER_SETTING) {
            throw Exception("PowerGovernorImp::" + std::string(__func__) + " invalid min_pkg_power bound.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (max_pkg_power > M_MAX_PKG_POWER_SETTING) {
            throw Exception("PowerGovernorImp::" + std::string(__func__) + " invalid max_pkg_power bound.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_min_pkg_power_policy = min_pkg_power;
        m_max_pkg_power_policy = max_pkg_power;
    }

    double PowerGovernorImp::power_package_time_window(void) const
    {
        return M_CPU_POWER_TIME_WINDOW;
    }
}
