/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ExampleIOGroup.hpp"

#include <cmath>

#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "geopm/SaveControl.hpp"

using geopm::Exception;
using geopm::PlatformTopo;

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
                       {"IDLE_TIME",            M_SIGNAL_IDLE_TIME}}
    , m_do_read(M_NUM_SIGNAL, false)
    , m_signal_value(M_NUM_SIGNAL, "")
    , m_tmp_file_path("/tmp/geopm_example_control." + std::to_string(getuid()))
    , m_tmp_file_msg("Could not open or parse text file \"" + m_tmp_file_path +
                     "\", create and populate with a floating point number to enable \"TMP_FILE_CONTROL\"")
    , m_do_write(false)
    , m_control_value(0.0)
    , m_is_control_enabled(false)
{
    try {
        m_control_value = parse_buffer(read_control());
        m_is_control_enabled = std::isfinite(m_control_value);
    }
    catch (const Exception &ex) {
        std::cerr << "Warning: " << ex.what() << std::endl;
    }
    if (m_is_control_enabled) {
        m_signal_idx_map["EXAMPLE::TMP_FILE_CONTROL"] = M_CONTROL_TMP_FILE;
        m_signal_idx_map["TMP_FILE_CONTROL"] = M_CONTROL_TMP_FILE;
        m_control_idx_map["EXAMPLE::TMP_FILE_CONTROL"] = M_CONTROL_TMP_FILE;
        m_control_idx_map["TMP_FILE_CONTROL"] = M_CONTROL_TMP_FILE;
    }
}

std::string ExampleIOGroup::read_control(void)
{
    std::string result;
    std::ifstream control_stream(m_tmp_file_path);
    if (!control_stream.good()) {
        throw Exception("File not found: " + m_tmp_file_path,
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    control_stream >> result;
    return result;
}

double ExampleIOGroup::parse_buffer(const std::string &buffer)
{
    try {
        return stod(buffer);
    }
    catch (const std::invalid_argument &ex) {
        throw Exception("ExampleIOGroup: Value could not be parsed: \"" + buffer + "\"",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    catch (const std::out_of_range &ex) {
        throw Exception("ExampleIOGroup: Value out of range: \"" + buffer + "\"",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
}
void ExampleIOGroup::write_control(double setting)
{
    std::ofstream control_stream(m_tmp_file_path);
    control_stream << setting;
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
    int result = GEOPM_DOMAIN_INVALID;
    if (is_valid_signal(signal_name)) {
        result = GEOPM_DOMAIN_BOARD;
    }
    return result;
}

// Return board domain for all valid controls
int ExampleIOGroup::control_domain_type(const std::string &control_name) const
{
    int result = GEOPM_DOMAIN_INVALID;
    if (is_valid_control(control_name)) {
        result = GEOPM_DOMAIN_BOARD;
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
    if (domain_type != GEOPM_DOMAIN_BOARD) {
        throw Exception("ExampleIOGroup::push_signal(): domain_type must be M_DOMAIN_BOARD.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD)) {
        throw Exception("ExampleIOGroup::push_signal(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int signal_idx = get_signal_index(signal_name);
    m_do_read[signal_idx] = true;
    m_do_batch_read = true;
    return signal_idx;
}

// Mark the given control to be written by write_batch()
int ExampleIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
{
    if (!m_is_control_enabled) {
        throw Exception(m_tmp_file_msg, GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    if (!is_valid_control(control_name)) {
        throw Exception("ExampleIOGroup::push_control(): control_name " + control_name +
                        " not valid for ExampleIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != GEOPM_DOMAIN_BOARD) {
        throw Exception("ExampleIOGroup::push_control(): domain_type must be M_DOMAIN_BOARD.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD)) {
        throw Exception("ExampleIOGroup::push_control(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    m_do_write = true;
    return 0;
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

int ExampleIOGroup::get_signal_index(const std::string &signal_name) const
{
    auto it = m_signal_idx_map.find(signal_name);
    if (it == m_signal_idx_map.end()) {
        throw Exception("Signal is not provided by ExampleIOGroup: " + signal_name,
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    return it->second;
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
        if (m_do_read[M_CONTROL_TMP_FILE]) {
            m_signal_value[M_CONTROL_TMP_FILE] = read_control();
        }
    }
}

// Print the saved values for controls
void ExampleIOGroup::write_batch(void)
{
    if (m_do_write) {
        write_control(m_control_value);
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
    return parse_buffer(m_signal_value[batch_idx]);
}

// Save a setting to be written by a future write_batch()
void ExampleIOGroup::adjust(int batch_idx, double setting)
{
    if (!m_is_control_enabled) {
        throw Exception("ExampleIOGroup::adjust(): File does not exist or could not be parsed:" + m_tmp_file_path,
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    if (batch_idx != 0) {
        throw Exception("ExampleIOGroup::adjust(): batch_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (!m_do_write) {
        throw Exception("ExampleIOGroup::adjust(): control has not been pushed.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    m_control_value = setting;
}

// Read the value of a signal immediately, bypassing read_batch()
double ExampleIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("ExampleIOGroup:read_signal(): " + signal_name +
                        "not valid for ExampleIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != GEOPM_DOMAIN_BOARD) {
        throw Exception("ExampleIOGroup::push_signal(): domain_type must be M_DOMAIN_BOARD.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD)) {
        throw Exception("ExampleIOGroup::push_signal(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double result = 0.0;
    std::vector<std::string> cpu_val = parse_proc_stat();
    int signal_idx = get_signal_index(signal_name);
    switch (signal_idx) {
        case M_SIGNAL_USER_TIME:
            result = parse_buffer(cpu_val[1]);
            break;
        case M_SIGNAL_NICE_TIME:
            result = parse_buffer(cpu_val[2]);
            break;
        case M_SIGNAL_SYSTEM_TIME:
            result = parse_buffer(cpu_val[3]);
            break;
        case M_SIGNAL_IDLE_TIME:
            result = parse_buffer(cpu_val[4]);
            break;
        case M_CONTROL_TMP_FILE:
            result = parse_buffer(read_control());
            break;
        default:
            break;
    }
    return result;
}

// Write to the control immediately, bypassing write_batch()
void ExampleIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
{
    if (!m_is_control_enabled) {
        throw Exception(m_tmp_file_msg, GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    if (!is_valid_control(control_name)) {
        throw Exception("ExampleIOGroup:write_control(): " + control_name +
                        "not valid for ExampleIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != GEOPM_DOMAIN_BOARD) {
        throw Exception("ExampleIOGroup::push_control(): domain_type must be M_DOMAIN_BOARD.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD)) {
        throw Exception("ExampleIOGroup::push_control(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    write_control(setting);
}

// Implemented to allow an IOGroup platform settings to be saved before starting
// to adjust them
void ExampleIOGroup::save_control(void)
{
    if (m_is_control_enabled && m_control_saver == nullptr) {
        m_control_saver = geopm::SaveControl::make_unique(*this);
    }
}

void ExampleIOGroup::save_control(const std::string &save_path)
{
    if (m_is_control_enabled && m_control_saver == nullptr) {
        m_control_saver = geopm::SaveControl::make_unique(*this);
    }
    m_control_saver->write_json(save_path);
}

// Implemented to allow an IOGroup to restore previously saved platform settings
void ExampleIOGroup::restore_control(void)
{
    if (m_control_saver != nullptr) {
        m_control_saver->restore(*this);
    }
}

void ExampleIOGroup::restore_control(const std::string &save_path)
{
    if (m_control_saver != nullptr) {
        m_control_saver->restore(*this);
    }
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

// Specifies how to print signals from this IOGroup
std::function<std::string(double)> ExampleIOGroup::format_function(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("ExampleIOGroup::format_function(): " + signal_name +
                        "not valid for TimeIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    return geopm::string_format_double;
}

// A user-friendly description of each signal
std::string ExampleIOGroup::signal_description(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("ExampleIOGroup::signal_description(): signal_name " + signal_name +
                        " not valid for ExampleIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    int signal_idx = get_signal_index(signal_name);
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
            break;
        case M_SIGNAL_IDLE_TIME:
            result = "CPU idle time";
            break;
        case M_CONTROL_TMP_FILE:
            result = "Value contained in file \"" + m_tmp_file_path + "\"";
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

    return "Writes a value to \"" + m_tmp_file_path + "\" but file must be created prior to startup";
}

int ExampleIOGroup::signal_behavior(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("ExampleIOGroup::signal_behavior(): signal_name " + signal_name +
                        " not valid for ExampleIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    // All example signals are time based and increase monotonically
    int result = M_SIGNAL_BEHAVIOR_MONOTONE;
    if (signal_name == "TMP_FILE_CONTROL" || signal_name == "EXAMPLE::TMP_FILE_CONTROL") {
        result = M_SIGNAL_BEHAVIOR_VARIABLE;
    }
    return result;
}

std::string ExampleIOGroup::name(void) const
{
    return plugin_name();
}

// Name used for registration with the IOGroup factory
std::string ExampleIOGroup::plugin_name(void)
{
    return "EXAMPLE";
}

// Function used by the factory to create objects of this type
std::unique_ptr<geopm::IOGroup> ExampleIOGroup::make_plugin(void)
{
    return std::make_unique<ExampleIOGroup>();
}
