/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <cmath>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <nvml.h>
#include <unistd.h>

#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
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
        , M_MAX_FREQUENCIES(200)
        , M_NUM_CPU(num_cpu)
    {
        nvmlReturn_t nvml_result;

        //Initialize NVML
        nvml_result = nvmlInit();
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to initialize.", __LINE__);

        //Query number of NVML GPUs
        nvml_result = nvmlDeviceGetCount(&m_num_gpu);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to query device count.", __LINE__);

        //Acquire device handle for each NVML GPU
        m_nvml_device.resize(m_num_gpu);
        for (unsigned int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
            nvml_result = nvmlDeviceGetHandleByIndex(gpu_idx,
                                                     &m_nvml_device.at(gpu_idx));
            check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                              ": NVML failed to get handle for GPU " +
                              std::to_string(gpu_idx) + ".", __LINE__);
        }
    }

    NVMLDevicePoolImp::~NVMLDevicePoolImp()
    {
        //Shutdown NVML
        nvmlReturn_t nvml_result = nvmlShutdown();
        if (nvml_result != NVML_SUCCESS) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> NVMLDevicePool::" << __func__
                      <<  ": NVML failed to shutdown. "
                      <<  nvmlErrorString(nvml_result) << std::endl;
#endif
        }
    }

    void NVMLDevicePoolImp::check_nvml_result(nvmlReturn_t nvml_result, int error, const std::string &message, int line) const
    {
        if (nvml_result != NVML_SUCCESS) {
            throw Exception(message + " : " + nvmlErrorString(nvml_result), error, __FILE__, line);
        }
    }

    int NVMLDevicePoolImp::num_gpu() const
    {
        return m_num_gpu;
    }

    void NVMLDevicePoolImp::check_gpu_range(int gpu_idx) const
    {
        if (gpu_idx < 0 || gpu_idx >= (int)m_num_gpu) {
            throw Exception("NVMLDevicePool::" + std::string(__func__) + ": gpu_idx " +
                            std::to_string(gpu_idx) + "  is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    cpu_set_t *NVMLDevicePoolImp::cpu_affinity_ideal_mask(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned int cpu_set_size = CPU_ALLOC_SIZE(M_NUM_CPU) / sizeof(unsigned long);
        cpu_set_t *gpu_cpuset = CPU_ALLOC(M_NUM_CPU);
        CPU_ZERO_S(CPU_ALLOC_SIZE(M_NUM_CPU), gpu_cpuset);

        if (!gpu_cpuset) {
            throw Exception("NVMLDevicePool: unable to allocate process CPU mask",
                            ENOMEM, __FILE__, __LINE__);
        }

        nvmlReturn_t nvml_result = nvmlDeviceGetCpuAffinity(m_nvml_device.at(gpu_idx), cpu_set_size,
                                                            (unsigned long *)gpu_cpuset);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get CPU Affinity bitmask for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);
        return gpu_cpuset;
    }

    uint64_t NVMLDevicePoolImp::frequency_status_sm(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetClock(m_nvml_device.at(gpu_idx), NVML_CLOCK_SM, NVML_CLOCK_ID_CURRENT,
                                         &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get SM Frequency for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);
        return (uint64_t)result;
    }

    std::vector<unsigned int> NVMLDevicePoolImp::frequency_supported_sm(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned int count = M_MAX_FREQUENCIES;
        std::vector<unsigned int> supported_freqs(count);

        nvmlReturn_t nvml_result = nvmlDeviceGetSupportedGraphicsClocks(m_nvml_device.at(gpu_idx),
                                                                        frequency_status_mem(gpu_idx),
                                                                        &count, supported_freqs.data());
        supported_freqs.resize(count);

        if (nvml_result == NVML_ERROR_INSUFFICIENT_SIZE) {
            nvml_result = nvmlDeviceGetSupportedGraphicsClocks(m_nvml_device.at(gpu_idx),
                                                               frequency_status_mem(gpu_idx),
                                                               &count, supported_freqs.data());
        }
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get SM Frequency for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return supported_freqs;
    }


    uint64_t NVMLDevicePoolImp::utilization(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        nvmlUtilization_t result;

        nvmlReturn_t nvml_result =  nvmlDeviceGetUtilizationRates(m_nvml_device.at(gpu_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get GPU Utilization for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);
        return (uint64_t)result.gpu;
    }

    uint64_t NVMLDevicePoolImp::power(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPowerUsage(m_nvml_device.at(gpu_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get power for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::power_limit(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPowerManagementLimit(m_nvml_device.at(gpu_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get power limit for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::frequency_status_mem(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetClock(m_nvml_device.at(gpu_idx), NVML_CLOCK_MEM, NVML_CLOCK_ID_CURRENT, &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get Memory Frequency for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::throttle_reasons(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned long long result;

        nvmlReturn_t nvml_result = nvmlDeviceGetCurrentClocksThrottleReasons(m_nvml_device.at(gpu_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get current clock throttle reasosn for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::temperature(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned int result;

        nvmlReturn_t nvml_result =  nvmlDeviceGetTemperature(m_nvml_device.at(gpu_idx), NVML_TEMPERATURE_GPU, &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get temperature for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::energy(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned long long result;

        nvmlReturn_t nvml_result =  nvmlDeviceGetTotalEnergyConsumption(m_nvml_device.at(gpu_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get energy for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::performance_state(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        nvmlPstates_t result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPerformanceState(m_nvml_device.at(gpu_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get performance state for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::throughput_rx_pcie(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPcieThroughput(m_nvml_device.at(gpu_idx), NVML_PCIE_UTIL_RX_BYTES, &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get PCIE received throughput rate for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::throughput_tx_pcie(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        unsigned int result;

        nvmlReturn_t nvml_result = nvmlDeviceGetPcieThroughput(m_nvml_device.at(gpu_idx), NVML_PCIE_UTIL_TX_BYTES, &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get PCIE transmitted throughput rate for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result;
    }

    uint64_t NVMLDevicePoolImp::utilization_mem(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        nvmlUtilization_t result;

        nvmlReturn_t nvml_result =  nvmlDeviceGetUtilizationRates(m_nvml_device.at(gpu_idx), &result);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to get memory utilization for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);

        return (uint64_t)result.memory;
    }

    std::vector<int> NVMLDevicePoolImp::active_process_list(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);
        std::vector<int> result;
        unsigned int temp = M_MAX_CONTEXTS;

        nvmlProcessInfo_t *process_info_list;
        process_info_list = new nvmlProcessInfo_t[temp];

        nvmlReturn_t nvml_result = nvmlDeviceGetComputeRunningProcesses(m_nvml_device[gpu_idx], &temp, &process_info_list[0]);
        if (nvml_result == NVML_SUCCESS) {
            for (unsigned int i = 0; i<temp; i++) {
                result.push_back(process_info_list[i].pid);
            }
        }
        else if (nvml_result == NVML_ERROR_INSUFFICIENT_SIZE) {
            // If the first attempt was unsuccessful due to process_info_list being too small the temp variable
            // will contain the correct size.  This allows us to retry the above call with better chances of
            // success.

            process_info_list = new nvmlProcessInfo_t[temp];
            nvml_result = nvmlDeviceGetComputeRunningProcesses(m_nvml_device[gpu_idx], &temp, &process_info_list[0]);

            if (nvml_result == NVML_ERROR_INSUFFICIENT_SIZE) {
                throw Exception("NVMLDevicePool::" + std::string(__func__) +
                                ": NVML failed to acquire running processes for GPU " +
                                std::to_string(gpu_idx) + ".  Increase M_MAX_CONTEXTS to resolve: " +
                                nvmlErrorString(nvml_result), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                              ": NVML failed to acquire running processes for GPU " +
                              std::to_string(gpu_idx) + ".", __LINE__);

            for (unsigned int i = 0; i<temp; i++) {
                result.push_back(process_info_list[i].pid);
            }
        }
        else {
            check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                              ": NVML failed to acquire running processes for GPU " +
                              std::to_string(gpu_idx) + ".", __LINE__);
        }
        return result;
    }

    void NVMLDevicePoolImp::frequency_control_sm(int gpu_idx, int min_freq, int max_freq) const
    {
        check_gpu_range(gpu_idx);

        nvmlReturn_t nvml_result = nvmlDeviceSetGpuLockedClocks(m_nvml_device[gpu_idx], (unsigned int) min_freq, (unsigned int) max_freq);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to set sm frequency for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);
    }

    void NVMLDevicePoolImp::frequency_reset_control(int gpu_idx) const
    {
        check_gpu_range(gpu_idx);

        nvmlReturn_t nvml_result =  nvmlDeviceResetGpuLockedClocks(m_nvml_device[gpu_idx]);
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to reset sm frequency for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);
    }

    void NVMLDevicePoolImp::power_control(int gpu_idx, int setting) const
    {
        check_gpu_range(gpu_idx);

        nvmlReturn_t nvml_result = nvmlDeviceSetPowerManagementLimit(m_nvml_device.at(gpu_idx), (unsigned int) (setting));
        check_nvml_result(nvml_result, GEOPM_ERROR_RUNTIME, "NVMLDevicePool::" + std::string(__func__) +
                          ": NVML failed to set power limit for GPU " +
                          std::to_string(gpu_idx) + ".", __LINE__);
    }

    bool NVMLDevicePoolImp::is_privileged_access(void) const
    {
        return (geteuid() == 0);
    }
}
