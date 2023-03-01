/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
            int num_gpu(void) const override;
            int num_gpu(int domain) const override;

            int frequency_domain_count(unsigned int l0_device_idx,
                                       int domain) const override;
            double frequency_status(unsigned int l0_device_idx,
                                    int l0_domain, int l0_domain_idx) const override;
            double frequency_efficient(unsigned int l0_device_idx,
                                       int l0_domain, int l0_domain_idx) const override;
            double frequency_min(unsigned int l0_device_idx, int l0_domain,
                                 int l0_domain_idx) const override;
            double frequency_max(unsigned int l0_device_idx, int l0_domain,
                                 int l0_domain_idx) const override;
            std::vector<double> frequency_supported(unsigned int l0_device_idx,
                                                    int l0_domain,
                                                    int l0_domain_idx) const override;
            uint32_t frequency_throttle_reasons(unsigned int l0_device_idx, int l0_domain,
                                                int l0_domain_idx) const override;
            std::pair<double, double> frequency_range(unsigned int l0_device_idx,
                                                      int l0_domain,
                                                      int l0_domain_idx) const override;

            int temperature_domain_count(unsigned int l0_device_idx,
                                         int l0_domain) const override;
            double temperature_max(unsigned int l0_device_idx, int l0_domain,
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
            int performance_domain_count(unsigned int l0_device_idx,
                                         int l0_domain) const override;
            double performance_factor(unsigned int l0_device_idx,
                                      int l0_domain, int l0_domain_idx) const override;

            int32_t power_limit_tdp(unsigned int l0_device_idx) const override;
            int32_t power_limit_min(unsigned int l0_device_idx) const override;
            int32_t power_limit_max(unsigned int l0_device_idx) const override;

            void frequency_control(unsigned int l0_device_idx, int l0_domain,
                                   int l0_domain_idx, double range_min,
                                   double range_max) const override;

            void performance_factor_control(unsigned int l0_device_idx,
                                            int l0_domain,
                                            int l0_domain_idx,
                                            double setting) const override;

            std::vector<double> metric_sample(unsigned int l0_device_idx,
                                              std::string metric_name) const override;
            uint32_t metric_update_rate(unsigned int l0_device_idx) const override;

            void metric_read(unsigned int l0_device_idx) override;
            void metric_init(unsigned int l0_device_idx) override;
            void metric_destroy(unsigned int l0_device_idx) override;
            void metric_update_rate_control(unsigned int l0_device_idx, uint32_t setting) override;

        private:
            struct m_frequency_s {
                double voltage = 0;
                double request  = 0;
                double tdp = 0;
                double efficient = 0;
                double actual = 0;
                uint32_t throttle_reasons = 0;
            };
            struct m_power_limit_s {
                int32_t tdp = 0;
                int32_t min = 0;
                int32_t max = 0;
            };

            struct m_subdevice_s {
                // These are enum geopm_levelzero_domain_e indexed, then subdevice indexed
                std::vector<std::vector<zes_freq_handle_t> > freq_domain;
                std::vector<std::vector<zes_temp_handle_t> > temp_domain_max;
                std::vector<std::vector<zes_engine_handle_t> > engine_domain;
                mutable std::vector<std::vector<uint64_t> > cached_timestamp;

                //uint32_t num_subdevice_perf_domain;
                std::vector<std::vector<zes_perf_handle_t>> perf_domain;

                uint32_t num_subdevice_power_domain;
                std::vector<zes_pwr_handle_t> power_domain;
                mutable std::vector<uint64_t> cached_energy_timestamp;

            };

            struct m_device_info_s {
                zes_device_handle_t device_handle;
                ze_device_properties_t property;
                uint32_t m_num_subdevice;
                std::vector<zes_device_handle_t> subdevice_handle;

                // Sub-Device domain tracking.  Because levelzero returns ALL handles for a
                // 'class' (freq, power, etc) regardless of subdevice it is easier to track
                // this as class.domain.subdevice where domain is compute/memory.  This avoids
                // an additional step of sorting handles to determine how many per subdevice
                m_subdevice_s subdevice;

                // Device/Package domains
                zes_pwr_handle_t power_domain;
                //ZE Context used for ZET data collection
                ze_context_handle_t context;

                uint32_t num_device_power_domain;
                mutable uint64_t cached_energy_timestamp;

                // required for L0 metric result tracking
                mutable std::map<std::string, std::vector<double>> m_metric_data;
                mutable bool metrics_initialized;

                // required for L0 metric querying
                uint32_t num_metric;
                uint32_t num_reports;
                uint32_t metric_sampling_period;
                bool metric_domain_cached;
                ze_event_pool_handle_t event_pool;
                ze_event_handle_t event; //TODO: rename metric_notification_event?
                zet_metric_streamer_handle_t metric_streamer;
                zet_metric_group_handle_t metric_group_handle; //compute basic only
            };


            void frequency_domain_cache(unsigned int l0_device_idx);
            void power_domain_cache(unsigned int l0_device_idx);
            void perf_domain_cache(unsigned int l0_device_idx);
            void engine_domain_cache(unsigned int l0_device_idx);
            void temperature_domain_cache(unsigned int l0_device_idx);
            void check_ze_result(ze_result_t ze_result, int error, std::string message,
                                 int line) const;

            std::pair<double, double> frequency_min_max(unsigned int l0_device_idx,
                                                        int l0_domain, int l0_domain_idx) const;

            m_power_limit_s power_limit_default(unsigned int l0_device_idx) const;
            m_frequency_s frequency_status_helper(unsigned int l0_device_idx,
                                                  int l0_domain, int l0_domain_idx) const;

            uint32_t m_num_gpu;
            uint32_t m_num_gpu_subdevice;

            std::vector<ze_driver_handle_t> m_levelzero_driver;
            std::vector<m_device_info_s> m_devices;

            void metric_group_cache(unsigned int l0_device_idx);
            void metric_calc(unsigned int l0_device_idx,
                             zet_metric_streamer_handle_t metric_streamer) const;
    };
}
#endif
