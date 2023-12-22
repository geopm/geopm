/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SysfsIOGroup.hpp"

#include <cmath>

#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <iterator>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "IOUring.hpp"
#include "SaveControl.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"

using geopm::Exception;
using geopm::PlatformTopo;

namespace geopm
{
static const std::string CPUFREQ_DIRECTORY = "/sys/devices/system/cpu/cpufreq";

// Open a cpufreq attribute file for a given cpufreq resource. Return the opened fd.
static UniqueFd open_resource_attribute(const std::string& path, bool do_write)
{
    UniqueFd fd = open(path.c_str(), do_write ? O_WRONLY : O_RDONLY);
    if (fd.get() == -1) {
        throw geopm::Exception("open_resource_attribute() failed to open " + path,
                               errno, __FILE__, __LINE__);
    }
    return fd;
}

// Read a double from a cpufreq resource's opened sysfs attribute file.
static std::string read_resource_attribute_fd(int fd)
{
    char buf[SysfsDriver::M_IO_BUFFER_SIZE] = {0};
    int read_bytes = read(fd, buf, sizeof buf - 1);
    if (read_bytes < 0) {
        throw geopm::Exception("SysfsIOGroup failed to read signal",
                               errno, __FILE__, __LINE__);
    }
    if (static_cast<size_t>(read_bytes) >= sizeof buf) {
        throw geopm::Exception("SysfsIOGroup truncated read signal",
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    buf[read_bytes] = '\0';

    return std::string(buf);
}

// Write a double to a cpufreq resource's opened sysfs attribute file.
static void write_resource_attribute_fd(int fd, const std::string &value)
{
    char buf[SysfsDriver::M_IO_BUFFER_SIZE] = {0};
    if (value.length() >= sizeof buf) {
        throw geopm::Exception("SysfsIOGroup truncated write control",
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }

    int write_bytes = pwrite(fd, buf, value.length() + 1, 0);
    if (write_bytes < 0) {
        throw geopm::Exception("SysfsIOGroup failed to write control",
                               errno, __FILE__, __LINE__);
    }
    if (static_cast<size_t>(write_bytes) < value.length() + 1) {
        throw geopm::Exception("SysfsIOGroup truncated write control",
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
}


SysfsIOGroup::SysfsIOGroup(std::shared_ptr<SysfsDriver> driver)
    : SysfsIOGroup(driver, platform_topo(), nullptr, nullptr, nullptr)
{
}

// Set up mapping between signal and control names and corresponding indices
SysfsIOGroup::SysfsIOGroup(
        std::shared_ptr<SysfsDriver> driver,
        const PlatformTopo &topo,
        std::shared_ptr<SaveControl> control_saver,
        std::shared_ptr<IOUring> batch_reader,
        std::shared_ptr<IOUring> batch_writer)
    : m_driver(driver)
    , m_platform_topo(topo)
    , m_do_batch_read(false)
    , m_is_batch_read(false)
    , m_is_batch_write(false)
    , m_control_value{}
    , m_properties(m_driver->properties())
    , m_pushed_info_signal{}
    , m_pushed_info_control{}
    , m_control_saver(control_saver)
    , m_batch_reader(batch_reader)
    , m_batch_writer(batch_writer)
{
    std::string iogroup_name = m_driver->driver();
    for (const auto &it : m_properties) {
        m_signals.try_emplace(it.first, std::cref(it.second));
        if (it.second.is_writable) {
            m_controls.try_emplace(it.first, std::cref(it.second));
            if (it.second.alias != "") {
                m_controls.try_emplace(it.second.alias, std::cref(it.second));
            }
        }
        if (it.second.alias != "") {
            m_signals.try_emplace(it.second.alias, std::cref(it.second));
        }
    }
}

SysfsIOGroup::~SysfsIOGroup()
{
}

// Extract the set of all signal names from the index map
std::set<std::string> SysfsIOGroup::signal_names(void) const
{
    std::set<std::string> result;
    for (const auto &it : m_signals) {
        result.insert(it.first);
    }
    return result;
}

// Extract the set of all control names from the index map
std::set<std::string> SysfsIOGroup::control_names(void) const
{
    std::set<std::string> result;
    for (const auto &it : m_controls) {
        result.insert(it.first);
    }
    return result;
}

// Check signal name using index map
bool SysfsIOGroup::is_valid_signal(const std::string &signal_name) const
{
    return m_signals.find(signal_name) != m_signals.end();
}

// Check control name using index map
bool SysfsIOGroup::is_valid_control(const std::string &control_name) const
{
    return m_controls.find(control_name) != m_controls.end();
}

// Return domain for all valid signals
int SysfsIOGroup::signal_domain_type(const std::string &signal_name) const
{
    int result = GEOPM_DOMAIN_INVALID;
    const auto it = m_signals.find(signal_name);
    if (it != m_signals.end()) {
        result = m_driver->domain_type(it->second.get().name);
    }
    return result;
}

// Return domain for all valid controls
int SysfsIOGroup::control_domain_type(const std::string &control_name) const
{
    int result = GEOPM_DOMAIN_INVALID;
    const auto it = m_controls.find(control_name);
    if (it != m_controls.end()) {
        result = m_driver->domain_type(it->second.get().name);
    }
    return result;
}

// Mark the given signal to be read by read_batch()
int SysfsIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    if (m_is_batch_read) {
        throw Exception("SysfsIOGroup::push_signal(): cannot push signal after call to read_batch().",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    std::string cname = check_request(__FUNCTION__, signal_name, "", domain_type, domain_idx);
    auto pushed_it = std::find_if(m_pushed_info_signal.begin(),
                                  m_pushed_info_signal.end(),
                                  [signal_name, domain_idx] (const m_pushed_info_s &info) {
                                      return info.name == signal_name &&
                                             info.domain_idx == domain_idx;

                                  });
    int signal_idx = -1;

    if (pushed_it != m_pushed_info_signal.end()) {
        // This has already been pushed. Return the same index as before.
        signal_idx = std::distance(m_pushed_info_signal.begin(), pushed_it);
    }
    else {
        auto path = m_driver->attribute_path(cname, domain_idx);
        UniqueFd fd = open_resource_attribute(path, false);

        // This is a newly-pushed signal. Give it a new index.
        m_pushed_info_signal.push_back(m_pushed_info_s{std::move(fd), signal_name, domain_type, domain_idx, NAN, false, std::make_shared<int>(0), {}, m_driver->signal_parse(cname), m_driver->control_gen(cname)});
        signal_idx = m_pushed_info_signal.size() - 1;
    }

    m_do_batch_read = true;
    return signal_idx;
}

// Mark the given control to be written by write_batch()
int SysfsIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
{
    if (m_is_batch_write) {
        throw Exception("SysfsIOGroup::push_control(): Cannot push control " + control_name +
                        "because batch writes have already been triggered.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    std::string cname = check_request(__FUNCTION__, "", control_name, domain_type, domain_idx);
    auto pushed_it = std::find_if(m_pushed_info_control.begin(),
                                  m_pushed_info_control.end(),
                                  [control_name, domain_idx](const m_pushed_info_s &info) {
                                      return info.name == control_name && info.domain_idx == domain_idx;
                                  });
    int control_idx = -1;

    if (pushed_it != m_pushed_info_control.end()) {
        // This has already been pushed. Return the same index as before.
        control_idx = std::distance(m_pushed_info_control.begin(), pushed_it);
    }
    else {
        // TODO: make this obvious: why get(name).name? Because of aliases
        auto path = m_driver->attribute_path(cname, domain_idx);
        UniqueFd fd = open_resource_attribute(path, true);

        // This is a newly-pushed control. Give it a new index.
        m_pushed_info_control.push_back(m_pushed_info_s{std::move(fd), control_name, domain_type, domain_idx, NAN, false, std::make_shared<int>(0), {}, m_driver->signal_parse(control_name), m_driver->control_gen(control_name)});
        control_idx = m_pushed_info_control.size() - 1;
    }
    return control_idx;
}

void SysfsIOGroup::read_batch(void)
{
    m_is_batch_read = true;
    if (m_do_batch_read) {
        if (!m_batch_reader) {
            m_batch_reader = IOUring::make_unique(m_pushed_info_signal.size());
        }
        for (auto &info : m_pushed_info_signal) {
            m_batch_reader->prep_read(
                info.last_io_return, info.fd.get(), info.buf.data(), info.buf.size(), 0);
        }
        m_batch_reader->submit();
        for (auto &info : m_pushed_info_signal) {
            if (*info.last_io_return < 0) {
                throw geopm::Exception("SysfsIOGroup failed to read signal",
                                       errno, __FILE__, __LINE__);
            }
            size_t bytes_read = static_cast<size_t>(*info.last_io_return);
            if (bytes_read >= info.buf.size()) {
                throw geopm::Exception("SysfsIOGroup truncated read signal",
                                       GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            info.buf[bytes_read] = '\0';

            // TODO (dcw): Make sure parser is scaling for us
            info.value = info.parse(std::string(info.buf.data()));
        }
    }
}

void SysfsIOGroup::write_batch(void)
{
    m_is_batch_write = true;
    if (!m_batch_writer) {
        m_batch_writer = IOUring::make_unique(m_pushed_info_signal.size());
    }

    for (auto &info : m_pushed_info_control) {
        if (info.do_write && !std::isnan(info.value)) {
            std::string setting = info.gen(info.value);
            if (setting.length() >= info.buf.size()) {
                throw geopm::Exception("SysfsIOGroup control value is too long",
                                       GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            std::strncpy(info.buf.data(), setting.c_str(), info.buf.size());
            m_batch_writer->prep_write(
                info.last_io_return, info.fd.get(), info.buf.data(),
                static_cast<unsigned>(setting.length() + 1), 0);
        }
    }
    m_batch_writer->submit();
    for (auto &info : m_pushed_info_control) {
        if (info.do_write && !std::isnan(info.value)) {
            if (*info.last_io_return < 0) {
                throw geopm::Exception("SysfsIOGroup failed to write control \"" +
                                       info.name + "\"",
                                       errno, __FILE__, __LINE__);
            }
        }
    }
}

double SysfsIOGroup::sample(int batch_idx)
{
    if (batch_idx < 0 || static_cast<size_t>(batch_idx) >= m_pushed_info_signal.size()) {
        throw Exception("SysfsIOGroup::sample(): batch_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (!m_is_batch_read) {
        throw Exception("SysfsIOGroup::sample(): signal has not been read.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    return m_pushed_info_signal[static_cast<size_t>(batch_idx)].value;
}

// Save a setting to be written by a future write_batch()
void SysfsIOGroup::adjust(int batch_idx, double setting)
{
    if (batch_idx < 0 || static_cast<size_t>(batch_idx) >= m_pushed_info_control.size()) {
        throw Exception("SysfsIOGroup::adjust(): batch_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    auto &info = m_pushed_info_control[static_cast<size_t>(batch_idx)];
    if (info.value != setting) {
        info.do_write = true;
        info.value = setting;
    }
}

double SysfsIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    std::string cname = check_request(__FUNCTION__, signal_name, "", domain_type, domain_idx);
    UniqueFd fd = open_resource_attribute(m_driver->attribute_path(cname, domain_idx), false);
    double read_value = m_driver->signal_parse(cname)(read_resource_attribute_fd(fd.get()));
    return read_value;
}

// Write to the control immediately, bypassing write_batch()
void SysfsIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
{
    std::string cname = check_request(__FUNCTION__, "", control_name, domain_type, domain_idx);
    UniqueFd fd = open_resource_attribute(m_driver->attribute_path(cname, domain_idx), true);
    write_resource_attribute_fd(fd.get(), m_driver->control_gen(cname)(setting));
}

void SysfsIOGroup::save_control(void)
{
    if (!m_control_saver) {
        m_control_saver = SaveControl::make_unique(*this);
    }
}

void SysfsIOGroup::save_control(const std::string &save_path)
{
    if (!m_control_saver) {
        m_control_saver = SaveControl::make_unique(*this);
    }
    m_control_saver->write_json(save_path);
}

void SysfsIOGroup::restore_control(void)
{
    if (m_control_saver) {
        m_control_saver->restore(*this);
    }
}

void SysfsIOGroup::restore_control(const std::string &save_path)
{
    if (!m_control_saver) {
        m_control_saver = SaveControl::make_unique(geopm::read_file(save_path));
    }
    m_control_saver->restore(*this);
}

std::function<double(const std::vector<double> &)> SysfsIOGroup::agg_function(
        const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("SysfsIOGroup:agg_function(): " + signal_name +
                        "not valid for SysfsIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    const auto &property = m_signals.at(signal_name);
    return property.get().aggregation_function;
}

std::function<std::string(double)> SysfsIOGroup::format_function(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("SysfsIOGroup::format_function(): " + signal_name +
                        "not valid for TimeIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    const auto &property = m_signals.at(signal_name);
    return property.get().format_function;
}

// A user-friendly description of each signal
std::string SysfsIOGroup::signal_description(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("SysfsIOGroup::signal_description(): signal_name " + signal_name +
                        " not valid for SysfsIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    const auto &property = m_signals.at(signal_name);
    std::ostringstream result;
    std::string cname = canonical_name(signal_name);
    result << "    description: " << property.get().description << "\n"
           << "    units: " << IOGroup::units_to_string(property.get().units) << '\n'
           << "    aggregation: " << geopm::Agg::function_to_name(property.get().aggregation_function) << '\n'
           << "    domain: " << m_platform_topo.domain_type_to_name(m_driver->domain_type(cname)) << '\n'
           << "    iogroup: " << m_driver->driver();

    return result.str();
}

std::string SysfsIOGroup::control_description(const std::string &control_name) const
{
    if (!is_valid_control(control_name)) {
        throw Exception("SysfsIOGroup::control_description(): " + control_name +
                        "not valid for SysfsIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    return signal_description(control_name);
}

int SysfsIOGroup::signal_behavior(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("SysfsIOGroup::signal_behavior(): signal_name " + signal_name +
                        " not valid for SysfsIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    const auto &info = m_signals.at(signal_name);
    return info.get().behavior;
}

std::string SysfsIOGroup::name(void) const
{
    return "cpufreq";
}

std::string SysfsIOGroup::canonical_name(const std::string &name) const
{
    /// Note: all controls are also signals
    return m_signals.at(name).get().name;
}

std::string SysfsIOGroup::check_request(const std::string &method_name,
                                        const std::string &signal_name,
                                        const std::string &control_name,
                                        int domain_type,
                                        int domain_idx) const
{
    std::string cname;
    if (signal_name != "") {
        cname = signal_name;
        if (!is_valid_signal(signal_name)) {
            throw Exception("SysfsIOGroup::" + method_name + "(): \"" + signal_name +
                            "\"not valid for ",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }
    else {
        cname = control_name;
        if (!is_valid_control(control_name)) {
            throw Exception("SysfsIOGroup::" + method_name + "(): \"" + control_name +
                            "\" not valid for " + name(),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }
    cname = canonical_name(cname);
    if (domain_type != m_driver->domain_type(cname)) {
        throw Exception("SysfsIOGroup::" + method_name + "(): domain_type must be " +
                        m_platform_topo.domain_type_to_name(m_driver->domain_type(cname)) + ".",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
        throw Exception("SysfsIOGroup::" + method_name + "(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    return cname;
}
    }
