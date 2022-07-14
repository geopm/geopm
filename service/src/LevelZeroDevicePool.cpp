/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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

    int LevelZeroDevicePoolImp::num_gpu(int domain) const
    {
        if (domain != GEOPM_DOMAIN_GPU
            && domain != GEOPM_DOMAIN_GPU_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_levelzero.num_gpu(domain);
    }

    void LevelZeroDevicePoolImp::check_idx_range(int domain, unsigned int domain_idx) const
    {
        if (domain_idx >= (unsigned int) num_gpu(domain)) {
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
        check_idx_range(GEOPM_DOMAIN_GPU_CHIP, sub_idx);
        unsigned int device_idx = 0;
        int subdevice_idx = 0;

        // TODO: this assumes a simple split of subdevice to device.
        // This may need to be adjusted based upon user preference or use case
        if (num_gpu(GEOPM_DOMAIN_GPU_CHIP) %
            num_gpu(GEOPM_DOMAIN_GPU) != 0) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": GEOPM Requires the number" +
                            " of subdevices to be evenly divisible by the number of devices. ",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int num_subdevice_per_device = num_gpu(GEOPM_DOMAIN_GPU_CHIP) /
                                       num_gpu(GEOPM_DOMAIN_GPU);

        device_idx = sub_idx / num_subdevice_per_device;
        check_idx_range(GEOPM_DOMAIN_GPU, device_idx);
        subdevice_idx = sub_idx % num_subdevice_per_device;
        return {device_idx, subdevice_idx};
    }

    double LevelZeroDevicePoolImp::frequency_status(int domain, unsigned int domain_idx,
                                                    int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_GPU_CHIP) {
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
        if (domain != GEOPM_DOMAIN_GPU_CHIP) {
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
        if (domain != GEOPM_DOMAIN_GPU_CHIP) {
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

    uint32_t LevelZeroDevicePoolImp::frequency_throttle_reasons(int domain, unsigned int domain_idx,
                                                              int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_GPU_CHIP) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                             ": domain " + std::to_string(domain) +
                            " is not supported for reading the \"frequency throttle reason\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(domain_idx);
        check_domain_exists(m_levelzero.frequency_domain_count(
                                        dev_subdev_idx_pair.first, l0_domain), __func__, __LINE__);

        return m_levelzero.frequency_throttle_reasons(dev_subdev_idx_pair.first, l0_domain,
                                                      dev_subdev_idx_pair.second);
    }


    std::pair<double, double> LevelZeroDevicePoolImp::frequency_range(int domain,
                                                                      unsigned int domain_idx,
                                                                      int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_GPU_CHIP) {
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
        if (domain != GEOPM_DOMAIN_GPU_CHIP) {
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
        if (domain != GEOPM_DOMAIN_GPU_CHIP) {
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
        if (domain != GEOPM_DOMAIN_GPU_CHIP) {
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
        if (domain != GEOPM_DOMAIN_GPU) {
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
        if (domain != GEOPM_DOMAIN_GPU) {
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
        if (domain != GEOPM_DOMAIN_GPU) {
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
        if (domain != GEOPM_DOMAIN_GPU) {
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
        if (domain != GEOPM_DOMAIN_GPU) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the power domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        check_idx_range(domain, domain_idx);
        return m_levelzero.energy_timestamp(domain_idx);
    }

    uint64_t LevelZeroDevicePoolImp::energy(int domain, unsigned int domain_idx,
                                            int l0_domain) const
    {
        if (domain != GEOPM_DOMAIN_GPU) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) +
                            ": domain " + std::to_string(domain) +
                            " is not supported for the power domain.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        check_idx_range(domain, domain_idx);
        return m_levelzero.energy(domain_idx);
    }

    void LevelZeroDevicePoolImp::frequency_control(int domain, unsigned int domain_idx,
                                                   int l0_domain, double range_min,
                                                   double range_max) const
    {
        if (domain != GEOPM_DOMAIN_GPU_CHIP) {
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
