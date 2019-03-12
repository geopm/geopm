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

#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>

#include <fstream>
#include <algorithm>
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    std::string read_file(const std::string& path)
    {
        std::string contents;
        std::ifstream input_file(path, std::ifstream::in);
        if (!input_file.is_open()) {
            throw Exception("Helper::" + std::string(__func__) + "(): file \"" + path +
                            "\" could not be opened", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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
        input_file.close();

        return contents;
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
                pieces.push_back(str.substr(start_pos, del_pos-start_pos));
                start_pos = del_pos + 1;
                del_pos = str.find(delim, start_pos);
            }
            /// add the last piece
            pieces.push_back(str.substr(start_pos, del_pos-start_pos));
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
}
