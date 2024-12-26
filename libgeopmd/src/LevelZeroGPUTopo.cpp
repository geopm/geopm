/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <iostream>
#include <string>
#include <map>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "LevelZeroDevicePool.hpp"
#include "LevelZeroGPUTopo.hpp"

namespace geopm
{
    static const std::string PCI_DEVICES_PATH = "/sys/bus/pci/devices";

    LevelZeroGPUTopo::LevelZeroGPUTopo()
        : LevelZeroGPUTopo(levelzero_device_pool(), PCI_DEVICES_PATH)
    {
    }

    LevelZeroGPUTopo::LevelZeroGPUTopo(const LevelZeroDevicePool &device_pool,
                                       const std::string &pci_devices_path)
        : m_levelzero_device_pool(device_pool)
        , m_pci_devices_path(pci_devices_path)
    {
        if (getenv("ZE_AFFINITY_MASK") != nullptr) {
            throw Exception("LevelZeroGPUTopo: Refusing to create a topology cache file while ZE_AFFINITY_MASK environment variable is set",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int num_gpu = m_levelzero_device_pool.num_gpu(GEOPM_DOMAIN_GPU);
        int num_gpu_chip = m_levelzero_device_pool.num_gpu(GEOPM_DOMAIN_GPU_CHIP);

        if (num_gpu == 0 || num_gpu_chip == 0) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZeroGPUTopo: No levelZero devices or chips detected.\n";
#endif
        }
        else {
            m_cpu_affinity_ideal_chip.reserve(num_gpu_chip);
            int num_chip_per_gpu = num_gpu_chip / num_gpu;

            for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
                std::string pci_address = m_levelzero_device_pool.pci_dbdf_address(GEOPM_DOMAIN_GPU, gpu_idx);
                std::string cpu_mask_path = m_pci_devices_path + "/" + pci_address + "/local_cpus";
                auto cpu_mask_buf = geopm::read_file(cpu_mask_path);
                auto cpu_set = linux_cpumask_buf_to_int_set(cpu_mask_buf);

                // This CPU set is local to the current iterated GPU and each
                // of the GPU's subdevices.
                m_cpu_affinity_ideal.push_back(cpu_set);
                for (int gpu_subdevice = 0; gpu_subdevice < num_chip_per_gpu; ++gpu_subdevice) {
                    m_cpu_affinity_ideal_chip.push_back(cpu_set);
                }
            }
        }
    }

    int LevelZeroGPUTopo::num_gpu() const
    {
        return num_gpu(GEOPM_DOMAIN_GPU);
    }

    int LevelZeroGPUTopo::num_gpu(int domain) const
    {
        int result = -1;
        if (domain == GEOPM_DOMAIN_GPU) {
            result = m_cpu_affinity_ideal.size();
        }
        else if (domain == GEOPM_DOMAIN_GPU_CHIP) {
            result = m_cpu_affinity_ideal_chip.size();
        }
        else {
            throw Exception("LevelZeroGPUTopo::" + std::string(__func__) + ": domain " +
                            std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    std::set<int> LevelZeroGPUTopo::cpu_affinity_ideal(int gpu_idx) const
    {
        return cpu_affinity_ideal(GEOPM_DOMAIN_GPU, gpu_idx);
    }

    std::set<int> LevelZeroGPUTopo::cpu_affinity_ideal(int domain, int gpu_idx) const
    {
        std::set<int> result = {};
        if (domain == GEOPM_DOMAIN_GPU) {
            if (gpu_idx < 0 || (unsigned int)gpu_idx >= m_cpu_affinity_ideal.size()) {
                throw Exception("LevelZeroGPUTopo::" + std::string(__func__) + ": gpu_idx " +
                                std::to_string(gpu_idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_ideal.at(gpu_idx);
        }
        else if (domain == GEOPM_DOMAIN_GPU_CHIP) {
            if (gpu_idx < 0 || (unsigned int)gpu_idx >= m_cpu_affinity_ideal_chip.size()) {
                throw Exception("LevelZeroGPUTopo::" + std::string(__func__) + ": gpu_idx " +
                                std::to_string(gpu_idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_ideal_chip.at(gpu_idx);
        }
        else {
            throw Exception("LevelZeroGPUTopo::" + std::string(__func__) + ": domain " +
                            std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }
}
