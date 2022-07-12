/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEVELZERODEVICEPOOL_HPP_INCLUDE
#define LEVELZERODEVICEPOOL_HPP_INCLUDE

#include <vector>
#include <string>
#include <cstdint>

#include "geopm_sched.h"
#include "geopm_topo.h"

namespace geopm
{
    class LevelZeroDevicePool
    {
        public:
            LevelZeroDevicePool() = default;
            virtual ~LevelZeroDevicePool() = default;
            /// @brief Number of GPUs on the platform.
            /// @return Number of LevelZero GPUs.
            /// @param [in] domain The GEOPM domain type being targeted
            virtual int num_gpu(int domain_type) const = 0;

            // FREQUENCY SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device actual frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU device core clock rate in MHz.
            virtual double frequency_status(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device mininmum frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU minimum frequency in MHz.
            virtual double frequency_min(int domain, unsigned int domain_idx,
                                         int l0_domain) const = 0;
            /// @brief Get the LevelZero device maximum frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU maximum frequency in MHz.
            virtual double frequency_max(int domain, unsigned int domain_idx,
                                         int l0_domain) const = 0;
            /// @brief Get the LevelZero device frequency throttle reasons
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. gpu being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Frequency throttle reasons
            virtual uint32_t frequency_throttle_reasons(int domain, unsigned int domain_idx,
                                                        int l0_domain) const = 0;
            virtual std::pair<double, double> frequency_range(int domain,
                                                              unsigned int domain_idx,
                                                              int l0_domain) const = 0;

            // UTILIZATION SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device active time and timestamp in microseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU active time and timestamp in microseconds.
            virtual std::pair<uint64_t, uint64_t> active_time_pair(int domain,
                                                                   unsigned int domain_idx,
                                                                   int l0_domain) const = 0;
            /// @brief Get the LevelZero device timestamp for the active time value in microseconds
            /// @brief Get the LevelZero device active time in microseconds
            /// @return GPU active time in microseconds.
            virtual uint64_t active_time(int domain, unsigned int domain_idx,
                                         int l0_domain) const = 0;
            /// @brief Get the LevelZero device timestamp for the active time value in microseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU device timestamp for the active time value in microseconds.
            virtual uint64_t active_time_timestamp(int domain, unsigned int domain_idx,
                                                   int l0_domain) const = 0;

            // POWER SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device default power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU default power limit in milliwatts
            virtual int32_t power_limit_tdp(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device minimum power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU minimum power limit in milliwatts
            virtual int32_t power_limit_min(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device maximum power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU maximum power limit in milliwatts
            virtual int32_t power_limit_max(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device sustained power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU sustained power limit in milliwatts
            virtual int32_t power_limit_sustained(int domain, unsigned int domain_idx,
                                                  int l0_domain) const = 0;

            /// @brief Get the LevelZero device sustained power limit enable
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU sustained power limit enable
            virtual bool power_limit_enabled_sustained(int domain, unsigned int domain_idx,
                                                       int l0_domain) const = 0;

            /// @brief Get the LevelZero device sustained power limit interval in milliseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU sustained power limit interval in milliseconds
            virtual int32_t power_limit_interval_sustained(int domain, unsigned int domain_idx,
                                                           int l0_domain) const = 0;

            // ENERGY SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device energy in microjoules and timestamp in microseconds.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU energy in microjoules and timestamp in microseconds.
            virtual std::pair<uint64_t, uint64_t> energy_pair(int domain, unsigned int domain_idx,
                                                              int l0_domain) const = 0;
            /// @brief Get the LevelZero device energy in microjoules.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU energy in microjoules.
            virtual uint64_t energy(int domain, unsigned int domain_idx,
                                    int l0_domain) const = 0;
            /// @brief Get the LevelZero device energy timestamp in microseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU energy timestamp in microseconds
            virtual uint64_t energy_timestamp(int domain, unsigned int domain_idx,
                                              int l0_domain) const = 0;

            // FREQUENCY CONTROL FUNCTIONS
            /// @brief Set min and max frequency for LevelZero device.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. GPU being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] range_min Min target frequency in MHz.
            /// @param [in] range_max Max target frequency in MHz.
            virtual void frequency_control(int domain, unsigned int domain_idx,
                                           int l0_domain, double range_min,
                                           double range_max) const = 0;

            virtual void power_limit_enable_sustained_control(int domain, unsigned int domain_idx,
                                                              int l0_domain, double setting) const = 0;

            virtual void power_limit_sustained_control(int domain, unsigned int domain_idx,
                                                       int l0_domain, double setting) const = 0;

            virtual void power_limit_interval_sustained_control(int domain, unsigned int domain_idx,
                                                                int l0_domain, double setting) const = 0;
    };

    const LevelZeroDevicePool &levelzero_device_pool();
}
#endif
