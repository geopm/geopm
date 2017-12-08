/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include "PerfmonInfo.hpp"
#include <json-c/json.h>

#include <map>
#include <stdexcept>
#include <string>
#include <sstream>

#include "Exception.hpp"

namespace geopm {

    /// Helper class to manage json-c objects
    class JsonObject
    {
        public:
            virtual ~JsonObject()
            {
                if (m_obj) {
                    json_object_put(m_obj);
                }
            }
            static JsonObject from_string(const std::string &str)
            {
                return JsonObject(json_tokener_parse(str.c_str()));
            }

            json_object* get()
            {
                return m_obj;
            }
        private:
            JsonObject(json_object *obj) : m_obj(obj)
            {
                if (nullptr == obj) {
                    throw Exception("Malformed json string",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }

            json_object *m_obj;
    };

    std::map<std::string, PerfmonInfo> parse_perfmon(const std::string &json_string)
    {
        std::map<std::string, PerfmonInfo> all_msr;

        JsonObject root = JsonObject::from_string(json_string);
        enum json_type type = json_object_get_type(root.get());
        if (type != json_type_array) {
            throw Exception("Malformed json config: must be list of objects.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        size_t num_msr = json_object_array_length(root.get());
        for (size_t i = 0; i < num_msr; ++i) {
            // owned by root
            json_object *obj = json_object_array_get_idx(root.get(), i);
            type = json_object_get_type(obj);
            if (type != json_type_object) {
                throw Exception("Malformed json config: item is not an object.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            std::string event_name;
            std::pair<int, int> event_code {-1, -1};
            int umask = -1;
            int offcore = -1;
            json_object_object_foreach(obj, key, val) {
                if (PMON_EVENT_NAME_KEY == key) {
                    event_name = json_object_get_string(val);
                } else if (PMON_EVENT_CODE_KEY == key) {
                    std::string event_codes = json_object_get_string(val);
                    auto comma = event_codes.find(",");
                    event_code.first = std::stoi(event_codes, 0, 16);
                    if (comma != std::string::npos) {
                        auto second = event_codes.substr(comma + 1, event_codes.length());
                        event_code.second = std::stoi(second, 0, 16);
                    }
                } else if (PMON_UMASK_KEY == key) {
                    umask = std::stol(json_object_get_string(val), 0, 16);
                } else if (PMON_OFFCORE_KEY == key) {
                    offcore = json_object_get_int(val);
                }
            }
            if (!event_name.empty() &&
                event_code.first >= 0 &&
                umask >= 0 &&
                offcore >= 0) {

                all_msr.emplace(event_name,
                                PerfmonInfo(event_name, event_code, umask, offcore));
            }
        }
        return all_msr;
    }

}
