/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

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
#include "geopm/json11.hpp"

using geopm::Exception;
using geopm::PlatformTopo;
using json11::Json;

static const std::string CPUFREQ_DIRECTORY = "/sys/devices/system/cpu/cpufreq";

// Arbitrary buffer size. We're generally looking at integer values much shorter
// than 100 digits in length. The IOGroup performs string truncation checks in
// case that ever changes.
static const size_t IO_BUFFER_SIZE = 128;

static std::map<int, std::string> load_cpufreq_resources_by_cpu()
{
    struct dirent *dir;
    char cpu_buf[IO_BUFFER_SIZE] = {0};
    static std::map<int, std::string> result;
    std::unique_ptr<DIR, int (*)(DIR *)> cpufreq_directory_object(opendir(CPUFREQ_DIRECTORY.c_str()), &closedir);

    if (cpufreq_directory_object) {
        while ((dir = readdir(cpufreq_directory_object.get())) != NULL) {
            if (strncmp(dir->d_name, "policy", sizeof "policy" - 1) != 0) {
                continue;
            }

            std::ostringstream oss;
            oss << CPUFREQ_DIRECTORY << "/" << dir->d_name << "/affected_cpus";
            auto cpu_map_path = oss.str();

            int cpu_map_fd = open(cpu_map_path.c_str(), O_RDONLY);
            if (cpu_map_fd == -1) {
                throw geopm::Exception("CpufreqIOGroup failed to open " + cpu_map_path,
                                       errno, __FILE__, __LINE__);
            }
            int read_bytes = pread(cpu_map_fd, cpu_buf, sizeof cpu_buf - 1, 0);
            close(cpu_map_fd);
            if (read_bytes < 0) {
                throw geopm::Exception("CpufreqIOGroup failed to read " + cpu_map_path,
                                       errno, __FILE__, __LINE__);
            }
            if (read_bytes == sizeof cpu_buf - 1) {
                throw geopm::Exception("CpufreqIOGroup truncated read from " + cpu_map_path,
                                       errno, __FILE__, __LINE__);
            }
            result.emplace(std::stoi(cpu_buf), dir->d_name);
        }
    }
    else {
        throw geopm::Exception("CpufreqIOGroup failed to open " + CPUFREQ_DIRECTORY,
                               errno, __FILE__, __LINE__);
    }

    return result;
}

// Open a cpufreq attribute file for a given cpufreq resource. Return the opened
// fd. Caller takes ownership of closing the fd.
static int open_resource_attribute(const std::string& resource, const std::string &attribute, bool do_write)
{
    std::ostringstream oss;
    oss << CPUFREQ_DIRECTORY << "/" << resource << "/" << attribute;
    auto cpu_freq_path = oss.str();
    int fd = open(cpu_freq_path.c_str(), do_write ? O_WRONLY : O_RDONLY);
    if (fd == -1) {
        throw geopm::Exception("open_resource_attribute() failed to open " + cpu_freq_path,
                               errno, __FILE__, __LINE__);
    }
    return fd;
}

// Read a double from a cpufreq resource's opened sysfs attribute file.
static double read_resource_attribute_fd(int fd)
{
    char buf[IO_BUFFER_SIZE] = {0};
    int read_bytes = read(fd, buf, sizeof buf - 1);
    if (read_bytes < 0) {
        throw geopm::Exception("CpufreqIOGroup failed to read signal",
                               errno, __FILE__, __LINE__);
    }
    if (static_cast<size_t>(read_bytes) >= sizeof buf) {
        throw geopm::Exception("CpufreqIOGroup truncated read signal",
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    buf[read_bytes] = '\0';

    return std::stoi(buf);
}

// Write a double to a cpufreq resource's opened sysfs attribute file.
static void write_resource_attribute_fd(int fd, double value)
{
    char buf[IO_BUFFER_SIZE] = {0};
    int string_length = snprintf(buf, sizeof buf, "%lld\n",
                                 static_cast<long long int>(value));
    if (string_length < 0) {
        throw geopm::Exception("CpufreqIOGroup failed to convert signal to string",
                               errno, __FILE__, __LINE__);
    }
    if (static_cast<size_t>(string_length) >= sizeof buf) {
        throw geopm::Exception("CpufreqIOGroup truncated write control",
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }

    int write_bytes = pwrite(fd, buf, string_length, 0);
    if (write_bytes < 0) {
        throw geopm::Exception("CpufreqIOGroup failed to write control",
                               errno, __FILE__, __LINE__);
    }
    if (write_bytes < string_length) {
        throw geopm::Exception("CpufreqIOGroup truncated write control",
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
}

namespace geopm
{
    const std::string cpufreq_sysfs_json(void);

/// @brief IOGroup that provides a signals for user and idle CPU time, and
///        a control for writing to standard output.
class CpufreqIOGroup : public geopm::IOGroup
{
    public:
        CpufreqIOGroup();
        CpufreqIOGroup(const PlatformTopo &topo,
                       std::shared_ptr<SaveControl> control_saver,
                       std::shared_ptr<IOUring> batch_reader,
                       std::shared_ptr<IOUring> batch_writer);
        virtual ~CpufreqIOGroup();
        std::set<std::string> signal_names(void) const override;
        std::set<std::string> control_names(void) const override;
        bool is_valid_signal(const std::string &signal_name) const override;
        bool is_valid_control(const std::string &control_name) const override;
        int signal_domain_type(const std::string &signal_name) const override;
        int control_domain_type(const std::string &control_name) const override;
        int push_signal(const std::string &signal_name, int domain_type, int domain_idx)  override;
        int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
        void read_batch(void) override;
        void write_batch(void) override;
        double sample(int batch_idx) override;
        void adjust(int batch_idx, double setting) override;
        double read_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
        void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
        void save_control(void) override;
        void save_control(const std::string &save_path) override;
        void restore_control(void) override;
        void restore_control(const std::string &save_path) override;
        std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
        std::function<std::string(double)> format_function(const std::string &signal_name) const override;
        std::string signal_description(const std::string &signal_name) const override;
        std::string control_description(const std::string &control_name) const override;
        int signal_behavior(const std::string &signal_name) const override;
        std::string name(void) const override;
        static std::string plugin_name(void);
        static std::unique_ptr<geopm::IOGroup> make_plugin(void);
    private:
        const geopm::PlatformTopo &m_platform_topo;
        /// Whether any signal has been pushed
        bool m_do_batch_read;
        /// Whether read_batch() has been called at least once
        bool m_is_batch_read;
        bool m_is_batch_write;
        std::vector<double> m_control_value;

        // Information about a type of signal
        // TODO: merge signal and control structures, and add a "writable" field, like in MSRIO
        struct m_signal_type_info_s {
            // Sysfs attribute name
            std::string attribute;
            double scaling_factor;
            std::string description;
            std::function<double(const std::vector<double> &)> aggregation_function;
            std::function<std::string(double)> format_function;
            int behavior;
            m_units_e units;
            bool is_writable;
        };

        static std::vector<CpufreqIOGroup::m_signal_type_info_s> parse_json(
                const std::string& json_text);

        const std::vector<struct m_signal_type_info_s> m_signal_type_info;

        // Maps names to indices into m_signal_type_info
        std::map<std::string, unsigned> m_signal_type_by_name;

        // Map of (cpu)->(cpufreq resource)
        std::map<int, std::string> m_cpufreq_resource_by_cpu;

        std::vector<bool> m_do_write;

        // Information about a single pushed signal index
        struct m_signal_info_s {
            int fd;
            unsigned signal_type;
            int cpu;
            double last_value;
            std::shared_ptr<int> last_io_return;
            std::array<char, IO_BUFFER_SIZE> buf;
        };

        // Pushed signals
        std::vector<m_signal_info_s> m_pushed_signal_info;
        std::vector<m_signal_info_s> m_pushed_control_info;
        std::shared_ptr<SaveControl> m_control_saver;
        std::shared_ptr<IOUring> m_batch_reader;
        std::shared_ptr<IOUring> m_batch_writer;
};

std::vector<CpufreqIOGroup::m_signal_type_info_s> CpufreqIOGroup::parse_json(
        const std::string& json_text)
{
    std::string err;
    Json root = Json::parse(json_text, err);
    if (!err.empty() || !root.is_object()) {
        throw Exception("CpufreqIOGroup::" + std::string(__func__) +
                            "(): detected a malformed json string: " + err,
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    if (!root.has_shape({{"attributes", Json::OBJECT}}, err)) {
        throw Exception("CpufreqIOGroup::" + std::string(__func__) +
                            "(): root of json string is malformed: " + err,
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    std::vector<CpufreqIOGroup::m_signal_type_info_s> signals;

    const auto& attribute_object = root["attributes"].object_items();
    for (const auto &signal_json : attribute_object) {
        const auto &signal_name = signal_json.first;
        const auto &signal_properties = signal_json.second;

        if (!signal_properties.has_shape({
                    {"attribute", Json::STRING},
                    {"scalar", Json::NUMBER},
                    {"description", Json::STRING},
                    {"aggregation", Json::STRING},
                    {"behavior", Json::STRING},
                    {"units", Json::STRING},
                    {"writeable", Json::BOOL}}, err)) {
            throw Exception("CpufreqIOGroup::" + std::string(__func__) +
                                "(): " + signal_name +
                                " json properties are malformed: " + err,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        signals.push_back({
                .attribute = signal_properties["attribute"].string_value(),
                .scaling_factor = signal_properties["scalar"].number_value(),
                .description = signal_properties["description"].string_value(),
                .aggregation_function = Agg::name_to_function(signal_properties["aggregation"].string_value()),
                .format_function = geopm::string_format_double,
                .behavior = IOGroup::string_to_behavior(signal_properties["behavior"].string_value()),
                .units = IOGroup::string_to_units(signal_properties["units"].string_value()),
                .is_writable = signal_properties["writeable"].bool_value()});
    }

    return signals;
}

CpufreqIOGroup::CpufreqIOGroup()
    : CpufreqIOGroup(platform_topo(), nullptr, nullptr, nullptr)
{
}

// Set up mapping between signal and control names and corresponding indices
CpufreqIOGroup::CpufreqIOGroup(
        const PlatformTopo &topo,
        std::shared_ptr<SaveControl> control_saver,
        std::shared_ptr<IOUring> batch_reader,
        std::shared_ptr<IOUring> batch_writer)
    : m_platform_topo(topo)
    , m_do_batch_read(false)
    , m_is_batch_read(false)
    , m_is_batch_write(false)
    , m_control_value{}
    , m_signal_type_info(parse_json(cpufreq_sysfs_json()))
    , m_signal_type_by_name()
    , m_cpufreq_resource_by_cpu(load_cpufreq_resources_by_cpu())
    , m_do_write(m_signal_type_info.size(), false)
    , m_pushed_signal_info{}
    , m_pushed_control_info{}
    , m_control_saver(control_saver)
    , m_batch_reader(batch_reader)
    , m_batch_writer(batch_writer)
{
    for (size_t signal_type_index = 0; signal_type_index < m_signal_type_info.size(); ++signal_type_index) {
        std::string signal_name = "CPUFREQ::" + m_signal_type_info[signal_type_index].attribute;
        std::transform(signal_name.begin(), signal_name.end(),
                       signal_name.begin(),
                       [](unsigned char c){ return std::toupper(c); });
        m_signal_type_by_name.emplace(signal_name, signal_type_index);
    }
}

CpufreqIOGroup::~CpufreqIOGroup()
{
    for (const auto &info : m_pushed_signal_info) {
        close(info.fd);
    }
    for (const auto &info : m_pushed_control_info) {
        close(info.fd);
    }
}

// Extract the set of all signal names from the index map
std::set<std::string> CpufreqIOGroup::signal_names(void) const
{
    std::set<std::string> result;
    for (const auto &sv : m_signal_type_by_name) {
        result.insert(sv.first);
    }
    return result;
}

// Extract the set of all control names from the index map
std::set<std::string> CpufreqIOGroup::control_names(void) const
{
    std::set<std::string> result;
    for (const auto &sv : m_signal_type_by_name) {
        auto &control_type_info = m_signal_type_info.at(sv.second);
        if (control_type_info.is_writable) {
            result.insert(sv.first);
        }
    }
    return result;
}

// Check signal name using index map
bool CpufreqIOGroup::is_valid_signal(const std::string &signal_name) const
{
    return m_signal_type_by_name.find(signal_name) != m_signal_type_by_name.end();
}

// Check control name using index map
bool CpufreqIOGroup::is_valid_control(const std::string &control_name) const
{
    bool is_valid = false;
    auto control_it = m_signal_type_by_name.find(control_name);
    if (control_it != m_signal_type_by_name.end()) {
        auto &control_type_info = m_signal_type_info.at(control_it->second);
        is_valid = control_type_info.is_writable;
    }
    return is_valid;
}

// Return domain for all valid signals
int CpufreqIOGroup::signal_domain_type(const std::string &signal_name) const
{
    int result = GEOPM_DOMAIN_INVALID;
    if (is_valid_signal(signal_name)) {
        result = GEOPM_DOMAIN_CPU;
    }
    return result;
}

// Return domain for all valid controls
int CpufreqIOGroup::control_domain_type(const std::string &control_name) const
{
    int result = GEOPM_DOMAIN_INVALID;
    if (is_valid_control(control_name)) {
        result = GEOPM_DOMAIN_CPU;
    }
    return result;
}

// Mark the given signal to be read by read_batch()
int CpufreqIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("CpufreqIOGroup::push_signal(): signal_name " + signal_name +
                        " not valid for CpufreqIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != GEOPM_DOMAIN_CPU) {
        throw Exception("CpufreqIOGroup::push_signal(): domain_type must be GEOPM_DOMAIN_CPU.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_CPU)) {
        throw Exception("CpufreqIOGroup::push_signal(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (m_is_batch_read) {
        throw Exception("CpufreqIOGroup::push_signal(): cannot push signal after call to read_batch().",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    unsigned signal_type = m_signal_type_by_name.at(signal_name);
    auto &signal_type_info = m_signal_type_info.at(signal_type);
    auto pushed_it = std::find_if(m_pushed_signal_info.begin(),
                                  m_pushed_signal_info.end(),
                                  [signal_type, domain_idx] (const m_signal_info_s &info) {
                                      return info.signal_type == signal_type && info.cpu == domain_idx;
                                  });
    int signal_idx = -1;

    if (pushed_it != m_pushed_signal_info.end()) {
        // This has already been pushed. Return the same index as before.
        signal_idx = std::distance(m_pushed_signal_info.begin(), pushed_it);
    }
    else {
        auto resource_it = m_cpufreq_resource_by_cpu.find(domain_idx);
        if (resource_it == m_cpufreq_resource_by_cpu.end()) {
            throw Exception("CpufreqIOGroup::push_signal(): Cannot push CPU "
                            + std::to_string(domain_idx)
                            + " because it does not have a cpufreq entry.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int fd = open_resource_attribute(resource_it->second, signal_type_info.attribute, false);

        // This is a newly-pushed signal. Give it a new index.
        m_pushed_signal_info.push_back(m_signal_info_s{fd, signal_type, domain_idx, NAN, std::make_shared<int>(0), {0}});
        signal_idx = m_pushed_signal_info.size() - 1;
    }

    m_do_batch_read = true;
    return signal_idx;
}

// Mark the given control to be written by write_batch()
int CpufreqIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
{
    if (m_is_batch_write) {
        throw Exception("CpufreqIOGroup::push_control(): Cannot push control " + control_name +
                        "because batch writes have already been triggered.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (!is_valid_control(control_name)) {
        throw Exception("CpufreqIOGroup::push_control(): control_name " + control_name +
                        " not valid for CpufreqIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != GEOPM_DOMAIN_CPU) {
        throw Exception("CpufreqIOGroup::push_control(): domain_type must be GEOPM_DOMAIN_CPU.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_CPU)) {
        throw Exception("CpufreqIOGroup::push_control(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    unsigned writable_signal_type = m_signal_type_by_name.at(control_name);
    auto &writable_signal_type_info = m_signal_type_info.at(writable_signal_type);
    auto pushed_it = std::find_if(m_pushed_control_info.begin(),
                                  m_pushed_control_info.end(),
                                  [writable_signal_type, domain_idx] (const m_signal_info_s &info) {
                                      return info.signal_type == writable_signal_type && info.cpu == domain_idx;
                                  });
    int control_idx = -1;

    if (pushed_it != m_pushed_control_info.end()) {
        // This has already been pushed. Return the same index as before.
        control_idx = std::distance(m_pushed_control_info.begin(), pushed_it);
    }
    else {
        auto resource_it = m_cpufreq_resource_by_cpu.find(domain_idx);
        if (resource_it == m_cpufreq_resource_by_cpu.end()) {
            throw Exception("CpufreqIOGroup::push_control(): Cannot push CPU "
                            + std::to_string(domain_idx)
                            + " because it does not have a cpufreq entry.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int fd = open_resource_attribute(resource_it->second, writable_signal_type_info.attribute, true);

        // This is a newly-pushed control. Give it a new index.
        m_pushed_control_info.push_back(m_signal_info_s{fd, writable_signal_type, domain_idx, NAN, std::make_shared<int>(0), {0}});
        control_idx = m_pushed_control_info.size() - 1;
    }
    return control_idx;
}

void CpufreqIOGroup::read_batch(void)
{
    m_is_batch_read = true;
    if (m_do_batch_read) {
        if (!m_batch_reader) {
            m_batch_reader = IOUring::make_unique(m_pushed_signal_info.size());
        }
        for (auto &info : m_pushed_signal_info) {
            m_batch_reader->prep_read(
                info.last_io_return, info.fd, info.buf.data(), info.buf.size(), 0);
        }
        m_batch_reader->submit();
        for (auto &info : m_pushed_signal_info) {
            if (*info.last_io_return < 0) {
                throw geopm::Exception("CpufreqIOGroup failed to read signal",
                                       errno, __FILE__, __LINE__);
            }
            size_t bytes_read = static_cast<size_t>(*info.last_io_return);
            if (bytes_read >= info.buf.size()) {
                throw geopm::Exception("CpufreqIOGroup truncated read signal",
                                       GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            info.buf[bytes_read] = '\0';

            info.last_value = std::stoi(info.buf.data()) *
                m_signal_type_info.at(info.signal_type).scaling_factor;
        }
    }
}

void CpufreqIOGroup::write_batch(void)
{
    m_is_batch_write = true;
    if (!m_batch_writer) {
        m_batch_writer = IOUring::make_unique(m_pushed_signal_info.size());
    }

    for (size_t i = 0; i < m_pushed_control_info.size(); ++i) {
        auto &info = m_pushed_control_info[i];
        if (m_do_write[i] && !std::isnan(info.last_value)) {
            auto scaled_value = info.last_value / m_signal_type_info.at(info.signal_type).scaling_factor;
            int string_length = snprintf(info.buf.data(), info.buf.size(), "%lld\n",
                                         static_cast<long long int>(scaled_value));
            if (string_length < 0) {
                throw geopm::Exception("CpufreqIOGroup failed to convert signal to string",
                                       errno, __FILE__, __LINE__);
            }
            if (static_cast<size_t>(string_length) >= info.buf.size()) {
                throw geopm::Exception("CpufreqIOGroup truncated write control",
                                       GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_batch_writer->prep_write(
                info.last_io_return, info.fd, info.buf.data(),
                static_cast<unsigned>(string_length), 0);
        }
    }
    m_batch_writer->submit();
    for (size_t i = 0; i < m_pushed_control_info.size(); ++i) {
        auto &info = m_pushed_control_info[i];
        if (m_do_write[i] && !std::isnan(info.last_value)) {
            if (*info.last_io_return < 0) {
                throw geopm::Exception("CpufreqIOGroup failed to write control",
                                       errno, __FILE__, __LINE__);
            }
        }
    }
}

double CpufreqIOGroup::sample(int batch_idx)
{
    if (batch_idx < 0 || static_cast<size_t>(batch_idx) >= m_pushed_signal_info.size()) {
        throw Exception("CpufreqIOGroup::sample(): batch_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (!m_is_batch_read) {
        throw Exception("CpufreqIOGroup::sample(): signal has not been read.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    return m_pushed_signal_info[static_cast<size_t>(batch_idx)].last_value;
}

// Save a setting to be written by a future write_batch()
void CpufreqIOGroup::adjust(int batch_idx, double setting)
{
    if (batch_idx < 0 || static_cast<size_t>(batch_idx) >= m_pushed_control_info.size()) {
        throw Exception("CpufreqIOGroup::adjust(): batch_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (m_pushed_control_info[static_cast<size_t>(batch_idx)].last_value != setting) {
        m_do_write[static_cast<size_t>(batch_idx)] = true;
        m_pushed_control_info[static_cast<size_t>(batch_idx)].last_value = setting;
    }
}

double CpufreqIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("CpufreqIOGroup:read_signal(): " + signal_name +
                        "not valid for CpufreqIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != GEOPM_DOMAIN_CPU) {
        throw Exception("CpufreqIOGroup::push_signal(): domain_type must be GEOPM_DOMAIN_CPU.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_CPU)) {
        throw Exception("CpufreqIOGroup::push_signal(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    auto resource_it = m_cpufreq_resource_by_cpu.find(domain_idx);
    if (resource_it == m_cpufreq_resource_by_cpu.end()) {
        throw Exception("CpufreqIOGroup::read_signal(): Cannot push CPU "
                        + std::to_string(domain_idx)
                        + " because it does not have a cpufreq entry.",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    unsigned signal_type = m_signal_type_by_name.at(signal_name);
    auto &signal_type_info = m_signal_type_info.at(signal_type);

    int fd = open_resource_attribute(resource_it->second, signal_type_info.attribute, false);
    return read_resource_attribute_fd(fd) * signal_type_info.scaling_factor;
}

// Write to the control immediately, bypassing write_batch()
void CpufreqIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
{
    if (!is_valid_control(control_name)) {
        throw Exception("CpufreqIOGroup:write_control(): " + control_name +
                        "not valid for CpufreqIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_type != GEOPM_DOMAIN_CPU) {
        throw Exception("CpufreqIOGroup::push_control(): domain_type must be GEOPM_DOMAIN_CPU.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_CPU)) {
        throw Exception("CpufreqIOGroup::push_control(): domain_idx out of range.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    auto resource_it = m_cpufreq_resource_by_cpu.find(domain_idx);
    if (resource_it == m_cpufreq_resource_by_cpu.end()) {
        throw Exception("CpufreqIOGroup::write_control(): Cannot push CPU "
                        + std::to_string(domain_idx)
                        + " because it does not have a cpufreq entry.",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    int writable_signal_type = m_signal_type_by_name.at(control_name);
    auto &writable_signal_type_info = m_signal_type_info.at(writable_signal_type);

    int fd = open_resource_attribute(resource_it->second, writable_signal_type_info.attribute, true);
    write_resource_attribute_fd(fd, setting / writable_signal_type_info.scaling_factor);
}

void CpufreqIOGroup::save_control(void)
{
    if (!m_control_saver) {
        m_control_saver = SaveControl::make_unique(*this);
    }
}

void CpufreqIOGroup::save_control(const std::string &save_path)
{
    if (!m_control_saver) {
        m_control_saver = SaveControl::make_unique(*this);
    }
    m_control_saver->write_json(save_path);
}

void CpufreqIOGroup::restore_control(void)
{
    if (m_control_saver) {
        m_control_saver->restore(*this);
    }
}

void CpufreqIOGroup::restore_control(const std::string &save_path)
{
    if (!m_control_saver) {
        m_control_saver = SaveControl::make_unique(geopm::read_file(save_path));
    }
    m_control_saver->restore(*this);
}

std::function<double(const std::vector<double> &)> CpufreqIOGroup::agg_function(
        const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("CpufreqIOGroup:agg_function(): " + signal_name +
                        "not valid for CpufreqIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    unsigned signal_type = m_signal_type_by_name.at(signal_name);
    auto &info = m_signal_type_info.at(signal_type);
    return info.aggregation_function;
}

std::function<std::string(double)> CpufreqIOGroup::format_function(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("CpufreqIOGroup::format_function(): " + signal_name +
                        "not valid for TimeIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    unsigned signal_type = m_signal_type_by_name.at(signal_name);
    auto &info = m_signal_type_info.at(signal_type);
    return info.format_function;
}

// A user-friendly description of each signal
std::string CpufreqIOGroup::signal_description(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("CpufreqIOGroup::signal_description(): signal_name " + signal_name +
                        " not valid for CpufreqIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    unsigned signal_type = m_signal_type_by_name.at(signal_name);
    auto &info = m_signal_type_info.at(signal_type);
    std::ostringstream result;
    result << "    description: " << info.description << "\n"
           << "    units: " << IOGroup::units_to_string(M_UNITS_SECONDS) << '\n'
           << "    aggregation: " << geopm::Agg::function_to_name(info.aggregation_function) << '\n'
           << "    domain: " << m_platform_topo.domain_type_to_name(GEOPM_DOMAIN_CPU) << '\n'
           << "    iogroup: " << name();

    return result.str();
}

std::string CpufreqIOGroup::control_description(const std::string &control_name) const
{
    if (!is_valid_control(control_name)) {
        throw Exception("CpufreqIOGroup::control_description(): " + control_name +
                        "not valid for CpufreqIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    return signal_description(control_name);
}

int CpufreqIOGroup::signal_behavior(const std::string &signal_name) const
{
    if (!is_valid_signal(signal_name)) {
        throw Exception("CpufreqIOGroup::signal_behavior(): signal_name " + signal_name +
                        " not valid for CpufreqIOGroup.",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    unsigned signal_type = m_signal_type_by_name.at(signal_name);
    auto &info = m_signal_type_info.at(signal_type);
    return info.behavior;
}

std::string CpufreqIOGroup::name(void) const
{
    return plugin_name();
}

// Name used for registration with the IOGroup factory
std::string CpufreqIOGroup::plugin_name(void)
{
    return "cpufreq";
}

// Function used by the factory to create objects of this type
std::unique_ptr<geopm::IOGroup> CpufreqIOGroup::make_plugin(void)
{
    return std::unique_ptr<geopm::IOGroup>(new CpufreqIOGroup);
}

// Registers this IOGroup with the IOGroup factory, making it visible
// to PlatformIO when the plugin is first loaded.
static void __attribute__((constructor)) load_iogroup(void)
{
    try {
        geopm::iogroup_factory().register_plugin(CpufreqIOGroup::plugin_name(),
                                                 CpufreqIOGroup::make_plugin);
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Error: unknown cause" << std::endl;
    }
}
}
