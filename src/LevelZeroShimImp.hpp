/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#ifndef LEVELZEROSHIMIMP_HPP_INCLUDE
#define LEVELZEROSHIMIMP_HPP_INCLUDE

#include <string>

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

#include "LevelZeroShim.hpp"

#include "geopm_time.h"

namespace geopm
{
    class LevelZeroShimImp : public LevelZeroShim
    {
        public:
            LevelZeroShimImp(const int num_cpu);
            virtual ~LevelZeroShimImp();
            virtual int num_accelerator(void) const override;
            virtual int frequency_domain_count(unsigned int accel_idx, geopm_levelzero_domain_e domain) const override;
            virtual double frequency_status(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const override;
            virtual double frequency_min(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const override;
            virtual double frequency_max(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const override;

            virtual int engine_domain_count(unsigned int accel_idx, geopm_levelzero_domain_e domain) const override;
            virtual uint64_t active_time(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const override;
            virtual uint64_t active_time_timestamp(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const override;

            virtual int energy_domain_count_device(unsigned int accel_idx) const override;
            virtual int energy_domain_count_subdevice(unsigned int accel_idx, int domain_idx) const override;
            virtual uint64_t energy(unsigned int accel_idx, int domain_idx) const override;
            virtual uint64_t energy_timestamp(unsigned int accel_idx, int domain_idx) const override;
            virtual int32_t power_limit_tdp(unsigned int accel_idx, int domain_idx) const override;
            virtual int32_t power_limit_min(unsigned int accel_idx, int domain_idx) const override;
            virtual int32_t power_limit_max(unsigned int accel_idx, int domain_idx) const override;

            virtual void frequency_control(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx, double setting) const override;

        private:
            const unsigned int M_NUM_CPU;

            struct frequency_s {
                double voltage = 0;
                double request  = 0;
                double tdp = 0;
                double efficient = 0;
                double actual = 0;
                uint64_t throttle_reasons = 0;
            };
            struct power_limit_s {
                int32_t tdp = 0;
                int32_t min= 0;
                int32_t max = 0;
            };

            virtual void domain_cache(unsigned int accel_idx);
            virtual void check_ze_result(ze_result_t ze_result, int error, std::string message, int line) const;
            virtual int num_accelerator(ze_device_type_t type) const;

            virtual std::pair<double, double> frequency_min_max(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const;
            virtual std::pair<uint64_t, uint64_t> energy_pair(unsigned int accel_idx, int domain_idx) const;
            virtual std::pair<uint64_t, uint64_t> active_time_pair(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const;

            virtual power_limit_s power_limit_default(unsigned int accel_idx, int domain_idx) const;
            virtual frequency_s frequency_status_shim(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const;

            uint32_t m_num_driver;
            uint32_t m_num_device;
            uint32_t m_num_integrated_gpu;
            uint32_t m_num_board_gpu;
            uint32_t m_num_fpga;
            uint32_t m_num_mca;

            std::vector<ze_driver_handle_t> m_levelzero_driver;
            // TODO: This will need to be a map or vector of vectors if we're supporting multiple
            //       types of sysman devices such as <BOARD GPU devices, PACKAGE GPU device, FPGA device>
            std::vector<zes_device_handle_t> m_sysman_device;

            struct subdevice_s {
                // Could treat this like the other domains to be consistent, but it does not
                // have GPU and MEM domains
                std::vector<zes_pwr_handle_t> m_power_domain;
                //These are enum geopm_levelzero_domain_e indexed
                std::vector<std::vector<zes_freq_handle_t> > m_freq_domain;
                std::vector<std::vector<zes_engine_handle_t> > m_engine_domain;
                //std::vector<std::vector<zes_perf_handle_t> > m_perf_domain;
                //std::vector<std::vector<zes_standby_handle_t> > m_standby_domain;
                //std::vector<std::vector<zes_mem_handle_t> > m_mem_domain;
                //std::vector<std::vector<zes_temp_handle_t> > m_temperature_domain;
                //std::vector<std::vector<zes_fabric_port_handle_t> > m_fabric_domain;
            };

            struct device_info_s {
                zes_device_handle_t device_handle; // likely not needed
                ze_device_properties_t property;
                //ze_device_type_t device_type; // already a part of property above
                uint32_t m_num_subdevice;
                // device type doesn't tell us board vs integrated GPU.  So we have our own
                //geopm_levelzero_type_e geopm_device_type;
                std::vector<zes_device_handle_t> subdevice_handle;

                // Sub-Device domain tracking
                // Currently tracking all sub-devices together
                // until we move to expose GEOPM_BOARD_ACCELERATOR_SUBDEVICE
                subdevice_s subdevice;
                //std::vector<subdevice_s> subdevice;

                // Device/Package domain
                std::vector<zes_pwr_handle_t> m_power_domain;
                std::vector<zes_temp_handle_t> m_temperature_domain;
            };
            std::vector<device_info_s> m_devices;
    };
}
#endif
