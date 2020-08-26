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

#include <cmath>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Exception.hpp"
#include "Agg.hpp"
#include "Helper.hpp"
#include "geopm_sched.h"

#include "NVMLDevicePoolImp.hpp"

namespace geopm
{

    const NVMLDevicePool &nvml_device_pool(const int num_cpu)
    {
        static NVMLDevicePoolImp instance(num_cpu);
        return instance;
    }

    NVMLDevicePoolImp::NVMLDevicePoolImp(const int num_cpu)
        : M_MAX_CONTEXTS(64)
        , M_NUM_CPU(num_cpu)
    {
        nvmlReturn_t nvml_result;

        //Initialize NVML
        nvml_result = nvmlInit();
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to initialize.", __LINE__);

        //Query number of NVML Accelerators
        nvml_result = nvmlDeviceGetCount(&m_num_accelerator);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to query device count.", __LINE__);

        //Acquire device handle for each NVML Accelerator
        m_nvml_device.resize(m_num_accelerator);
        for (unsigned int accel_idx = 0; accel_idx < m_num_accelerator; ++accel_idx) {
            nvml_result = nvmlDeviceGetHandleByIndex(accel_idx,
                                                     &m_nvml_device.at(accel_idx));
            check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                              ": NVML failed to get handle for accelerator " +
                              std::to_string(accel_idx) + ".", __LINE__);
        }
    }

    NVMLDevicePoolImp::~NVMLDevicePoolImp()
    {
        //Shutdown NVML
        nvmlReturn_t nvml_result = nvmlShutdown();
#ifdef GEOPM_DEBUG
        if (nvml_result != NVML_SUCCESS) {
            std::cerr << "NVMLDevicePool::" << __func__ <<  ": NVML failed to shutdown."
                      << "  Error: " <<  nvmlErrorString(nvml_result) << std::endl;
        }
#endif
    }

    void NVMLDevicePoolImp::check_nvml_result(nvmlReturn_t nvml_result, int error, std::string message, int line) const
    {
        if (nvml_result != NVML_SUCCESS) {
            throw Exception(message + "  Error: " + nvmlErrorString(nvml_result), error, __FILE__, line);
        }
    }

    int NVMLDevicePoolImp::num_accelerator() const
    {
        return m_num_accelerator;
    }

    void NVMLDevicePoolImp::check_accel_range(int accel_idx) const
    {
        if (accel_idx < 0 || accel_idx >= m_num_accelerator) {
            throw Exception("NVMLDevicePool::" + std::string(__func__) + ": accel_idx " +
                            std::to_string(accel_idx) + "  is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    cpu_set_t *NVMLDevicePoolImp::cpu_affinity_ideal_mask(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned int cpu_set_size = CPU_ALLOC_SIZE(M_NUM_CPU) / sizeof(unsigned long);
        cpu_set_t *accel_cpuset = CPU_ALLOC(M_NUM_CPU);
        CPU_ZERO_S(CPU_ALLOC_SIZE(M_NUM_CPU), accel_cpuset);

        if (!accel_cpuset) {
            throw Exception("NVMLDevicePool: unable to allocate process CPU mask",
                            ENOMEM, __FILE__, __LINE__);
        }

        nvmlReturn_t nvml_result = nvmlDeviceGetCpuAffinity(m_nvml_device.at(accel_idx), cpu_set_size,
                                                            (unsigned long *)accel_cpuset);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get CPU Affinity bitmask for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);
        return accel_cpuset;
    }

    uint64_t NVMLDevicePoolImp::frequency_status_sm(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetClock(m_nvml_device.at(accel_idx), NVML_CLOCK_SM, NVML_CLOCK_ID_CURRENT,
                                         &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get SM Frequency for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);
        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::utilization(int accel_idx) const
    {
        check_accel_range(accel_idx);
        nvmlUtilization_t result;

        nvmlReturn_t nvml_result =  nvmlDeviceGetUtilizationRates(m_nvml_device.at(accel_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get GPU Utilization for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);
        return (uint64_t)result.gpu;
    }

    uint64_t NVMLDevicePoolImp::power(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPowerUsage(m_nvml_device.at(accel_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get power for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::power_limit(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPowerManagementLimit(m_nvml_device.at(accel_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get power limit for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::frequency_status_mem(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetClock(m_nvml_device.at(accel_idx), NVML_CLOCK_MEM, NVML_CLOCK_ID_CURRENT, &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get Memory Frequency for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::throttle_reasons(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned long long result;

        nvmlReturn_t nvml_result = nvmlDeviceGetCurrentClocksThrottleReasons(m_nvml_device.at(accel_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get current clock throttle reasosn for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::temperature(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned int result;

        nvmlReturn_t nvml_result =  nvmlDeviceGetTemperature(m_nvml_device.at(accel_idx), NVML_TEMPERATURE_GPU, &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get temperature for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::energy(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned long long result;

        nvmlReturn_t nvml_result =  nvmlDeviceGetTotalEnergyConsumption(m_nvml_device.at(accel_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get energy for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::performance_state(int accel_idx) const
    {
        check_accel_range(accel_idx);
        nvmlPstates_t result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPerformanceState(m_nvml_device.at(accel_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get performance state for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::throughput_rx_pcie(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPcieThroughput(m_nvml_device.at(accel_idx), NVML_PCIE_UTIL_RX_BYTES, &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get PCIE received throughput rate for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::throughput_tx_pcie(int accel_idx) const
    {
        check_accel_range(accel_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPcieThroughput(m_nvml_device.at(accel_idx), NVML_PCIE_UTIL_TX_BYTES, &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get PCIE transmitted throughput rate for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::utilization_mem(int accel_idx) const
    {
        check_accel_range(accel_idx);
        nvmlUtilization_t result;

        nvmlReturn_t nvml_result =  nvmlDeviceGetUtilizationRates(m_nvml_device.at(accel_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get memory utilization for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);

        return (uint64_t)result.memory;
    }

    std::vector<int> NVMLDevicePoolImp::active_process_list(int accel_idx) const
    {
        check_accel_range(accel_idx);
        std::vector<int> result;
        unsigned int temp = M_MAX_CONTEXTS;

                nvmlProcessInfo_t *process_info_list;
        process_info_list = new nvmlProcessInfo_t[temp];

        nvmlReturn_t nvml_result = nvmlDeviceGetComputeRunningProcesses(m_nvml_device[accel_idx], &temp, &process_info_list[0]);
        if (nvml_result == NVML_SUCCESS) {
            for (int i = 0; i<temp; i++) {
                result.push_back(process_info_list[i].pid);
            }
        }
        else if (nvml_result == NVML_ERROR_INSUFFICIENT_SIZE) {
            // If the first attempt was unsuccessful due to process_info_list being too small the temp variable
            // will contain the correct size.  This allows us to retry the above call with better chances of
            // success.

            process_info_list = new nvmlProcessInfo_t[temp];
            nvml_result = nvmlDeviceGetComputeRunningProcesses(m_nvml_device[accel_idx], &temp, &process_info_list[0]);

            if (nvml_result == NVML_ERROR_INSUFFICIENT_SIZE) {
                throw Exception("NVMLDevicePool::" + std::string(__func__) +
                                ": NVML failed to acquire running processes for accelerator " +
                                std::to_string(accel_idx) + ".  Increase M_MAX_CONTEXTS to resolve. Error: " +
                                nvmlErrorString(nvml_result), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                              ": NVML failed to acquire running processes for accelerator " +
                              std::to_string(accel_idx) + ".", __LINE__);

            for (int i = 0; i<temp; i++) {
                result.push_back(process_info_list[i].pid);
            }
        }
        else {
            check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                              ": NVML failed to acquire running processes for accelerator " +
                              std::to_string(accel_idx) + ".", __LINE__);
        }
        return result;
    }

    void NVMLDevicePoolImp::frequency_control_sm(int accel_idx, int min_freq, int max_freq) const
    {
        check_accel_range(accel_idx);

        nvmlReturn_t nvml_result = nvmlDeviceSetGpuLockedClocks(m_nvml_device[accel_idx], (unsigned int) min_freq, (unsigned int) max_freq);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to set sm frequency for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);
    }

    void NVMLDevicePoolImp::frequency_reset_control(int accel_idx) const
    {
        check_accel_range(accel_idx);

        nvmlReturn_t nvml_result =  nvmlDeviceResetGpuLockedClocks(m_nvml_device[accel_idx]);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to reset sm frequency for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);
    }

    void NVMLDevicePoolImp::power_control(int accel_idx, int setting) const
    {
        check_accel_range(accel_idx);

        nvmlReturn_t nvml_result = nvmlDeviceSetPowerManagementLimit(m_nvml_device.at(accel_idx), (unsigned int) (setting));
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to set power limit for accelerator " +
                          std::to_string(accel_idx) + ".", __LINE__);
    }
}
