/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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
#include <string>

#include "ExampleIOGroup.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"

using geopm::Exception;
using geopm::IPlatformTopo;

// Registers this IOGroup with the IOGroup factory, making it visible
// to PlatformIO when the plugin is first loaded.
static void __attribute__((constructor)) example_iogroup_load(void)
{
    geopm::iogroup_factory().register_plugin(ExampleIOGroup::plugin_name(),
                                             ExampleIOGroup::make_plugin);
}

// Set up mapping between signal and control names and corresponding indices
ExampleIOGroup::ExampleIOGroup()
    : m_platform_topo(geopm::platform_topo())
    , m_do_batch_read(false)
    , m_is_batch_read(false)
    , m_signal_idx_map{{"EXAMPLE::USER_TIME",   M_SIGNAL_USER_TIME},
                       {"USER_TIME",            M_SIGNAL_USER_TIME},   // alias for EXAMPLE::USER_TIME
                       {"EXAMPLE::NICE_TIME",   M_SIGNAL_NICE_TIME},
                       {"NICE_TIME",            M_SIGNAL_NICE_TIME},   // alias for EXAMPLE::NICE_TIME
                       {"EXAMPLE::SYSTEM_TIME", M_SIGNAL_SYSTEM_TIME},
                       {"SYSTEM_TIME",          M_SIGNAL_SYSTEM_TIME}, // alias for EXAMPLE::SYSTEM_TIME
                       {"EXAMPLE::IDLE_TIME",   M_SIGNAL_IDLE_TIME},
                       {"IDLE_TIME",            M_SIGNAL_IDLE_TIME}}   // alias for EXAMPLE::IDLE_TIME
    , m_control_idx_map{{"EXAMPLE::STDOUT",     M_CONTROL_STDOUT},
                        {"STDOUT",              M_CONTROL_STDOUT},     // alias for EXAMPLE::STDOUT
                        {"EXAMPLE::STDERR",     M_CONTROL_STDERR},
                        {"STDERR",              M_CONTROL_STDERR}}     // alias for EXAMPLE::STDERR
    , m_do_read(M_NUM_SIGNAL, false)
    , m_do_write(M_NUM_SIGNAL, false)
    , m_signal_value(M_NUM_SIGNAL)
    , m_control_value(M_NUM_CONTROL)
{

}

// Extract the set of all signal names from the index map
std::set<std::string> ExampleIOGroup::signal_names(void) const
{
    std::set<std::string> result;
    for (const auto &sv : m_signal_idx_map) {
        result.insert(sv.first);
    }
    return result;
}

// Extract the set of all control names from the index map
std::set<std::string> ExampleIOGroup::control_names(void) const
{
    std::set<std::string> result;
    for (const auto &sv : m_control_idx_map) {
        result.insert(sv.first);
    }
    return result;
}

// Check signal name using index map
bool ExampleIOGroup::is_valid_signal(const std::string &signal_name) const
{
    return m_signal_idx_map.find(signal_name) != m_signal_idx_map.end();
}

// Check control name using index map
bool ExampleIOGroup::is_valid_control(const std::string &control_name) const
{
    return m_control_idx_map.find(control_name) != m_control_idx_map.end();
}

// Return board domain for all valid signals
int ExampleIOGroup::signal_domain_type(const std::string &signal_name) const
{
    int result = IPlatformTopo::M_DOMAIN_INVALID;
    if (is_valid_signal(signal_name)) {
        result = IPlatformTopo::M_DOMAIN_BOARD;
    }
    return result;
}

// Return board domain for all valid controls
int ExampleIOGroup::control_domain_type(const std::string &control_name) const
{
    int result = IPlatformTopo::M_DOMAIN_INVALID;
    if (is_valid_control(control_name)) {
        result = IPlatformTopo::M_DOMAIN_BOARD;
    }
    return result;
}

// Mark the given signal to be read by read_batch()
int ExampleIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("ExampleIOGroup::push_signal(): signal_name " + signal_name +
                        " not valid for ExampleIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != IPlatformTopo::M_DOMAIN_BOARD) {
        throw Exception("ExampleIOGroup::push_signal(): domain_type must be M_DOMAIN_BOARD.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_BOARD)) {
        throw Exception("ExampleIOGroup::push_signal(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (m_is_batch_read) {
        throw Exception("ExampleIOGroup::push_signal(): cannot push signal after call to read_batch().",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    int signal_idx = m_signal_idx_map.at(signal_name);
    m_do_read[signal_idx] = true;
    m_do_batch_read = true;
    return signal_idx;
}

// Mark the given control to be written by write_batch()
int ExampleIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
{
    if (!is_valid_control(control_name)) {
        throw Exception("ExampleIOGroup::push_control(): control_name " + control_name +
                        " not valid for ExampleIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != IPlatformTopo::M_DOMAIN_BOARD) {
        throw Exception("ExampleIOGroup::push_control(): domain_type must be M_DOMAIN_BOARD.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_BOARD)) {
        throw Exception("ExampleIOGroup::push_control(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    int control_idx = m_control_idx_map.at(control_name);
    m_do_write[control_idx] = true;
    return control_idx;
}

// Parse /proc/stat for values in the cpu row
std::vector<std::string> ExampleIOGroup::parse_proc_stat(void)
{
    // find the line starting with "cpu "
    bool found = false;
    std::string line;
    std::ifstream infile("/proc/stat");
    while (!found && std::getline(infile, line)) {
        if (line.find("cpu ") == 0) {
            found = true;
        }
    }
    if (!found) {
        throw Exception("ExampleIOGroup::parse_proc_stat(): unable to find 'cpu' row in /proc/stat.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    // split line on spaces
    std::vector<std::string> cpu_val;
    std::string val_str;
    size_t begin = 0;
    size_t end = -1;
    do {
        begin = end + 1;
        end = line.find(" ", begin);
        val_str = line.substr(begin, end - begin);
        if (!val_str.empty()) {
            cpu_val.push_back(val_str);
        }
    }
    while (end != std::string::npos);
    if (cpu_val.size() < 5) {
        throw Exception("ExampleIOGroup::parse_proc_stat(): expected at least 5 columns for cpu.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    return cpu_val;
}

// Parse /proc/stat and update saved values for signals
void ExampleIOGroup::read_batch(void)
{
    m_is_batch_read = true;
    if (m_do_batch_read) {
        std::vector<std::string> cpu_val = parse_proc_stat();
        if (m_do_read[M_SIGNAL_USER_TIME]) {
            m_signal_value[M_SIGNAL_USER_TIME] = cpu_val[1];
        }
        if (m_do_read[M_SIGNAL_NICE_TIME]) {
            m_signal_value[M_SIGNAL_NICE_TIME] = cpu_val[2];
        }
        if (m_do_read[M_SIGNAL_SYSTEM_TIME]) {
            m_signal_value[M_SIGNAL_SYSTEM_TIME] = cpu_val[3];
        }
        if (m_do_read[M_SIGNAL_IDLE_TIME]) {
            m_signal_value[M_SIGNAL_IDLE_TIME] = cpu_val[4];
        }
    }
}

// Print the saved values for controls
void ExampleIOGroup::write_batch(void)
{
    if (m_do_write[M_CONTROL_STDOUT]) {
        std::cout << m_control_value[M_CONTROL_STDOUT] << std::endl;
    }
    if (m_do_write[M_CONTROL_STDERR]) {
        std::cerr << m_control_value[M_CONTROL_STDERR] << std::endl;
    }
}

// Return the latest value read by read_batch()
double ExampleIOGroup::sample(int batch_idx)
{
    if (batch_idx < 0 || batch_idx >= M_NUM_SIGNAL) {
        throw Exception("ExampleIOGroup::sample(): batch_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (!m_do_read[batch_idx]) {
        throw Exception("ExampleIOGroup::sample(): signal has not been pushed.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (!m_is_batch_read) {
        throw Exception("ExampleIOGroup::sample(): signal has not been read.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    /// @todo more error handling
    return std::stod(m_signal_value[batch_idx]);
}

// Save a setting to be written by a future write_batch()
void ExampleIOGroup::adjust(int batch_idx, double setting)
{
    if (batch_idx < 0 || batch_idx >= M_NUM_CONTROL) {
        throw Exception("ExampleIOGroup::adjust(): batch_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (!m_do_write[batch_idx]) {
        throw Exception("ExampleIOGroup::adjust(): control has not been pushed.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    m_control_value[batch_idx] = std::to_string(setting);
}

// Read the value of a signal immediately, bypassing read_batch()
double ExampleIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("ExampleIOGroup:read_signal(): " + signal_name +
                        "not valid for ExampleIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != IPlatformTopo::M_DOMAIN_BOARD) {
        throw Exception("ExampleIOGroup::push_signal(): domain_type must be M_DOMAIN_BOARD.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_BOARD)) {
        throw Exception("ExampleIOGroup::push_signal(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double result = NAN;
    std::vector<std::string> cpu_val = parse_proc_stat();
    int signal_idx = m_signal_idx_map.at(signal_name);
    switch (signal_idx) {
        case M_SIGNAL_USER_TIME:
            result = std::stod(cpu_val[1]);
            break;
        case M_SIGNAL_NICE_TIME:
            result = std::stod(cpu_val[2]);
            break;
        case M_SIGNAL_SYSTEM_TIME:
            result = std::stod(cpu_val[3]);
            break;
        case M_SIGNAL_IDLE_TIME:
            result = std::stod(cpu_val[4]);
            break;
        default:
#ifdef GEOPM_DEBUG
            throw Exception("ExampleIOGroup::read_signal: Invalid signal index or name: " + signal_name,
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
            break;
    }
    return result;
}

// Write to the control immediately, bypassing write_batch()
void ExampleIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
{
    if (!is_valid_control(control_name)) {
        throw Exception("ExampleIOGroup:write_control(): " + control_name +
                        "not valid for ExampleIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != IPlatformTopo::M_DOMAIN_BOARD) {
        throw Exception("ExampleIOGroup::push_control(): domain_type must be M_DOMAIN_BOARD.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_BOARD)) {
        throw Exception("ExampleIOGroup::push_control(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int control_idx = m_control_idx_map.at(control_name);
    switch (control_idx) {
        case M_CONTROL_STDOUT:
            std::cout << setting << std::endl;
            break;
        case M_CONTROL_STDERR:
            std::cerr << setting << std::endl;
            break;
        default:
#ifdef GEOPM_DEBUG
            throw Exception("ExampleIOGroup::write_control: Invalid control index or name: " + control_name,
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
            break;
    }
}

// Implemented to allow an IOGroup platform settings before starting
// to adjust them
void ExampleIOGroup::save_control(void)
{

}

// Implemented to allow an IOGroup to restore previously saved
// platform settings
void ExampleIOGroup::restore_control(void)
{

}

// Hint to Agent about how to aggregate signals from this IOGroup
std::function<double(const std::vector<double> &)> ExampleIOGroup::agg_function(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("ExampleIOGroup:agg_function(): " + signal_name +
                        "not valid for ExampleIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    // All signals will be aggregated as an average
    return geopm::Agg::average;
}

// A user-friendly description of each signal
std::string ExampleIOGroup::signal_description(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("ExampleIOGroup::signal_description(): signal_name " + signal_name +
                        " not valid for ExampleIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    int signal_idx = m_signal_idx_map.at(signal_name);
    std::string result = "";
    switch (signal_idx) {
        case M_SIGNAL_USER_TIME:
            result = "CPU time spent in user mode";
            break;
        case M_SIGNAL_NICE_TIME:
            result = "CPU time spend in user mode with low priority";
            break;
        case M_SIGNAL_SYSTEM_TIME:
            result = "CPU time spent in system mode";
        case M_SIGNAL_IDLE_TIME:
            result = "CPU idle time";
            break;
        default:
            break;
    }
    return result;
}

// A user-friendly description of each control
std::string ExampleIOGroup::control_description(const std::string &control_name) const
{
    if (!is_valid_control(control_name)) {
        throw Exception("ExampleIOGroup::control_description(): " + control_name +
                        "not valid for ExampleIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    std::string result = "";
    int control_idx = m_control_idx_map.at(control_name);
    switch (control_idx) {
        case M_CONTROL_STDOUT:
            result = "Writes a floating point value to standard output";
            break;
        case M_CONTROL_STDERR:
            result = "Writes a floating point value to standard error";
            break;
        default:
            break;
    }
    return result;
}

// Name used for registration with the IOGroup factory
std::string ExampleIOGroup::plugin_name(void)
{
    return "example";
}

// Function used by the factory to create objects of this type
std::unique_ptr<geopm::IOGroup> ExampleIOGroup::make_plugin(void)
{
    return std::unique_ptr<geopm::IOGroup>(new ExampleIOGroup);
}
