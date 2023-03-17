/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <string>
#include <iostream>
#include <map>
#include <stdlib.h>

#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"

#include "LevelZeroImp.hpp"

static void __attribute__((constructor)) geopm_levelzero_init(void)
{
    setenv("ZES_ENABLE_SYSMAN", "1", 1);
    setenv("ZET_ENABLE_METRICS", "1", 1);
}

namespace geopm
{

    LevelZero &levelzero()
    {
        static LevelZeroImp instance;
        return instance;
    }

    LevelZeroImp::LevelZeroImp()
        : m_num_gpu(0)
        , m_num_gpu_subdevice(0)
    {
        if (getenv("ZE_AFFINITY_MASK") != nullptr) {
            throw Exception("LevelZero: Cannot be used directly when ZE_AFFINITY_MASK environment "
                            "variable is set, must use service to access LevelZero in this case.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
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
                                 "Please check ZE_AFFINITY_MASK environment variable "
                                 "setting.  Forcing device to act as sub-device" << std::endl;
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

                        m_devices.push_back({
                            m_levelzero_driver.at(driver),
                            device_handle.at(device_idx),
                            property,
                            num_subdevice, //if there are no subdevices leave this as 0
                            subdevice_handle,
                            {}, //subdevice
                            0, //num_device_power_domain
                            {}, //power domain
                        });
                    }
#ifdef GEOPM_DEBUG
                    else {
                        std::cerr << "Warning: <geopm> LevelZero: Integrated "
                                     "GPU access is not currently supported by GEOPM.\n";
                    }
#endif
                }
#ifdef GEOPM_DEBUG
                else if (property.type == ZE_DEVICE_TYPE_CPU) {
                    // All CPU functionality is handled by GEOPM & MSR Safe currently
                    std::cerr << "Warning: <geopm> LevelZero: CPU access "
                                 "via LevelZero is not currently supported by GEOPM.\n";
                }
                else if (property.type == ZE_DEVICE_TYPE_FPGA) {
                    // FPGA functionality is not currently supported by GEOPM, but should not cause
                    // an error if the devices are present
                    std::cerr << "Warning: <geopm> LevelZero: Field Programmable "
                                 "Gate Arrays are not currently supported by GEOPM.\n";
                }
                else if (property.type == ZE_DEVICE_TYPE_MCA) {
                    // MCA functionality is not currently supported by GEOPM, but should not cause
                    // an error if the devices are present
                    std::cerr << "Warning: <geopm> LevelZero: Memory Copy GPUs "
                                 "are not currently supported by GEOPM.\n";
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
                if (m_devices.at(idx).m_num_subdevice != m_devices.at(idx - 1).m_num_subdevice) {
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
            metric_group_cache(gpu_idx);
       }
    }

    void LevelZeroImp::frequency_domain_cache(unsigned int device_idx) {
        //Cache frequency domains
        uint32_t num_domain = 0;
        ze_result_t ze_result = zesDeviceEnumFrequencyDomains(m_devices.at(device_idx).device_handle,
                                                              &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: Frequency domain detection is "
                         "not supported.\n";
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

                if (property.onSubdevice == 0 && m_devices.at(device_idx).m_num_subdevice != 0) {
#ifdef GEOPM_DEBUG
                    std::cerr << "Warning: <geopm> LevelZero: A device level "
                                 "frequency domain was found but is not currently supported.\n";
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

    void LevelZeroImp::power_domain_cache(unsigned int device_idx) {
        //Cache power domains
        uint32_t num_domain = 0;
        ze_result_t ze_result = zesDeviceEnumPowerDomains(m_devices.at(device_idx).device_handle,
                                                          &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: Power domain detection is "
                         "not supported.\n";
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

            if (num_subdevice_power_domain > m_devices.at(device_idx).m_num_subdevice) {
                throw Exception("LevelZero::" + std::string(__func__) +
                                ": Number of subdevice power domains (" +
                                std::to_string(num_device_power_domain) +
                                ") exceeds the number of subdevices (" +
                                std::to_string(m_devices.at(device_idx).m_num_subdevice) + ").",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            m_devices.at(device_idx).num_device_power_domain = num_device_power_domain;
            m_devices.at(device_idx).subdevice.num_subdevice_power_domain = num_subdevice_power_domain;
            m_devices.at(device_idx).subdevice.
                cached_energy_timestamp.resize(m_devices.at(device_idx).subdevice.power_domain.size());
        }

    }

    void LevelZeroImp::perf_domain_cache(unsigned int device_idx) {
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
                         "not supported.\n";
        }
#endif
    }

    void LevelZeroImp::engine_domain_cache(unsigned int device_idx) {
        //Cache engine domains
        uint32_t num_domain = 0;
        ze_result_t ze_result = zesDeviceEnumEngineGroups(m_devices.at(device_idx).device_handle,
                                                          &num_domain, nullptr);

        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: Engine domain detection is "
                         "not supported.\n";
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

                if (property.onSubdevice == 0 && m_devices.at(device_idx).m_num_subdevice != 0) {
#ifdef GEOPM_DEBUG
                    std::cerr << "Warning: <geopm> LevelZero: A device level "
                                 "engine domain was found but is not currently supported.\n";
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
                             "did not find ZES_ENGINE_GROUP_COMPUTE_ALL.\n";
            }
            if (num_domain != 0 &&
                m_devices.at(device_idx).
                    subdevice.engine_domain.at(geopm::LevelZero::M_DOMAIN_MEMORY).size() == 0) {
                std::cerr << "Warning: <geopm> LevelZero: Engine domain detection "
                             "did not find ZES_ENGINE_GROUP_COPY_ALL.\n";
            }
#endif
        }
    }

    void LevelZeroImp::temperature_domain_cache(unsigned int device_idx) {
        //Cache frequency domains
        uint32_t num_domain = 0;
        ze_result_t ze_result = zesDeviceEnumTemperatureSensors(m_devices.at(
                                                                device_idx).device_handle,
                                                                &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> LevelZero: Temperature domain detection is "
                         "not supported.\n";
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
                                 "temperature domain was found but is not currently supported.\n";
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
    void LevelZeroImp::metric_group_cache(unsigned int device_idx) {
        for (int subdevice_idx = 0;
         subdevice_idx < m_devices.at(device_idx).m_num_subdevice;
         ++subdevice_idx) {
            //assume false
            m_devices.at(device_idx).subdevice.metric_domain_cached.push_back(false);
            m_devices.at(device_idx).subdevice.metrics_initialized.push_back(false);

            char *zet_enable_metrics = getenv("ZET_ENABLE_METRICS");
            if (zet_enable_metrics == NULL || strcmp(zet_enable_metrics, "1") != 0) {
#ifdef GEOPM_DEBUG
                std::cerr << "Warning: <geopm>: ZET_ENABLE_METRICS not set to 1.  Skipping metric caching" <<
                             " for device " << std::to_string(device_idx) << " subdevice " <<
                             std::to_srting(subdevice_idx) << std::endl;
#endif
            }
            else {
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

                //TODO: save the relevant per GPU pointers
                ze_result = zetMetricGroupGet(m_devices.at(device_idx).subdevice_handle.at(subdevice_idx),
                                              &num_metric_group, metric_group_handle.data());

                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                                "LevelZero::" + std::string(__func__) +
                                ": LevelZero Metric Group handle acquisition failed",
                                __LINE__);

                //sampling period in nanoseconds
                m_devices.at(device_idx).metric_sampling_period = 2000000;

                m_devices.at(device_idx).subdevice.m_metric_data.push_back({});

                for (unsigned int metric_group_idx = 0; metric_group_idx < num_metric_group;
                     metric_group_idx++) {
                     zet_metric_group_properties_t metric_group_properties;
                     ze_result = zetMetricGroupGetProperties(metric_group_handle.at(metric_group_idx),
                                                 &metric_group_properties);
                     check_ze_result(ze_result,GEOPM_ERROR_RUNTIME,
                                     "LevelZero::" + std::string(__func__) +
                                     ": LevelZero Metric Group property acquisition failed",
                                     __LINE__);

                     //Confirm metric groups of interest exist
                     if (metric_group_properties.samplingType == ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED
                         && strcmp(metric_group_properties.name, "ComputeBasic") == 0) {

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
                        //m_devices.at(device_idx).subdevice.num_metric.at(subdevice_idx) = num_metric;
                        m_devices.at(device_idx).subdevice.num_metric.push_back(num_metric);

                        //Build metric map
                        for (unsigned int metric_idx = 0; metric_idx < num_metric; metric_idx++)
                        {

                            std::vector<zet_metric_handle_t> metric_handle(num_metric);
                            ze_result = zetMetricGet(metric_group_handle.at(metric_group_idx),
                                                     &num_metric, metric_handle.data());

                            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                                            "LevelZero::" + std::string(__func__) +
                                            ": LevelZero Metric handle acquisition failed",
                                            __LINE__);
                            zet_metric_properties_t metric_properties;
                            ze_result = zetMetricGetProperties(metric_handle.at(metric_idx), &metric_properties);
                            std::string metric_name (metric_properties.name);

                            if(m_devices.at(device_idx).subdevice.m_metric_data.at(subdevice_idx).count(metric_name) == 0) {
                                m_devices.at(device_idx).subdevice.m_metric_data.at(subdevice_idx)[metric_name] = {};
                            }
                        }
                    }
                }
                m_devices.at(device_idx).subdevice.metric_domain_cached.at(subdevice_idx) = true;
            }
        }
    }

    void LevelZeroImp::metric_destroy(unsigned int l0_device_idx, unsigned int l0_domain_idx)
    {
////        //std::cout << "ZET_DESTROY - gpu " << std::to_string(l0_device_idx) << std::endl;
//        ze_result_t ze_result;
//
//        if (m_devices.at(l0_device_idx).metrics_initialized) {
//            m_devices.at(l0_device_idx).metrics_initialized = false;
//
//            // Close metric streamer
//            ze_result = zetMetricStreamerClose(m_devices.at(l0_device_idx).metric_streamer);
//            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
//                            "LevelZero::" + std::string(__func__) +
//                            ": LevelZero Metric Streamer Close failed",
//                            __LINE__);
//
//            // Destroy Event
//            ze_result = zeEventDestroy(m_devices.at(l0_device_idx).event);
//            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
//                            "LevelZero::" + std::string(__func__) +
//                            ": LevelZero Metric Event Destroy failed",
//                            __LINE__);
//
//            // Destroy Event Pool
//            ze_result = zeEventPoolDestroy(m_devices.at(l0_device_idx).event_pool);
//            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
//                            "LevelZero::" + std::string(__func__) +
//                            ": LevelZero Metric Event Pool Destroy failed",
//                            __LINE__);
//
//            // Deactivate the device context
//            ze_context_handle_t context = m_devices.at(l0_device_idx).context;
//            ze_result = zetContextActivateMetricGroups(context,
//                                                       m_devices.at(l0_device_idx).device_handle,
//                                                       0, nullptr);
//            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
//                            "LevelZero::" + std::string(__func__) +
//                            ": LevelZero Metric Context Deactivation failed",
//                            __LINE__);
//        }
    }


    void LevelZeroImp::metric_init(unsigned int l0_device_idx, unsigned int l0_domain_idx)
    {
        ze_result_t ze_result;
        ze_context_handle_t context = m_devices.at(l0_device_idx).subdevice.context.at(l0_domain_idx);
        //ze_result = zetContextActivateMetricGroups(context, m_devices.at(l0_device_idx).device_handle,
        //                                           1, &m_devices.at(l0_device_idx).metric_group_handle);
        ze_result = zetContextActivateMetricGroups(context, m_devices.at(l0_device_idx).subdevice_handle.at(l0_domain_idx),
                                                   1, &m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx));

        ze_event_pool_handle_t event_pool_handle = nullptr;
        ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, 1};

        ze_result = zeEventPoolCreate(context, &event_pool_desc, 1,
                                      //&m_devices.at(l0_device_idx).device_handle,
                                      &m_devices.at(l0_device_idx).subdevice_handle.at(l0_domain_idx),
                                      &event_pool_handle);

        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": LevelZero Event Pool Create failed",
                        __LINE__);
        //m_devices.at(l0_device_idx).event_pool = event_pool_handle;
        m_devices.at(l0_device_idx).subdevice.event_pool.push_back(event_pool_handle);

        ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0,
                                      ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};

        ze_event_handle_t event = nullptr;
        ze_result = zeEventCreate(event_pool_handle, &event_desc, &event);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": LevelZero Event Create failed",
                        __LINE__);

        //m_devices.at(l0_device_idx).event = event;

//        std::cout << "ZET_INIT - pushing back event for gpu " << std::to_string(l0_device_idx) << std::endl;
        m_devices.at(l0_device_idx).subdevice.event.push_back(event);

        zet_metric_streamer_desc_t metric_streamer_desc = {
            ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC,
            nullptr,
            32768, /* reports to collect before notify */
            m_devices.at(l0_device_idx).metric_sampling_period};
        zet_metric_streamer_handle_t metric_streamer = nullptr;

        //ze_result = zetMetricStreamerOpen(context, m_devices.at(l0_device_idx).device_handle, m_devices.at(l0_device_idx).metric_group_handle, &metric_streamer_desc, event, &metric_streamer);
        ze_result = zetMetricStreamerOpen(context, m_devices.at(l0_device_idx).subdevice_handle.at(l0_domain_idx), m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx), &metric_streamer_desc, event, &metric_streamer);

        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": LevelZero Metric Streamer Open failed",
                        __LINE__);

        //m_devices.at(l0_device_idx).metric_streamer = metric_streamer;
        m_devices.at(l0_device_idx).subdevice.metric_streamer.push_back(metric_streamer);
    }

    void LevelZeroImp::metric_calc(unsigned int l0_device_idx, unsigned int l0_domain_idx,
                                   zet_metric_streamer_handle_t metric_streamer) const
    {
        ze_result_t ze_result;
        //////////////////////
        // Convert Raw Data //
        //////////////////////
        size_t data_size = 0;
        uint32_t report_count_req = 100;//UINT32_MAX; //100;
        ze_result = zetMetricStreamerReadData(metric_streamer, report_count_req, &data_size, nullptr );
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": LevelZero Read Data get size failed",
                        __LINE__);

        std::vector<uint8_t> data(data_size);
        zetMetricStreamerReadData(metric_streamer, report_count_req, &data_size, data.data());
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": LevelZero Read Data failed",
                        __LINE__);

        /////////////////////////////////////
        // Calculate & convert metric data //
        /////////////////////////////////////
        uint32_t num_metric_values = 0;
        uint32_t data_count = 0;
        zet_metric_group_calculation_type_t calculation_type = ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES;
        ze_result = zetMetricGroupCalculateMultipleMetricValuesExp(m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx), calculation_type, data_size, data.data(), &data_count, &num_metric_values, nullptr, nullptr);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                        "LevelZero::" + std::string(__func__) +
                        ": LevelZero Metric group calculate metric values to find num metrics failed",
                        __LINE__);

        std::vector<uint32_t> metric_count(data_count);
        std::vector<zet_typed_value_t> metric_values(num_metric_values);
        ze_result = zetMetricGroupCalculateMultipleMetricValuesExp(m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx), calculation_type, data_size, data.data(), &data_count, &num_metric_values, metric_count.data(), metric_values.data());
//        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
//                        "LevelZero::" + std::string(__func__) +
//                        ": LevelZero Metric group calculate metric values to calculate data failed",
//                        __LINE__);
//        //std::cout << "\tmetric group calculate metric values: " << std::to_string(num_metric_values) << std::endl;
//        ze_result = ZE_RESULT_ERROR_UNKNOWN;

        uint32_t num_metric = m_devices.at(l0_device_idx).subdevice.num_metric.at(l0_domain_idx);
        if (ze_result == ZE_RESULT_SUCCESS) {
            unsigned int num_reports = num_metric_values / num_metric;

            for (unsigned int metric_idx = 0; metric_idx < num_metric; metric_idx++)
            {
                //TODO: It is possible that simply parsing all the metrics is
                //      faster than the additional API calls to check the metric
                //      name.  This should be studied
                std::vector<zet_metric_handle_t> metric_handle(num_metric);
                ze_result = zetMetricGet(m_devices.at(l0_device_idx).subdevice.metric_group_handle.at(l0_domain_idx),
                                         &num_metric, metric_handle.data());
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                                "LevelZero::" + std::string(__func__) +
                                ": LevelZero Metric handle acquisition failed",
                                __LINE__);

                // process metrics
                zet_metric_properties_t metric_properties;
                ze_result = zetMetricGetProperties(metric_handle.at(metric_idx), &metric_properties);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME,
                                "LevelZero::" + std::string(__func__) +
                                ": LevelZero Metric property acquisition failed",
                                __LINE__);

                std::string metric_name (metric_properties.name);

                //TODO: check timing impact of only processing metrics supported by IOGroup
                if(metric_name.compare("XVE_ACTIVE") == 0 ||
                   metric_name.compare("XVE_STALL") == 0) {
                    for (unsigned int report_idx = 0; report_idx < num_reports; report_idx++)
                    {
                        //This is the actual gathering of the data
                        zet_typed_value_t data = metric_values.at(report_idx*num_metric + metric_idx);
                        double data_double = NAN;
                        switch( data.type )
                        {
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
                        //Clear cached values
                        if(report_idx == 0) {
                            //m_devices.at(l0_device_idx).m_metric_data.at(metric_name) = {};
                            m_devices.at(l0_device_idx).subdevice.m_metric_data.at(l0_domain_idx).at(metric_name) = {};
                        }
                        //m_devices.at(l0_device_idx).m_metric_data.at(metric_name).push_back(data_double);
                        m_devices.at(l0_device_idx).subdevice.m_metric_data.at(l0_domain_idx).at(metric_name).push_back(data_double);
                    }
                }
            }
        }
        else {
            m_devices.at(l0_device_idx).subdevice.m_metric_data.at(l0_domain_idx).at("XVE_ACTIVE") = {};
            m_devices.at(l0_device_idx).subdevice.m_metric_data.at(l0_domain_idx).at("XVE_STALL") = {};
        }
    }

    void LevelZeroImp::metric_read(unsigned int l0_device_idx, unsigned int l0_domain_idx)
    {
        if (m_devices.at(l0_device_idx).subdevice.metric_domain_cached.at(l0_domain_idx)) {
            if (!m_devices.at(l0_device_idx).subdevice.metrics_initialized.at(l0_domain_idx)) {
                metric_init(l0_device_idx, l0_domain_idx);
                m_devices.at(l0_device_idx).subdevice.metrics_initialized.at(l0_domain_idx) = true;
            }

            ze_result_t ze_host_result = zeEventHostSynchronize(m_devices.at(l0_device_idx).subdevice.event.at(l0_domain_idx), 0);

            if (ze_host_result != ZE_RESULT_NOT_READY) {
                metric_calc(l0_device_idx, l0_domain_idx,
                            m_devices.at(l0_device_idx).subdevice.metric_streamer.at(l0_domain_idx));
            }
            else {
                metric_read(l0_device_idx, l0_domain_idx); //TODO: possible infinite loop
            }
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

        if(m_devices.at(l0_device_idx).subdevice.m_metric_data.at(l0_domain_idx).count(metric_name) == 0) {
            throw Exception("LevelZero::" + std::string(__func__) +
                            ": No metric named " + metric_name  + " found." ,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        result = m_devices.at(l0_device_idx).subdevice.m_metric_data.at(l0_domain_idx).at(metric_name);
        return result;
    }

    int LevelZeroImp::num_gpu() const
    {
        //  TODO: this should be expanded to return all supported GPU types.
        //  Right now that is only GPUs
        return num_gpu(GEOPM_DOMAIN_GPU);
    }

    int LevelZeroImp::num_gpu(int domain_type) const
    {
        //  TODO: this should be expanded to return all supported GPU types.
        //  Right now that is only GPUs
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

    double LevelZeroImp::frequency_status(unsigned int l0_device_idx,
                                          int l0_domain, int l0_domain_idx) const
    {
        return frequency_status_helper(l0_device_idx, l0_domain, l0_domain_idx).actual;
    }

    double LevelZeroImp::frequency_efficient(unsigned int l0_device_idx,
                                             int l0_domain, int l0_domain_idx) const
    {
        return frequency_status_helper(l0_device_idx, l0_domain, l0_domain_idx).efficient;
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
                                                              int l0_domain,
                                                              int l0_domain_idx) const
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
                                                          int l0_domain,
                                                          int l0_domain_idx) const
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
                                                            int l0_domain,
                                                            int l0_domain_idx) const
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
                                                                int l0_domain,
                                                                int l0_domain_idx) const
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

    uint64_t LevelZeroImp::energy_timestamp(int geopm_domain,
                                            unsigned int l0_device_idx,
                                            int l0_domain,
                                            int l0_domain_idx) const
    {
        uint64_t timestamp = 0;
        //PACKAGE
        if(geopm_domain == GEOPM_DOMAIN_GPU) {
            timestamp = m_devices.at(l0_device_idx).cached_energy_timestamp;
        }
        //TILE
        else if(geopm_domain == GEOPM_DOMAIN_GPU_CHIP) {
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
                                 power_domain_count(GEOPM_DOMAIN_GPU_CHIP, l0_device_idx,
                                                    M_DOMAIN_ALL) >= l0_domain_idx) {
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
            return power_limit_default(l0_device_idx).min;
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

    void LevelZeroImp::performance_factor_control(unsigned int l0_device_idx,
                                                  int l0_domain,
                                                  int l0_domain_idx,
                                                  double setting) const
    {
        zes_perf_handle_t handle = m_devices.at(l0_device_idx).
                                       subdevice.perf_domain.at(l0_domain).at(l0_domain_idx);
        check_ze_result(zesPerformanceFactorSetConfig(handle, setting),
                        GEOPM_ERROR_RUNTIME, "LevelZero::" + std::string(__func__) +
                        ": Sysman failed to set performance factor values", __LINE__);
    }

    uint32_t LevelZeroImp::metric_update_rate(unsigned int l0_device_idx) const
    {
        return m_devices.at(l0_device_idx).metric_sampling_period;
    }

    void LevelZeroImp::metric_update_rate_control(unsigned int l0_device_idx, uint32_t setting)
    {
        m_devices.at(l0_device_idx).metric_sampling_period = setting;
    }

    uint32_t LevelZeroImp::metric_update_rate(unsigned int l0_device_idx) const
    {
        return m_devices.at(l0_device_idx).metric_sampling_period;
    }

    void LevelZeroImp::metric_update_rate_control(unsigned int l0_device_idx, uint32_t setting)
    {
        m_devices.at(l0_device_idx).metric_sampling_period = setting;
    }

    void LevelZeroImp::check_ze_result(ze_result_t ze_result, int error,
                                       std::string message, int line) const
    {
        std::map<ze_result_t, std::string> error_mapping =
            {{ZE_RESULT_SUCCESS,
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
