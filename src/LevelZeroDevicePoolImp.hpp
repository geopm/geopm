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

#ifndef LEVELZERODEVICEPOOLIMP_HPP_INCLUDE
#define LEVELZERODEVICEPOOLIMP_HPP_INCLUDE

#include <string>

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

#include "LevelZeroDevicePool.hpp"

#include "geopm_time.h"

namespace geopm
{

    class LevelZeroDevicePoolImp : public LevelZeroDevicePool
    {
        public:
            LevelZeroDevicePoolImp(const int num_cpu);
            virtual ~LevelZeroDevicePoolImp();
            virtual int num_accelerator(void) const override;
            virtual double frequency_status_gpu(unsigned int accel_idx) const override;
            virtual double frequency_status_mem(unsigned int accel_idx) const override;
            virtual double frequency_min_gpu(unsigned int accel_idx) const override;
            virtual double frequency_max_gpu(unsigned int accel_idx) const override;
            virtual double frequency_min_mem(unsigned int accel_idx) const override;
            virtual double frequency_max_mem(unsigned int accel_idx) const override;
            virtual double frequency_range_min_gpu(unsigned int accel_idx) const override;
            virtual double frequency_range_max_gpu(unsigned int accel_idx) const override;
            virtual uint64_t frequency_status_throttle_reason_gpu(unsigned int accel_idx) const override;
            virtual uint64_t frequency_throttle_time_gpu(unsigned int accel_idx) const override;
            virtual uint64_t frequency_throttle_time_timestamp_gpu(unsigned int accel_idx) const override;
            virtual double temperature(unsigned int accel_idx) const override;
            virtual double temperature_gpu(unsigned int accel_idx) const override;
            virtual double temperature_memory(unsigned int accel_idx) const override;
            virtual uint64_t active_time(unsigned int accel_idx) const override;
            virtual uint64_t active_time_compute(unsigned int accel_idx) const override;
            virtual uint64_t active_time_copy(unsigned int accel_idx) const override;
            virtual uint64_t active_time_media_decode(unsigned int accel_idx) const override;
            virtual uint64_t active_time_timestamp(unsigned int accel_idx) const override;
            virtual uint64_t active_time_timestamp_compute(unsigned int accel_idx) const override;
            virtual uint64_t active_time_timestamp_copy(unsigned int accel_idx) const override;
            virtual uint64_t active_time_timestamp_media_decode(unsigned int accel_idx) const override;

            virtual int32_t power_limit_tdp(unsigned int accel_idx) const override;
            virtual int32_t power_limit_min(unsigned int accel_idx) const override;
            virtual int32_t power_limit_max(unsigned int accel_idx) const override;
            virtual bool power_limit_enabled_sustained(unsigned int accel_idx) const override;
            virtual bool power_limit_enabled_burst(unsigned int accel_idx) const override;
            virtual int32_t power_limit_interval_sustained(unsigned int accel_idx) const override;
            virtual int32_t power_limit_sustained(unsigned int accel_idx) const override;
            virtual int32_t power_limit_burst(unsigned int accel_idx) const override;
            virtual int32_t power_limit_peak_ac(unsigned int accel_idx) const override;
            virtual uint64_t energy(unsigned int accel_idx) const override;
            virtual uint64_t energy_timestamp(unsigned int accel_idx) const override;
            virtual double performance_factor(unsigned int accel_idx) const override;
            virtual std::vector<uint32_t> active_process_list(unsigned int accel_idx) const override;
            virtual double standby_mode(unsigned int accel_idx) const override;
            virtual double memory_allocated(unsigned int accel_idx) const override;
            virtual void energy_threshold_control(unsigned int accel_idx, double setting) const override;
            virtual void frequency_control_gpu(unsigned int accel_idx,  double setting) const override;
            virtual void standby_mode_control(unsigned int accel_idx, double setting) const override;

        private:
            const unsigned int M_NUM_CPU;
            virtual void domain_cache(unsigned int accel_idx);
            virtual void check_accel_range(unsigned int accel_idx) const;
            virtual void check_domain_range(int size, const char *func, int line) const;
            virtual void check_ze_result(ze_result_t ze_result, int error, std::string message, int line) const;
            virtual int num_accelerator(ze_device_type_t type) const;

            virtual std::tuple<zes_power_sustained_limit_t,
                               zes_power_burst_limit_t,
                               zes_power_peak_limit_t> power_limit(unsigned int accel_idx) const;

            virtual std::tuple<int32_t, int32_t, int32_t> power_limit_default(unsigned int accel_idx) const;
            virtual std::tuple<double, double, double, double, double, uint64_t> frequency_status(unsigned int accel_idx, zes_freq_domain_t) const;
            virtual std::pair<double, double> frequency_min_max(unsigned int accel_idx, zes_freq_domain_t type) const;
            virtual std::pair<double, double> frequency_range(unsigned int accel_idx, zes_freq_domain_t type) const;
            virtual std::pair<uint64_t, uint64_t> frequency_throttle_time(unsigned int accel_idx, zes_freq_domain_t type) const;
            virtual std::pair<uint64_t, uint64_t> energy_pair(unsigned int accel_idx) const;
            virtual std::pair<uint64_t, uint64_t> active_time(unsigned int accel_idx, zes_engine_group_t engine_type) const;
            virtual double temperature(int accel_idx, zes_temp_sensors_t sensor_type) const;
            virtual void frequency_control(unsigned int accel_idx, double min_freq, double max_freq, zes_freq_domain_t) const;

            std::string ze_error_string(ze_result_t result);

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

            // TODO: These will need to be something like a map of vector of vectors if we're supporting multiple
            //       types of sysman devices such as <Temperature<device<Board GPU, Package GPU, FPGA>>>
            std::vector<std::vector<zes_freq_handle_t> > m_freq_domain;
            std::vector<std::vector<zes_pwr_handle_t> > m_power_domain;
            std::vector<std::vector<zes_engine_handle_t> > m_engine_domain;
            std::vector<std::vector<zes_perf_handle_t> > m_perf_domain;
            std::vector<std::vector<zes_standby_handle_t> > m_standby_domain;
            std::vector<std::vector<zes_mem_handle_t> > m_mem_domain;
            std::vector<std::vector<zes_fabric_port_handle_t> > m_fabric_domain;
            std::vector<std::vector<zes_temp_handle_t> > m_temperature_domain;
            std::vector<std::vector<zes_fan_handle_t> > m_fan_domain;

    };
}
#endif
