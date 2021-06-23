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
    {
        //TODO: change to a check and error if not enabled.  All ENV handling goes through environment class
        char *zes_enable_sysman = getenv("ZES_ENABLE_SYSMAN");
        if (zes_enable_sysman == NULL || strcmp(zes_enable_sysman, "1") != 0) {
            std::cout << "GEOPM Debug: ZES_ENABLE_SYSMAN not set to 1.  Forcing to 1" << std::endl;
            setenv("ZES_ENABLE_SYSMAN", "1", 1);
        }

        ze_result_t ze_result;
        //Initialize
        ze_result = zeInit(0);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                        ": LevelZero Driver failed to initialize.", __LINE__);
        // Discover drivers
        ze_result = zeDriverGet(&m_num_driver, nullptr);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                        ": LevelZero Driver enumeration failed.", __LINE__);
        m_levelzero_driver.resize(m_num_driver);
        ze_result = zeDriverGet(&m_num_driver, m_levelzero_driver.data());
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                        ": LevelZero Driver acquisition failed.", __LINE__);

        for (unsigned int driver = 0; driver < m_num_driver; driver++) {
            // Discover devices in a driver
            uint32_t num_device = 0;

            ze_result = zeDeviceGet(m_levelzero_driver.at(driver), &num_device, nullptr);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": LevelZero Device enumeration failed.", __LINE__);
            std::vector<zes_device_handle_t> device_handle(num_device);
            ze_result = zeDeviceGet(m_levelzero_driver.at(driver), &num_device, device_handle.data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": LevelZero Device acquisition failed.", __LINE__);

            for(unsigned int dev_idx = 0; dev_idx < num_device; ++dev_idx) {
                ze_device_properties_t property;
                ze_result = zeDeviceGetProperties(device_handle.at(dev_idx), &property);

#ifdef GEOPM_DEBUG
                uint32_t num_sub_device = 0;

                ze_result = zeDeviceGetSubDevices(device_handle.at(dev_idx), &num_sub_device, nullptr);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                                ": LevelZero Sub-Device enumeration failed.", __LINE__);
                std::cout << "Debug: levelZero sub-devices: " << std::to_string(num_sub_device) << std::endl;
#endif



                if (property.type == ZE_DEVICE_TYPE_GPU) {
                    if ((property.flags & ZE_DEVICE_PROPERTY_FLAG_INTEGRATED) == 0) {
                        m_sysman_device.push_back(device_handle.at(dev_idx));
                        ++m_num_board_gpu;
                    }
#ifdef GEOPM_DEBUG
                    else {
                        std::cerr << "Warning: <geopm> LevelZeroDevicePool: Integrated GPU access is not "
                                     "currently supported by GEOPM.\n";
                    }
#endif
                }
#ifdef GEOPM_DEBUG
                else if (property.type == ZE_DEVICE_TYPE_CPU) {
                    // All CPU functionality is handled by GEOPM & MSR Safe currently
                    std::cerr << "Warning: <geopm> LevelZeroDevicePool: CPU access via LevelZero is not "
                                 "currently supported by GEOPM.\n";
                }
                else if (property.type == ZE_DEVICE_TYPE_FPGA) {
                    // FPGA functionality is not currently supported by GEOPM, but should not cause
                    // an error if the devices are present
                    std::cerr << "Warning: <geopm> LevelZeroDevicePool: Field Programmable Gate Arrays are not "
                                 "currently supported by GEOPM.\n";
                }
                else if (property.type == ZE_DEVICE_TYPE_MCA) {
                    // MCA functionality is not currently supported by GEOPM, but should not cause
                    // an error if the devices are present
                    std::cerr << "Warning: <geopm> LevelZeroDevicePool: Memory Copy Accelerators are not "
                                 "currently supported by GEOPM.\n";
                }
#endif
            }
            m_num_device = m_num_board_gpu + m_num_integrated_gpu + m_num_fpga + m_num_mca;

            // This approach is far simpler, but does not allow for functioning in systems with multiple
            // accelerator types but unsupported accel types OR split accel indexes (i.e. BOARD_ACCELERATOR
            // vs PACKAGE_ACCELERATOR)
            //m_sysman_device.insert(m_sysman_device.end(), device_handle.begin(), device_handle.end());
            //m_num_device = m_num_device + num_device;
        }

        m_fan_domain.resize(m_num_device);
        m_temperature_domain.resize(m_num_device);
        m_fabric_domain.resize(m_num_device);
        m_mem_domain.resize(m_num_device);
        m_standby_domain.resize(m_num_device);
        m_freq_domain.resize(m_num_device);
        m_power_domain.resize(m_num_device);
        m_engine_domain.resize(m_num_device);
        m_perf_domain.resize(m_num_device);

        // TODO: When additional device types such as FPGA, MCA, and Integrated GPU are supported by GEOPM
        // This should be changed to a more general loop iterating over type and caching appropriately
        for (unsigned int board_gpu_idx = 0; board_gpu_idx < m_num_board_gpu; board_gpu_idx++) {
            ze_device_properties_t property;
            ze_result = zeDeviceGetProperties(m_sysman_device.at(board_gpu_idx), &property);

            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": failed to get device properties.", __LINE__);
            domain_cache(board_gpu_idx);
       }
    }

    void LevelZeroDevicePoolImp::domain_cache(unsigned int accel_idx) {
        ze_result_t ze_result;
        uint32_t num_domain = 0;

        //Cache frequency domains
        ze_result = zesDeviceEnumFrequencyDomains(m_sysman_device.at(accel_idx), &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroDevicePool: Frequency domain detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains.", __LINE__);
            m_freq_domain.at(accel_idx).resize(num_domain);

            ze_result = zesDeviceEnumFrequencyDomains(m_sysman_device.at(accel_idx), &num_domain, m_freq_domain.at(accel_idx).data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                        ": Sysman failed to get domain handles.", __LINE__);
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero frequency domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Cache power domains
        num_domain = 0;
        ze_result = zesDeviceEnumPowerDomains(m_sysman_device.at(accel_idx), &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroDevicePool: Power domain detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            m_power_domain.at(accel_idx).resize(num_domain);
            ze_result = zesDeviceEnumPowerDomains(m_sysman_device.at(accel_idx), &num_domain, m_power_domain.at(accel_idx).data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain handle(s).", __LINE__);
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero power domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Cache engine domains
        num_domain = 0;
        ze_result = zesDeviceEnumEngineGroups(m_sysman_device.at(accel_idx), &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroDevicePool: Engine domain detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            m_engine_domain.at(accel_idx).resize(num_domain);
            ze_result = zesDeviceEnumEngineGroups(m_sysman_device.at(accel_idx), &num_domain, m_engine_domain.at(accel_idx).data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero engine domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Cache performance domains
        num_domain = 0;
        ze_result = zesDeviceEnumPerformanceFactorDomains(m_sysman_device.at(accel_idx), &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroDevicePool: Performance Factor domain detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            m_perf_domain.at(accel_idx).resize(num_domain);
            ze_result = zesDeviceEnumPerformanceFactorDomains(m_sysman_device.at(accel_idx), &num_domain, m_perf_domain.at(accel_idx).data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero performance factor domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Standby domain signals
        num_domain = 0;
        ze_result = zesDeviceEnumStandbyDomains(m_sysman_device.at(accel_idx), &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroDevicePool: Standby domain detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            m_standby_domain.at(accel_idx).resize(num_domain);
            ze_result = zesDeviceEnumStandbyDomains(m_sysman_device.at(accel_idx), &num_domain, m_standby_domain.at(accel_idx).data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero standby domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Memory domain signals
        num_domain = 0;
        ze_result = zesDeviceEnumMemoryModules(m_sysman_device.at(accel_idx), &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroDevicePool: Memory module detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            m_mem_domain.at(accel_idx).resize(num_domain);
            ze_result = zesDeviceEnumMemoryModules(m_sysman_device.at(accel_idx), &num_domain, m_mem_domain.at(accel_idx).data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero memory domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Fabric domain signals
        num_domain = 0;
        ze_result = zesDeviceEnumFabricPorts(m_sysman_device.at(accel_idx), &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroDevicePool: Fabric port detection is not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            m_fabric_domain.at(accel_idx).resize(num_domain);
            ze_result = zesDeviceEnumFabricPorts(m_sysman_device.at(accel_idx), &num_domain, m_fabric_domain.at(accel_idx).data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero fabric domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Temperature domain signals
        num_domain = 0;
        ze_result = zesDeviceEnumTemperatureSensors(m_sysman_device.at(accel_idx), &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroDevicePool: Temperature sensor domain detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            m_temperature_domain.at(accel_idx).resize(num_domain);
            ze_result = zesDeviceEnumTemperatureSensors(m_sysman_device.at(accel_idx), &num_domain, m_temperature_domain.at(accel_idx).data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero temperature domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Fan domain signals
        num_domain = 0;
        ze_result = zesDeviceEnumFans(m_sysman_device.at(accel_idx), &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroDevicePool: Fan detection is not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            m_fan_domain.at(accel_idx).resize(num_domain);
            ze_result = zesDeviceEnumFans(m_sysman_device.at(accel_idx), &num_domain, m_fan_domain.at(accel_idx).data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero fan domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

    }

    LevelZeroDevicePoolImp::~LevelZeroDevicePoolImp()
    {
    }

    int LevelZeroDevicePoolImp::num_accelerator() const
    {
        return num_accelerator(ZE_DEVICE_TYPE_GPU);
    }

    int LevelZeroDevicePoolImp::num_accelerator(ze_device_type_t type) const
    {
        if (type == ZE_DEVICE_TYPE_GPU) {
            // TODO: add Integrated vs Board nuance
            return m_num_board_gpu;
        }
        else if (type == ZE_DEVICE_TYPE_CPU) {
            return M_NUM_CPU;
        }
        else if (type == ZE_DEVICE_TYPE_FPGA) {
            return m_num_fpga;
        }
        else if (type == ZE_DEVICE_TYPE_MCA) {
            return m_num_mca;
        }
        else {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) + ": accelerator type " +
                            std::to_string(type) + "  is unsupported", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void LevelZeroDevicePoolImp::check_accel_range(unsigned int accel_idx) const
    {
        if (accel_idx >= (unsigned int) num_accelerator()) {
            throw Exception("LevelZeroDevicePool::" + std::string(__func__) + ": accel_idx " +
                            std::to_string(accel_idx) + "  is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    double LevelZeroDevicePoolImp::frequency_status_gpu(unsigned int accel_idx) const
    {
        return std::get<4>(frequency_status(accel_idx, ZES_FREQ_DOMAIN_GPU));
    }

    uint64_t LevelZeroDevicePoolImp::frequency_status_throttle_reason_gpu(unsigned int accel_idx) const
    {
        return std::get<5>(frequency_status(accel_idx, ZES_FREQ_DOMAIN_GPU));
    }

    double LevelZeroDevicePoolImp::frequency_status_mem(unsigned int accel_idx) const
    {
        return std::get<4>(frequency_status(accel_idx, ZES_FREQ_DOMAIN_MEMORY));
    }

    //TODO: provide frequency: efficient (analogous to sticker?), tdp, and t hrottle
    //      see: https://spec.oneapi.com/level-zero/latest/sysman/api.html#_CPPv416zes_freq_state_t
    //TODO: add zesFrequencyGetAvailableClocks for getting all available frequencies?
    std::tuple<double, double, double, double, double, uint64_t> LevelZeroDevicePoolImp::frequency_status(unsigned int accel_idx, zes_freq_domain_t type) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_freq_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;
        double voltage = 0;
        double request  = 0;
        double tdp = 0;
        double efficient = 0;
        double actual = 0;
        uint64_t throttle_reasons = 0;
        double result_cnt = 0;

        for (auto handle : m_freq_domain.at(accel_idx)) {
            zes_freq_properties_t property;
            ze_result = zesFrequencyGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain properties.", __LINE__);

            if (type == property.type) {
                zes_freq_state_t state;
                ze_result = zesFrequencyGetState(handle, &state);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                                ": Sysman failed to get frequency state", __LINE__);
                voltage += state.currentVoltage;
                request += state.request;
                tdp += state.tdp;
                efficient += state.efficient;
                actual += state.actual;
                throttle_reasons |= state.throttleReasons;
                ++result_cnt; //TODO: change for official multi-tile support
            }
        }

        return std::make_tuple(voltage/result_cnt, request/result_cnt, tdp/result_cnt, efficient/result_cnt, actual/result_cnt, throttle_reasons);
    }


    double LevelZeroDevicePoolImp::frequency_min_gpu(unsigned int accel_idx) const
    {
        return frequency_min_max(accel_idx, ZES_FREQ_DOMAIN_GPU).first;
    }

    double LevelZeroDevicePoolImp::frequency_max_gpu(unsigned int accel_idx) const
    {
        return frequency_min_max(accel_idx, ZES_FREQ_DOMAIN_GPU).second;
    }

    double LevelZeroDevicePoolImp::frequency_min_mem(unsigned int accel_idx) const
    {
        return frequency_min_max(accel_idx, ZES_FREQ_DOMAIN_MEMORY).first;
    }

    double LevelZeroDevicePoolImp::frequency_max_mem(unsigned int accel_idx) const
    {
        return frequency_min_max(accel_idx, ZES_FREQ_DOMAIN_MEMORY).second;
    }

    std::pair<double, double> LevelZeroDevicePoolImp::frequency_min_max(unsigned int accel_idx, zes_freq_domain_t type) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_freq_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;
        double result_min = 0;
        double result_max = 0;
        double result_cnt = 0;

        for (auto handle : m_freq_domain.at(accel_idx)) {
            zes_freq_properties_t property;
            ze_result = zesFrequencyGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain properties.", __LINE__);
            if (type == property.type) {
                result_min += property.min;
                result_max += property.max;
                ++result_cnt; //TODO: change for official multi-tile support
            }
        }

        return {result_min/result_cnt, result_max/result_cnt};
    }

    double LevelZeroDevicePoolImp::frequency_range_min_gpu(unsigned int accel_idx) const
    {
        return frequency_min_max(accel_idx, ZES_FREQ_DOMAIN_GPU).first;
    }

    double LevelZeroDevicePoolImp::frequency_range_max_gpu(unsigned int accel_idx) const
    {
        return frequency_min_max(accel_idx, ZES_FREQ_DOMAIN_GPU).second;
    }

    std::pair<double, double> LevelZeroDevicePoolImp::frequency_range(unsigned int accel_idx, zes_freq_domain_t type) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_freq_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;
        zes_freq_range_t range;
        double result_min = 0;
        double result_max = 0;
        double result_cnt = 0;

        for (auto handle : m_freq_domain.at(accel_idx)) {
            zes_freq_properties_t property;
            ze_result = zesFrequencyGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain properties.", __LINE__);
            if (type == property.type) {
                ze_result = zesFrequencyGetRange(handle, &range);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                                ": Sysman failed to set frequency.", __LINE__);
                result_min += range.min;
                result_max += range.max;
                ++result_cnt; //TODO: change for official multi-tile support
            }
        }

        return {result_min/result_cnt, result_max/result_cnt};
    }

    uint64_t LevelZeroDevicePoolImp::frequency_throttle_time_gpu(unsigned int accel_idx) const
    {
        return frequency_throttle_time(accel_idx, ZES_FREQ_DOMAIN_GPU).first;
    }

    uint64_t LevelZeroDevicePoolImp::frequency_throttle_time_timestamp_gpu(unsigned int accel_idx) const
    {
        return frequency_throttle_time(accel_idx, ZES_FREQ_DOMAIN_GPU).second;
    }

    std::pair<uint64_t, uint64_t> LevelZeroDevicePoolImp::frequency_throttle_time(unsigned int accel_idx, zes_freq_domain_t type) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_power_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;
        uint64_t result_time = 0;
        uint64_t result_timestamp = 0;

        for (auto handle : m_freq_domain.at(accel_idx)) {
            zes_freq_properties_t property;
            ze_result = zesFrequencyGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain properties.", __LINE__);
            if (type == property.type) {
                zes_freq_throttle_time_t throttle_counter;
                ze_result = zesFrequencyGetThrottleTime(handle,  &throttle_counter);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                                ": Sysman failed to get throttle reasons.", __LINE__);

                result_time += throttle_counter.throttleTime;
                result_timestamp += throttle_counter.timestamp;
            }
        }
        return {result_time, result_timestamp};
    }

    double LevelZeroDevicePoolImp::temperature(unsigned int accel_idx) const
    {
        return temperature(accel_idx, ZES_TEMP_SENSORS_GLOBAL);
    }

    double LevelZeroDevicePoolImp::temperature_gpu(unsigned int accel_idx) const
    {
        return temperature(accel_idx, ZES_TEMP_SENSORS_GPU);
    }

    double LevelZeroDevicePoolImp::temperature_memory(unsigned int accel_idx) const
    {
        return temperature(accel_idx, ZES_TEMP_SENSORS_MEMORY);
    }

    double LevelZeroDevicePoolImp::temperature(int accel_idx, zes_temp_sensors_t sensor_type) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_engine_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;
        double result = 0;
        double temp = 0;
        double result_cnt = 0;
        bool domain_match = false;

        zes_temp_properties_t property;

        //for each engine group
        for (auto handle : m_temperature_domain.at(accel_idx)) {
            ze_result = zesTemperatureGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get temperature sensor properties.", __LINE__);

            if (sensor_type == property.type) {
                domain_match = true;
                ze_result = zesTemperatureGetState(handle, &temp);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                                ": Sysman failed to get temperature sensor reading.", __LINE__);
                result += temp;
                ++result_cnt; //TODO: change for official multi-tile support
            }
        }

        if (!domain_match) {
            result = NAN;
        }

        return result/result_cnt;
    }

    uint64_t LevelZeroDevicePoolImp::active_time_timestamp(unsigned int accel_idx) const
    {
        return active_time(accel_idx, ZES_ENGINE_GROUP_ALL).second;
    }

    uint64_t LevelZeroDevicePoolImp::active_time_timestamp_compute(unsigned int accel_idx) const
    {
        //TODO: transition to return active_time(accel_idx, ZES_ENGINE_GROUP_COMPUTE_ALL)
        return active_time(accel_idx, ZES_ENGINE_GROUP_COMPUTE_SINGLE).second;
    }

    uint64_t LevelZeroDevicePoolImp::active_time_timestamp_media_decode(unsigned int accel_idx) const
    {
        //TODO: transition to return active_time(accel_idx, ZES_ENGINE_MEDIA_ALL);
        return active_time(accel_idx, ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE).second;
    }

    uint64_t LevelZeroDevicePoolImp::active_time_timestamp_copy(unsigned int accel_idx) const
    {
        //TODO: transition to return active_time(accel_idx, ZES_ENGINE_GROUP_COPY_ALL);
        return active_time(accel_idx, ZES_ENGINE_GROUP_COPY_SINGLE).second;
    }

    uint64_t LevelZeroDevicePoolImp::active_time(unsigned int accel_idx) const
    {
        return active_time(accel_idx, ZES_ENGINE_GROUP_ALL).first;
    }

    uint64_t LevelZeroDevicePoolImp::active_time_compute(unsigned int accel_idx) const
    {
        //TODO: transition to return active_time(accel_idx, ZES_ENGINE_GROUP_COMPUTE_ALL)
        return active_time(accel_idx, ZES_ENGINE_GROUP_COMPUTE_SINGLE).first;
    }

    uint64_t LevelZeroDevicePoolImp::active_time_media_decode(unsigned int accel_idx) const
    {
        //TODO: transition to return active_time(accel_idx, ZES_ENGINE_MEDIA_ALL);
        return active_time(accel_idx, ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE).first;
    }

    uint64_t LevelZeroDevicePoolImp::active_time_copy(unsigned int accel_idx) const
    {
        //TODO: transition to return active_time(accel_idx, ZES_ENGINE_GROUP_COPY_ALL);
        return active_time(accel_idx, ZES_ENGINE_GROUP_COPY_SINGLE).first;
    }

    std::pair<uint64_t,uint64_t> LevelZeroDevicePoolImp::active_time(unsigned int accel_idx, zes_engine_group_t engine_type) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_engine_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;
        uint64_t result_active = 0;
        uint64_t result_timestamp = 0;

        zes_engine_properties_t property;
        zes_engine_stats_t stats;

        //for each engine group
        for (auto handle : m_engine_domain.at(accel_idx)) {
            ze_result = zesEngineGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get engine properties.", __LINE__);
            if (engine_type == property.type) {
                ze_result = zesEngineGetActivity(handle, &stats);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                                ": Sysman failed to get engine group activity.", __LINE__);
                result_active += stats.activeTime;
                result_timestamp += stats.timestamp;
            }
        }

        return {result_active, result_timestamp};
    }

    int32_t LevelZeroDevicePoolImp::power_limit_min(unsigned int accel_idx) const
    {
        return std::get<0>(power_limit_default(accel_idx));
    }

    int32_t LevelZeroDevicePoolImp::power_limit_max(unsigned int accel_idx) const
    {
        return std::get<1>(power_limit_default(accel_idx));
    }

    int32_t LevelZeroDevicePoolImp::power_limit_tdp(unsigned int accel_idx) const
    {
        return std::get<2>(power_limit_default(accel_idx));
    }

    std::tuple<int32_t, int32_t, int32_t> LevelZeroDevicePoolImp::power_limit_default(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_power_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;

        zes_power_properties_t property;
        uint64_t tdp = 0;
        uint64_t min_power_limit = 0;
        uint64_t max_power_limit = 0;

        for (auto handle : m_power_domain.at(accel_idx)) {
            ze_result = zesPowerGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain power properties", __LINE__);

            //For initial GEOPM support we're only providing device level power
            //finding non-subdevice domain.
            if (property.onSubdevice == 0) {
                tdp = property.defaultLimit;
                min_power_limit = property.minLimit;
                max_power_limit = property.maxLimit;
            }
        }

        return std::make_tuple(min_power_limit, max_power_limit, tdp);
    }

    int32_t LevelZeroDevicePoolImp::power_limit_peak_ac(unsigned int accel_idx) const
    {
        zes_power_peak_limit_t peak = {};
        peak = std::get<2>(power_limit(accel_idx));
        return peak.powerAC;
    }

    bool LevelZeroDevicePoolImp::power_limit_enabled_burst(unsigned int accel_idx) const
    {
        zes_power_burst_limit_t burst = {};
        burst = std::get<1>(power_limit(accel_idx));
        return (bool)burst.enabled;
    }

    int32_t LevelZeroDevicePoolImp::power_limit_burst(unsigned int accel_idx) const
    {
        zes_power_burst_limit_t burst = {};
        burst = std::get<1>(power_limit(accel_idx));
        return burst.power;
    }

    bool LevelZeroDevicePoolImp::power_limit_enabled_sustained(unsigned int accel_idx) const
    {
        zes_power_sustained_limit_t sustained = {};
        sustained = std::get<0>(power_limit(accel_idx));
        return (bool)sustained.enabled;
    }

    int32_t LevelZeroDevicePoolImp::power_limit_sustained(unsigned int accel_idx) const
    {
        zes_power_sustained_limit_t sustained = {};
        sustained = std::get<0>(power_limit(accel_idx));
        return sustained.power;
    }

    int32_t LevelZeroDevicePoolImp::power_limit_interval_sustained(unsigned int accel_idx) const
    {
        zes_power_sustained_limit_t sustained = {};
        sustained = std::get<0>(power_limit(accel_idx));
        return sustained.interval;
    }

    std::tuple<zes_power_sustained_limit_t, zes_power_burst_limit_t,
               zes_power_peak_limit_t> LevelZeroDevicePoolImp::power_limit(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_power_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;

        zes_power_sustained_limit_t sustained = {};
        zes_power_burst_limit_t burst = {};
        zes_power_peak_limit_t peak = {};

        for (auto handle : m_power_domain.at(accel_idx)) {
            zes_power_properties_t property;
            ze_result = zesPowerGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain power properties", __LINE__);

            //For initial GEOPM support we're only providing device level power
            //finding non-subdevice domain.
            if (property.onSubdevice == 0) {
                ze_result = zesPowerGetLimits(handle, &sustained, &burst, &peak);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get power limits", __LINE__);
            }
        }

        return std::make_tuple(sustained, burst, peak);
    }

    std::pair<uint64_t,uint64_t> LevelZeroDevicePoolImp::energy_pair(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_power_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;
        uint64_t result_energy = 0;
        uint64_t result_timestamp = 0;

        for (auto handle : m_power_domain.at(accel_idx)) {
            zes_power_energy_counter_t energy_counter;
            zes_power_properties_t property;
            ze_result = zesPowerGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain power properties", __LINE__);

            //For initial GEOPM support we're only providing device level power
            //finding non-subdevice domain.
            if (property.onSubdevice == 0) {
                ze_result = zesPowerGetEnergyCounter(handle, &energy_counter);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                                ": Sysman failed to get energy_counter values", __LINE__);
                result_energy += energy_counter.energy;
                result_timestamp += energy_counter.timestamp;
            }
        }
        return {result_energy, result_timestamp};
    }

    uint64_t LevelZeroDevicePoolImp::energy_timestamp(unsigned int accel_idx) const
    {
        //TODO: for performance testing we may want to cache either the timestamp or the energy reading
        return energy_pair(accel_idx).second;
    }

    uint64_t LevelZeroDevicePoolImp::energy(unsigned int accel_idx) const
    {
        //TODO: for performance testing we may want to cache either the timestamp or the energy reading
        return energy_pair(accel_idx).first;
    }

    void LevelZeroDevicePoolImp::check_domain_range(int size, const char *func, int line) const
    {
        if (size == 0) {
            throw Exception("LevelZeroDevicePool::" + std::string(func) + ": Not supported on this hardware",
                             GEOPM_ERROR_INVALID, __FILE__, line);
        }
    }

    double LevelZeroDevicePoolImp::performance_factor(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_perf_domain.at(accel_idx).size(), __func__, __LINE__);

        ze_result_t ze_result;
        double performance_factor;
        double result_cnt = 0;
        double result = 0;

        for (auto handle : m_perf_domain.at(accel_idx)) {
            zes_perf_properties_t property;
            ze_result = zesPerformanceFactorGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain performance factor properties",
                                                             __LINE__);

            // TODO: Additional splitting of performance factor into type based upon zes_engine_type_flags_t
            // may be required
            ze_result = zesPerformanceFactorGetConfig(handle, &performance_factor);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                          ": Sysman failed to get performance factor", __LINE__);
            result += performance_factor;
            ++result_cnt; //TODO: change for official multi-tile support
        }

        return result/result_cnt;
    }

    std::vector<uint32_t> LevelZeroDevicePoolImp::active_process_list(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        ze_result_t ze_result;

        uint32_t num_process = 0;
        std::vector<zes_process_state_t> processes = {};
        std::vector<unsigned int> result;

        ze_result = zesDeviceProcessesGetState(m_sysman_device.at(accel_idx), &num_process, nullptr);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                        ": Sysman failed to get running process count",
                                                        __LINE__);

        processes.resize(num_process);
        ze_result = zesDeviceProcessesGetState(m_sysman_device.at(accel_idx), &num_process, processes.data());
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                        ": Sysman failed to get running processes",
                                                        __LINE__);

        result.resize(num_process);
        for (uint32_t i = 0; i < num_process; i++) {
            result.push_back(processes.at(i).processId);
        }

        return result;
    }

    double LevelZeroDevicePoolImp::standby_mode(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_standby_domain.at(accel_idx).size(), __func__, __LINE__);
        zes_standby_promo_mode_t mode = {};
        double result = 0;
        double result_cnt = 0;

        ze_result_t ze_result;
        for (auto handle : m_standby_domain.at(accel_idx)) {
            zes_standby_properties_t property;
            ze_result = zesStandbyGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain standby properties",
                                                             __LINE__);

            ze_result = zesStandbyGetMode(handle, &mode);
            result += mode;
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                          ": Sysman failed to get standby mode", __LINE__);
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return result/result_cnt;
    }

    double LevelZeroDevicePoolImp::memory_allocated(unsigned int accel_idx) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_mem_domain.at(accel_idx).size(), __func__, __LINE__);
        double allocated_ratio = NAN;
        double result_cnt = 0;

        for (auto handle : m_mem_domain.at(accel_idx)) {
            ze_result_t ze_result;
            zes_mem_properties_t property;
            zes_mem_state_t state = {};

            ze_result = zesMemoryGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain memory properties",
                                                             __LINE__);
            //TODO: consider memory location (on device, in system)
            ze_result = zesMemoryGetState(handle, &state);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                          ": Sysman failed to get memory allocated", __LINE__);

            //TODO: Fix the assumption that there's only a single domain. For now we're assuming 1 or
            //      taking the last domain basically...could be HBM, DDR3/4/5, LPDDR, SRAM, GRF, ...
            allocated_ratio += (double)(state.size - state.free) / (double)state.size;
            ++result_cnt; //TODO: change for official multi-tile support
        }
        return allocated_ratio/result_cnt;
    }

    void LevelZeroDevicePoolImp::energy_threshold_control(unsigned int accel_idx, double setting) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_power_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;

        for (auto handle : m_power_domain.at(accel_idx)) {
            zes_power_properties_t property;
            ze_result = zesPowerGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain power properties", __LINE__);
            ze_result = zesPowerSetEnergyThreshold(handle, setting);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                        ": Sysman failed to set domain energy threshold", __LINE__);
        }
    }


    void LevelZeroDevicePoolImp::frequency_control_gpu(unsigned int accel_idx, double setting) const
    {
        frequency_control(accel_idx, setting, setting, ZES_FREQ_DOMAIN_GPU);
    }

    void LevelZeroDevicePoolImp::frequency_control(unsigned int accel_idx, double min_freq, double max_freq, zes_freq_domain_t type) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_freq_domain.at(accel_idx).size(), __func__, __LINE__);
        ze_result_t ze_result;

        zes_freq_properties_t property;
        zes_freq_range_t range;
        range.min = min_freq;
        range.max = max_freq;
        //zes_freq_range_t range_check;

        for (auto handle : m_freq_domain.at(accel_idx)) {
            //zes_freq_properties_t properts = {ZES_STRUCTURE_TYPE_FREQ_PROPERTIES,
            ze_result = zesFrequencyGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain properties.", __LINE__);

            if (property.type == type) {
                if (property.canControl == 0) {
                    throw Exception("LevelZeroDevicePool::" + std::string(__func__) + ": Attempted to set frequency " +
                                    "for non controllable domain",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                ze_result = zesFrequencySetRange(handle, &range);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                                ": Sysman failed to set frequency.", __LINE__);
            }
        }
    }

    void LevelZeroDevicePoolImp::standby_mode_control(unsigned int accel_idx, double setting) const
    {
        check_accel_range(accel_idx);
        check_domain_range(m_standby_domain.at(accel_idx).size(), __func__, __LINE__);

        ze_result_t ze_result;
        for (auto handle : m_standby_domain.at(accel_idx)) {
            zes_standby_properties_t property;
            ze_result = zesStandbyGetProperties(handle, &property);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                            ": Sysman failed to get domain standby properties",
                                                             __LINE__);

            ze_result = zesStandbySetMode(handle, (zes_standby_promo_mode_t)setting);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                          ": Sysman failed to set standby mode", __LINE__);
        }
    }

    void LevelZeroDevicePoolImp::check_ze_result(ze_result_t ze_result, int error, std::string message, int line) const
    {
        if(ze_result != ZE_RESULT_SUCCESS) {
            std::string error_string = "Unknown ze_result_t value";

            if (ze_result == ZE_RESULT_SUCCESS) {
                error_string = "ZE_RESULT_SUCCESS";
            }
            else if (ze_result == ZE_RESULT_NOT_READY) {
                error_string = "ZE_RESULT_NOT_READY";
            }
            else if (ze_result == ZE_RESULT_ERROR_UNINITIALIZED) {
                error_string = "ZE_RESULT_ERROR_UNINITIALIZED";
            }
            else if (ze_result == ZE_RESULT_ERROR_DEVICE_LOST) {
                error_string = "ZE_RESULT_ERROR_DEVICE_LOST";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_ARGUMENT) {
                error_string = "ZE_RESULT_ERROR_INVALID_ARGUMENT";
            }
            else if (ze_result == ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY) {
                error_string = "ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY";
            }
            else if (ze_result == ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY) {
                error_string = "ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY";
            }
            else if (ze_result == ZE_RESULT_ERROR_MODULE_BUILD_FAILURE) {
                error_string = "ZE_RESULT_ERROR_MODULE_BUILD_FAILURE";
            }
            else if (ze_result == ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS) {
                error_string = "ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS";
            }
            else if (ze_result == ZE_RESULT_ERROR_NOT_AVAILABLE) {
                error_string = "ZE_RESULT_ERROR_NOT_AVAILABLE";
            }
            else if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_VERSION) {
                error_string = "ZE_RESULT_ERROR_UNSUPPORTED_VERSION";
            }
            else if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
                error_string = "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_NULL_HANDLE) {
                error_string = "ZE_RESULT_ERROR_INVALID_NULL_HANDLE";
            }
            else if (ze_result == ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE) {
                error_string = "ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_NULL_POINTER) {
                error_string = "ZE_RESULT_ERROR_INVALID_NULL_POINTER";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_SIZE) {
                error_string = "ZE_RESULT_ERROR_INVALID_SIZE";
            }
            else if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_SIZE) {
                error_string = "ZE_RESULT_ERROR_UNSUPPORTED_SIZE";
            }
            else if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT) {
                error_string = "ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT) {
                error_string = "ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_ENUMERATION) {
                error_string = "ZE_RESULT_ERROR_INVALID_ENUMERATION";
            }
            else if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION) {
                error_string = "ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION";
            }
            else if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT) {
                error_string = "ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_NATIVE_BINARY) {
                error_string = "ZE_RESULT_ERROR_INVALID_NATIVE_BINARY";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_GLOBAL_NAME) {
                error_string = "ZE_RESULT_ERROR_INVALID_GLOBAL_NAME";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_KERNEL_NAME) {
                error_string = "ZE_RESULT_ERROR_INVALID_KERNEL_NAME";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_FUNCTION_NAME) {
                error_string = "ZE_RESULT_ERROR_INVALID_FUNCTION_NAME";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION) {
                error_string = "ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION) {
                error_string = "ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX) {
                error_string = "ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE) {
                error_string = "ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE) {
                error_string = "ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE";
            }
            else if (ze_result == ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE) {
                error_string = "ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE";
            }
            else if (ze_result == ZE_RESULT_ERROR_OVERLAPPING_REGIONS) {
                error_string = "ZE_RESULT_ERROR_OVERLAPPING_REGIONS";
            }
            else if (ze_result == ZE_RESULT_ERROR_UNKNOWN) {
                error_string = "ZE_RESULT_ERROR_UNKNOWN";
            }

            throw Exception(message + "  Error: " + error_string, error, __FILE__, line);
        }
    }



}
