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

#include "config.h"

#include <cmath>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <thread>
#include <chrono>
#include <time.h>

#include "Exception.hpp"
#include "Agg.hpp"
#include "Helper.hpp"
#include "geopm_sched.h"

#include "LevelZeroDevicePoolImp.hpp"

namespace geopm
{
    const LevelZeroDevicePool &levelzero_device_pool()
    {
        static LevelZeroDevicePoolImp instance(levelzero_shim());
        return instance;
    }

    LevelZeroDevicePoolImp::LevelZeroDevicePoolImp(const LevelZeroShim &shim)
        : m_shim(shim)
    {
    }

    LevelZeroDevicePoolImp::LevelZeroDevicePoolImp()
        : LevelZeroDevicePoolImp(levelzero_shim())
    {
    }

    int LevelZeroDevicePoolImp::num_accelerator() const
    {
        return m_shim.num_accelerator();
    }

    int LevelZeroDevicePoolImp::num_accelerator_subdevice() const
    {
        return m_shim.num_accelerator_subdevice();
    }

    void LevelZeroDevicePoolImp::check_device_range(unsigned int dev_idx) const
    {
        if (dev_idx >= (unsigned int) num_accelerator()) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) + ": device idx " +
                            std::to_string(dev_idx) + " is out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void LevelZeroDevicePoolImp::check_subdevice_range(unsigned int sub_idx) const
    {
        if (sub_idx >= (unsigned int) num_accelerator_subdevice()) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) + ": subdevice idx " +
                            std::to_string(sub_idx) + " is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void LevelZeroDevicePoolImp::check_domain_exists(int size, const char *func, int line) const
    {
        if (size == 0) {
            throw Exception("LevelZeroDevicePool::" + std::string(func) + ": Not supported on this hardware",
                             GEOPM_ERROR_INVALID, __FILE__, line);
        }
    }

    std::pair<unsigned int, unsigned int> LevelZeroDevicePoolImp::subdevice_device_conversion(unsigned int sub_idx) const
    {
        check_subdevice_range(sub_idx);
        unsigned int device_idx = 0;
        int subdevice_idx = 0;

        // TODO: this assumes a simple split of subdevice to device.
        // This may need to be adjusted based upon user preference or use case
        if ((num_accelerator_subdevice() % num_accelerator()) != 0) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) + ": GEOPM Requires the number" +
                            " of subdevices to be evenly divisible by the number of devices. ",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int num_subdevice_per_device = num_accelerator_subdevice()/num_accelerator();

        device_idx = sub_idx / num_subdevice_per_device;
        check_device_range(device_idx);
        subdevice_idx = sub_idx % num_subdevice_per_device;
        return {device_idx, subdevice_idx};
    }

    double LevelZeroDevicePoolImp::frequency_status(unsigned int subdevice_idx, int l0_domain) const
    {
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(subdevice_idx);
        check_domain_exists(m_shim.frequency_domain_count(dev_subdev_idx_pair.first, l0_domain), __func__, __LINE__);

        return m_shim.frequency_status(dev_subdev_idx_pair.first, l0_domain, dev_subdev_idx_pair.second);
    }

    double LevelZeroDevicePoolImp::frequency_min(unsigned int subdevice_idx, int l0_domain) const
    {
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(subdevice_idx);
        check_domain_exists(m_shim.frequency_domain_count(dev_subdev_idx_pair.first, l0_domain), __func__, __LINE__);

        return m_shim.frequency_min(dev_subdev_idx_pair.first, l0_domain, dev_subdev_idx_pair.second);
    }

    double LevelZeroDevicePoolImp::frequency_max(unsigned int subdevice_idx, int l0_domain) const
    {
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(subdevice_idx);
        check_domain_exists(m_shim.frequency_domain_count(dev_subdev_idx_pair.first, l0_domain), __func__, __LINE__);

        return m_shim.frequency_max(dev_subdev_idx_pair.first, l0_domain, dev_subdev_idx_pair.second);
    }

    uint64_t LevelZeroDevicePoolImp::active_time_timestamp(unsigned int subdevice_idx, int l0_domain) const
    {
        // TODO: Some devices may not support ZES_ENGINE_GROUP_COMPUTE/COPY_ALL. In that case this should be a
        //       device level signal that handles aggregation of domains directly here
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(subdevice_idx);
        check_domain_exists(m_shim.engine_domain_count(dev_subdev_idx_pair.first, l0_domain), __func__, __LINE__);

        return m_shim.active_time_timestamp(dev_subdev_idx_pair.first, l0_domain, dev_subdev_idx_pair.second);
    }

    uint64_t LevelZeroDevicePoolImp::active_time(unsigned int subdevice_idx, int l0_domain) const
    {
        //TODO: Some devices may not support ZES_ENGINE_GROUP_COMPUTE/COPY_ALL. In that case this should be a
        //      device level signal that handles aggregation of domains directly here
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(subdevice_idx);
        check_domain_exists(m_shim.engine_domain_count(dev_subdev_idx_pair.first, l0_domain), __func__, __LINE__);

        return m_shim.active_time(dev_subdev_idx_pair.first, l0_domain, dev_subdev_idx_pair.second);
    }

    int32_t LevelZeroDevicePoolImp::power_limit_min(unsigned int dev_idx) const
    {
        check_device_range(dev_idx);
        return m_shim.power_limit_min(dev_idx);
    }

    int32_t LevelZeroDevicePoolImp::power_limit_max(unsigned int dev_idx) const
    {
        check_device_range(dev_idx);
        return m_shim.power_limit_max(dev_idx);
    }

    int32_t LevelZeroDevicePoolImp::power_limit_tdp(unsigned int dev_idx) const
    {
        check_device_range(dev_idx);
        return m_shim.power_limit_tdp(dev_idx);
    }

    uint64_t LevelZeroDevicePoolImp::energy_timestamp(unsigned int dev_idx) const
    {
        check_device_range(dev_idx);
        return m_shim.energy_timestamp(dev_idx);
    }

    uint64_t LevelZeroDevicePoolImp::energy(unsigned int dev_idx) const
    {
        check_device_range(dev_idx);
        return m_shim.energy(dev_idx);
    }

    void LevelZeroDevicePoolImp::frequency_control(unsigned int subdevice_idx, int l0_domain, double setting) const
    {
        std::pair<unsigned int, unsigned int> dev_subdev_idx_pair;
        dev_subdev_idx_pair = subdevice_device_conversion(subdevice_idx);
        check_domain_exists(m_shim.frequency_domain_count(dev_subdev_idx_pair.first, l0_domain), __func__, __LINE__);

        m_shim.frequency_control(dev_subdev_idx_pair.first, l0_domain, dev_subdev_idx_pair.second, setting);
    }
}
