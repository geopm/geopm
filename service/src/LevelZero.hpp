/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEVELZERO_HPP_INCLUDE
#define LEVELZERO_HPP_INCLUDE

#include <vector>
#include <string>
#include <cstdint>

#include "geopm_topo.h"

namespace geopm
{
    class LevelZero
    {
        public:
            enum geopm_levelzero_domain_e {
                M_DOMAIN_ALL = 0,
                M_DOMAIN_COMPUTE = 1,
                M_DOMAIN_MEMORY = 2,
                M_DOMAIN_SIZE = 3
            };

            LevelZero() = default;
            virtual ~LevelZero() = default;
            /// @brief Number of GPUs on the platform.
            /// @return Number of LevelZero GPUs.
            virtual int num_gpu() const = 0;

            /// @brief Number of GPUs on the platform.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @return Number of LevelZero GPUs or GPU chips.
            virtual int num_gpu(int domain) const = 0;

            /// @brief Get the number of LevelZero frequency domains of a certain type
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU frequency domain count.
            virtual int frequency_domain_count(unsigned int l0_device_idx,
                                               int l0_domain) const = 0;
            /// @brief Get the LevelZero device actual frequency in MHz
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU.
            /// @return GPU device core clock rate in MHz.
            virtual double frequency_status(unsigned int l0_device_idx,
                                            int l0_domain, int l0_domain_idx) const = 0;
            /// @brief Get the LevelZero device efficient frequency in MHz
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU.
            /// @return GPU device efficient clock rate in MHz.
            virtual double frequency_efficient(unsigned int l0_device_idx,
                                               int l0_domain, int l0_domain_idx) const = 0;
            /// @brief Get the LevelZero device mininmum frequency in MHz
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU.
            /// @return GPU minimum frequency in MHz.
            virtual double frequency_min(unsigned int l0_device_idx, int l0_domain,
                                         int l0_domain_idx) const = 0;
            /// @brief Get the LevelZero device maximum frequency in MHz
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU.
            /// @return GPU maximum frequency in MHz.
            virtual double frequency_max(unsigned int l0_device_idx, int l0_domain,
                                         int l0_domain_idx) const = 0;
            /// @brief Get the LevelZero device frequency throttle reasons
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU..
            /// @return Frequency throttle reasons
            virtual uint32_t frequency_throttle_reasons(unsigned int l0_device_idx, int l0_domain,
                                                        int l0_domain_idx) const = 0;
            /// @brief Get the LevelZero device minimum and maximum frequency
            ///        control range in MHz
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU.
            /// @return GPU minimum and maximum frequency range in MHz.
            virtual std::pair<double, double> frequency_range(unsigned int l0_device_idx,
                                                              int l0_domain,
                                                              int l0_domain_idx) const = 0;

            /// @brief Get the number of LevelZero temperature domains
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU temperature domain count.
            virtual int temperature_domain_count(unsigned int l0_device_idx, int l0_domain) const = 0;
            /// @brief Get the LevelZero device maximum temperature in Celsius
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU.
            /// @return Domain maximum temperature in Celsius.
            virtual double temperature_max(unsigned int l0_device_idx, int l0_domain,
                                           int l0_domain_idx) const = 0;
            /// @brief Get the number of LevelZero engine domains
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU engine domain count.
            virtual int engine_domain_count(unsigned int l0_device_idx, int l0_domain) const = 0;
            /// @brief Get the LevelZero device active time and timestamp in microseconds
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU.
            /// @return GPU active time and timestamp in microseconds.
            virtual std::pair<uint64_t, uint64_t> active_time_pair(unsigned int l0_device_idx,
                                                                   int l0_domain,
                                                                   int l0_domain_idx) const = 0;
            /// @brief Get the LevelZero device active time in microseconds
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU.
            /// @return GPU active time in microseconds.
            virtual uint64_t active_time(unsigned int l0_device_idx, int l0_domain,
                                         int l0_domain_idx) const = 0;
            /// @brief Get the cachced LevelZero device timestamp for the
            ///        active time value in microseconds
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The LevelZero index indicating a particular
            ///        domain of the GPU.
            /// @return GPU device timestamp for the active time value in microseconds.
            virtual uint64_t active_time_timestamp(unsigned int l0_device_idx,
                                                   int l0_domain, int l0_domain_idx) const = 0;

            /// @brief Get the number of LevelZero power domains of a certain type
            /// @param [in] geopm_domain The GEOPM domain being targeted
            /// @param [in] l0_device_idx The LevelZero device being targeted
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU frequency domain count.
            virtual int power_domain_count(int geopm_domain,
                                           unsigned int l0_device_idx,
                                           int l0_domain) const = 0;
            /// @brief Get the number of LevelZero perf domains of a certain type
            /// @param [in] l0_device_idx The LevelZero device being targeted
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return GPU perf domain count.
            virtual int performance_domain_count(unsigned int l0_device_idx,
                                                 int l0_domain) const = 0;
            /// @brief Get the performance factor value of various LevelZero domains
            /// @param [in] l0_device_idx The LevelZero device being targeted
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The index indicating a particular
            ///        Level Zero domain.
            /// @return Subdevice performance factor value
            virtual double performance_factor(unsigned int l0_device_idx,
                                              int l0_domain, int l0_domain_idx) const = 0;

            /// @brief Get the LevelZero device default power limit in milliwatts
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @return GPU default power limit in milliwatts
            virtual int32_t power_limit_tdp(unsigned int l0_device_idx) const = 0;
            /// @brief Get the LevelZero device minimum power limit in milliwatts
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @return GPU minimum power limit in milliwatts
            virtual int32_t power_limit_min(unsigned int l0_device_idx) const = 0;
            /// @brief Get the LevelZero device maximum power limit in milliwatts
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @return GPU maximum power limit in milliwatts
            virtual int32_t power_limit_max(unsigned int l0_device_idx) const = 0;

            /// @brief Get the LevelZero device energy and timestamp
            ///        in microjoules and microseconds
            /// @param [in] geopm_domain The GEOPM domain being targeted
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain_idx The index indicating a particular
            ///        Level Zero domain.
            /// @return GPU energy in microjoules and timestamp in microseconds
            virtual std::pair<uint64_t, uint64_t> energy_pair(int geopm_domain, unsigned int l0_device_idx,
                                                              int l0_domain_idx) const = 0;
            /// @brief Get the LevelZero device energy in microjoules.
            /// @param [in] geopm_domain The GEOPM domain being targeted
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The index indicating a particular
            ///        Level Zero domain.
            /// @return GPU energy in microjoules.
            virtual uint64_t energy(int geopm_domain, unsigned int l0_device_idx, int l0_domain,
                                    int l0_domain_idx) const = 0;
            /// @brief Get the LevelZero device energy cached timestamp in microseconds
            /// @param [in] geopm_domain The GEOPM domain being targeted
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] l0_domain_idx The index indicating a particular
            ///        Level Zero domain.
            /// @return Accelerator energy timestamp in microseconds
            virtual uint64_t energy_timestamp(int geopm_domain, unsigned int l0_device_idx, int l0_domain,
                                              int l0_domain_idx) const = 0;

            /// @brief Set min and max frequency for LevelZero device.
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero GPU.
            /// @param [in] l0_domain The domain type being targeted
            /// @param [in] l0_domain_idx The domain being targeted
            /// @param [in] range_min Min target frequency in MHz.
            /// @param [in] range_max Max target frequency in MHz.
            virtual void frequency_control(unsigned int l0_device_idx, int l0_domain,
                                           int l0_domain_idx, double range_min,
                                           double range_max) const = 0;

            /// @brief Set the performance factor for the LevelZero device.
            /// @param [in] l0_device_idx The index indicating a particular
            ///        Level Zero accelerator.
            /// @param [in] l0_domain The level zero domain type being targeted
            /// @param [in] l0_domain_idx The level zero domain being targeted
            /// @param [in] setting The performance factor value, 0-100
            virtual void performance_factor_control(unsigned int l0_device_idx,
                                                    int l0_domain,
                                                    int l0_domain_idx,
                                                    double setting) const = 0;
    };

    const LevelZero &levelzero();
}
#endif
