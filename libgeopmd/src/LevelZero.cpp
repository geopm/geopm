/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <string>
#include <iostream>
#include <map>
#include <cstdlib>
#include <utility>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"

#include "LevelZeroImp.hpp"

static void __attribute__((constructor)) geopm_levelzero_init(void)
{
    setenv("ZES_ENABLE_SYSMAN", "1", 1);
    setenv("ZET_ENABLE_METRICS", "1", 1);
}

namespace geopm
{
    static double convert_nan(double result)
    {
        if (result < 0) {
            result = NAN;
        }
        return result;
    }

    LevelZero &levelzero()
    {
        static LevelZeroImp instance;
        return instance;
    }

    LevelZeroImp::LevelZeroImp()
        : m_num_gpu(0)
        , m_num_gpu_subdevice(0)
        , m_metric_thread_active(false)
        , m_metric_thread_started(false)
    {
        if (getenv("ZE_AFFINITY_MASK") != nullptr) {
            throw Exception("LevelZero: Cannot be used directly when ZE_AFFINITY_MASK environment "
                            "variable is set, must use service to access LevelZero in this case.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (geopm::has_cap_sys_admin()) {
            // LevelZeroIOGroup is only loaded when PID has CAP_SYS_ADMIN
            bool do_warn = false;
            if (getenv("ZES_ENABLE_SYSMAN") == nullptr ||
                std::string(getenv("ZES_ENABLE_SYSMAN")) != "1") {
                do_warn = true;
                std::cerr << "Warning: <geopm> LevelZero features require the environment variable setting \"ZES_ENABLE_SYSMAN=1\".\n";
            }
            if (getenv("ZE_FLAT_DEVICE_HIERARCHY") == nullptr ||
                std::string(getenv("ZE_FLAT_DEVICE_HIERARCHY")) != "COMPOSITE") {
                if (do_warn == false) {
                    std::cerr << "Warning: <geopm> ";
                }
                else {
                    std::cerr << "                 ";
                }
                do_warn = true;
                std::cerr << "LevelZero features require the environment variable setting \"ZE_FLAT_DEVICE_HIERARCHY=COMPOSITE\".\n";
            }
            if (do_warn) {
                std::cerr << "                 This process may corrupt the topology cache file if LevelZero enabled GPUs are present.\n"
                          << "                 Run the following commands to fix:\n"
                          << "                     sudo rm -f /run/geopm/geopm-topo-cache\n"
                          << "                     sudo env ZES_ENABLE_SYSMAN=1 ZE_FLAT_DEVICE_HIERARCHY=COMPOSITE geopmread --cache\n";
            }
        }
        //Initialize
        check_ze_result(zeInit(ZE_INIT_FLAG_GPU_ONLY),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": LevelZero Driver failed to initialize.", __LINE__);

        // Discover drivers
        uint32_t num_driver = 0;
        check_ze_result(zeDriverGet(&num_driver, nullptr),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": LevelZero Driver enumeration failed.", __LINE__);

        m_levelzero_driver.resize(num_driver);

        check_ze_result(zeDriverGet(&num_driver, m_levelzero_driver.data()),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": LevelZero Driver acquisition failed.", __LINE__);

        for (unsigned int driver = 0; driver < num_driver; driver++) {
            // Discover devices in a driver
            uint32_t num_device = 0;
            check_ze_result(zeDeviceGet(m_levelzero_driver.at(driver), &num_device, nullptr),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": LevelZero Device enumeration failed.", __LINE__);
            std::vector<zes_device_handle_t> device_handle(num_device);
            check_ze_result(zeDeviceGet(m_levelzero_driver.at(driver), &num_device, device_handle.data()),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": LevelZero Device acquisition failed.", __LINE__);

            for (unsigned int device_idx = 0; device_idx < num_device; ++device_idx) {
                ze_device_properties_t property = {};
                check_ze_result(zeDeviceGetProperties(device_handle.at(device_idx), &property),
                                GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                                ": failed to get device properties.", __LINE__);

                uint32_t num_subdevice = 0;
                check_ze_result(zeDeviceGetSubDevices(device_handle.at(device_idx), &num_subdevice, nullptr),
                                GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                                ": LevelZero Sub-Device enumeration failed.", __LINE__);

                std::vector<zes_device_handle_t> subdevice_handle(num_subdevice);
                check_ze_result(zeDeviceGetSubDevices(device_handle.at(device_idx),
                                                      &num_subdevice, subdevice_handle.data()),
                                GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                                ": LevelZero Sub-Device acquisition failed.", __LINE__);
#ifdef GEOPM_DEBUG
                if (num_subdevice == 0) {
                    std::cerr << "LevelZero::" << std::string(__func__)
                              << ": GEOPM Requires at least one subdevice. "
                              << "Please check ZE_AFFINITY_MASK environment variable "
                              << "setting.  Forcing device to act as sub-device" << std::endl;
                }
#endif
                if (property.type == ZE_DEVICE_TYPE_GPU) {
                    if ((property.flags & ZE_DEVICE_PROPERTY_FLAG_INTEGRATED) == 0) {
                        ++m_num_gpu;
                        m_num_gpu_subdevice += num_subdevice;
                        if (num_subdevice == 0) {
                            // If there are no subdevices we are going to treat the
                            // device as a subdevice.
                            m_num_gpu_subdevice += 1;
                        }

                        // We create a context to support the ZET commands
                        // NOTE: a context is being created per subdevice, making this context
                        // unnecessary. Commenting out for now.
                        // TODO: Explore replacing per-subdevice contexts with a single context
                        // (per-device or globally)
                        // ze_context_desc_t context_desc = {
                        //    ZE_STRUCTURE_TYPE_CONTEXT_DESC,
                        //    nullptr,
                        //    0
                        // };
                        // ze_context_handle_t context = nullptr;
                        // ze_result_t ze_result = zeContextCreate(m_levelzero_driver.at(driver),
                        //                             &context_desc, &context);
                        // check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        //                 "LevelZero::" + std::string(__func__) +
                        //                 ": LevelZero context creation failed",
                        //                 __LINE__);

                        m_devices.push_back({
                            m_levelzero_driver.at(driver),
                            device_handle.at(device_idx),
                            property,
                            num_subdevice, //if there are no subdevices leave this as 0
                            std::move(subdevice_handle),
                            {}, //subdevice
                            0, //num_device_power_domain
                            {}, //power domain
                            0, // cached_energy_timestamp
                            0, // metric_sampling_period_ns
                        });
                    }
#ifdef GEOPM_DEBUG
                    else {
                        std::cerr << "Warning: <geopm> LevelZero: Integrated "
                                  << "GPU access is not currently supported by GEOPM.\n";
                    }
#endif
                }
#ifdef GEOPM_DEBUG
                else if (property.type == ZE_DEVICE_TYPE_CPU) {
                    // All CPU functionality is handled by GEOPM & MSR Safe currently
                    std::cerr << "Warning: <geopm> LevelZero: CPU access "
                              << "via LevelZero is not currently supported by GEOPM.\n";
                }
                else if (property.type == ZE_DEVICE_TYPE_FPGA) {
                    // FPGA functionality is not currently supported by GEOPM, but should not cause
                    // an error if the devices are present
                    std::cerr << "Warning: <geopm> LevelZero: Field Programmable "
                              << "Gate Arrays are not currently supported by GEOPM.\n";
                }
                else if (property.type == ZE_DEVICE_TYPE_MCA) {
                    // MCA functionality is not currently supported by GEOPM, but should not cause
                    // an error if the devices are present
                    std::cerr << "Warning: <geopm> LevelZero: Memory Copy GPUs "
                              << "are not currently supported by GEOPM.\n";
                }
#endif
            }

            if (m_num_gpu != 0 && m_num_gpu_subdevice % m_num_gpu != 0) {
                throw Exception("LevelZero::" + std::string(__func__) +
                                ": GEOPM Requires the number of subdevices to be" +
                                " evenly divisible by the number of devices. " +
                                " Please check ZE_AFFINITY_MASK environment variable settings",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            // If we have more than one device confirm all devices have the same
            // number of subdevices
            for (unsigned int idx = 1; idx < m_devices.size(); ++idx) {
                if (m_devices.at(idx).num_subdevice != m_devices.at(idx - 1).num_subdevice) {
                    throw Exception("LevelZero::" + std::string(__func__) +
                                    ": GEOPM Requires the number of subdevices to be" +
                                    " the same on all devices. " +
                                    " Please check ZE_AFFINITY_MASK environment variable settings",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
        }

        // TODO: When additional device types such as FPGA, MCA, and Integrated GPU are supported by GEOPM
        // This should be changed to a more general loop iterating over type and caching appropriately
        for (unsigned int gpu_idx = 0; gpu_idx < m_num_gpu; gpu_idx++) {
            frequency_domain_cache(gpu_idx);
            power_domain_cache(gpu_idx);
            perf_domain_cache(gpu_idx);
            engine_domain_cache(gpu_idx);
            temperature_domain_cache(gpu_idx);
            ras_domain_cache(gpu_idx);
            metric_group_init(gpu_idx);
        }
    }

    LevelZeroImp::~LevelZeroImp() {
        // Stop the background sampling thread before tearing down streamers.
        metric_thread_stop();
        for (unsigned int gpu_idx = 0; gpu_idx < m_num_gpu; ++gpu_idx) {
            for (unsigned int subdevice_idx = 0;
             subdevice_idx < m_devices.at(gpu_idx).num_subdevice;
             ++subdevice_idx) {
                metric_destroy(gpu_idx, subdevice_idx);
            }
        }
    }

    void LevelZeroImp::frequency_domain_cache(unsigned int device_idx)
    {
        //Cache frequency domains
        uint32_t num_domain = 0;
        ze_result_t ze_result = zesDeviceEnumFrequencyDomains(m_devices.at(device_idx).device_handle,
                                                              &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: Frequency domain detection is "
                      << "not supported.\n";
#endif
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get number of domains.", __LINE__);
            //make temp var
            std::vector<zes_freq_handle_t> freq_domain(num_domain);
            check_ze_result(zesDeviceEnumFrequencyDomains(m_devices.at(device_idx).device_handle,
                                                          &num_domain, freq_domain.data()),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get domain handles.", __LINE__);

            m_devices.at(device_idx).subdevice.freq_domain.resize(geopm::LevelZero::M_DOMAIN_SIZE);

            for (auto handle : freq_domain) {
                zes_freq_properties_t property = {};
                check_ze_result(zesFrequencyGetProperties(handle, &property),
                                GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                                ": Sysman failed to get domain properties.", __LINE__);

                if (property.onSubdevice == 0 && m_devices.at(device_idx).num_subdevice != 0) {
#ifdef GEOPM_DEBUG
                    std::cerr << "Warning: <geopm> LevelZero: A device level "
                              << "frequency domain was found but is not currently supported.\n";
#endif
                }
                else {
                    if (property.type == ZES_FREQ_DOMAIN_GPU) {
                        m_devices.at(device_idx).
                            subdevice.freq_domain.at(geopm::LevelZero::M_DOMAIN_COMPUTE).push_back(handle);
                    }
                    else if (property.type == ZES_FREQ_DOMAIN_MEMORY) {
                        m_devices.at(device_idx).
                            subdevice.freq_domain.at(geopm::LevelZero::M_DOMAIN_MEMORY).push_back(handle);
                    }
                }
            }
        }
    }

    void LevelZeroImp::power_domain_cache(unsigned int device_idx)
    {
        //Cache power domains
        uint32_t num_domain = 0;
        ze_result_t ze_result = zesDeviceEnumPowerDomains(m_devices.at(device_idx).device_handle,
                                                          &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: Power domain detection is "
                      << "not supported.\n";
#endif
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get number of domains", __LINE__);

            std::vector<zes_pwr_handle_t> power_domain(num_domain);
            check_ze_result(zesDeviceEnumPowerDomains(m_devices.at(device_idx).device_handle,
                                                      &num_domain, power_domain.data()),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get domain handle(s).", __LINE__);

            uint32_t num_device_power_domain = 0;
            uint32_t num_subdevice_power_domain = 0;
            for (auto handle : power_domain) {
                zes_power_properties_t property = {};
                check_ze_result(zesPowerGetProperties(handle, &property),
                                GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                                ": Sysman failed to get domain power properties", __LINE__);

                //Finding non-subdevice domain.
                if (property.onSubdevice == 0) {
                    m_devices.at(device_idx).power_domain = handle;
                    ++num_device_power_domain;
                    if (num_device_power_domain != 1) {
                        throw Exception("LevelZero::" + std::string(__func__) +
                                        ": Multiple device level power domains "
                                        "detected.  This may lead to incorrect power readings",
                                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                    }
                }
                else {
                    ++num_subdevice_power_domain;
                    m_devices.at(device_idx).subdevice.power_domain.push_back(handle);
                }
            }

            if (num_device_power_domain != 1) {
                throw Exception("LevelZero::" + std::string(__func__) +
                                ": GEOPM requires one and only one device "+
                                "level power domain (detected: " +
                                std::to_string(num_device_power_domain) +  ").",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            if (num_subdevice_power_domain > m_devices.at(device_idx).num_subdevice) {
                throw Exception("LevelZero::" + std::string(__func__) +
                                ": Number of subdevice power domains (" +
                                std::to_string(num_device_power_domain) +
                                ") exceeds the number of subdevices (" +
                                std::to_string(m_devices.at(device_idx).num_subdevice) + ").",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            m_devices.at(device_idx).num_device_power_domain = num_device_power_domain;
            m_devices.at(device_idx).subdevice.num_subdevice_power_domain = num_subdevice_power_domain;
            m_devices.at(device_idx).subdevice.
                cached_energy_timestamp.resize(m_devices.at(device_idx).subdevice.power_domain.size());
        }

    }

    void LevelZeroImp::perf_domain_cache(unsigned int device_idx)
    {
        //Cache performance domains
        uint32_t num_domain = 0;
        ze_result_t ze_result = zesDeviceEnumPerformanceFactorDomains(m_devices.at(device_idx).device_handle,
                                                                      &num_domain, nullptr);
        if (ze_result != ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get number of domains", __LINE__);

            std::vector<zes_perf_handle_t> perf_domain(num_domain);
            check_ze_result(zesDeviceEnumPerformanceFactorDomains(m_devices.at(device_idx).device_handle,
                                                                  &num_domain, perf_domain.data()),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get domain handle(s).", __LINE__);

            m_devices.at(device_idx).subdevice.perf_domain.resize(geopm::LevelZero::M_DOMAIN_SIZE);

            for (auto handle : perf_domain) {
                zes_perf_properties_t property = {};
                check_ze_result(zesPerformanceFactorGetProperties(handle, &property),
                                GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                                ": Sysman failed to get domain performance factor properties", __LINE__);

                //Finding subdevice domain.
                if (property.onSubdevice != 0) {
                    if (property.engines == ZES_ENGINE_TYPE_FLAG_COMPUTE) {
                        m_devices.at(device_idx).subdevice.perf_domain.at(
                            geopm::LevelZero::M_DOMAIN_COMPUTE).push_back(handle);
                    }
#ifdef GEOPM_DEBUG
                    else {
                        std::cerr << "Warning: <geopm> LevelZero:"
                                  << " Unsupported sub-device level performance factor domain ("
                                  << std::to_string(property.engines) << ") detected." << std::endl;
                    }
#endif
                }
            }
        }
#ifdef GEOPM_DEBUG
        else {
            std::cerr << "Warning: <geopm> LevelZero: Performance domain detection is "
                      << "not supported.\n";
        }
#endif
    }

    void LevelZeroImp::engine_domain_cache(unsigned int device_idx)
    {
        //Cache engine domains
        uint32_t num_domain = 0;
        ze_result_t ze_result = zesDeviceEnumEngineGroups(m_devices.at(device_idx).device_handle,
                                                          &num_domain, nullptr);

        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: Engine domain detection is "
                      << "not supported.\n";
#endif
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZero::" +
                            std::string(__func__) +
                            ": Sysman failed to get number of domains", __LINE__);

            std::vector<zes_engine_handle_t> engine_domain(num_domain);
            check_ze_result(zesDeviceEnumEngineGroups(m_devices.at(device_idx).device_handle,
                                                      &num_domain, engine_domain.data()),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get number of domains", __LINE__);

            m_devices.at(device_idx).subdevice.engine_domain.resize(geopm::LevelZero::M_DOMAIN_SIZE);
            m_devices.at(device_idx).subdevice.cached_timestamp.resize(geopm::LevelZero::M_DOMAIN_SIZE);

            for (auto handle : engine_domain) {
                zes_engine_properties_t property = {};
                check_ze_result(zesEngineGetProperties(handle, &property),
                                GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                                ": Sysman failed to get domain engine properties", __LINE__);

                if (property.onSubdevice == 0 && m_devices.at(device_idx).num_subdevice != 0) {
#ifdef GEOPM_DEBUG
                    std::cerr << "Warning: <geopm> LevelZero: A device level "
                              << "engine domain was found but is not currently supported.\n";
#endif
                }
                else {
                    if (property.type == ZES_ENGINE_GROUP_ALL) {
                        m_devices.at(device_idx).
                            subdevice.engine_domain.at(geopm::LevelZero::M_DOMAIN_ALL).push_back(handle);
                        m_devices.at(device_idx).
                            subdevice.cached_timestamp.at(geopm::LevelZero::M_DOMAIN_ALL).push_back(0);
                    }

                    //TODO: Some devices may not support ZES_ENGINE_GROUP_COMPUTE/COPY_ALL.
                    //      We can do a check for COMPUTE_ALL and then fallback to change to
                    //      ZES_ENGINE_GROUP_COMPUTE/COPY_SINGLE, but we have to
                    //      aggregate the signals in that case
                    else if (property.type == ZES_ENGINE_GROUP_COMPUTE_ALL) {
                        m_devices.at(device_idx).
                            subdevice.engine_domain.at(geopm::LevelZero::M_DOMAIN_COMPUTE).push_back(handle);
                        m_devices.at(device_idx).
                            subdevice.cached_timestamp.at(geopm::LevelZero::M_DOMAIN_COMPUTE).push_back(0);
                    }
                    else if (property.type == ZES_ENGINE_GROUP_COPY_ALL) {
                        m_devices.at(device_idx).
                            subdevice.engine_domain.at(geopm::LevelZero::M_DOMAIN_MEMORY).push_back(handle);
                        m_devices.at(device_idx).
                            subdevice.cached_timestamp.at(geopm::LevelZero::M_DOMAIN_MEMORY).push_back(0);
                    }
                }
            }

#ifdef GEOPM_DEBUG
            if (num_domain != 0 &&
                m_devices.at(device_idx).
                    subdevice.engine_domain.at(geopm::LevelZero::M_DOMAIN_COMPUTE).size() == 0) {
                std::cerr << "Warning: <geopm> LevelZero: Engine domain detection "
                          << "did not find ZES_ENGINE_GROUP_COMPUTE_ALL.\n";
            }
            if (num_domain != 0 &&
                m_devices.at(device_idx).
                    subdevice.engine_domain.at(geopm::LevelZero::M_DOMAIN_MEMORY).size() == 0) {
                std::cerr << "Warning: <geopm> LevelZero: Engine domain detection "
                          << "did not find ZES_ENGINE_GROUP_COPY_ALL.\n";
            }
#endif
        }
    }

    void LevelZeroImp::temperature_domain_cache(unsigned int device_idx)
    {
        //Cache frequency domains
        uint32_t num_domain = 0;
        ze_result_t ze_result = zesDeviceEnumTemperatureSensors(m_devices.at(device_idx).device_handle,
                                                                &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: Temperature domain detection is "
                      << "not supported.\n";
#endif
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get number of temperature domains.", __LINE__);
            //make temp var
            std::vector<zes_temp_handle_t> temp_domain(num_domain);
            check_ze_result(zesDeviceEnumTemperatureSensors(m_devices.at(device_idx).device_handle,
                                                            &num_domain, temp_domain.data()),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get temperature domain handles.", __LINE__);

            m_devices.at(device_idx).subdevice.temp_domain_max.resize(geopm::LevelZero::M_DOMAIN_SIZE);

            for (auto handle : temp_domain) {
                zes_temp_properties_t property = {};
                check_ze_result(zesTemperatureGetProperties(handle, &property),
                                GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                                ": Sysman failed to get temperature domain properties.", __LINE__);

                if (property.onSubdevice == 0) {
#ifdef GEOPM_DEBUG
                    std::cerr << "Warning: <geopm> LevelZero: A device level "
                              << "temperature domain was found but is not currently supported.\n";
#endif
                }
                else {
                    if (property.type == ZES_TEMP_SENSORS_GPU) {
                        m_devices.at(device_idx).
                            subdevice.temp_domain_max.at(geopm::LevelZero::M_DOMAIN_COMPUTE).push_back(handle);
                    }
                    else if (property.type == ZES_TEMP_SENSORS_MEMORY) {
                        m_devices.at(device_idx).
                            subdevice.temp_domain_max.at(geopm::LevelZero::M_DOMAIN_MEMORY).push_back(handle);
                    }
                    else if (property.type == ZES_TEMP_SENSORS_GLOBAL) {
                        m_devices.at(device_idx).
                            subdevice.temp_domain_max.at(geopm::LevelZero::M_DOMAIN_ALL).push_back(handle);
                    }
                }
            }
        }
    }

    //  Cache the following per gpu_idx:
    //      - caching & initialization tracking variable
    //      - L0 context
    //      - metric group handle
    //      - sampling period
    //      - data storage for the ComputeBasics metric group
    void LevelZeroImp::metric_group_init(unsigned int device_idx) {
        for (unsigned int subdevice_idx = 0;
             subdevice_idx < m_devices.at(device_idx).num_subdevice;
             ++subdevice_idx) {
            // Setup tracking of the caching and initialization steps
            m_devices.at(device_idx).subdevice.metric_domain_cached.push_back(false);
            m_devices.at(device_idx).subdevice.metrics_initialized.push_back(false);

            ze_result_t ze_result;

            // We create a context to support the ZET commands
            ze_context_desc_t context_desc = {
               ZE_STRUCTURE_TYPE_CONTEXT_DESC,
               nullptr,
               0
            };
            ze_context_handle_t context = nullptr;
            ze_result = zeContextCreate(m_devices.at(device_idx).driver,
                                        &context_desc, &context);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": LevelZero context creation failed",
                            __LINE__);

            m_devices.at(device_idx).subdevice.context.push_back(context);

            //Metric groups
            uint32_t num_metric_group = 0;
            ze_result = zetMetricGroupGet(m_devices.at(device_idx).subdevice_handle.at(subdevice_idx),
                                          &num_metric_group, nullptr);

            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": LevelZero Metric Group enumeration failed.",
                             __LINE__);

            std::vector<zet_metric_group_handle_t> metric_group_handle(num_metric_group);
            ze_result = zetMetricGroupGet(m_devices.at(device_idx).subdevice_handle.at(subdevice_idx),
                                          &num_metric_group, metric_group_handle.data());

            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": LevelZero Metric Group handle acquisition failed",
                            __LINE__);

            // set metric_notifcation_event sampling period in nanoseconds
            m_devices.at(device_idx).metric_sampling_period_ns = SAMPLING_PERIOD_NS;

            m_devices.at(device_idx).subdevice.metric_data.push_back({});
            m_devices.at(device_idx).subdevice.metric_data_accum.push_back({});
            m_devices.at(device_idx).subdevice.metric_active.push_back(false);
            m_devices.at(device_idx).subdevice.metric_last_drain.push_back(
                std::chrono::steady_clock::time_point{});

            for (unsigned int metric_group_idx = 0; metric_group_idx < num_metric_group;
                 metric_group_idx++) {
                zet_metric_group_properties_t metric_group_properties;
                ze_result = zetMetricGroupGetProperties(metric_group_handle.at(metric_group_idx),
                                            &metric_group_properties);
                check_ze_result(ze_result,GEOPM_ERROR_RUNTIME,
                                "LevelZero::" + std::string(__func__) +
                                ": LevelZero Metric Group property acquisition failed",
                                __LINE__);

                std::string metric_group_name (metric_group_properties.name);

                // Confirm metric groups of interest exist
                // Eventually the metric group of interest may be configurable,
                // to start we're using compute basics
                if (metric_group_properties.samplingType == ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED
                    && metric_group_name == "ComputeBasic") {

                   //cache compute basic metric group
                   m_devices.at(device_idx).subdevice.metric_group_handle.push_back(metric_group_handle.at(metric_group_idx));

                   // could likely use metric_group_properties.metricCount instead
                   uint32_t num_metric = 0;
                   ze_result = zetMetricGet(metric_group_handle.at(metric_group_idx), &num_metric, nullptr );
                   check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                                   "LevelZero::" + std::string(__func__) +
                                   ": LevelZero Metric Count query failed",
                                   __LINE__);

                   //Cache compute basic number of metrics
                   m_devices.at(device_idx).subdevice.num_metric.push_back(num_metric);

                   std::vector<zet_metric_handle_t> metric_handle(num_metric);
                   ze_result = zetMetricGet(metric_group_handle.at(metric_group_idx),
                                            &num_metric, metric_handle.data());

                   check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                                   "LevelZero::" + std::string(__func__) +
                                   ": LevelZero Metric handle acquisition failed",
                                   __LINE__);

                   //Build metric map and name-to-index cache
                   std::map<std::string, size_t> name_idx;
                   for (unsigned int metric_idx = 0; metric_idx < num_metric; ++metric_idx)
                   {
                       zet_metric_properties_t metric_properties;
                       ze_result = zetMetricGetProperties(metric_handle.at(metric_idx), &metric_properties);

                       check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                                       "LevelZero::" + std::string(__func__) +
                                       ": LevelZero Metric Property acquisition failed",
                                       __LINE__);

                       std::string metric_name (metric_properties.name);

                       //m_devices is a std::vector of structs, indexed by GPU
                       // subdevice is a struct inside of it.  One per GPU
                       //  metric_data is a vector, indexed by CHIP
                       //    it contains a map of metric name (string) -> report data for N reports (vector<double>)
                       if (m_devices.at(device_idx).subdevice.metric_data.at(subdevice_idx).count(metric_name) != 0) {
                           throw Exception("LevelZero::" + std::string(__func__) +
                                           ": Metric group has two metrics with the same name",
                                           GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                       }
                       m_devices.at(device_idx).subdevice.metric_data.at(subdevice_idx)[metric_name] = {};
                       m_devices.at(device_idx).subdevice.metric_data.at(subdevice_idx)["NUM_REPORTS"] = {};

                       // Cache index for metrics we process in metric_calc
                       if (metric_name == "XVE_ACTIVE" ||
                           metric_name == "XVE_STALL") {
                           name_idx[metric_name] = metric_idx;
                       }
                   }
                   m_devices.at(device_idx).subdevice.metric_name_idx.push_back(std::move(name_idx));
                   // Break out of the metric group for loop once we've found the group of interest (ComputeBasic, time based sampling).
                   break;
                }
            }
            m_devices.at(device_idx).subdevice.metric_domain_cached.at(subdevice_idx) = true;
        }
    }

    void LevelZeroImp::metric_destroy(unsigned int l0_device_idx, unsigned int l0_domain_idx)
    {
        if (!m_devices.at(l0_device_idx).subdevice.metric_domain_cached.at(l0_domain_idx) ||
            m_devices.at(l0_device_idx).subdevice.metrics_initialized.at(l0_domain_idx))
            return;

        ze_result_t ze_result;

        if (m_devices.at(l0_device_idx).subdevice.metrics_initialized.at(l0_domain_idx)) {
            m_devices.at(l0_device_idx).subdevice.metrics_initialized.at(l0_domain_idx) = false;

            // Close metric streamer
            ze_result = zetMetricStreamerClose(m_devices.at(l0_device_idx).subdevice.metric_streamer.at(l0_domain_idx));
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": LevelZero Metric Streamer Close failed",
                            __LINE__);

            // Deactivate the device context
            ze_context_handle_t context = m_devices.at(l0_device_idx).subdevice.context.at(l0_domain_idx);
            ze_result = zetContextActivateMetricGroups(context,
                                                       m_devices.at(l0_device_idx).device_handle,
                                                       0, nullptr);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": LevelZero Metric Context Deactivation failed",
                            __LINE__);
            zeContextDestroy(context);
        }
    }


    void LevelZeroImp::metric_execute(unsigned int l0_device_idx, unsigned int l0_domain_idx)
    {
        ze_result_t ze_result;
        ze_context_handle_t context = m_devices.at(l0_device_idx).subdevice.context.at(l0_domain_idx);
        ze_result = zetContextActivateMetricGroups(context, m_devices.at(l0_device_idx).subdevice_handle.at(l0_domain_idx),
                                                   1, &m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx));

        zet_metric_streamer_desc_t metric_streamer_desc = {
            ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC,
            nullptr,
            0, // notifyEveryNReports: unused (no event handle registered)
            m_devices.at(l0_device_idx).metric_sampling_period_ns};
        zet_metric_streamer_handle_t metric_streamer = nullptr;

        ze_result = zetMetricStreamerOpen(context, m_devices.at(l0_device_idx).subdevice_handle.at(l0_domain_idx), m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx), &metric_streamer_desc, nullptr, &metric_streamer);

        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": LevelZero Metric Streamer Open failed",
                        __LINE__);

        // TODO: nothing guarantees CHIP 0 was called first
        m_devices.at(l0_device_idx).subdevice.metric_streamer.push_back(metric_streamer);

        // Allocate memory for future reads
        // Doing a read might not be the best way to set a buffer size, as the amount of data
        // available at this point in time will likely not match the amount of data at any future
        // point in time while sampling. Using a default buffer size for now...
        size_t data_size = DEFAULT_REPORT_BUFFER_SIZE;
        // ze_result = zetMetricStreamerReadData(m_devices.at(l0_device_idx).subdevice.metric_streamer.at(l0_domain_idx),
        //                                       UINT32_MAX, &data_size, nullptr); //TODO: this value should match the report_count_req in metric_read
        // check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
        //                 "LevelZero::" + std::string(__func__) +
        //                 ": LevelZero Read Data get size failed",
        //                 __LINE__);

        std::vector<uint8_t> data(data_size);
        m_devices.at(l0_device_idx).subdevice.zet_data_size.push_back(data_size);
        m_devices.at(l0_device_idx).subdevice.zet_data.push_back(data);
        m_devices.at(l0_device_idx).subdevice.report_byte_size.push_back(0);
    }

    // TODO don't pass metric_streamer
    void LevelZeroImp::metric_calc(unsigned int l0_device_idx, unsigned int l0_domain_idx,
                                   size_t data_size, const uint8_t *data)
    {

        GEOPM_DEBUG_ASSERT(m_devices.at(l0_device_idx).subdevice.metric_domain_cached.at(l0_domain_idx) == true,
                           "metric caching for GPU " + std::to_string(l0_device_idx) +
                           ", CHIP " + std::to_string(l0_domain_idx) +
                           " not completed prior to metric_calc call.");

        GEOPM_DEBUG_ASSERT(m_devices.at(l0_device_idx).subdevice.metrics_initialized.at(l0_domain_idx) == true,
                           "metric initialization for GPU " + std::to_string(l0_device_idx) +
                           ", CHIP " + std::to_string(l0_domain_idx) +
                           " not completed prior to metric_calc call.");

        ze_result_t ze_result;
        /////////////////////////////////////
        // Calculate & convert metric data //
        /////////////////////////////////////
        uint32_t num_metric_values = 0;
        zet_metric_group_calculation_type_t calculation_type = ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES;
        ze_result = zetMetricGroupCalculateMetricValues(m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx), calculation_type, data_size, data, &num_metric_values, nullptr);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": LevelZero Metric group calculate metric values to find num metrics failed",
                        __LINE__);

        std::vector<zet_typed_value_t> metric_values(num_metric_values);
        ze_result = zetMetricGroupCalculateMetricValues(m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx), calculation_type, data_size, data, &num_metric_values, metric_values.data());
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": LevelZero Metric group calculate metric values to calculate data failed",
                        __LINE__);

        uint32_t num_metric = m_devices.at(l0_device_idx).subdevice.num_metric.at(l0_domain_idx);
        unsigned int num_reports = num_metric_values / num_metric;

        // If num_metric_values is not evenly divisible by num_metrics
        // skip the metric and report processing for this sample
        if (num_metric_values % num_metric != 0) {
#ifdef GEOPM_DEBUG
            std::cerr << "LevelZero::" << std::string(__func__)  <<
                         ": ZET metric_calc call returned a number of metric values "
                         "that is not evenly divisible by the number of metrics.  This "
                         "may indicate a ZET report erroor, skipping data processing." << std::endl;
#endif
            return;
        }

        // Use the cached name→index map to avoid calling zetMetricGet and
        // zetMetricGetProperties on every sample iteration.  The report values
        // are appended to the accumulator; the caller (metric_drain) holds
        // m_metric_mutex, and the controller snapshots (moves) the accumulator
        // into metric_data once per read_batch().
        const auto &name_idx = m_devices.at(l0_device_idx).subdevice.metric_name_idx.at(l0_domain_idx);
        auto &accum = m_devices.at(l0_device_idx).subdevice.metric_data_accum.at(l0_domain_idx);
        for (const auto &kv : name_idx) {
            const std::string &metric_name = kv.first;
            size_t metric_idx = kv.second;

            std::vector<double> &values = accum[metric_name];
            for (unsigned int report_idx = 0; report_idx < num_reports; report_idx++) {
                zet_typed_value_t data = metric_values.at(report_idx * num_metric + metric_idx);
                values.push_back(metric_data_convert(data));
            }
        }
    }

    double LevelZeroImp::metric_data_convert(zet_typed_value_t data) const
    {
        double data_double = NAN;
        switch(data.type) {
        case ZET_VALUE_TYPE_UINT32:
            data_double = data.value.ui32;
            break;
        case ZET_VALUE_TYPE_UINT64:
            data_double = data.value.ui64;
            break;
        case ZET_VALUE_TYPE_FLOAT32:
            data_double = data.value.fp32;
            break;
        case ZET_VALUE_TYPE_FLOAT64:
            data_double = data.value.fp64;
            break;
        case ZET_VALUE_TYPE_BOOL8:
            data_double = data.value.ui32;
            break;
        default:
            break;
        };
        return data_double;
    }

    void LevelZeroImp::metric_read(unsigned int l0_device_idx, unsigned int l0_domain_idx)
    {
        if (!m_devices.at(l0_device_idx).subdevice.metric_domain_cached.at(l0_domain_idx)) {
            return;
        }

        const auto &name_idx = m_devices.at(l0_device_idx).subdevice.metric_name_idx.at(l0_domain_idx);
        {
            std::lock_guard<std::mutex> lock(m_metric_mutex);

            // Open the streamer on first use.  Done from the controller thread in
            // chip order so the streamer vectors grow safely, and serialized with
            // the background thread by m_metric_mutex.
            if (!m_devices.at(l0_device_idx).subdevice.metrics_initialized.at(l0_domain_idx)) {
                metric_execute(l0_device_idx, l0_domain_idx);
                m_devices.at(l0_device_idx).subdevice.metrics_initialized.at(l0_domain_idx) = true;
            }
            m_devices.at(l0_device_idx).subdevice.metric_active.at(l0_domain_idx) = true;

            // Drain now so this sample always has fresh data, regardless of how
            // the controller period compares to the background drain cadence.
            metric_drain(l0_device_idx, l0_domain_idx);

            // Snapshot the accumulated reports (this drain plus any the
            // background thread gathered since the last read_batch) into
            // metric_data.  metric_data therefore holds every report gathered
            // over the controller period (averaged later by metric_sample).
            auto &accum = m_devices.at(l0_device_idx).subdevice.metric_data_accum.at(l0_domain_idx);
            auto &current = m_devices.at(l0_device_idx).subdevice.metric_data.at(l0_domain_idx);
            size_t num_reports = 0;
            for (const auto &kv : name_idx) {
                const std::string &metric_name = kv.first;
                current[metric_name] = std::move(accum[metric_name]);
                accum[metric_name].clear();
                num_reports = current[metric_name].size();
            }
            current["NUM_REPORTS"] = std::vector<double>{ static_cast<double>(num_reports) };
        }

        // Launch the background sampling thread on first use.
        metric_thread_start();
    }

    void LevelZeroImp::metric_thread_start(void)
    {
        // Called only from the controller thread (metric_read), so the started
        // flag need not be atomic.
        if (!m_metric_thread_started) {
            m_metric_thread_started = true;
            m_metric_thread_active.store(true);
            m_metric_thread = std::thread(&LevelZeroImp::metric_sample_thread, this);
        }
    }

    void LevelZeroImp::metric_thread_stop(void)
    {
        if (m_metric_thread_active.exchange(false)) {
            if (m_metric_thread.joinable()) {
                m_metric_thread.join();
            }
        }
    }

    void LevelZeroImp::metric_sample_thread(void)
    {
        // Keep-alive draining: the controller drains each chip itself in
        // metric_read(), so this thread only needs to drain a chip when the
        // controller hasn't drained it recently.  This keeps the metric streamer
        // from stalling during long controller periods, while avoiding redundant
        // draining (and lock contention) when the controller drains frequently.
        while (m_metric_thread_active.load()) {
            auto now = std::chrono::steady_clock::now();
            for (unsigned int l0_device_idx = 0; l0_device_idx < m_num_gpu; ++l0_device_idx) {
                unsigned int num_subdevice = m_devices.at(l0_device_idx).num_subdevice;
                for (unsigned int l0_domain_idx = 0; l0_domain_idx < num_subdevice; ++l0_domain_idx) {
                    std::lock_guard<std::mutex> lock(m_metric_mutex);
                    auto &sub = m_devices.at(l0_device_idx).subdevice;
                    if (!sub.metric_active.at(l0_domain_idx) ||
                        !sub.metric_domain_cached.at(l0_domain_idx) ||
                        !sub.metrics_initialized.at(l0_domain_idx)) {
                        continue;
                    }
                    auto elapsed = now - sub.metric_last_drain.at(l0_domain_idx);
                    if (elapsed >= std::chrono::microseconds(METRIC_DRAIN_PERIOD_US)) {
                        metric_drain(l0_device_idx, l0_domain_idx);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(METRIC_DRAIN_PERIOD_US));
        }
    }

    void LevelZeroImp::metric_drain(unsigned int l0_device_idx, unsigned int l0_domain_idx)
    {
        // Caller must hold m_metric_mutex.
        ze_result_t ze_result;
        uint32_t report_count_req = 1;
        zet_metric_streamer_handle_t metric_streamer =
            m_devices.at(l0_device_idx).subdevice.metric_streamer.at(l0_domain_idx);

        // Record the drain attempt so the keep-alive thread can tell whether the
        // controller is draining this chip on its own.
        m_devices.at(l0_device_idx).subdevice.metric_last_drain.at(l0_domain_idx) =
            std::chrono::steady_clock::now();

        ///////////////////
        // Read Raw Data //
        ///////////////////
        // Always read with full buffer to drain the FIFO completely
        size_t read_size = m_devices.at(l0_device_idx).subdevice.zet_data.at(l0_domain_idx).size();
        ze_result = zetMetricStreamerReadData(metric_streamer, report_count_req,
                                              &read_size,
                                              m_devices.at(l0_device_idx).subdevice.zet_data.at(l0_domain_idx).data());

        // Dropped-data is a warning with still-valid reports (FIFO overran); not a fault.
        if (ze_result == ZE_RESULT_WARNING_DROPPED_DATA) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero::" << std::string(__func__)
                      << ": metric streamer dropped data; processing remaining reports."
                      << std::endl;
#endif
            ze_result = ZE_RESULT_SUCCESS;
        }

        // Skip when no data is available
        if (ze_result != ZE_RESULT_NOT_READY && read_size > 0) {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                            "LevelZero::" + std::string(__func__) +
                            ": LevelZero Read Data failed",
                            __LINE__);

            // Learn per-report byte size from first successful read
            size_t &report_byte_size = m_devices.at(l0_device_idx).subdevice.report_byte_size.at(l0_domain_idx);
            if (report_byte_size == 0) {
                uint32_t tmp_num_values = 0;
                zet_metric_group_calculation_type_t tmp_calc_type = ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES;
                zetMetricGroupCalculateMetricValues(
                    m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx),
                    tmp_calc_type, read_size,
                    m_devices.at(l0_device_idx).subdevice.zet_data.at(l0_domain_idx).data(),
                    &tmp_num_values, nullptr);
                uint32_t num_metric = m_devices.at(l0_device_idx).subdevice.num_metric.at(l0_domain_idx);
                size_t first_num_reports = (num_metric > 0) ? tmp_num_values / num_metric : 1;
                if (first_num_reports > 0) {
                    report_byte_size = read_size / first_num_reports;
                }
            }

            // Only process the most recent DEFAULT_MAX_REPORTS_PER_READ reports
            size_t process_size = read_size;
            const uint8_t *process_data = m_devices.at(l0_device_idx).subdevice.zet_data.at(l0_domain_idx).data();
            if (report_byte_size > 0 && read_size > report_byte_size * DEFAULT_MAX_REPORTS_PER_READ) {
                process_size = report_byte_size * DEFAULT_MAX_REPORTS_PER_READ;
                process_data = m_devices.at(l0_device_idx).subdevice.zet_data.at(l0_domain_idx).data()
                               + (read_size - process_size);
            }

            metric_calc(l0_device_idx, l0_domain_idx, process_size, process_data);
        }
    }

    std::vector<double> LevelZeroImp::metric_sample(unsigned int l0_device_idx,
                                                    unsigned int l0_domain_idx,
                                                    std::string metric_name) const
    {
        std::vector<double> result = {};
        if (!m_devices.at(l0_device_idx).subdevice.metric_domain_cached.at(l0_domain_idx)) {
            throw Exception("LevelZero::" + std::string(__func__) +
                            ": Metric groups not cached" ,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (m_devices.at(l0_device_idx).subdevice.metric_data.at(l0_domain_idx).count(metric_name) == 0) {
            throw Exception("LevelZero::" + std::string(__func__) +
                            ": No metric named " + metric_name  + " found." ,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        result = m_devices.at(l0_device_idx).subdevice.metric_data.at(l0_domain_idx).at(metric_name);
        return result;
    }

    void LevelZeroImp::ras_domain_cache(unsigned int device_idx)
    {
        uint32_t ras_handle_count = 0;
        uint32_t num_subdevice = m_devices.at(device_idx).num_subdevice;
        // Find number of RAS error sets for the GPU
        uint32_t num_errset = 0;
        ze_result_t ze_result = zesDeviceEnumRasErrorSets(m_devices.at(device_idx).device_handle,
                                                          &num_errset, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE || num_errset == 0) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: RAS Error set detection is "
                      << "not supported.\n";
#endif
        }
        else {
            // Get handle of all RAS errorsets
            std::vector<zes_ras_handle_t> error_set(num_errset);

            check_ze_result(zesDeviceEnumRasErrorSets(m_devices.at(device_idx).device_handle,
                                                      &num_errset, error_set.data()),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get errorset handle(s).", __LINE__);

	        // Note: RAS domain errorset handles are being stored in a 2D vector with
	        //       dimensions := (number of subdevices) x (number of RAS error types)

            // Allocate size for a 2D vector to store all the RAS domain handles for errorsets:
            //       m_num_subdevice = number of subdevices on specific GPU device
            //       M_NUM_ERROR_TYPE = number of RAS error types

            m_devices.at(device_idx).subdevice.ras_domain.resize(num_subdevice * M_NUM_ERROR_TYPE);

            // Iterate over errorset handles
            for (auto handle : error_set) {
                // Get properties corresponding to each RAS errorset handle
                zes_ras_properties_t property;
                ze_result = zesRasGetProperties(handle, &property);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                                "LevelZero::" + std::string(__func__) +
                                ": Sysman failed to get RAS properties",
                                __LINE__);

		// Check if the RAS errorset handle maps to a subdevice
                if (property.onSubdevice == 0) {
#ifdef GEOPM_DEBUG
                    std::cerr << "Warning: <geopm> LevelZero: A device level "
                              << "RAS domain was found but is not currently supported.\n";
#endif
                }
		// Check if the RAS errorset handle maps to a known subdevice
		else if (property.subdeviceId >= num_subdevice) {
#ifdef GEOPM_DEBUG
                    std::cerr << "Warning: <geopm> LevelZero: A RAS domain handle "
                              << "was found to map to an unaccounted subdevice #"
                              << property.subdeviceId << "\n";
#endif

                }
		// Cache the RAS errorset handle into index - [subdevice Id][error type]
                else if (property.type == ZES_RAS_ERROR_TYPE_CORRECTABLE ||
                         property.type == ZES_RAS_ERROR_TYPE_UNCORRECTABLE) {
                   int ras_idx = property.subdeviceId * M_NUM_ERROR_TYPE +
                                 (property.type == ZES_RAS_ERROR_TYPE_CORRECTABLE ?
                                  M_ERROR_TYPE_CORRECTABLE : M_ERROR_TYPE_UNCORRECTABLE);
                   m_devices.at(device_idx).subdevice.ras_domain.at(ras_idx) = handle;
                   ++ras_handle_count;
                }
            }
        }
        if (ras_handle_count != num_subdevice * M_NUM_ERROR_TYPE) {
            throw Exception("LevelZero: Number of RAS error handles is incorrect",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    int LevelZeroImp::num_gpu() const
    {
        //  TODO: this should be expanded to return all supported GPU types.
        //  Right now that is only GPUs
        return num_gpu(GEOPM_DOMAIN_GPU);
    }

    int LevelZeroImp::num_gpu(int domain_type) const
    {
        int result = -1;
        switch(domain_type) {
            case GEOPM_DOMAIN_GPU:
                result = m_num_gpu;
                break;
            case GEOPM_DOMAIN_GPU_CHIP:
                result = m_num_gpu_subdevice;
                break;
            default :
                throw Exception("LevelZero::" + std::string(__func__) +
                                ": domain type " + std::to_string(domain_type) +
                                " is not supported.", GEOPM_ERROR_INVALID,
                                __FILE__, __LINE__);
                break;
        }
        return result;
    }

    int LevelZeroImp::power_domain_count(int geopm_domain,
                                         unsigned int l0_device_idx,
                                         int l0_domain) const
    {
        int count = 0;
        if (l0_domain == M_DOMAIN_ALL) {
            if (geopm_domain == GEOPM_DOMAIN_GPU) {
                count = m_devices.at(l0_device_idx).num_device_power_domain;
            }
            else if (geopm_domain == GEOPM_DOMAIN_GPU_CHIP) {
                count = m_devices.at(l0_device_idx).subdevice.num_subdevice_power_domain;
            }
        }
        return count;
    }

    int LevelZeroImp::frequency_domain_count(unsigned int l0_device_idx, int l0_domain) const
    {
        return m_devices.at(l0_device_idx).subdevice.freq_domain.at(l0_domain).size();
    }

    int LevelZeroImp::engine_domain_count(unsigned int l0_device_idx, int l0_domain) const
    {
        return m_devices.at(l0_device_idx).subdevice.engine_domain.at(l0_domain).size();
    }

    int LevelZeroImp::performance_domain_count(unsigned int l0_device_idx, int l0_domain) const
    {
        return m_devices.at(l0_device_idx).subdevice.perf_domain.at(l0_domain).size();
    }

    int LevelZeroImp::temperature_domain_count(unsigned int l0_device_idx, int l0_domain) const
    {
        return m_devices.at(l0_device_idx).subdevice.temp_domain_max.at(l0_domain).size();
    }

    double LevelZeroImp::performance_factor(unsigned int l0_device_idx,
                                            int l0_domain, int l0_domain_idx) const
    {
        double result = NAN;
        zes_perf_handle_t handle = m_devices.at(l0_device_idx).
                                       subdevice.perf_domain.at(l0_domain).at(l0_domain_idx);
        check_ze_result(zesPerformanceFactorGetConfig(handle, &result),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get performance factor values", __LINE__);
        return result;
    }


    int LevelZeroImp::ras_domain_count(unsigned int l0_device_idx,
                                       int l0_domain) const
    {
        uint32_t num_errset = 0;
        ze_result_t ze_result = zesDeviceEnumRasErrorSets(m_devices.at(l0_device_idx).
							  device_handle,
                                                          &num_errset, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE || num_errset == 0) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: RAS Error set detection is "
                      << "not supported.\n";
#endif
        }
        return num_errset;
    }

    double LevelZeroImp::ras_reset_count_correctable(unsigned int l0_device_idx,
                                                     int l0_domain,
                                                     int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_RESET, M_ERROR_TYPE_CORRECTABLE);
    }

    double LevelZeroImp::ras_programming_errcount_correctable(unsigned int l0_device_idx,
                                                              int l0_domain,
                                                              int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_PROGRAMMING_ERRORS, M_ERROR_TYPE_CORRECTABLE);
    }

    double LevelZeroImp::ras_driver_errcount_correctable(unsigned int l0_device_idx,
                                                         int l0_domain,
                                                         int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_DRIVER_ERRORS, M_ERROR_TYPE_CORRECTABLE);
    }

    double LevelZeroImp::ras_compute_errcount_correctable(unsigned int l0_device_idx,
                                                          int l0_domain,
                                                          int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_COMPUTE_ERRORS, M_ERROR_TYPE_CORRECTABLE);
    }

    double LevelZeroImp::ras_noncompute_errcount_correctable(unsigned int l0_device_idx,
                                                             int l0_domain,
                                                             int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_NON_COMPUTE_ERRORS, M_ERROR_TYPE_CORRECTABLE);
    }

    double LevelZeroImp::ras_cache_errcount_correctable(unsigned int l0_device_idx,
                                                        int l0_domain,
                                                        int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_CACHE_ERRORS, M_ERROR_TYPE_CORRECTABLE);
    }

    double LevelZeroImp::ras_display_errcount_correctable(unsigned int l0_device_idx,
                                                          int l0_domain,
                                                          int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_DISPLAY_ERRORS, M_ERROR_TYPE_CORRECTABLE);
    }

    double LevelZeroImp::ras_reset_count_uncorrectable(unsigned int l0_device_idx,
                                                       int l0_domain,
                                                       int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_RESET, M_ERROR_TYPE_UNCORRECTABLE);
    }

    double LevelZeroImp::ras_programming_errcount_uncorrectable(unsigned int l0_device_idx,
                                                                int l0_domain,
                                                                int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_PROGRAMMING_ERRORS, M_ERROR_TYPE_UNCORRECTABLE);
    }

    double LevelZeroImp::ras_driver_errcount_uncorrectable(unsigned int l0_device_idx,
                                                           int l0_domain,
                                                           int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_DRIVER_ERRORS, M_ERROR_TYPE_UNCORRECTABLE);
    }

    double LevelZeroImp::ras_compute_errcount_uncorrectable(unsigned int l0_device_idx,
                                                            int l0_domain,
                                                            int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_COMPUTE_ERRORS, M_ERROR_TYPE_UNCORRECTABLE);
    }

    double LevelZeroImp::ras_noncompute_errcount_uncorrectable(unsigned int l0_device_idx,
                                                               int l0_domain,
                                                               int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_NON_COMPUTE_ERRORS, M_ERROR_TYPE_UNCORRECTABLE);
    }

    double LevelZeroImp::ras_cache_errcount_uncorrectable(unsigned int l0_device_idx,
                                                          int l0_domain,
                                                          int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_CACHE_ERRORS, M_ERROR_TYPE_UNCORRECTABLE);
    }

    double LevelZeroImp::ras_display_errcount_uncorrectable(unsigned int l0_device_idx,
                                                            int l0_domain,
                                                            int l0_domain_idx) const
    {
        return ras_status_helper(l0_device_idx, l0_domain, l0_domain_idx,
                                 ZES_RAS_ERROR_CAT_DISPLAY_ERRORS, M_ERROR_TYPE_UNCORRECTABLE);
    }



    // RAS Helper function that extracts the errorset counters using the cached errorset handle
    uint64_t LevelZeroImp::ras_status_helper(unsigned int l0_device_idx,
                                             int l0_domain,
                                             int l0_domain_idx,
                                             zes_ras_error_cat_t errorcat,
                                             int errortype) const
    {
        zes_ras_state_t pState;
        int ras_idx = (l0_domain_idx * M_NUM_ERROR_TYPE) + errortype;
        zes_ras_handle_t handle = m_devices.at(l0_device_idx).subdevice.ras_domain.at(ras_idx);
        check_ze_result(zesRasGetState(handle, 0, &pState),
                        GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get RAS counters",
                        __LINE__);
	return pState.category[errorcat];
    }

    double LevelZeroImp::frequency_status(unsigned int l0_device_idx,
                                          int l0_domain, int l0_domain_idx) const
    {
        return convert_nan(frequency_status_helper(l0_device_idx, l0_domain, l0_domain_idx).actual);
    }

    double LevelZeroImp::frequency_efficient(unsigned int l0_device_idx,
                                             int l0_domain, int l0_domain_idx) const
    {
        return convert_nan(frequency_status_helper(l0_device_idx, l0_domain, l0_domain_idx).efficient);
    }

    uint32_t LevelZeroImp::frequency_throttle_reasons(unsigned int l0_device_idx,
                                                      int l0_domain, int l0_domain_idx) const
    {
        return frequency_status_helper(l0_device_idx, l0_domain, l0_domain_idx).throttle_reasons;
    }

    LevelZeroImp::m_frequency_s LevelZeroImp::frequency_status_helper(unsigned int l0_device_idx,
                                                                      int l0_domain, int l0_domain_idx) const
    {
        m_frequency_s result = {};
        zes_freq_handle_t handle = m_devices.at(l0_device_idx).
                                       subdevice.freq_domain.at(l0_domain).at(l0_domain_idx);
        zes_freq_state_t state = {};
        check_ze_result(zesFrequencyGetState(handle, &state),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get frequency state", __LINE__);

        result.voltage = state.currentVoltage;
        result.request = state.request;
        result.tdp = state.tdp;
        result.efficient = state.efficient;
        result.actual = state.actual;
        result.throttle_reasons = state.throttleReasons;
        return result;
    }

    double LevelZeroImp::frequency_min(unsigned int l0_device_idx,
                                       int l0_domain, int l0_domain_idx) const
    {
        return frequency_min_max(l0_device_idx, l0_domain, l0_domain_idx).first;
    }

    double LevelZeroImp::frequency_max(unsigned int l0_device_idx,
                                       int l0_domain, int l0_domain_idx) const
    {
        return frequency_min_max(l0_device_idx, l0_domain, l0_domain_idx).second;
    }

    std::pair<double, double> LevelZeroImp::frequency_min_max(unsigned int l0_device_idx,
                                                              int l0_domain, int l0_domain_idx) const
    {
        zes_freq_handle_t handle = m_devices.at(l0_device_idx).
                                       subdevice.freq_domain.at(l0_domain).at(l0_domain_idx);
        zes_freq_properties_t property = {};
        check_ze_result(zesFrequencyGetProperties(handle, &property),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get domain properties.", __LINE__);
        return {property.min, property.max};
    }

    std::vector<double> LevelZeroImp::frequency_supported(unsigned int l0_device_idx,
                                                          int l0_domain, int l0_domain_idx) const
    {
        zes_freq_handle_t handle = m_devices.at(l0_device_idx).
                                       subdevice.freq_domain.at(l0_domain).at(l0_domain_idx);
        uint32_t num_freq = 0;
        check_ze_result(zesFrequencyGetAvailableClocks(handle, &num_freq, nullptr),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get supported frequency count.", __LINE__);

        std::vector<double> result(num_freq);
        check_ze_result(zesFrequencyGetAvailableClocks(handle, &num_freq, result.data()),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get supported frequency list.", __LINE__);
        return result;
    }


    std::pair<double, double> LevelZeroImp::frequency_range(unsigned int l0_device_idx,
                                                            int l0_domain, int l0_domain_idx) const
    {
        zes_freq_handle_t handle = m_devices.at(l0_device_idx).
                                       subdevice.freq_domain.at(l0_domain).at(l0_domain_idx);
        zes_freq_range_t range = {};
        check_ze_result(zesFrequencyGetRange(handle, &range),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get frequency range.", __LINE__);
        return {range.min, range.max};
    }

    double LevelZeroImp::temperature_max(unsigned int l0_device_idx,
                                         int l0_domain, int l0_domain_idx) const
    {
        double result = NAN;
        zes_temp_handle_t handle = m_devices.at(l0_device_idx).
                                       subdevice.temp_domain_max.at(l0_domain).at(l0_domain_idx);
        check_ze_result(zesTemperatureGetState(handle, &result),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get temperature.", __LINE__);
        return result;
    }

    uint64_t LevelZeroImp::active_time_timestamp(unsigned int l0_device_idx,
                                                 int l0_domain, int l0_domain_idx) const
    {
        return m_devices.at(l0_device_idx).subdevice.cached_timestamp.at(l0_domain).at(l0_domain_idx);
    }

    uint64_t LevelZeroImp::active_time(unsigned int l0_device_idx,
                                       int l0_domain, int l0_domain_idx) const
    {
        return active_time_pair(l0_device_idx, l0_domain, l0_domain_idx).first;
    }

    std::pair<uint64_t,uint64_t> LevelZeroImp::active_time_pair(unsigned int l0_device_idx,
                                                                int l0_domain, int l0_domain_idx) const
    {
        uint64_t result_active = 0;
        uint64_t result_timestamp = 0;
        zes_engine_stats_t stats = {};
        zes_engine_handle_t handle = m_devices.at(l0_device_idx).
                                         subdevice.engine_domain.at(l0_domain).at(l0_domain_idx);
        check_ze_result(zesEngineGetActivity(handle, &stats),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get engine group activity.", __LINE__);
        result_active = stats.activeTime;
        result_timestamp = stats.timestamp;
        m_devices.at(l0_device_idx).
            subdevice.cached_timestamp.at(l0_domain).at(l0_domain_idx) = result_timestamp;
        return {result_active, result_timestamp};
    }

    uint64_t LevelZeroImp::energy_timestamp(int geopm_domain, unsigned int l0_device_idx,
                                            int l0_domain, int l0_domain_idx) const
    {
        uint64_t timestamp = 0;
        //PACKAGE
        if (geopm_domain == GEOPM_DOMAIN_GPU) {
            timestamp = m_devices.at(l0_device_idx).cached_energy_timestamp;
        }
        //TILE
        else if (geopm_domain == GEOPM_DOMAIN_GPU_CHIP) {
            timestamp = m_devices.at(l0_device_idx).subdevice.cached_energy_timestamp.at(l0_domain_idx);
        }
        return timestamp;
    }

    uint64_t LevelZeroImp::energy(int geopm_domain, unsigned int l0_device_idx,
                                  int l0_domain, int l0_domain_idx) const
    {
        return energy_pair(geopm_domain, l0_device_idx, l0_domain_idx).first;
    }

    std::pair<uint64_t,uint64_t> LevelZeroImp::energy_pair(int geopm_domain,
                                                           unsigned int l0_device_idx,
                                                           int l0_domain_idx) const
    {
        uint64_t result_energy = 0;
        uint64_t result_timestamp = 0;

        if (geopm_domain == GEOPM_DOMAIN_GPU &&
            power_domain_count(GEOPM_DOMAIN_GPU, l0_device_idx, M_DOMAIN_ALL) == 1) {
            //DEVICE LEVEL
            zes_pwr_handle_t handle = m_devices.at(l0_device_idx).power_domain;
            zes_power_energy_counter_t energy_counter = {};
            check_ze_result(zesPowerGetEnergyCounter(handle, &energy_counter),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get energy_counter values", __LINE__);
            result_energy += energy_counter.energy;
            result_timestamp += energy_counter.timestamp;
            m_devices.at(l0_device_idx).cached_energy_timestamp = result_timestamp;
        }
        else if (geopm_domain == GEOPM_DOMAIN_GPU_CHIP &&
                 power_domain_count(GEOPM_DOMAIN_GPU_CHIP, l0_device_idx, M_DOMAIN_ALL) >= l0_domain_idx) {
            //SUBDEVICE LEVEL
            zes_pwr_handle_t handle = m_devices.at(l0_device_idx).subdevice.power_domain.at(l0_domain_idx);
            zes_power_energy_counter_t energy_counter = {};
            check_ze_result(zesPowerGetEnergyCounter(handle, &energy_counter),
                            GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                            ": Sysman failed to get energy_counter values", __LINE__);
            result_energy += energy_counter.energy;
            result_timestamp += energy_counter.timestamp;
            m_devices.at(l0_device_idx).subdevice.cached_energy_timestamp.at(l0_domain_idx) = result_timestamp;
        }
        return {result_energy, result_timestamp};
    }

    int32_t LevelZeroImp::power_limit_tdp(unsigned int l0_device_idx) const
    {
        int32_t tdp = 0;
        if (m_devices.at(l0_device_idx).num_device_power_domain == 1) {
            tdp = power_limit_default(l0_device_idx).tdp;
        }
        return tdp;
    }

    int32_t LevelZeroImp::power_limit_min(unsigned int l0_device_idx) const
    {
        int32_t min = 0;
        if (m_devices.at(l0_device_idx).num_device_power_domain == 1) {
            min = power_limit_default(l0_device_idx).min;
        }
        return min;
    }

    int32_t LevelZeroImp::power_limit_max(unsigned int l0_device_idx) const
    {
        int32_t max = 0;
        if (m_devices.at(l0_device_idx).num_device_power_domain == 1) {
            max = power_limit_default(l0_device_idx).max;
        }
        return max;
    }

    LevelZeroImp::m_power_limit_s LevelZeroImp::power_limit_default(unsigned int l0_device_idx) const
    {
        zes_power_properties_t property = {};
        m_power_limit_s result_power = {};
        if (m_devices.at(l0_device_idx).num_device_power_domain == 1) {
            zes_pwr_handle_t handle = m_devices.at(l0_device_idx).power_domain;
            check_ze_result(zesPowerGetProperties(handle, &property),
                            GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                            ": Sysman failed to get domain power properties", __LINE__);
            result_power.tdp = property.defaultLimit;
            result_power.min = property.minLimit;
            result_power.max = property.maxLimit;
        }
        return result_power;
    }

    void LevelZeroImp::frequency_control(unsigned int l0_device_idx, int l0_domain,
                                         int l0_domain_idx, double range_min,
                                         double range_max) const
    {
        zes_freq_properties_t property = {};
        zes_freq_range_t range = {};
        range.min = range_min;
        range.max = range_max;
        zes_freq_handle_t handle = m_devices.at(l0_device_idx).
                                       subdevice.freq_domain.at(l0_domain).at(l0_domain_idx);
        check_ze_result(zesFrequencyGetProperties(handle, &property),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to get domain properties.", __LINE__);
        if (property.canControl == 0) {
            throw Exception("LevelZero::" + std::string(__func__) +
                            ": Attempted to set frequency " +
                            "for non controllable domain",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        check_ze_result(zesFrequencySetRange(handle, &range),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to set frequency.", __LINE__);
    }

    void LevelZeroImp::performance_factor_control(unsigned int l0_device_idx, int l0_domain,
                                                  int l0_domain_idx, double setting) const
    {
        zes_perf_handle_t handle = m_devices.at(l0_device_idx).
                                       subdevice.perf_domain.at(l0_domain).at(l0_domain_idx);
        check_ze_result(zesPerformanceFactorSetConfig(handle, setting),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to set performance factor values", __LINE__);
    }

    uint32_t LevelZeroImp::metric_update_rate(unsigned int l0_device_idx) const
    {
        return m_devices.at(l0_device_idx).metric_sampling_period_ns;
    }

    void LevelZeroImp::metric_update_rate_control(unsigned int l0_device_idx, uint32_t setting)
    {
        m_devices.at(l0_device_idx).metric_sampling_period_ns = setting;
    }

    void LevelZeroImp::check_ze_result(ze_result_t ze_result, int error,
                                       std::string message, int line) const
    {
        std::map<ze_result_t, std::string> error_mapping = {
              {ZE_RESULT_SUCCESS,
              "ZE_RESULT_SUCCESS"},
              {ZE_RESULT_NOT_READY,
               "ZE_RESULT_NOT_READY"},
              {ZE_RESULT_ERROR_UNINITIALIZED,
               "ZE_RESULT_ERROR_UNINITIALIZED"},
              {ZE_RESULT_ERROR_DEVICE_LOST,
               "ZE_RESULT_ERROR_DEVICE_LOST"},
              {ZE_RESULT_ERROR_INVALID_ARGUMENT,
               "ZE_RESULT_ERROR_INVALID_ARGUMENT"},
              {ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS,
               "ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS"},
              {ZE_RESULT_ERROR_NOT_AVAILABLE,
               "ZE_RESULT_ERROR_NOT_AVAILABLE"},
              {ZE_RESULT_ERROR_UNSUPPORTED_FEATURE,
               "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE"},
              {ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
               "ZE_RESULT_ERROR_INVALID_NULL_HANDLE"},
              {ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE,
               "ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE"},
              {ZE_RESULT_ERROR_INVALID_NULL_POINTER,
               "ZE_RESULT_ERROR_INVALID_NULL_POINTER"},
              {ZE_RESULT_ERROR_UNKNOWN,
               "ZE_RESULT_ERROR_UNKNOWN"},
              {ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY,
               "ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY"},
              {ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY,
               "ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY"},
              {ZE_RESULT_WARNING_DROPPED_DATA,
               "ZE_RESULT_WARNING_DROPPED_DATA"}
        };

        std::string error_string = std::to_string(ze_result);
        if (error_mapping.find(ze_result) != error_mapping.end()) {
            error_string = error_mapping.at(ze_result);
        }

        if (ze_result != ZE_RESULT_SUCCESS) {
            throw Exception(message + " Level Zero Error: " + error_string,
                            error, __FILE__, line);
        }
    }
}
