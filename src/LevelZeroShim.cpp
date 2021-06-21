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

#include "LevelZeroShimImp.hpp"

namespace geopm
{
    const LevelZeroShim &levelzero_shim(const int num_cpu)
    {
        static LevelZeroShimImp instance(num_cpu);
        return instance;
    }

    LevelZeroShimImp::LevelZeroShimImp(const int num_cpu)
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
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                        ": LevelZero Driver failed to initialize.", __LINE__);
        // Discover drivers
        ze_result = zeDriverGet(&m_num_driver, nullptr);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                        ": LevelZero Driver enumeration failed.", __LINE__);
        m_levelzero_driver.resize(m_num_driver);
        ze_result = zeDriverGet(&m_num_driver, m_levelzero_driver.data());
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                        ": LevelZero Driver acquisition failed.", __LINE__);

        for (unsigned int driver = 0; driver < m_num_driver; driver++) {
            // Discover devices in a driver
            uint32_t num_device = 0;

            ze_result = zeDeviceGet(m_levelzero_driver.at(driver), &num_device, nullptr);
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                            ": LevelZero Device enumeration failed.", __LINE__);
            std::vector<zes_device_handle_t> device_handle(num_device);
            ze_result = zeDeviceGet(m_levelzero_driver.at(driver), &num_device, device_handle.data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                            ": LevelZero Device acquisition failed.", __LINE__);


            for(unsigned int accel_idx = 0; accel_idx < num_device; ++accel_idx) {
                ze_device_properties_t property;
                ze_result = zeDeviceGetProperties(device_handle.at(accel_idx), &property);

                uint32_t num_subdevice = 0;

                ze_result = zeDeviceGetSubDevices(device_handle.at(accel_idx), &num_subdevice, nullptr);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                                ": LevelZero Sub-Device enumeration failed.", __LINE__);

                std::vector<zes_device_handle_t> subdevice_handle(num_subdevice);
                ze_result = zeDeviceGetSubDevices(device_handle.at(accel_idx), &num_subdevice, subdevice_handle.data());
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                                ": LevelZero Sub-Device acquisition failed.", __LINE__);

#ifdef GEOPM_DEBUG
                std::cout << "Debug: levelZero sub-devices: " << std::to_string(num_subdevice) << std::endl;
#endif

                if (property.type == ZE_DEVICE_TYPE_GPU) {
                    if ((property.flags & ZE_DEVICE_PROPERTY_FLAG_INTEGRATED) == 0) {
                        ++m_num_board_gpu;

                        //NOTE: We're only supporting Board GPUs to start with
                        m_devices.push_back({
                            device_handle.at(accel_idx),
                            property,
                            num_subdevice,
                            subdevice_handle,
                            {}, //subdevice
                            {}, //power domain
                            {}  //temp domain
                        });
                    }
#ifdef GEOPM_DEBUG
                    else {
                        std::cerr << "Warning: <geopm> LevelZeroShim: Integrated GPU access is not "
                                     "currently supported by GEOPM.\n";
                    }
#endif
                }
#ifdef GEOPM_DEBUG
                else if (property.type == ZE_DEVICE_TYPE_CPU) {
                    // All CPU functionality is handled by GEOPM & MSR Safe currently
                    std::cerr << "Warning: <geopm> LevelZeroShim: CPU access via LevelZero is not "
                                 "currently supported by GEOPM.\n";
                }
                else if (property.type == ZE_DEVICE_TYPE_FPGA) {
                    // FPGA functionality is not currently supported by GEOPM, but should not cause
                    // an error if the devices are present
                    std::cerr << "Warning: <geopm> LevelZeroShim: Field Programmable Gate Arrays are not "
                                 "currently supported by GEOPM.\n";
                }
                else if (property.type == ZE_DEVICE_TYPE_MCA) {
                    // MCA functionality is not currently supported by GEOPM, but should not cause
                    // an error if the devices are present
                    std::cerr << "Warning: <geopm> LevelZeroShim: Memory Copy Accelerators are not "
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

        // TODO: When additional device types such as FPGA, MCA, and Integrated GPU are supported by GEOPM
        // This should be changed to a more general loop iterating over type and caching appropriately
        for (unsigned int board_gpu_idx = 0; board_gpu_idx < m_num_board_gpu; board_gpu_idx++) {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                            ": failed to get device properties.", __LINE__);
            domain_cache(board_gpu_idx);
       }
    }

    void LevelZeroShimImp::domain_cache(unsigned int accel_idx) {
        ze_result_t ze_result;
        uint32_t num_domain = 0;

        //Cache frequency domains
        ze_result = zesDeviceEnumFrequencyDomains(m_devices.at(accel_idx).device_handle, &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroShim: Frequency domain detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains.", __LINE__);
            //make temp var
            std::vector<zes_freq_handle_t> freq_domain;
            freq_domain.resize(num_domain);

            ze_result = zesDeviceEnumFrequencyDomains(m_devices.at(accel_idx).device_handle, &num_domain, freq_domain.data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                        ": Sysman failed to get domain handles.", __LINE__);

            m_devices.at(accel_idx).subdevice.m_freq_domain.resize(GEOPM_LEVELZERO_DOMAIN_SIZE);

            for (auto handle : freq_domain) {
                zes_freq_properties_t property;
                ze_result = zesFrequencyGetProperties(handle, &property);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                                ": Sysman failed to get domain properties.", __LINE__);

                if (property.type == ZES_FREQ_DOMAIN_GPU) {
                    m_devices.at(accel_idx).subdevice.m_freq_domain.at(GEOPM_LEVELZERO_DOMAIN_COMPUTE).push_back(handle);
                }
                else if (property.type == ZES_FREQ_DOMAIN_MEMORY) {
                    m_devices.at(accel_idx).subdevice.m_freq_domain.at(GEOPM_LEVELZERO_DOMAIN_MEMORY).push_back(handle);
                }
            }
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero frequency domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Cache power domains
        num_domain = 0;
        ze_result = zesDeviceEnumPowerDomains(m_devices.at(accel_idx).device_handle, &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroShim: Power domain detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            std::vector<zes_pwr_handle_t> power_domain;
            power_domain.resize(num_domain);

            ze_result = zesDeviceEnumPowerDomains(m_devices.at(accel_idx).device_handle, &num_domain, power_domain.data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                            ": Sysman failed to get domain handle(s).", __LINE__);

            for (auto handle : power_domain) {
                zes_power_properties_t property;
                ze_result = zesPowerGetProperties(handle, &property);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                                ": Sysman failed to get domain power properties", __LINE__);

                //For initial GEOPM support we're only providing device level power, but are tracking sub-device
                //for future use
                //finding non-subdevice domain.
                if (property.onSubdevice == 0) {
                    m_devices.at(accel_idx).m_power_domain.push_back(handle);
                }
                else {
                    m_devices.at(accel_idx).subdevice.m_power_domain.push_back(handle);
                }
            }
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero power domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

        //Cache engine domains
        num_domain = 0;
        ze_result = zesDeviceEnumEngineGroups(m_devices.at(accel_idx).device_handle, &num_domain, nullptr);
        if (ze_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            std::cerr << "Warning: <geopm> LevelZeroShim: Engine domain detection is "
                         "not supported.\n";
        }
        else {
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);
            std::vector<zes_engine_handle_t> engine_domain;
            engine_domain.resize(num_domain);

            ze_result = zesDeviceEnumEngineGroups(m_devices.at(accel_idx).device_handle, &num_domain, engine_domain.data());
            check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                            ": Sysman failed to get number of domains", __LINE__);

            m_devices.at(accel_idx).subdevice.m_engine_domain.resize(GEOPM_LEVELZERO_DOMAIN_SIZE);

            for (auto handle : engine_domain) {
                zes_engine_properties_t property;
                ze_result = zesEngineGetProperties(handle, &property);
                check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                                ": Sysman failed to get domain engine properties", __LINE__);
                if (property.type == ZES_ENGINE_GROUP_ALL) {
                    m_devices.at(accel_idx).subdevice.m_engine_domain.at(GEOPM_LEVELZERO_DOMAIN_ALL).push_back(handle);
                }
                //TODO: change to ZES_ENGINE_GROUP_COMPUTE_ALL?  Or check for COMPUTE_ALL and then do _SINGLE
                else if (property.type == ZES_ENGINE_GROUP_COMPUTE_SINGLE) {
                    m_devices.at(accel_idx).subdevice.m_engine_domain.at(GEOPM_LEVELZERO_DOMAIN_COMPUTE).push_back(handle);
                }
                //TODO: change to ZES_ENGINE_GROUP_COPY_ALL?  Or check for COMPUTE_ALL and then do _SINGLE
                else if (property.type == ZES_ENGINE_GROUP_COPY_SINGLE) {
                    m_devices.at(accel_idx).subdevice.m_engine_domain.at(GEOPM_LEVELZERO_DOMAIN_MEMORY).push_back(handle);
                }
            }
#ifdef GEOPM_DEBUG
            std::cout << "Debug: levelZero engine domains: " << std::to_string(num_domain) << std::endl;
#endif
        }

    }

    LevelZeroShimImp::~LevelZeroShimImp()
    {
    }

    int LevelZeroShimImp::num_accelerator() const
    {
        return num_accelerator(ZE_DEVICE_TYPE_GPU);
    }

    int LevelZeroShimImp::num_accelerator(ze_device_type_t type) const
    {
        //switch/case
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
            throw Exception("LevelZeroShim::" + std::string(__func__) + ": accelerator type " +
                            std::to_string(type) + "  is unsupported", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    int LevelZeroShimImp::frequency_domain_count(unsigned int accel_idx, geopm_levelzero_domain_e domain) const
    {
        return m_devices.at(accel_idx).subdevice.m_freq_domain.at(domain).size();
    }

    int LevelZeroShimImp::engine_domain_count(unsigned int accel_idx, geopm_levelzero_domain_e domain) const
    {
        return m_devices.at(accel_idx).subdevice.m_engine_domain.at(domain).size();
    }

    int LevelZeroShimImp::energy_domain_count_device(unsigned int accel_idx) const
    {
        return m_devices.at(accel_idx).m_power_domain.size();
    }

    int LevelZeroShimImp::energy_domain_count_subdevice(unsigned int accel_idx, int domain_idx) const
    {
        return m_devices.at(accel_idx).subdevice.m_power_domain.size();
    }

    double LevelZeroShimImp::frequency_status(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    {
        return frequency_status_shim(accel_idx, domain, domain_idx).actual;
    }

    //double LevelZeroShimImp::frequency_tdp(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    //{
    //    return frequency_status_shim(accel_idx, domain, domain_idx).tdp;
    //}

    //double LevelZeroShimImp::frequency_efficient(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    //{
    //    return frequency_status_shim(accel_idx, domain, domain_idx).efficient;
    //}

    //uint64_t LevelZeroShimImp::frequency_throttle_reasons(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    //{
    //    return frequency_status_shim(accel_idx, domain, domain_idx).throttle_reasons;
    //}

    //std::tuple<double, double, double, double, double, uint64_t> LevelZeroShimImp::frequency_status_shim(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    LevelZeroShimImp::frequency_s LevelZeroShimImp::frequency_status_shim(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    {
        ze_result_t ze_result;
        zes_freq_domain_t type;

        frequency_s result;

        zes_freq_handle_t handle = m_devices.at(accel_idx).subdevice.m_freq_domain.at(domain).at(domain_idx);
        zes_freq_state_t state;
        ze_result = zesFrequencyGetState(handle, &state);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                            ": Sysman failed to get frequency state", __LINE__);

        result.voltage = state.currentVoltage;
        result.request = state.request;
        result.tdp = state.tdp;
        result.efficient = state.efficient;
        result.actual = state.actual;
        result.throttle_reasons = state.throttleReasons;

        return result;
    }

    double LevelZeroShimImp::frequency_min(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    {
        return frequency_min_max(accel_idx, domain, domain_idx).first;
    }

    double LevelZeroShimImp::frequency_max(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    {
        return frequency_min_max(accel_idx, domain, domain_idx).second;
    }

    std::pair<double, double> LevelZeroShimImp::frequency_min_max(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    {
        ze_result_t ze_result;
        double result_min = 0;
        double result_max = 0;

        zes_freq_handle_t handle = m_devices.at(accel_idx).subdevice.m_freq_domain.at(domain).at(domain_idx);
        zes_freq_properties_t property;
        ze_result = zesFrequencyGetProperties(handle, &property);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                        ": Sysman failed to get domain properties.", __LINE__);
        result_min += property.min;
        result_max += property.max;

        return {result_min, result_max};
    }

    uint64_t LevelZeroShimImp::active_time_timestamp(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    {
        return active_time_pair(accel_idx, domain, domain_idx).second;
    }

    uint64_t LevelZeroShimImp::active_time(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    {
        return active_time_pair(accel_idx, domain, domain_idx).first;
    }

    std::pair<uint64_t,uint64_t> LevelZeroShimImp::active_time_pair(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx) const
    {
        ze_result_t ze_result;
        uint64_t result_active = 0;
        uint64_t result_timestamp = 0;

        zes_engine_properties_t property;
        zes_engine_stats_t stats;

        zes_engine_handle_t handle = m_devices.at(accel_idx).subdevice.m_engine_domain.at(domain).at(domain_idx);
        ze_result = zesEngineGetActivity(handle, &stats);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                        ": Sysman failed to get engine group activity.", __LINE__);
        result_active += stats.activeTime;
        result_timestamp += stats.timestamp;

        return {result_active, result_timestamp};
    }

    std::pair<uint64_t,uint64_t> LevelZeroShimImp::energy_pair(unsigned int accel_idx, int domain_idx) const
    {
        ze_result_t ze_result;
        uint64_t result_energy = 0;
        uint64_t result_timestamp = 0;

        zes_pwr_handle_t handle = m_devices.at(accel_idx).m_power_domain.at(domain_idx);
        //For initial GEOPM support we're only providing device level power.  Eventually we'll want to
        //choose between these two
        //zes_power_handle_t subdevice_handle = m_devices.at(accel_idx).subdevice.m_power_domain.at(domain).at(domain_idx);

        zes_power_energy_counter_t energy_counter;
        ze_result = zesPowerGetEnergyCounter(handle, &energy_counter);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                        ": Sysman failed to get energy_counter values", __LINE__);
        result_energy += energy_counter.energy;
        result_timestamp += energy_counter.timestamp;
        return {result_energy, result_timestamp};
    }

    uint64_t LevelZeroShimImp::energy_timestamp(unsigned int accel_idx, int domain_idx) const
    {
        //TODO: for performance testing we may want to cache either the timestamp or the energy reading
        return energy_pair(accel_idx, domain_idx).second;
    }

    uint64_t LevelZeroShimImp::energy(unsigned int accel_idx, int domain_idx) const
    {
        //TODO: for performance testing we may want to cache either the timestamp or the energy reading
        return energy_pair(accel_idx, domain_idx).first;
    }

    int32_t LevelZeroShimImp::power_limit_tdp(unsigned int accel_idx, int domain_idx) const
    {
        //return std::get<2>(power_limit_default(accel_idx, domain_idx));
        return power_limit_default(accel_idx, domain_idx).tdp;
    }

    int32_t LevelZeroShimImp::power_limit_min(unsigned int accel_idx, int domain_idx) const
    {
        //return std::get<0>(power_limit_default(accel_idx, domain_idx));
        return power_limit_default(accel_idx, domain_idx).min;
    }

    int32_t LevelZeroShimImp::power_limit_max(unsigned int accel_idx, int domain_idx) const
    {
        //return std::get<1>(power_limit_default(accel_idx, domain_idx));
        return power_limit_default(accel_idx, domain_idx).max;
    }

    //std::tuple<int32_t, int32_t, int32_t> LevelZeroShimImp::power_limit_default(unsigned int accel_idx, int domain_idx) const
    LevelZeroShimImp::power_limit_s LevelZeroShimImp::power_limit_default(unsigned int accel_idx, int domain_idx) const
    {
        ze_result_t ze_result;

        zes_power_properties_t property;
        uint64_t tdp = 0;
        uint64_t min_power_limit = 0;
        uint64_t max_power_limit = 0;
        power_limit_s result_power;

        //For initial GEOPM support we're only providing device level power.  Eventually we'll want to
        //choose between these two
        //zes_power_handle_t subdevice_handle = m_devices.at(accel_idx).subdevice.m_power_domain.at(domain).at(domain_idx);
        zes_pwr_handle_t handle = m_devices.at(accel_idx).m_power_domain.at(domain_idx);

        //TODO: these could cached at init time
        ze_result = zesPowerGetProperties(handle, &property);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroDevicePool::" + std::string(__func__) +
                                                        ": Sysman failed to get domain power properties", __LINE__);
        tdp = property.defaultLimit;
        min_power_limit = property.minLimit;
        max_power_limit = property.maxLimit;
        result_power.tdp = property.defaultLimit;
        result_power.min = property.minLimit;
        result_power.max = property.maxLimit;

        //return std::make_tuple(min_power_limit, max_power_limit, tdp);
        return result_power;
    }

    void LevelZeroShimImp::frequency_control(unsigned int accel_idx, geopm_levelzero_domain_e domain, int domain_idx, double setting) const
    {
        ze_result_t ze_result;
        zes_freq_properties_t property;
        zes_freq_range_t range;
        range.min = setting;
        range.max = setting;

        zes_freq_handle_t handle = m_devices.at(accel_idx).subdevice.m_freq_domain.at(domain).at(domain_idx);

        ze_result = zesFrequencyGetProperties(handle, &property);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                        ": Sysman failed to get domain properties.", __LINE__);

        if (property.canControl == 0) {
            throw Exception("LevelZeroShim::" + std::string(__func__) + ": Attempted to set frequency " +
                            "for non controllable domain",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        ze_result = zesFrequencySetRange(handle, &range);
        check_ze_result(ze_result, GEOPM_ERROR_RUNTIME, "LevelZeroShim::" + std::string(__func__) +
                                                        ": Sysman failed to set frequency.", __LINE__);
    }

    void LevelZeroShimImp::check_ze_result(ze_result_t ze_result, int error, std::string message, int line) const
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
