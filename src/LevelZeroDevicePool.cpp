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

    const LevelZeroDevicePool &levelzero_device_pool(const int num_cpu)
    {
        static LevelZeroDevicePoolImp instance(num_cpu);
        return instance;
    }

    LevelZeroDevicePoolImp::LevelZeroDevicePoolImp(const int num_cpu)
        : M_NUM_CPU(num_cpu)
        , m_shim(levelzero_shim(num_cpu))
    {
    }

    LevelZeroDevicePoolImp::~LevelZeroDevicePoolImp()
    {
    }

    int LevelZeroDevicePoolImp::num_accelerator() const
    {
        return m_shim.num_accelerator();
    }

    void LevelZeroDevicePoolImp::check_accel_range(unsigned int accel_idx) const
    {
        if (accel_idx >= (unsigned int) num_accelerator()) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) + ": accel_idx " +
                            std::to_string(accel_idx) + "  is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void LevelZeroDevicePoolImp::check_domain_range(int size, const char *func, int line) const
    {
        if (size == 0) {
            throw Exception("LevelZeroDevicePool::" + std::string(func) + ": Not supported on this hardware",
                             GEOPM_ERROR_INVALID, __FILE__, line);
        }
    }

    double LevelZeroDevicePoolImp::frequency_status(unsigned int accel_idx, geopm_levelzero_domain_e domain) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;
        int domain_size = m_shim.frequency_domain_count(accel_idx, domain);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.frequency_status(accel_idx, domain, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    double LevelZeroDevicePoolImp::frequency_min(unsigned int accel_idx, geopm_levelzero_domain_e domain) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;
        int domain_size = m_shim.frequency_domain_count(accel_idx, domain);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.frequency_min(accel_idx, domain, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    double LevelZeroDevicePoolImp::frequency_max(unsigned int accel_idx, geopm_levelzero_domain_e domain) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;
        int domain_size = m_shim.frequency_domain_count(accel_idx, domain);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.frequency_max(accel_idx, domain, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    uint64_t LevelZeroDevicePoolImp::active_time_timestamp(unsigned int accel_idx, geopm_levelzero_domain_e domain) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;
        int domain_size = m_shim.engine_domain_count(accel_idx, domain);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.active_time_timestamp(accel_idx, domain, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    uint64_t LevelZeroDevicePoolImp::active_time(unsigned int accel_idx, geopm_levelzero_domain_e domain) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;
        int domain_size = m_shim.engine_domain_count(accel_idx, domain);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.active_time(accel_idx, domain, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    int32_t LevelZeroDevicePoolImp::power_limit_min(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;

        int domain_size = m_shim.energy_domain_count_device(accel_idx);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.power_limit_min(accel_idx, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    int32_t LevelZeroDevicePoolImp::power_limit_max(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;

        int domain_size = m_shim.energy_domain_count_device(accel_idx);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.power_limit_max(accel_idx, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    int32_t LevelZeroDevicePoolImp::power_limit_tdp(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;

        int domain_size = m_shim.energy_domain_count_device(accel_idx);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.power_limit_tdp(accel_idx, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    uint64_t LevelZeroDevicePoolImp::energy_timestamp(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;

        int domain_size = m_shim.energy_domain_count_device(accel_idx);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.energy_timestamp(accel_idx, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    uint64_t LevelZeroDevicePoolImp::energy(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        double result = 0;
        double result_cnt = 0;

        int domain_size = m_shim.energy_domain_count_device(accel_idx);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            result += m_shim.energy(accel_idx, domain_idx);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    void LevelZeroDevicePoolImp::frequency_control(unsigned int accel_idx, geopm_levelzero_domain_e domain, double setting) const
    {
        check_accel_range(accel_idx);
        int domain_size = m_shim.frequency_domain_count(accel_idx, domain);
        check_domain_range(domain_size, __func__, __LINE__);
        for (int domain_idx = 0; domain_idx < domain_size; domain_idx++){
            m_shim.frequency_control(accel_idx, domain, domain_idx, setting);
        }
    }

}
