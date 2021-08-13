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
            virtual int num_accelerator(void) const = 0;
            /// @brief Number of accelerator subdevices on the platform.
            /// @return Number of LevelZero accelerator subdevices.
            virtual int num_accelerator_subdevice(void) const = 0;

            /// @brief Get the LevelZero device actual frequency in MHz
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @param [in] domain The LevelZero domain type being targeted
            /// @return Accelerator device core clock rate in MHz.
            virtual double frequency_status(unsigned int subdevice_idx, int l0_domain) const = 0;
            /// @brief Get the LevelZero device mininmum frequency in MHz
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @param [in] domain The LevelZero domain type being targeted
            /// @return Accelerator minimum frequency in MHz.
            virtual double frequency_min(unsigned int subdevice_idx, int l0_domain) const = 0;
            /// @brief Get the LevelZero device maximum frequency in MHz
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @param [in] domain The LevelZero domain type being targeted
            /// @return Accelerator maximum frequency in MHz.
            virtual double frequency_max(unsigned int subdevice_idx, int l0_domain) const = 0;

            /// @brief Get the LevelZero device active time in microseconds
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @param [in] domain The LevelZero domain type being targeted
            /// @return Accelerator active time in microseconds.
            virtual uint64_t active_time(unsigned int subdevice_idx, int l0_domain) const = 0;
            /// @brief Get the LevelZero device timestamp for the active time value in microseconds
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @param [in] domain The LevelZero domain type being targeted
            /// @return Accelerator device timestamp for the active time value in microseconds.
            virtual uint64_t active_time_timestamp(unsigned int subdevice_idx, int l0_domain) const = 0;

            /// @brief Get the LevelZero device default power limit in milliwatts
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @return Accelerator default power limit in milliwatts
            virtual int32_t power_limit_tdp(unsigned int idx) const = 0;
            /// @brief Get the LevelZero device minimum power limit in milliwatts
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @param [in] domain_idx The index indicating a particular
            ///        domain of the accelerator.
            /// @return Accelerator minimum power limit in milliwatts
            virtual int32_t power_limit_min(unsigned int idx) const = 0;
            /// @brief Get the LevelZero device maximum power limit in milliwatts
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @param [in] domain_idx The index indicating a particular
            ///        domain of the accelerator.
            /// @return Accelerator maximum power limit in milliwatts
            virtual int32_t power_limit_max(unsigned int idx) const = 0;

            /// @brief Get the LevelZero device energy in microjoules.
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @return Accelerator energy in microjoules.
            virtual uint64_t energy(unsigned int idx) const = 0;
            /// @brief Get the LevelZero device energy timestamp in microseconds
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @return Accelerator energy timestamp in microseconds
            virtual uint64_t energy_timestamp(unsigned int idx) const = 0;

            /// @brief Set min and max frequency for LevelZero device.
            /// @param [in] idx The index indicating a particular
            ///        accelerator.
            /// @param [in] domain The LevelZero domain type being targeted
            /// @param [in] setting Target frequency in MHz.
            virtual void frequency_control(unsigned int idx, int l0_domain, double setting) const = 0;

        private:
    };

    const LevelZeroDevicePool &levelzero_device_pool();
}
#endif
