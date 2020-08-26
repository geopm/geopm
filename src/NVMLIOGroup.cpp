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

#include "NVMLIOGroup.hpp"

#include <cmath>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sched.h>

#include "IOGroup.hpp"
#include "PlatformTopo.hpp"
#include "NVMLDevicePool.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "Helper.hpp"

namespace geopm
{
    NVMLIOGroup::NVMLIOGroup()
        : NVMLIOGroup(platform_topo(), nvml_device_pool(platform_topo().num_domain(GEOPM_DOMAIN_CPU)))
    {
    }

    // Set up mapping between signal and control names and corresponding indices
    NVMLIOGroup::NVMLIOGroup(const PlatformTopo &platform_topo, const NVMLDevicePool &device_pool)
        : m_platform_topo(platform_topo)
        , m_nvml_device_pool(device_pool)
        , m_is_batch_read(false)
        , m_signal_available({{"NVML::FREQUENCY", {
                                  "Streaming multiprocessor frequency in hertz",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double
                                  }},
                              {"NVML::UTILIZATION_ACCELERATOR", {
                                  "Percentage of time the accelerator operated on a kernel in the last set of driver samples",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double
                                  }},
                              {"NVML::POWER", {
                                  "Accelerator power usage in watts",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::sum,
                                  string_format_double
                                  }},
                              {"NVML::POWER_LIMIT", {
                                  "Accelerator power limit in watts",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::sum,
                                  string_format_double
                                  }},
                              {"NVML::FREQUENCY_MEMORY", {
                                  "Accelerator memory frequency in hertz",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double
                                  }},
                              {"NVML::THROTTLE_REASONS", {
                                  "Accelerator clock throttling reasons",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double
                                  }},
                              {"NVML::TEMPERATURE", {
                                  "Accelerator temperature in degrees Celsius",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double
                                  }},
                              {"NVML::TOTAL_ENERGY_CONSUMPTION", {
                                  "Accelerator energy consumption in joules since the driver was loaded",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::sum,
                                  string_format_double
                                  }},
                              {"NVML::PERFORMANCE_STATE", {
                                  "Accelerator performance state",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double
                                  }},
                              {"NVML::PCIE_RX_THROUGHPUT", {
                                  "Accelerator PCIE receive throughput in bytes per second over a 20 millisecond period",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double
                                  }},
                              {"NVML::PCIE_TX_THROUGHPUT", {
                                  "Accelerator PCIE transmit throughput in bytes per second over a 20 millisecond period",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double
                                  }},
                              {"NVML::CPU_ACCELERATOR_ACTIVE_AFFINITIZATION", {
                                  "Returns the associated accelerator for a given CPU as determined by running processes."
                                  "\n  If no accelerators map to the CPU then NAN is returned"
                                  "\n  If multiple accelerators map to the CPU -1 is returned",
                                  {},
                                  GEOPM_DOMAIN_CPU,
                                  Agg::average,
                                  string_format_double
                                  }},
                              {"NVML::UTILIZATION_MEMORY", {
                                  "Percentage of time the accelerator memory was accessed in the last set of driver samples",
                                  {},
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::max,
                                  string_format_double
                                  }}
                             })
        , m_control_available({{"NVML::FREQUENCY_CONTROL", {
                                    "Sets streaming multiprocessor frequency min and max to the same limit",
                                    {},
                                    GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                    Agg::average,
                                    string_format_double
                                    }},
                               {"NVML::FREQUENCY_RESET_CONTROL", {
                                    "Resets streaming multiprocessor frequency min and max limits to default values",
                                    {},
                                    GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                    Agg::average,
                                    string_format_double
                                    }},
                               {"NVML::POWER_LIMIT_CONTROL", {
                                    "Sets accelerator power limit",
                                    {},
                                    GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                    Agg::average,
                                    string_format_double
                                    }}
                              })
    {
        // populate signals for each domain
        for (auto &sv : m_signal_available) {
            std::vector<std::shared_ptr<signal_s> > result;
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(signal_domain_type(sv.first)); ++domain_idx) {
                std::shared_ptr<signal_s> sgnl = std::make_shared<signal_s>(signal_s{0, false});
                result.push_back(sgnl);
            }
            sv.second.signals = result;
        }
        register_signal_alias("POWER_ACCELERATOR", "NVML::POWER");
        register_signal_alias("FREQUENCY_ACCELERATOR", "NVML::FREQUENCY");

        // populate controls for each domain
        for (auto &sv : m_control_available) {
            std::vector<std::shared_ptr<control_s> > result;
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(control_domain_type(sv.first)); ++domain_idx) {
                std::shared_ptr<control_s> ctrl = std::make_shared<control_s>(control_s{0, false});
                result.push_back(ctrl);
            }
            sv.second.controls = result;
        }
        register_control_alias("POWER_ACCELERATOR_LIMIT_CONTROL", "NVML::POWER_LIMIT_CONTROL");
        register_control_alias("FREQUENCY_ACCELERATOR_CONTROL", "NVML::FREQUENCY_CONTROL");
    }

    // Extract the set of all signal names from the index map
    std::set<std::string> NVMLIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_signal_available) {
            result.insert(sv.first);
        }
        return result;
    }

    // Extract the set of all control names from the index map
    std::set<std::string> NVMLIOGroup::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_control_available) {
            result.insert(sv.first);
        }
        return result;
    }

    // Check signal name using index map
    bool NVMLIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_available.find(signal_name) != m_signal_available.end();
    }

    // Check control name using index map
    bool NVMLIOGroup::is_valid_control(const std::string &control_name) const
    {
        return m_control_available.find(control_name) != m_control_available.end();
    }

    // Return domain for all valid signals
    int NVMLIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    // Return domain for all valid controls
    int NVMLIOGroup::control_domain_type(const std::string &control_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_control_available.find(control_name);
        if (it != m_control_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    // Mark the given signal to be read by read_batch()
    int NVMLIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": signal_name " + signal_name +
                            " not valid for NVMLIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name + ": domain_type must be " +
                            std::to_string(signal_domain_type(signal_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(signal_domain_type(signal_name))) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_is_batch_read) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": cannot push signal after call to read_batch().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int result = -1;
        bool is_found = false;
        std::shared_ptr<signal_s> signal = m_signal_available.at(signal_name).signals.at(domain_idx);

        // Check if signal was already pushed
        for (size_t ii = 0; !is_found && ii < m_signal_pushed.size(); ++ii) {
            // same location means this signal or its alias was already pushed
            if (m_signal_pushed[ii].get() == signal.get()) {
                result = ii;
                is_found = true;
            }
        }
        if (!is_found) {
            // If not pushed, add to pushed signals and configure for batch reads
            result = m_signal_pushed.size();
            signal->m_do_read = true;
            m_signal_pushed.push_back(signal);
        }

        return result;
    }

    // Mark the given control to be written by write_batch()
    int NVMLIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": control_name " + control_name +
                            " not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + control_name + ": domain_type must be " +
                            std::to_string(control_domain_type(control_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int result = -1;
        bool is_found = false;
        std::shared_ptr<control_s> control = m_control_available.at(control_name).controls.at(domain_idx);

        // Check if control was already pushed
        for (size_t ii = 0; !is_found && ii < m_control_pushed.size(); ++ii) {
            // same location means this control or its alias was already pushed
            if (m_control_pushed[ii] == control) {
                result = ii;
                is_found = true;
            }
        }
        if (!is_found) {
            // If not pushed, add to pushed control
            result = m_control_pushed.size();
            m_control_pushed.push_back(control);
        }

        return result;
    }

    // The active process list NVML call can be costly, 0.5-2ms per call was seen in early testing on average,
    // with a worst case of 8ms per call.  Because of this we cache the processes in a PID <-> Accelerator map
    // before using them elsewhere
    std::map<pid_t, int> NVMLIOGroup::accelerator_process_map(void) const
    {
        std::map<pid_t,int> accelerator_pid_map;

        for (int accel_idx = 0; accel_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR); ++accel_idx) {
            std::vector<int> active_process_list = m_nvml_device_pool.active_process_list(accel_idx);
            for (auto proc_itr : active_process_list) {
                // If a process is associated with multiple accelerators we have no good means of
                // signaling the user beyond providing an error value (-1).
                if (!accelerator_pid_map.count((pid_t)proc_itr)) {
                    accelerator_pid_map[(pid_t)proc_itr] = accel_idx;
                }
                else {
                    accelerator_pid_map[(pid_t)proc_itr] = -1;
                }
            }
        }
        return accelerator_pid_map;
    }

    // Parse PID to CPU affinitzation and use process list --> accelerator map to get CPU --> accelerator
    double NVMLIOGroup::cpu_accelerator_affinity(int cpu_idx, std::map<pid_t, int> process_map) const
    {
        double result = NAN;
        cpu_set_t *proc_cpuset = NULL;
        proc_cpuset = CPU_ALLOC(m_platform_topo.num_domain(GEOPM_DOMAIN_CPU));
        if (proc_cpuset != NULL) {
            for (auto &proc : process_map) {
                if (sched_getaffinity(proc.first,
                                      CPU_ALLOC_SIZE(m_platform_topo.num_domain(GEOPM_DOMAIN_CPU)),
                                      proc_cpuset) == 0) {
                    if (CPU_ISSET(cpu_idx, proc_cpuset)) {
                        result = proc.second;
                        break;
                    }
                }
            }
        }
        else {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": failed to allocate process CPU mask",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }

    // Parse and update saved values for signals
    void NVMLIOGroup::read_batch(void)
    {
        m_is_batch_read = true;
        for (auto &sv : m_signal_available) {
            if (sv.first == "NVML::CPU_ACCELERATOR_ACTIVE_AFFINITIZATION") {
                std::map<pid_t,int> process_map = accelerator_process_map();

                for (int domain_idx = 0; domain_idx < sv.second.signals.size(); ++domain_idx) {
                    if (sv.second.signals.at(domain_idx)->m_do_read) {
                        sv.second.signals.at(domain_idx)->m_value = cpu_accelerator_affinity(domain_idx, process_map);
                    }
                }
            }
            else {
                for (int domain_idx = 0; domain_idx < sv.second.signals.size(); ++domain_idx) {
                    if (sv.second.signals.at(domain_idx)->m_do_read) {
                        sv.second.signals.at(domain_idx)->m_value = read_signal(sv.first, sv.second.domain, domain_idx);
                    }
                }
            }
        }

    }

    // Write all controls that have been pushed and adjusted
    void NVMLIOGroup::write_batch(void)
    {
        for (auto &sv : m_control_available) {
            for (int domain_idx = 0; domain_idx < sv.second.controls.size(); ++domain_idx) {
                if (sv.second.controls.at(domain_idx)->m_is_adjusted) {
                    write_control(sv.first, sv.second.domain, domain_idx, sv.second.controls.at(domain_idx)->m_setting);
                }
            }
        }
    }

    // Return the latest value read by read_batch()
    double NVMLIOGroup::sample(int batch_idx)
    {
        // Do conversion of signal values stored in read batch
        if (batch_idx < 0 || batch_idx >= (int)m_signal_pushed.size()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": batch_idx " +std::to_string(batch_idx)+ " out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_batch_read) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": signal has not been read.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return m_signal_pushed[batch_idx]->m_value;
    }

    // Save a setting to be written by a future write_batch()
    void NVMLIOGroup::adjust(int batch_idx, double setting)
    {
        if (batch_idx < 0 || (unsigned)batch_idx >= m_control_pushed.size()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + "(): batch_idx " +std::to_string(batch_idx)+ " out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_control_pushed.at(batch_idx)->m_setting = setting;
        m_control_pushed.at(batch_idx)->m_is_adjusted = true;
    }

    // Read the value of a signal immediately, bypassing read_batch().  Should not modify m_signal_value
    double NVMLIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name +
                            " not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name + ": domain_type must be " +
                            std::to_string(signal_domain_type(signal_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(signal_domain_type(signal_name))) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        double result = NAN;
        if (signal_name == "NVML::FREQUENCY" || signal_name == "FREQUENCY_ACCELERATOR") {
            result = (double) m_nvml_device_pool.frequency_status_sm(domain_idx)*1e6;
        }
        else if (signal_name == "NVML::UTILIZATION_ACCELERATOR") {
            result = (double) m_nvml_device_pool.utilization(domain_idx)/100;
        }
        else if (signal_name == "NVML::THROTTLE_REASONS") {
            result = (double) m_nvml_device_pool.throttle_reasons(domain_idx);
        }
        else if (signal_name == "NVML::POWER" || signal_name == "POWER_ACCELERATOR") {
            result = (double) m_nvml_device_pool.power(domain_idx)/1e3;
        }
        else if (signal_name == "NVML::POWER_LIMIT") {
            result = (double) m_nvml_device_pool.power_limit(domain_idx)/1e3;
        }
        else if (signal_name == "NVML::FREQUENCY_MEMORY") {
            result = (double) m_nvml_device_pool.frequency_status_mem(domain_idx)*1e6;
        }
        else if (signal_name == "NVML::TEMPERATURE") {
            result = (double) m_nvml_device_pool.temperature(domain_idx);
        }
        else if (signal_name == "NVML::TOTAL_ENERGY_CONSUMPTION" ) {
            result = (double) m_nvml_device_pool.energy(domain_idx)/1e3;
        }
        else if (signal_name == "NVML::PERFORMANCE_STATE") {
            result = (double) m_nvml_device_pool.performance_state(domain_idx);
        }
        else if (signal_name == "NVML::PCIE_RX_THROUGHPUT") {
            result = (double) m_nvml_device_pool.throughput_rx_pcie(domain_idx)*1024;
        }
        else if (signal_name == "NVML::PCIE_TX_THROUGHPUT") {
            result = (double) m_nvml_device_pool.throughput_tx_pcie(domain_idx)*1024;
        }
        else if (signal_name == "NVML::UTILIZATION_MEMORY") {
            result = (double) m_nvml_device_pool.utilization_mem(domain_idx)/100;
        }
        else if (signal_name == "NVML::CPU_ACCELERATOR_ACTIVE_AFFINITIZATION") {
            std::map<pid_t,int> process_map = accelerator_process_map();
            result = cpu_accelerator_affinity(domain_idx, process_map);
        }
        else {
    #ifdef GEOPM_DEBUG
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": Handling not defined for " +
                            signal_name, GEOPM_ERROR_LOGIC, __FILE__, __LINE__);

    #endif
        }
        return result;
    }

    // Write to the control immediately, bypassing write_batch()
    void NVMLIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + control_name +
                            " not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != GEOPM_DOMAIN_BOARD_ACCELERATOR) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + control_name + ": domain_type must be " +
                            std::to_string(control_domain_type(control_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (control_name == "NVML::FREQUENCY_CONTROL" || control_name == "FREQUENCY_ACCELERATOR_CONTROL") {
            m_nvml_device_pool.frequency_control_sm(domain_idx, setting/1e6, setting/1e6);
        }
        else if (control_name == "NVML::FREQUENCY_RESET_CONTROL") {
            m_nvml_device_pool.frequency_reset_control(domain_idx);
        }
        else if (control_name == "NVML::POWER_LIMIT_CONTROL" || control_name == "POWER_ACCELERATOR_LIMIT_CONTROL") {
            m_nvml_device_pool.power_control(domain_idx, setting*1e3);
        }
        else {
    #ifdef GEOPM_DEBUG
                throw Exception("NVMLIOGroup::" + std::string(__func__) + "Handling not defined for "
                                + control_name, GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
    #endif
        }
    }

    // Implemented to allow an IOGroup to save platform settings before starting
    // to adjust them
    void NVMLIOGroup::save_control(void)
    {
        // Read NVML Power Limit
        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR); ++domain_idx) {
            m_initial_power_limit.push_back(m_nvml_device_pool.power_limit(domain_idx));
        }
    }

    // Implemented to allow an IOGroup to restore previously saved
    // platform settings
    void NVMLIOGroup::restore_control(void)
    {
        /// @todo: Usage of the NVML API for setting frequency, power, etc requires root privileges.
        ///        As such several unit tests will fail when calling restore_control.  Once a non-
        ///        privileged solution is available this code may be restored
    }

    // Hint to Agent about how to aggregate signals from this IOGroup
    std::function<double(const std::vector<double> &)> NVMLIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name +
                            "not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.m_agg_function;
    }

    // Specifies how to print signals from this IOGroup
    std::function<std::string(double)> NVMLIOGroup::format_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + signal_name +
                            "not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.m_format_function;
    }

    // A user-friendly description of each signal
    std::string NVMLIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": signal_name " + signal_name +
                            " not valid for NVMLIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_available.at(signal_name).m_description;
    }

    // A user-friendly description of each control
    std::string NVMLIOGroup::control_description(const std::string &control_name) const
    {
        if (!is_valid_control(control_name)) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": " + control_name +
                            "not valid for NVMLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return m_control_available.at(control_name).m_description;
    }

    // Name used for registration with the IOGroup factory
    std::string NVMLIOGroup::plugin_name(void)
    {
        return "nvml";
    }

    // Function used by the factory to create objects of this type
    std::unique_ptr<IOGroup> NVMLIOGroup::make_plugin(void)
    {
        return geopm::make_unique<NVMLIOGroup>();
    }

    void NVMLIOGroup::register_signal_alias(const std::string &alias_name,
                                            const std::string &signal_name)
    {
        if (m_signal_available.find(alias_name) != m_signal_available.end()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": signal_name " + alias_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            // skip adding an alias if underlying signal is not found
            return;
        }
        // copy signal info but append to description
        m_signal_available[alias_name] = it->second;
        m_signal_available[alias_name].m_description =
            m_signal_available[signal_name].m_description + '\n' + "    alias_for: " + signal_name;
    }

    void NVMLIOGroup::register_control_alias(const std::string &alias_name,
                                           const std::string &control_name)
    {
        if (m_control_available.find(alias_name) != m_control_available.end()) {
            throw Exception("NVMLIOGroup::" + std::string(__func__) + ": contro1_name " + alias_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_control_available.find(control_name);
        if (it == m_control_available.end()) {
            // skip adding an alias if underlying control is not found
            return;
        }
        // copy control info but append to description
        m_control_available[alias_name] = it->second;
        m_control_available[alias_name].m_description =
        m_control_available[control_name].m_description + '\n' + "    alias_for: " + control_name;
    }
}
