/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "SSTClosGovernor.hpp"

#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "SSTClosGovernorImp.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"

#include "config.h"

namespace geopm
{
    std::unique_ptr<SSTClosGovernor> SSTClosGovernor::make_unique(void)
    {
        return geopm::make_unique<SSTClosGovernorImp>();
    }

    std::shared_ptr<SSTClosGovernor> SSTClosGovernor::make_shared(void)
    {
        return std::make_shared<SSTClosGovernorImp>();
    }

    SSTClosGovernorImp::SSTClosGovernorImp()
        : SSTClosGovernorImp(platform_io(), platform_topo())
    {
    }

    SSTClosGovernorImp::SSTClosGovernorImp(PlatformIO &platform_io, const PlatformTopo &platform_topo)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_do_write_batch(false)
        , m_is_enabled(true)
        , m_clos_assoc_ctl_domain_type(m_platform_io.control_domain_type("SST::COREPRIORITY:ASSOCIATION"))
        , m_num_clos_assoc_ctl_domain(m_platform_topo.num_domain(m_clos_assoc_ctl_domain_type))
        , m_clos_config_ctl_domain_type(m_platform_io.control_domain_type("SST::COREPRIORITY:0:FREQUENCY_MIN"))
        , m_num_clos_config_ctl_domain(m_platform_topo.num_domain(m_clos_config_ctl_domain_type))
        , m_frequency_min(m_platform_io.read_signal("CPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        , m_frequency_sticker(m_platform_io.read_signal("CPU_FREQUENCY_STICKER", GEOPM_DOMAIN_BOARD, 0))
        , m_frequency_max(m_platform_io.read_signal("CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        , m_last_clos(m_num_clos_assoc_ctl_domain, HIGH_PRIORITY)
    {
    }

    bool SSTClosGovernor::is_supported(PlatformIO &platform_io)
    {
        bool result = false;

        try {
            result = platform_io.read_signal("SST::COREPRIORITY_SUPPORT:CAPABILITIES",
                                             GEOPM_DOMAIN_BOARD, 0) > 0;
            result &= platform_io.read_signal("SST::TURBOFREQ_SUPPORT:SUPPORTED",
                                              GEOPM_DOMAIN_BOARD, 0) > 0;
        }
        catch (...) {
            result = false;
        }

        return result;
    }

    void SSTClosGovernorImp::init_platform_io(void)
    {
        for (size_t ctl_idx = 0; ctl_idx < m_num_clos_assoc_ctl_domain; ++ctl_idx) {
            m_clos_control_idx.push_back(m_platform_io.push_control(
                "SST::COREPRIORITY:ASSOCIATION",
                m_clos_assoc_ctl_domain_type,
                static_cast<int>(ctl_idx)));
        }
        // Increase the turbo ratio limits so we can take advantage of the
        // increased range offered by SST-TF
        m_platform_io.write_control("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0",
                                    GEOPM_DOMAIN_BOARD, 0, 255e8);
        m_platform_io.write_control("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_1",
                                    GEOPM_DOMAIN_BOARD, 0, 255e8);
        m_platform_io.write_control("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_2",
                                    GEOPM_DOMAIN_BOARD, 0, 255e8);
        m_platform_io.write_control("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_3",
                                    GEOPM_DOMAIN_BOARD, 0, 255e8);
        m_platform_io.write_control("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_4",
                                    GEOPM_DOMAIN_BOARD, 0, 255e8);
        m_platform_io.write_control("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_5",
                                    GEOPM_DOMAIN_BOARD, 0, 255e8);
        m_platform_io.write_control("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_6",
                                    GEOPM_DOMAIN_BOARD, 0, 255e8);
        m_platform_io.write_control("MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7",
                                    GEOPM_DOMAIN_BOARD, 0, 255e8);

        // Start everything in the high-priority class of service.
        m_platform_io.write_control("SST::COREPRIORITY:ASSOCIATION",
                                    GEOPM_DOMAIN_BOARD, 0, HIGH_PRIORITY);

        enable_sst_turbo_prioritization();

        // Highest priority bucket. Start by distributing at the bottom of the
        // turbo range, but give high priority to distribute up to max.
        m_platform_io.write_control("SST::COREPRIORITY:0:PRIORITY",
                                    GEOPM_DOMAIN_BOARD, 0, 0.0);
        m_platform_io.write_control("SST::COREPRIORITY:0:FREQUENCY_MIN",
                                    GEOPM_DOMAIN_BOARD, 0, m_frequency_sticker);
        m_platform_io.write_control("SST::COREPRIORITY:0:FREQUENCY_MAX",
                                    GEOPM_DOMAIN_BOARD, 0, m_frequency_max);

        // Next-highest bucket. Apply the same ranges, but with less priority.
        m_platform_io.write_control("SST::COREPRIORITY:1:PRIORITY",
                                    GEOPM_DOMAIN_BOARD, 0, 0.34);
        m_platform_io.write_control("SST::COREPRIORITY:1:FREQUENCY_MIN",
                                    GEOPM_DOMAIN_BOARD, 0, m_frequency_sticker);
        m_platform_io.write_control("SST::COREPRIORITY:1:FREQUENCY_MAX",
                                    GEOPM_DOMAIN_BOARD, 0, m_frequency_max);

        // First low-priority bucket. Initially just give it the bottom of the
        // turbo range. Potentially go lower at run time.
        m_platform_io.write_control("SST::COREPRIORITY:2:PRIORITY",
                                    GEOPM_DOMAIN_BOARD, 0, 0.67);
        m_platform_io.write_control("SST::COREPRIORITY:2:FREQUENCY_MIN",
                                    GEOPM_DOMAIN_BOARD, 0, m_frequency_sticker);
        m_platform_io.write_control("SST::COREPRIORITY:2:FREQUENCY_MAX",
                                    GEOPM_DOMAIN_BOARD, 0, (m_frequency_sticker + m_frequency_max) / 2);

        // Least prioritized bucket. Initially just give it the bottom of the
        // turbo range. Potentially go lower at run time.
        m_platform_io.write_control("SST::COREPRIORITY:3:PRIORITY",
                                    GEOPM_DOMAIN_BOARD, 0, 1.0);
        m_platform_io.write_control("SST::COREPRIORITY:3:FREQUENCY_MIN",
                                    GEOPM_DOMAIN_BOARD, 0, m_frequency_sticker);
        m_platform_io.write_control("SST::COREPRIORITY:3:FREQUENCY_MAX",
                                    GEOPM_DOMAIN_BOARD, 0, m_frequency_sticker);
    }

    int SSTClosGovernorImp::clos_domain_type(void) const
    {
        return m_clos_assoc_ctl_domain_type;
    }

    void SSTClosGovernorImp::adjust_platform(
        const std::vector<double> &clos_by_core)
    {
        if (clos_by_core.size() != m_num_clos_assoc_ctl_domain) {
            throw Exception("SSTClosGovernorImp::" + std::string(__func__) +
                                "(): size of request vector does not match size of control domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_do_write_batch = m_is_enabled && clos_by_core != m_last_clos;
        if (m_is_enabled) {
            for (size_t idx = 0; idx < m_num_clos_assoc_ctl_domain; ++idx) {
                m_platform_io.adjust(m_clos_control_idx[idx], clos_by_core[idx]);
            }
            m_last_clos = clos_by_core;
        }
    }

    bool SSTClosGovernorImp::do_write_batch(void) const
    {
        return m_do_write_batch;
    }

    void SSTClosGovernorImp::enable_sst_turbo_prioritization()
    {
        // Enable prioritized turbo by cores
        m_platform_io.write_control("SST::COREPRIORITY_ENABLE:ENABLE",
                                    GEOPM_DOMAIN_BOARD, 0, 1);

        // Enable the ability to extend the turbo range of high priority cores
        // by decreasing the turbo range of low priority cores
        m_platform_io.write_control("SST::TURBO_ENABLE:ENABLE",
                                    GEOPM_DOMAIN_BOARD, 0, 1);

        m_is_enabled = true;
    }

    void SSTClosGovernorImp::disable_sst_turbo_prioritization()
    {
        m_is_enabled = false;

        // Disable the ability to extend the turbo range of high priority cores
        // by decreasing the turbo range of low priority cores
        m_platform_io.write_control("SST::TURBO_ENABLE:ENABLE",
                                    GEOPM_DOMAIN_BOARD, 0, 0);

        // Disable prioritized turbo by cores
        m_platform_io.write_control("SST::COREPRIORITY_ENABLE:ENABLE",
                                    GEOPM_DOMAIN_BOARD, 0, 0);
    }
}
