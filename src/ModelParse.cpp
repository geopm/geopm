/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ModelParse.hpp"

#include <cmath>
#include <unistd.h>
#include <fstream>
#include <limits.h>

#include "geopm/json11.hpp"

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
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
        int num_host = hostname.size();
        std::string this_hostname = geopm::hostname();
        for (int host_idx = 0; host_idx != num_host; ++host_idx) {
            if (hostname.at(host_idx) == this_hostname) {
                int err = geopm_imbalancer_frac(imbalance.at(host_idx));
                if (err) {
                    throw geopm::Exception("model_parse_confg(): imbalance fraction is negative",
                                           GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
        }
    }

}
