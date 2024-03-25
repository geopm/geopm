/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef SSTCLOSGOVERNORIMP_HPP_INCLUDE
#define SSTCLOSGOVERNORIMP_HPP_INCLUDE

#include "SSTClosGovernor.hpp"

namespace geopm
{
    class SSTClosGovernorImp : public SSTClosGovernor
    {
        public:
            SSTClosGovernorImp();
            SSTClosGovernorImp(PlatformIO &platform_io, const PlatformTopo &platform_topo);
            virtual ~SSTClosGovernorImp() = default;
            void init_platform_io(void) override;
            int clos_domain_type(void) const override;
            void adjust_platform(const std::vector<double> &clos_by_core) override;
            bool do_write_batch(void) const override;

            // No-op if sst-tf is not supported. Mods both tf and cp
            void enable_sst_turbo_prioritization() override;
            void disable_sst_turbo_prioritization() override;

        private:
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            bool m_do_write_batch;
            bool m_is_enabled;
            int m_clos_assoc_ctl_domain_type;
            size_t m_num_clos_assoc_ctl_domain;
            int m_clos_config_ctl_domain_type;
            size_t m_num_clos_config_ctl_domain;
            double m_frequency_min;
            double m_frequency_sticker;
            double m_frequency_max;
            std::vector<int> m_clos_control_idx;
            std::vector<int> m_frequency_control_idx;
            std::vector<double> m_last_clos;
    };
}

#endif // SSTCLOSGOVERNORIMP_HPP_INCLUDE
