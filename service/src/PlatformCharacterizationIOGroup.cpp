/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "PlatformCharacterizationIOGroup.hpp"

#include <cmath>

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <algorithm>
#include <string>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "geopm_time.h"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "SaveControl.hpp"

namespace geopm
{
    const std::string PlatformCharacterizationIOGroup::M_PLUGIN_NAME = "NODE_CHARACTERIZATION";
    const std::string PlatformCharacterizationIOGroup::M_NAME_PREFIX = M_PLUGIN_NAME + "::";
    const std::string PlatformCharacterizationIOGroup::M_CACHE_FILE_NAME = "/tmp/geopm-characterization-cache-" +
                                                                        std::to_string(getuid());
    const std::string PlatformCharacterizationIOGroup::M_SERVICE_CACHE_FILE_NAME = "/run/geopm-service/geopm-characterization-cache";

    PlatformCharacterizationIOGroup::PlatformCharacterizationIOGroup()
        : PlatformCharacterizationIOGroup(platform_topo(), "")
    {
    }

    // Set up mapping between signal and control names and corresponding indices
    PlatformCharacterizationIOGroup::PlatformCharacterizationIOGroup(const PlatformTopo &platform_topo,
                                                                     const std::string &test_cache_file_name)
        : m_platform_topo(platform_topo)
        , M_TEST_CACHE_FILE_NAME(test_cache_file_name)
        , m_is_batch_read(false)
        , m_signal_available({{M_NAME_PREFIX + "GPU_CORE_FREQUENCY_EFFICIENT", {
                                  "GPU Compute Domain energy efficient frequency in hertz.",
                                  {},
                                  GEOPM_DOMAIN_BOARD,
                                  Agg::average,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "CPU_CORE_FREQUENCY_EFFICIENT", {
                                  "CPU Core Domain energy efficient frequency in hertz.",
                                  {},
                                  GEOPM_DOMAIN_BOARD,
                                  Agg::average,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                              {M_NAME_PREFIX + "CPU_UNCORE_FREQUENCY_EFFICIENT", {
                                  "CPU Uncore Domain energy efficient frequency in hertz.",
                                  {},
                                  GEOPM_DOMAIN_BOARD,
                                  Agg::average,
                                  IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                  string_format_double
                                  }},
                             })
    {
        for (int unc_entry = 0; unc_entry < 15; unc_entry++) {
            m_signal_available[M_NAME_PREFIX + "CPU_UNCORE_FREQUENCY_" + std::to_string(unc_entry)] = {
                                          "CPU Uncore frequency associated with "
                                          "CPU_UNCORE_MAXIMIMUM_MEMORY_BANDWIDTH_" + std::to_string(unc_entry),
                                          {},
                                          GEOPM_DOMAIN_BOARD,
                                          Agg::average,
                                          IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                          string_format_double};

            m_signal_available[M_NAME_PREFIX + "CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_" + std::to_string(unc_entry)] = {
                                          "Maximum memory bandwidth associated with "
                                          "CPU_UNCORE_FREQUENCY_" + std::to_string(unc_entry),
                                          {},
                                          GEOPM_DOMAIN_BOARD,
                                          Agg::average,
                                          IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE,
                                          string_format_double};
        }

        // populate signals for each domain
        std::vector<std::string> control_signal_names;
        for (auto &sv : m_signal_available) {
            std::vector<std::shared_ptr<signal_s> > result;
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(signal_domain_type(sv.first)); ++domain_idx) {
                std::shared_ptr<signal_s> sgnl = std::make_shared<signal_s>(signal_s{0, false});
                result.push_back(sgnl);
            }
            sv.second.signals = result;
            control_signal_names.push_back(sv.first);
        }

        // This IO Group is simply reading & writing to a file, which means
        // all signals directly map to controls and vice versa.
        for (auto &sv : m_signal_available) {
            std::vector<std::shared_ptr<control_s> > result;
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(signal_domain_type(sv.first)); ++domain_idx) {
                std::shared_ptr<control_s> ctrl = std::make_shared<control_s>(control_s{0, false});
                result.push_back(ctrl);
            }
            m_control_available[sv.first] = {sv.second.m_description,
                                             result,
                                             sv.second.domain,
                                             sv.second.agg_function,
                                             sv.second.format_function};
        }

        // TODO: the primary reason to cache this as a member variable is to use it for writing later.
        //       right now I'm still directly writing all values to file using ofstream in the write_control
        //       function, so this does not currently need to be a member variable
        m_cache_contents = read_cache();
        parse_cache(m_cache_contents);
    }

    void PlatformCharacterizationIOGroup::parse_cache(const std::string cache_contents) {
        //TODO:
        std::istringstream content_stream (cache_contents);

        // parse each line
        std::string line;
        while (std::getline(content_stream, line)) {
            // split each space delimited line into a vector
            std::stringstream ss_line(line);
            std::vector<std::string> content = std::vector<std::string> (std::istream_iterator<std::string>(ss_line),
                                             std::istream_iterator<std::string>());

            // The file format is 'SIGNAL DOMAIN DOMAIN_IDX STORED_VALUE'.
            // Any deviations from this should cause an exception
            if (content.size() != 4 || // wrong number of entries in a line
                m_signal_available.find(content.at(0)) == m_signal_available.end() || // signal in file not supported
                m_signal_available.at(content.at(0)).domain != (int)std::stoi(content.at(1)) || // domain in file mismatches signal domain
                m_signal_available.at(content.at(0)).signals.size() < (unsigned int)std::stoi(content.at(2)) ){ // file domain value exceeds domain count
                throw Exception("PlatformCharacterization::parse_cache(): Invalid characterization line: " +
                                line, GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            else {
                m_signal_available.at(content.at(0)).signals.at(std::stoi(content.at(2)))->m_value = std::stod(content.at(3));
            }
        }

    }

    std::string PlatformCharacterizationIOGroup::read_cache(void) {
        std::string result;
        if (M_TEST_CACHE_FILE_NAME.size()) {
            create_cache(M_TEST_CACHE_FILE_NAME);
            result = geopm::read_file(M_TEST_CACHE_FILE_NAME);
            active_cache_file = M_TEST_CACHE_FILE_NAME;
        }
        else {
            try {
                create_cache(M_SERVICE_CACHE_FILE_NAME);
                result = geopm::read_file(M_SERVICE_CACHE_FILE_NAME);
                active_cache_file = M_SERVICE_CACHE_FILE_NAME;
            }
            catch (const geopm::Exception &ex) {
                create_cache(M_CACHE_FILE_NAME);
                result = geopm::read_file(M_CACHE_FILE_NAME);
                active_cache_file = M_CACHE_FILE_NAME;
            }
        }
        return result;
    }

    void PlatformCharacterizationIOGroup::create_cache(void)
    {
        try {
            PlatformCharacterizationIOGroup::create_cache(M_SERVICE_CACHE_FILE_NAME);
        }
        catch (const geopm::Exception &ex) {
            PlatformCharacterizationIOGroup::create_cache(M_CACHE_FILE_NAME);
        }
    }

    void PlatformCharacterizationIOGroup::create_cache(const std::string &cache_file_name)
    {
        // If cache file is not present, or is too old, create it
        bool is_file_ok = false;
        try {
            is_file_ok = check_file(cache_file_name);
        }
        catch (const geopm::Exception &ex) {
            // sysinfo or stat failed; file does not exist (2) or permission denied (13)
            if (ex.err_value() == EACCES) {
                throw; // Permission was denied; Cannot create files at the desired path
            }
        }

        if (is_file_ok == false) {
            mode_t perms;
            if (cache_file_name == M_SERVICE_CACHE_FILE_NAME) {
                perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0o644
            }
            else {
                perms = S_IRUSR | S_IWUSR; // 0o600
            }

            std::string tmp_string = cache_file_name + "XXXXXX";
            char tmp_path[NAME_MAX];
            tmp_path[NAME_MAX - 1] = '\0';
            strncpy(tmp_path, tmp_string.c_str(), NAME_MAX - 1);
            int tmp_fd = mkstemp(tmp_path);
            if (tmp_fd == -1) {
                throw Exception("PlatformCharacterization::create_cache(): Could not create temp file: ",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            int err = fchmod(tmp_fd, perms);
            if (err) {
                close(tmp_fd);
                throw Exception("PlatformCharacterization::create_cache(): Could not chmod tmp_path: ",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            close(tmp_fd);

            // TODO: do we care about saving as temp and renaming for this?  It made sense for the
            //       platformtopo since lscpu populates it here, but we're not modifying the file
            std::ofstream tmp_file(tmp_path);
            for (auto &sv : m_signal_available) {
                // Only save signals, excluding any that are related to controls
                for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(signal_domain_type(sv.first)); ++domain_idx) {
                    tmp_file << sv.first << " " <<  sv.second.domain <<
                                " " << domain_idx << " " <<  "0" << std::endl;
                }
            }
            tmp_file.close();

            err = rename(tmp_path, cache_file_name.c_str());
            if (err) {
                throw Exception("PlatformCharacterization::create_cache(): Could not rename tmp_path: ",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    bool PlatformCharacterizationIOGroup::check_file(const std::string &file_path)
    {
        struct sysinfo si;
        int err = sysinfo(&si);
        if (err) {
            throw Exception("PlatformCharacterizationIOGroup::check_file(): sysinfo err: ",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        struct stat file_stat;
        err = stat(file_path.c_str(), &file_stat);
        if (err) {
            throw Exception("PlatformCharacterizationIOGroup::create_cache(): stat failure:",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        struct geopm_time_s current_time;
        geopm_time_real(&current_time);

        // TODO: we don't want to overwrite with every reboot.
        //       we do want to overwrite/remove entries for hardware
        //       that doesn't match what's in the file (i.e. a piece of
        //       hardware has been replaced).
        unsigned int last_boot_time = current_time.t.tv_sec - si.uptime;
        if (file_stat.st_mtime < last_boot_time) {
            return false; // file is older than last boot
        }
        else {
            mode_t expected_perms;
            if (file_path == M_SERVICE_CACHE_FILE_NAME) {
                expected_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0o644
            }
            else {
                expected_perms = S_IRUSR | S_IWUSR; // 0o600
            }

            mode_t actual_perms = file_stat.st_mode & ~S_IFMT;
            if (expected_perms == actual_perms) {
                return true; // file has been created since boot with the right permissions
            }
            else {
                return false;
            }
        }
    }


    // Extract the set of all signal names from the index map
    std::set<std::string> PlatformCharacterizationIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_signal_available) {
            result.insert(sv.first);
        }
        return result;
    }

    // Extract the set of all control names from the index map
    std::set<std::string> PlatformCharacterizationIOGroup::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_control_available) {
            result.insert(sv.first);
        }
        return result;
    }

    // Check signal name using index map
    bool PlatformCharacterizationIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_available.find(signal_name) != m_signal_available.end();
    }

    // Check control name using index map
    bool PlatformCharacterizationIOGroup::is_valid_control(const std::string &control_name) const
    {
        return m_control_available.find(control_name) != m_control_available.end();
    }

    // Return domain for all valid signals
    int PlatformCharacterizationIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    // Return domain for all valid controls
    int PlatformCharacterizationIOGroup::control_domain_type(const std::string &control_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_control_available.find(control_name);
        if (it != m_control_available.end()) {
            result = it->second.domain;
        }
        return result;
    }

    // Mark the given signal to be read by read_batch()
    int PlatformCharacterizationIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": signal_name " + signal_name +
                            " not valid for PlatformCharacterizationIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": " + signal_name + ": domain_type must be " +
                            std::to_string(signal_domain_type(signal_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(signal_domain_type(signal_name))) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_is_batch_read) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": cannot push signal after call to read_batch().",
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
    int PlatformCharacterizationIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": control_name " + control_name +
                            " not valid for PlatformCharacterizationIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": " + control_name + ": domain_type must be " +
                            std::to_string(control_domain_type(control_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
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

    // Parse and update saved values for signals
    void PlatformCharacterizationIOGroup::read_batch(void)
    {
        m_is_batch_read = true;
        for (auto &sv : m_signal_available) {
            for (unsigned int domain_idx = 0; domain_idx < sv.second.signals.size(); ++domain_idx) {
                if (sv.second.signals.at(domain_idx)->m_do_read) {
                    sv.second.signals.at(domain_idx)->m_value = read_signal(sv.first, sv.second.domain, domain_idx);
                }
            }
        }
    }

    // Write all controls that have been pushed and adjusted
    void PlatformCharacterizationIOGroup::write_batch(void)
    {
        for (auto &sv : m_control_available) {
            for (unsigned int domain_idx = 0; domain_idx < sv.second.controls.size(); ++domain_idx) {
                if (sv.second.controls.at(domain_idx)->m_is_adjusted) {
                    write_control(sv.first, sv.second.domain, domain_idx, sv.second.controls.at(domain_idx)->m_setting);
                }
            }
        }
    }

    // Return the latest value read by read_batch()
    double PlatformCharacterizationIOGroup::sample(int batch_idx)
    {
        // Do conversion of signal values stored in read batch
        if (batch_idx < 0 || batch_idx >= (int)m_signal_pushed.size()) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": batch_idx " +std::to_string(batch_idx)+ " out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_batch_read) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": signal has not been read.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return m_signal_pushed[batch_idx]->m_value;
    }

    // Save a setting to be written by a future write_batch()
    void PlatformCharacterizationIOGroup::adjust(int batch_idx, double setting)
    {
        if (batch_idx < 0 || (unsigned)batch_idx >= m_control_pushed.size()) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + "(): batch_idx " +std::to_string(batch_idx)+ " out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_control_pushed.at(batch_idx)->m_setting = setting;
        m_control_pushed.at(batch_idx)->m_is_adjusted = true;
    }

    // Read the value of a signal immediately, bypassing read_batch().  Should not modify m_signal_value
    double PlatformCharacterizationIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": " + signal_name +
                            " not valid for PlatformCharacterizationIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": " + signal_name + ": domain_type must be " +
                            std::to_string(signal_domain_type(signal_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(signal_domain_type(signal_name))) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        double result = NAN;
        result = m_signal_available.at(signal_name).signals.at(domain_idx)->m_value;
        return result;
    }

    // Write to the control immediately, bypassing write_batch()
    void PlatformCharacterizationIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": " + control_name +
                            " not valid for PlatformCharacterizationIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": " + control_name + ": domain_type must be " +
                            std::to_string(control_domain_type(control_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(control_domain_type(control_name))) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        bool update_cache = true;

        if (is_valid_signal(control_name)) {
            m_signal_available.at(control_name).signals.at(domain_idx)->m_value = setting;
        }
        else {
            update_cache = false;
    #ifdef GEOPM_DEBUG
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + "Handling not defined for "
                            + control_name, GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
    #endif
        }

        // Make sure the cached file reflects local changes made
        if (update_cache) {
            std::ofstream tmp_file(active_cache_file);
            for (auto &sv : m_signal_available) {
                for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(signal_domain_type(sv.first)); ++domain_idx) {
                    tmp_file << sv.first << " " <<  sv.second.domain << " " <<
                                domain_idx << " " << sv.second.signals.at(domain_idx)->m_value <<
                                std::endl;
                }
            }
            tmp_file.close();
        }
    }

    // Implemented to allow an IOGroup to save platform settings before starting
    // to adjust them
    void PlatformCharacterizationIOGroup::save_control(void)
    {
    }

    // Implemented to allow an IOGroup to restore previously saved
    // platform settings
    void PlatformCharacterizationIOGroup::restore_control(void)
    {
    }

    // Hint to Agent about how to aggregate signals from this IOGroup
    std::function<double(const std::vector<double> &)> PlatformCharacterizationIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": " + signal_name +
                            "not valid for PlatformCharacterizationIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.agg_function;
    }

    // Specifies how to print signals from this IOGroup
    std::function<std::string(double)> PlatformCharacterizationIOGroup::format_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": " + signal_name +
                            "not valid for PlatformCharacterizationIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.format_function;
    }

    // A user-friendly description of each signal
    std::string PlatformCharacterizationIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": signal_name " + signal_name +
                            " not valid for PlatformCharacterizationIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_available.at(signal_name).m_description;
    }

    // A user-friendly description of each control
    std::string PlatformCharacterizationIOGroup::control_description(const std::string &control_name) const
    {
        if (!is_valid_control(control_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": " + control_name +
                            "not valid for PlatformCharacterizationIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return m_control_available.at(control_name).m_description;
    }

    int PlatformCharacterizationIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": signal_name " + signal_name +
                            " not valid for PlatformCharacterizationIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_available.at(signal_name).behavior;
    }

    void PlatformCharacterizationIOGroup::save_control(const std::string &save_path)
    {
    }

    void PlatformCharacterizationIOGroup::restore_control(const std::string &save_path)
    {
    }

    std::string PlatformCharacterizationIOGroup::name(void) const
    {
        return plugin_name();
    }

    // Name used for registration with the IOGroup factory
    std::string PlatformCharacterizationIOGroup::plugin_name(void)
    {
        return M_PLUGIN_NAME;
    }

    // Function used by the factory to create objects of this type
    std::unique_ptr<IOGroup> PlatformCharacterizationIOGroup::make_plugin(void)
    {
        return geopm::make_unique<PlatformCharacterizationIOGroup>();
    }

    void PlatformCharacterizationIOGroup::register_signal_alias(const std::string &alias_name,
                                            const std::string &signal_name)
    {
        if (m_signal_available.find(alias_name) != m_signal_available.end()) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": signal_name " + alias_name +
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

    void PlatformCharacterizationIOGroup::register_control_alias(const std::string &alias_name,
                                           const std::string &control_name)
    {
        if (m_control_available.find(alias_name) != m_control_available.end()) {
            throw Exception("PlatformCharacterizationIOGroup::" + std::string(__func__) + ": contro1_name " + alias_name +
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
