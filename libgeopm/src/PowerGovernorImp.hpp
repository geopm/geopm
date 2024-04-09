/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POWERGOVERNORIMP_HPP_INCLUDE
#define POWERGOVERNORIMP_HPP_INCLUDE

#include <memory>
#include <vector>

#include "geopm/PowerGovernor.hpp"

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;

    class PowerGovernorImp : public PowerGovernor
    {
        public:
            PowerGovernorImp();
            PowerGovernorImp(PlatformIO &platform_io, const PlatformTopo &platform_topo);
            virtual ~PowerGovernorImp();
            void init_platform_io(void) override;
            virtual void sample_platform(void) override;
            void adjust_platform(double node_power_request, double &node_power_actual) override;
            bool do_write_batch(void) const override;
            void set_power_bounds(double min_pkg_power, double max_pkg_power) override;
            double power_package_time_window(void) const override;
        private:
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            const double M_CPU_POWER_TIME_WINDOW;
            int m_pkg_pwr_domain_type;
            int m_num_pkg;
            const double M_MIN_PKG_POWER_SETTING;
            const double M_MAX_PKG_POWER_SETTING;
            double m_min_pkg_power_policy;
            double m_max_pkg_power_policy;
            std::vector<int> m_control_idx;
            double m_last_pkg_power_setting;
            bool m_do_write_batch;
    };
}

#endif
