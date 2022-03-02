/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#ifndef LEVELZEROIMP_HPP_INCLUDE
#define LEVELZEROIMP_HPP_INCLUDE

#include <string>

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include <level_zero/zet_api.h>

#include "LevelZero.hpp"

#include "geopm_time.h"

namespace geopm
{
    class LevelZeroImp : public LevelZero
    {
        public:
            LevelZeroImp();
            virtual ~LevelZeroImp() = default;
            int num_accelerator(void) const override;
            int num_accelerator(int domain) const override;

            int frequency_domain_count(unsigned int l0_device_idx,
                                       int domain) const override;
            double frequency_status(unsigned int l0_device_idx,
                                       int l0_domain, int l0_domain_idx) const override;
            double frequency_min(unsigned int l0_device_idx, int l0_domain,
                                 int l0_domain_idx) const override;
            double frequency_max(unsigned int l0_device_idx, int l0_domain,
                                 int l0_domain_idx) const override;
            std::pair<double, double> frequency_range(unsigned int l0_device_idx,
                                                      int l0_domain,
                                                      int l0_domain_idx) const override;

            int engine_domain_count(unsigned int l0_device_idx, int domain) const override;
            std::pair<uint64_t, uint64_t> active_time_pair(unsigned int l0_device_idx,
                                                           int l0_domain, int l0_domain_idx) const override;
            uint64_t active_time(unsigned int l0_device_idx, int l0_domain,
                                 int l0_domain_idx) const override;
            uint64_t active_time_timestamp(unsigned int l0_device_idx,
                                          int l0_domain, int l0_domain_idx) const override;

            int power_domain_count(int geopm_domain, unsigned int l0_device_idx,
                                   int l0_domain) const override;
            std::pair<uint64_t, uint64_t> energy_pair(int geopm_domain, unsigned int l0_device_idx,
                                                      int l0_domain_idx) const override;
            uint64_t energy(int geopm_domain, unsigned int l0_device_idx,
                            int l0_domain, int l0_domain_idx) const override;
            uint64_t energy_timestamp(int geopm_domain,
                                      unsigned int l0_device_idx,
                                      int l0_domain,
                                      int l0_domain_idx) const override;
            int32_t power_limit_tdp(unsigned int l0_device_idx) const override;
            int32_t power_limit_min(unsigned int l0_device_idx) const override;
            int32_t power_limit_max(unsigned int l0_device_idx) const override;

            std::vector<double> metric_sample(unsigned int l0_device_idx,
                                              std::string metric_name) const override;
            void metric_read(unsigned int l0_device_idx) const override;
            void metric_init(unsigned int l0_device_idx) override;
            void metric_destroy(unsigned int l0_device_idx) override;

            void frequency_control(unsigned int l0_device_idx, int l0_domain,
                                   int l0_domain_idx, double range_min,
                                   double range_max) const override;

        private:
            struct m_frequency_s {
                double voltage = 0;
                double request  = 0;
                double tdp = 0;
                double efficient = 0;
                double actual = 0;
                uint64_t throttle_reasons = 0;
            };
            struct m_power_limit_s {
                int32_t tdp = 0;
                int32_t min = 0;
                int32_t max = 0;
            };

            struct m_subdevice_s {
                // These are enum geopm_levelzero_domain_e indexed, then subdevice indexed
                std::vector<std::vector<zes_freq_handle_t> > freq_domain;
                std::vector<std::vector<zes_engine_handle_t> > engine_domain;
                mutable std::vector<std::vector<uint64_t> > cached_timestamp;

                uint32_t num_subdevice_power_domain;
                std::vector<zes_pwr_handle_t> power_domain;
                mutable std::vector<uint64_t> cached_energy_timestamp;
            };

            struct m_device_info_s {
                zes_device_handle_t device_handle;
                ze_device_properties_t property;
                uint32_t m_num_subdevice;
                std::vector<zes_device_handle_t> subdevice_handle;

                // required for L0 metric querying
                ze_context_handle_t context;

                // Sub-Device domain tracking.  Because levelzero returns ALL handles for a
                // 'class' (freq, power, etc) regardless of subdevice it is easier to track
                // this as class.domain.subdevice where domain is compute/memory.  This avoids
                // an additional step of sorting handles to determine how many per subdevice
                m_subdevice_s subdevice;

                // Device/Package domains
                uint32_t num_device_power_domain;
                zes_pwr_handle_t power_domain;
                mutable uint64_t cached_energy_timestamp;

                // required for L0 metric result tracking
                mutable std::map<std::string, std::vector<double>> m_metric_data;
                mutable bool metrics_initialized;

                // required for L0 metric querying
                uint32_t num_metric;
                ze_event_pool_handle_t event_pool;
                ze_event_handle_t event; //TODO: rename metric_notification_event?
                zet_metric_streamer_handle_t metric_streamer;
                zet_metric_group_handle_t metric_group_handle; //compute basic only
            };


            void domain_cache(unsigned int l0_device_idx);
            void metric_group_cache(unsigned int l0_device_idx);
            void metric_calc(unsigned int l0_device_idx,
                             zet_metric_streamer_handle_t metric_streamer) const;

            void check_ze_result(ze_result_t ze_result, int error, std::string message,
                                 int line) const;

            std::pair<double, double> frequency_min_max(unsigned int l0_device_idx,
                                                        int l0_domain, int l0_domain_idx) const;

            m_power_limit_s power_limit_default(unsigned int l0_device_idx) const;
            m_frequency_s frequency_status_helper(unsigned int l0_device_idx,
                                                  int l0_domain, int l0_domain_idx) const;

            uint32_t m_num_board_gpu;
            uint32_t m_num_board_gpu_subdevice;

            std::vector<ze_driver_handle_t> m_levelzero_driver;
            std::vector<m_device_info_s> m_devices;
    };
}
#endif
