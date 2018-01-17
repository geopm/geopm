/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
#include <fstream>

#include <json-c/json.h>

#include "Exception.hpp"
#include "ModelParse.hpp"
#include "imbalancer.h"

#ifndef NAME_MAX
#define NAME_MAX 512
#endif


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

        json_object *object = json_tokener_parse(config_string.c_str());

        enum json_type type = json_object_get_type(object);

        if (type != json_type_object) {
            throw Exception("model_parse_config(): malformed json configuration file",
                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }

        std::vector<std::string> hostname;
        std::vector<double> imbalance;
        std::string key_string;
        json_object_object_foreach(object, key, val) {
            key_string = key;
            if (key_string == "loop-count") {
                if (json_object_get_type(val) == json_type_int) {
                    loop_count = json_object_get_int(val);
                }
                else {
                    throw Exception("model_parse_config(): loop-count expected to be an integer type",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else if (key_string == "region") {
                if (json_object_get_type(val) == json_type_array) {
                    json_object *region_obj;
                    for (size_t i = 0; i < (size_t)json_object_array_length(val); ++i) {
                        region_obj = json_object_array_get_idx(val, i);
                        if (json_object_get_type(region_obj) == json_type_string) {
                            region_name.push_back(json_object_get_string(region_obj));
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
                if (json_object_get_type(val) == json_type_array) {
                    json_object *big_o_obj;
                    for (size_t i = 0; i < (size_t)json_object_array_length(val); ++i) {
                        big_o_obj = json_object_array_get_idx(val, i);
                        if (json_object_get_type(big_o_obj) == json_type_double) {
                            big_o.push_back(json_object_get_double(big_o_obj));
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
                if (json_object_get_type(val) == json_type_array) {
                    json_object *hostname_obj;
                    for (size_t i = 0; i < (size_t)json_object_array_length(val); ++i) {
                        hostname_obj = json_object_array_get_idx(val, i);
                        if (json_object_get_type(hostname_obj) == json_type_string) {
                            hostname.push_back(json_object_get_string(hostname_obj));
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
                if (json_object_get_type(val) == json_type_array) {
                    json_object *imbalance_obj;
                    for (size_t i = 0; i < (size_t)json_object_array_length(val); ++i) {
                        imbalance_obj = json_object_array_get_idx(val, i);
                        if (json_object_get_type(imbalance_obj) == json_type_double) {
                            imbalance.push_back(json_object_get_double(imbalance_obj));
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
                    int err = imbalancer_frac(*imbalance_it);
                    if (err) {
                        throw geopm::Exception("model_parse_confg(): imbalance fraction is negative",
                                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

}
