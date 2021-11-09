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

#include "config.h"

#include <string>
#include <cstdint>

#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "geopm_sched.h"

#include "LevelZeroDevicePoolImp.hpp"

namespace geopm
{
    const LevelZeroDevicePool &levelzero_device_pool()
    {
        static LevelZeroDevicePoolImp instance(levelzero());
        return instance;
    }

    LevelZeroDevicePoolImp::LevelZeroDevicePoolImp(const LevelZero &levelzero)
        : m_levelzero(levelzero)
    {
    }

    LevelZeroDevicePoolImp::LevelZeroDevicePoolImp()
        : LevelZeroDevicePoolImp(levelzero())
    {
    }

    int LevelZeroDevicePoolImp::num_accelerator(int domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR
            && domain != GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_levelzero.num_accelerator(domain);
    }

    void LevelZeroDevicePoolImp::check_idx_range(int domain, unsigned int domain_idx) const
    {
        if (domain_idx >= (unsigned int) num_accelerator(domain)) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) + ": domain "
                            + std::to_string(domain) + " idx " + std::to_string(domain_idx) +
                            " is out of range.", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void LevelZeroDevicePoolImp::check_domain_exists(int size, const char *func, int line) const
    {
        if (size == 0) {
            throw Exception("LevelZeroDevicePool::" + std::string(func) +
                            ": Not supported on this hardware for the specified "
                            "LevelZero domain",
                            GEOPM_ERROR_INVALID, __FILE__, line);
        }
    }

    std::pair<unsigned int, unsigned int> LevelZeroDevicePoolImp::subdevice_device_conversion(unsigned int sub_idx) const
    {
        check_idx_range(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx);
        unsigned int device_idx = 0;
        int subdevice_idx = 0;

        // TODO: this assumes a simple split of subdevice to device.
        // This may need to be adjusted based upon user preference or use case
        if (num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) %
            num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR) != 0) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": GEOPM Requires the number" +
                            " of subdevices to be evenly divisible by the number of devices. ",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int num_subdevice_per_device = num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) /
                                       num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR);

        device_idx = sub_idx / num_subdevice_per_device;
        check_idx_range(GEOPM_DOMAIN_BOARD_ACCELERATOR, device_idx);
        subdevice_idx = sub_idx % num_subdevice_per_device;
        return {device_idx, subdevice_idx};
    }

    double LevelZeroDevicePoolImp::frequency_status(int domain, unsigned int domain_idx,
                                                    int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the frequency domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);
        check_domain_exists(m_levelzero.frequency_domain_count(
                                        dev_subdev_idx_pair.first, l0_domain),
                                        __func__, __LINE__);

        return m_levelzero.frequency_status(dev_subdev_idx_pair.first, l0_domain,
                                            dev_subdev_idx_pair.second);
    }

    double LevelZeroDevicePoolImp::frequency_min(int domain, unsigned int domain_idx,
                                                 int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the frequency domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);
        check_domain_exists(m_levelzero.frequency_domain_count(dev_subdev_idx_pair.first,
                                        l0_domain), __func__, __LINE__);

        return m_levelzero.frequency_min(dev_subdev_idx_pair.first,
                                         l0_domain, dev_subdev_idx_pair.second);
    }

    double LevelZeroDevicePoolImp::frequency_max(int domain, unsigned int domain_idx,
                                                 int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                             ": domain " + std::to_string(domain) +
                            " is not supported for the frequency domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);
        check_domain_exists(m_levelzero.frequency_domain_count(
                                        dev_subdev_idx_pair.first, l0_domain), __func__, __LINE__);

        return m_levelzero.frequency_max(dev_subdev_idx_pair.first, l0_domain,
                                         dev_subdev_idx_pair.second);
    }

    std::pair<double, double> LevelZeroDevicePoolImp::frequency_range(int domain,
                                                                      unsigned int domain_idx,
                                                                      int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the frequency domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);
        check_domain_exists(m_levelzero.frequency_domain_count(
                                        dev_subdev_idx_pair.first, l0_domain), __func__, __LINE__);

        return m_levelzero.frequency_range(dev_subdev_idx_pair.first, l0_domain,
                                           dev_subdev_idx_pair.second);

    }

    std::pair<uint64_t, uint64_t> LevelZeroDevicePoolImp::active_time_pair(int domain,
                                                                           unsigned int domain_idx,
                                                                           int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the engine domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // TODO: Some devices may not support ZES_ENGINE_GROUP_COMPUTE/COPY_ALL. In that case this should be a
        //       device level signal that handles aggregation of domains directly here
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);
        check_domain_exists(m_levelzero.engine_domain_count(
                                        dev_subdev_idx_pair.first, l0_domain),
                                        __func__, __LINE__);

        return m_levelzero.active_time_pair(dev_subdev_idx_pair.first, l0_domain,
                                            dev_subdev_idx_pair.second);
    }

    uint64_t LevelZeroDevicePoolImp::active_time_timestamp(int domain,
                                                           unsigned int domain_idx,
                                                           int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                             ": domain " + std::to_string(domain) +
                            " is not supported for the engine domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // TODO: Some devices may not support ZES_ENGINE_GROUP_COMPUTE/COPY_ALL. In that case this should be a
        //       device level signal that handles aggregation of domains directly here
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);
        check_domain_exists(m_levelzero.engine_domain_count(
                                        dev_subdev_idx_pair.first, l0_domain),
                                        __func__, __LINE__);

        return m_levelzero.active_time_timestamp(dev_subdev_idx_pair.first, l0_domain,
                                                 dev_subdev_idx_pair.second);
    }

    uint64_t LevelZeroDevicePoolImp::active_time(int domain, unsigned int domain_idx,
                                                 int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " +std::to_string(domain) +
                            " is not supported for the engine domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        //TODO: Some devices may not support ZES_ENGINE_GROUP_COMPUTE/COPY_ALL. In that case this should be a
        //      device level signal that handles aggregation of domains directly here
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);
        check_domain_exists(m_levelzero.engine_domain_count(
                                        dev_subdev_idx_pair.first, l0_domain),
                                        __func__, __LINE__);
        return m_levelzero.active_time(dev_subdev_idx_pair.first, l0_domain,
                                       dev_subdev_idx_pair.second);
    }

    int32_t LevelZeroDevicePoolImp::power_limit_min(int domain, unsigned int domain_idx,
                                                    int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the power domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        check_idx_range(domain, domain_idx);
        return m_levelzero.power_limit_min(domain_idx);
    }

    int32_t LevelZeroDevicePoolImp::power_limit_max(int domain,
                                                    unsigned int domain_idx,
                                                    int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the power domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        check_idx_range(domain, domain_idx);
        return m_levelzero.power_limit_max(domain_idx);
    }

    int32_t LevelZeroDevicePoolImp::power_limit_tdp(int domain,
                                                    unsigned int domain_idx,
                                                    int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the power domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        check_idx_range(domain, domain_idx);
        return m_levelzero.power_limit_tdp(domain_idx);
    }

    std::pair<uint64_t, uint64_t> LevelZeroDevicePoolImp::energy_pair(int domain,
                                                                      unsigned int domain_idx,
                                                                      int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the power domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        check_idx_range(domain, domain_idx);
        return m_levelzero.energy_pair(domain_idx);
    }

    uint64_t LevelZeroDevicePoolImp::energy_timestamp(int domain,
                                                      unsigned int domain_idx,
                                                      int l0_domain) const
    {
        uint64_t energy_timestamp = 0;
        if (domain == GEOPM_DOMAIN_BOARD_ACCELERATOR) {
            check_idx_range(domain, domain_idx);
            energy_timestamp = m_levelzero.energy_timestamp(domain_idx);
        }
        else if (domain == GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP){
            std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
            dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);

            //TODO: check l0_domain
            check_domain_exists(m_levelzero.power_domain_count(domain,
                                            dev_subdev_idx_pair.first, l0_domain),
                                            __func__, __LINE__);

            energy_timestamp = m_levelzero.energy_timestamp(dev_subdev_idx_pair.first, l0_domain,
                                                            dev_subdev_idx_pair.second);
        }
        else {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the power domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return energy_timestamp;
    }

    uint64_t LevelZeroDevicePoolImp::energy(int domain, unsigned int domain_idx,
                                            int l0_domain) const
    {
        uint64_t energy = 0;
        if (domain == GEOPM_DOMAIN_BOARD_ACCELERATOR) {
            check_idx_range(domain, domain_idx);
            energy = m_levelzero.energy(domain_idx);
        }
        else if (domain == GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP){
            std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
            dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);

            //TODO: check l0_domain
            check_domain_exists(m_levelzero.power_domain_count(domain,
                                            dev_subdev_idx_pair.first, l0_domain),
                                            __func__, __LINE__);

            energy = m_levelzero.energy(dev_subdev_idx_pair.first, l0_domain,
                                        dev_subdev_idx_pair.second);
        }
        else {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the power domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return energy;
    }

    void LevelZeroDevicePoolImp::frequency_control(int domain, unsigned int domain_idx,
                                                   int l0_domain, double range_min,
                                                   double range_max) const
    {
        if (domain != GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the frequency domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);
        check_domain_exists(m_levelzero.frequency_domain_count(
                                        dev_subdev_idx_pair.first, l0_domain),
                                        __func__, __LINE__);

        m_levelzero.frequency_control(dev_subdev_idx_pair.first, l0_domain,
                                      dev_subdev_idx_pair.second, range_min,
                                      range_max);
    }
}
