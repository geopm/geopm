/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "geopm/Helper.hpp"

#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <cmath>
#include <climits>
#include <cinttypes>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include "geopm_field.h"
#include "geopm/Exception.hpp"
#include "config.h"

namespace geopm
{
    std::string read_file(const std::string &path)
    {
        std::ifstream input_file(path, std::ifstream::in);
        if (!input_file.is_open()) {
            throw Exception("Helper::" + std::string(__func__) + "(): file \"" + path +
                            "\" could not be opened",
                            errno ? errno : GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::stringstream buffer;
        buffer << input_file.rdbuf();
        input_file.close();
        if (!buffer.good()) {
            throw Exception("Helper::" + std::string(__func__) + "(): input file invalid",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return buffer.str();
    }

    double read_double_from_file(const std::string &path, const std::string &expected_units)
    {
        const std::string separators(" \t\n\0", 4);
        auto file_contents = read_file(path);
        size_t value_length = 0;
        auto value = std::stod(file_contents, &value_length);
        auto units_offset = file_contents.find_first_not_of(separators, value_length);
        auto units_end = file_contents.find_last_not_of(separators);
        auto units_length = units_end == std::string::npos
                                ? std::string::npos
                                : units_end - units_offset + 1;
        bool units_exist = units_offset != std::string::npos;
        bool units_are_expected = !expected_units.empty();

        if ((units_exist != units_are_expected) ||
            (units_exist &&
             (units_offset == value_length ||
              file_contents.substr(units_offset, units_length) != expected_units))) {
            throw Exception("Unexpected format in " + path, GEOPM_ERROR_RUNTIME,
                            __FILE__, __LINE__);
        }
        return value;
    }

    void write_file(const std::string &path, const std::string &contents)
    {
        std::ofstream output_file(path, std::ofstream::out);
        if (!output_file.is_open()) {
            throw Exception("Helper::" + std::string(__func__) + "(): file \"" + path +
                            "\" could not be opened for writing",
                            errno ? errno : GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        output_file.seekp(0, std::ios::beg);
        output_file.write(contents.c_str(), contents.size());
    }

    std::vector<std::string> string_split(const std::string &str,
                                          const std::string &delim)
    {
        if (delim.empty()) {
            throw Exception("Helper::" + std::string(__func__) + "(): invalid delimiter",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::vector<std::string> pieces;
        if (!str.empty()) {
            size_t start_pos = 0;
            size_t del_pos = str.find(delim);
            while (del_pos != std::string::npos) {
                pieces.push_back(str.substr(start_pos, del_pos - start_pos));
                start_pos = del_pos + delim.size();
                del_pos = str.find(delim, start_pos);
            }
            // add the last piece
            pieces.push_back(str.substr(start_pos));
        }
        return pieces;
    }

    std::string string_join(const std::vector<std::string> &list,
                            const std::string &delim)
    {
        std::ostringstream result;
        if (!list.empty()) {
            auto back_it = list.end() - 1;
            for (auto str_it = list.begin(); str_it != back_it; ++str_it) {
                result << *str_it << delim;
            }
            result << *back_it;
        }
        return result.str();
    }

    std::string hostname(void)
    {
        char hostname[NAME_MAX];
        hostname[NAME_MAX - 1] = '\0';
        int err = gethostname(hostname, NAME_MAX - 1);
        if (err) {
            throw Exception("Helper::hostname() gethostname() failed", err, __FILE__, __LINE__);
        }
        return hostname;
    }

    std::vector<std::string> list_directory_files(const std::string &path)
    {
        std::vector<std::string> file_list;
        DIR *did = opendir(path.c_str());
        if (did) {
            struct dirent *entry;
            while ((entry = readdir(did))) {
                file_list.emplace_back(entry->d_name);
            }
            closedir(did);
        }
        else if (path != GEOPM_DEFAULT_PLUGIN_PATH) {
            // Default plugin path may not be valid in some cases, e.g. when running unit tests
            // before installing
            throw Exception("Helper::" + std::string(__func__) + "(): failed to open directory '" + path + "': " + strerror(errno),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return file_list;
    }

    bool string_begins_with(const std::string &str, const std::string &key)
    {
        return (str.find(key) == 0);
    }

    bool string_ends_with(std::string str, std::string key)
    {
        std::reverse(str.begin(), str.end());
        std::reverse(key.begin(), key.end());
        return string_begins_with(str, key);
    }

    std::string string_format_double(double signal)
    {
        char result[NAME_MAX];
        snprintf(result, NAME_MAX, "%.16g", signal);
        return result;
    }

    std::string string_format_float(double signal)
    {
        char result[NAME_MAX];
        snprintf(result, NAME_MAX, "%g", signal);
        return result;
    }

    std::string string_format_integer(double signal)
    {
        char result[NAME_MAX];
        if (std::isnan(signal)) {
            snprintf(result, NAME_MAX, "%g", signal);
        }
        else {
            snprintf(result, NAME_MAX, "%lld", (long long)signal);
        }
        return result;
    }

    std::string string_format_hex(double signal)
    {
        if (std::isnan(signal)) {
            return "NAN";
        }
        char result[NAME_MAX];
        snprintf(result, NAME_MAX, "0x%08" PRIx64, (uint64_t)signal);
        return result;
    }

    std::string string_format_raw64(double signal)
    {
        char result[NAME_MAX];
        snprintf(result, NAME_MAX, "0x%016" PRIx64, geopm_signal_to_field(signal));
        return result;
    }


    std::function<std::string(double)> string_format_type_to_function(int format_type)
    {
        static const std::map<int, std::function<std::string(double)> > function_map {
            {STRING_FORMAT_DOUBLE, string_format_double},
            {STRING_FORMAT_INTEGER, string_format_integer},
            {STRING_FORMAT_HEX, string_format_hex},
            {STRING_FORMAT_RAW64, string_format_raw64},
        };
        auto it = function_map.find(format_type);
        if (it == function_map.end()) {
            throw Exception("geopm::string_format_function(): format_type out of range: " + std::to_string(format_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    int string_format_function_to_type(std::function<std::string(double)> format_function)
    {
        std::map<decltype(&string_format_double), int> function_map = {
            {string_format_double, STRING_FORMAT_DOUBLE},
            {string_format_integer, STRING_FORMAT_INTEGER},
            {string_format_hex, STRING_FORMAT_HEX},
            {string_format_raw64, STRING_FORMAT_RAW64},
        };
        auto f_ref = *(format_function.target<decltype(&string_format_double)>());
        auto result = function_map.find(f_ref);
        if (result == function_map.end()) {
            throw Exception("string_format_function_to_type(): unknown format function.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result->second;
    }

    std::string get_env(const std::string &name)
    {
        std::string env_string;
        char *check_string = getenv(name.c_str());
        if (check_string != NULL) {
            env_string = check_string;
        }
        return env_string;
    }

    unsigned int pid_to_uid(const int pid) {
        int err = 0;
        std::string proc_path = "/proc/" + std::to_string(pid);
        struct stat stat_struct;
        err = stat(proc_path.c_str(), &stat_struct);
        if (err) {
            throw Exception("pid_to_uid(): ", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return stat_struct.st_uid;
    };

    unsigned int pid_to_gid(const int pid) {
        int err = 0;
        std::string proc_path = "/proc/" + std::to_string(pid);
        struct stat stat_struct;
        err = stat(proc_path.c_str(), &stat_struct);
        if (err) {
            throw Exception("pid_to_gid(): ", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return stat_struct.st_gid;
    };

    std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
        make_cpu_set(int num_cpu, const std::set<int> &cpu_enabled)
    {
        if (num_cpu < 128) {
            num_cpu = 128;
        }
        std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> > result(
            CPU_ALLOC(num_cpu),
            [](cpu_set_t *ptr)
            {
                CPU_FREE(ptr);
            });

        auto enabled_it = cpu_enabled.cbegin();
        for (int cpu_idx = 0; cpu_idx != num_cpu; ++cpu_idx) {
            if (enabled_it != cpu_enabled.cend() &&
                *enabled_it == cpu_idx) {
                CPU_SET(cpu_idx, result.get());
                ++enabled_it;
            }
            else {
                CPU_CLR(cpu_idx, result.get());
            }
        }
        return result;
    }
}
