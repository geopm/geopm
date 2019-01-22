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
#include <unistd.h>
#include <fstream>
#include <limits.h>

#include "contrib/json11/json11.hpp"

#include "Exception.hpp"
#include "ModelParse.hpp"
#include "geopm_imbalancer.h"
#include "config.h"

using json11::Json;

namespace geopm
{
    void model_parse_config(const std::string config_path, uint64_t &loop_count,
                            std::vector<std::string> &region_name, std::vector<double> &big_o)
    {
        std::ifstream config_stream;
        config_stream.open(config_path, std::ifstream::in);
        if (!config_stream.is_open()) {
            throw Exception("model_parse_config(): could not open file: " + config_path,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        config_stream.seekg(0, std::ios::end);
        size_t file_size = config_stream.tellg();
        if (file_size <= 0) {
            throw Exception("model_parse_config(): file empty or invalid: " + config_path,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        config_stream.seekg(0, std::ios::beg);
        std::string config_string;
        config_string.reserve(file_size);
        config_string.assign(std::istreambuf_iterator<char>(config_stream),
                             std::istreambuf_iterator<char>());

        std::string err;
        Json root = Json::parse(config_string, err);

        if (!err.empty() || !root.is_object()) {
            throw Exception("model_parse_config(): malformed json configuration file",
                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }

        std::vector<std::string> hostname;
        std::vector<double> imbalance;
        for (const auto &obj : root.object_items()) {
            std::string key_string = obj.first;
            Json val = obj.second;
            if (key_string == "loop-count") {
                if (val.is_number() && floor(val.number_value()) == val.number_value()) {
                    loop_count = val.number_value();
                }
                else {
                    throw Exception("model_parse_config(): loop-count expected to be an integer type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "region") {
                if (val.is_array()) {
                    for (const auto &region_obj : val.array_items()) {
                        if (region_obj.is_string()) {
                            region_name.push_back(region_obj.string_value());
                        }
                        else {
                            throw Exception("model_parse_config(): region array value is not a string type",
                                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                        }
                    }
                }
                else {
                    throw Exception("model_parse_config(): region must specify an array",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "big-o") {
                if (val.is_array()) {
                    for (const auto &big_o_obj : val.array_items()) {
                        if (big_o_obj.is_number()) {
                            big_o.push_back(big_o_obj.number_value());
                        }
                        else {
                           throw Exception("model_parse_config(): big-o expected to be a double type",
                                   GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                        }
                    }
                }
                else {
                    throw Exception("model_parse_config(): big-o must specify an array",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "hostname") {
                if (val.is_array()) {
                    for (const auto &hostname_obj : val.array_items()) {
                        if (hostname_obj.is_string()) {
                            hostname.push_back(hostname_obj.string_value());
                        }
                        else {
                            throw Exception("model_parse_config(): hostname array value is not a string type",
                                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                        }
                    }
                }
                else {
                    throw Exception("model_parse_config(): hostname must specify an array",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "imbalance") {
                if (val.is_array()) {
                    for (const auto &imbalance_obj : val.array_items()) {
                        if (imbalance_obj.is_number()) {
                            imbalance.push_back(imbalance_obj.number_value());
                        }
                        else {
                           throw Exception("model_parse_config(): imbalance expected to be a double type",
                                   GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                        }
                    }
                }
                else {
                    throw Exception("model_parse_config(): imbalance must specify an array",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else {
                throw Exception("model_parse_config(): unknown key: " + key_string,
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }
        config_stream.close();

        if (region_name.size() != big_o.size() ||
            hostname.size() != imbalance.size()) {
            throw geopm::Exception("model_parse_config(): array length mismatch",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (hostname.size()) {
            char hostname_tmp[NAME_MAX];
            hostname_tmp[NAME_MAX - 1] = '\0';
            if (gethostname(hostname_tmp, NAME_MAX - 1)) {
               throw geopm::Exception("gethostname():", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            std::string this_hostname(hostname_tmp);
            auto hostname_it = hostname.begin();
            for (auto imbalance_it = imbalance.begin(); imbalance_it != imbalance.end(); ++imbalance_it, ++hostname_it) {
                if (this_hostname == *hostname_it) {
                    int err = geopm_imbalancer_frac(*imbalance_it);
                    if (err) {
                        throw geopm::Exception("model_parse_confg(): imbalance fraction is negative",
                                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

}
