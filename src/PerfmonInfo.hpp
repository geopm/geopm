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

#ifndef PERFMONINFO_HPP_INCLUDE
#define PERFMONINFO_HPP_INCLUDE

#include <map>
#include <stdexcept>
#include <string>

namespace geopm {

    const std::string PMON_EVENT_NAME_KEY = "EventName";
    const std::string PMON_EVENT_CODE_KEY = "EventCode";
    const std::string PMON_UMASK_KEY = "UMask";
    const std::string PMON_OFFCORE_KEY = "Offcore";

    /// Used to hold information about performance counters, as described in
    /// files found at https://download.01.org/perfmon/
    class PerfmonInfo
    {
        public:
            PerfmonInfo(std::string name, std::pair<int, int> event_code,
                        uint64_t umask, bool offcore)
                : m_event_name(name)
                , m_event_code(event_code)
                , m_umask(umask)
                , m_offcore(offcore)
            {
            }

            std::string m_event_name;
            // TODO: what size of int to use?
            std::pair<int, int> m_event_code;
            uint64_t m_umask;
            bool m_offcore;
    };

    /// Reads a collection of perfmon counters from a json string. The key of the
    /// map is the same as the EventName field.
    std::map<std::string, PerfmonInfo> parse_perfmon(const std::string &json_string);
}

#endif
