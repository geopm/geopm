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
            /// @brief Number of accelerators on the platform.
            /// @return Number of LevelZero accelerators.
            /// @param [in] domain The GEOPM domain type being targeted
            virtual int num_accelerator(int domain_type) const = 0;

            // FREQUENCY SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device actual frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator device core clock rate in MHz.
            virtual double frequency_status(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device mininmum frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator minimum frequency in MHz.
            virtual double frequency_min(int domain, unsigned int domain_idx,
                                         int l0_domain) const = 0;
            /// @brief Get the LevelZero device maximum frequency in MHz
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator maximum frequency in MHz.
            virtual double frequency_max(int domain, unsigned int domain_idx,
                                         int l0_domain) const = 0;
            virtual std::pair<double, double> frequency_range(int domain,
                                                              unsigned int domain_idx,
                                                              int l0_domain) const = 0;

            // UTILIZATION SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device active time and timestamp in microseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator active time and timestamp in microseconds.
            virtual std::pair<uint64_t, uint64_t> active_time_pair(int domain,
                                                                   unsigned int domain_idx,
                                                                   int l0_domain) const = 0;
            /// @brief Get the LevelZero device timestamp for the active time value in microseconds
            /// @brief Get the LevelZero device active time in microseconds
            /// @return Accelerator active time in microseconds.
            virtual uint64_t active_time(int domain, unsigned int domain_idx,
                                         int l0_domain) const = 0;
            /// @brief Get the LevelZero device timestamp for the active time value in microseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator device timestamp for the active time value in microseconds.
            virtual uint64_t active_time_timestamp(int domain, unsigned int domain_idx,
                                                   int l0_domain) const = 0;

            // POWER SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device default power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator default power limit in milliwatts
            virtual int32_t power_limit_tdp(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device minimum power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator minimum power limit in milliwatts
            virtual int32_t power_limit_min(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;
            /// @brief Get the LevelZero device maximum power limit in milliwatts
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator maximum power limit in milliwatts
            virtual int32_t power_limit_max(int domain, unsigned int domain_idx,
                                            int l0_domain) const = 0;

            // ENERGY SIGNAL FUNCTIONS
            /// @brief Get the LevelZero device energy in microjoules and timestamp in microseconds.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator energy in microjoules and timestamp in microseconds.
            virtual std::pair<uint64_t, uint64_t> energy_pair(int domain, unsigned int domain_idx,
                                                              int l0_domain) const = 0;
            /// @brief Get the LevelZero device energy in microjoules.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator energy in microjoules.
            virtual uint64_t energy(int domain, unsigned int domain_idx,
                                    int l0_domain) const = 0;
            /// @brief Get the LevelZero device energy timestamp in microseconds
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @return Accelerator energy timestamp in microseconds
            virtual uint64_t energy_timestamp(int domain, unsigned int domain_idx,
                                              int l0_domain) const = 0;

            virtual double metric_sample(int domain, unsigned int domain_idx,
                                         std::string metric) const = 0;
            virtual void metric_read(int domain, unsigned int domain_idx) const = 0;
            virtual void metric_polling_disable(void) = 0;

            // FREQUENCY CONTROL FUNCTIONS
            /// @brief Set min and max frequency for LevelZero device.
            /// @param [in] domain The GEOPM domain type being targeted
            /// @param [in] domain_idx The GEOPM domain index
            ///             (i.e. accelerator being targeted)
            /// @param [in] l0_domain The LevelZero domain type being targeted
            /// @param [in] range_min Min target frequency in MHz.
            /// @param [in] range_max Max target frequency in MHz.
            virtual void frequency_control(int domain, unsigned int domain_idx,
                                           int l0_domain, double range_min,
                                           double range_max) const = 0;

        private:
    };

    LevelZeroDevicePool &levelzero_device_pool();
}
#endif
