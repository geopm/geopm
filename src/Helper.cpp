/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "Helper.hpp"

#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include <climits>
#include <cinttypes>
#include <fstream>
#include <algorithm>
#include "geopm_hash.h"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    std::string read_file(const std::string &path)
    {
        std::string contents;
        std::ifstream input_file(path, std::ifstream::in);
        if (!input_file.is_open()) {
            throw Exception("Helper::" + std::string(__func__) + "(): file \"" + path +
                            "\" could not be opened",
                            errno ? errno : GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        input_file.seekg(0, std::ios::end);
        size_t file_size = input_file.tellg();
        if (file_size <= 0) {
            throw Exception("Helper::" + std::string(__func__) + "(): input file invalid",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        contents.resize(file_size);
        input_file.seekg(0, std::ios::beg);
        input_file.read(&contents[0], file_size);
        return contents;
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
        snprintf(result, NAME_MAX, "%lld", (long long)signal);
        return result;
    }

    std::string string_format_hex(double signal)
    {
        char result[NAME_MAX];
        snprintf(result, NAME_MAX, "0x%016" PRIx64, (uint64_t)signal);
        return result;
    }

    std::string string_format_raw64(double signal)
    {
        char result[NAME_MAX];
        snprintf(result, NAME_MAX, "0x%016" PRIx64, geopm_signal_to_field(signal));
        return result;
    }
}
