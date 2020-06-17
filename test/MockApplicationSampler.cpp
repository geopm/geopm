/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "config.h"

#include <cmath>
#include <map>

#include "MockApplicationSampler.hpp"
#include "record.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "geopm.h"

void MockApplicationSampler::update_time(double time)
{
    if (isnan(m_time_1)) {
        m_time_1 = time;
    }
    else {
        m_time_0 = m_time_1;
        m_time_1 = time;
    }
}

std::vector<geopm::record_s> MockApplicationSampler::get_records(void) const
{
    std::vector<geopm::record_s> result;
    if (isnan(m_time_1)) {
        result = m_records;
    }
    else {
        for (const auto &it : m_records) {
            if (it.time >= m_time_0 &&
                it.time < m_time_1) {
                result.push_back(it);
            }
        }
    }
    return result;
}

void MockApplicationSampler::inject_records(const std::vector<geopm::record_s> &records)
{
    m_records = records;
    m_time_0 = 0.0;
    m_time_1 = NAN;
}

static uint64_t name_to_hint(std::string name)
{
    uint64_t result = GEOPM_REGION_HINT_UNKNOWN;
    static const std::map<std::string, uint64_t> result_map {
        {"COMPUTE", GEOPM_REGION_HINT_COMPUTE},
        {"MEMORY", GEOPM_REGION_HINT_MEMORY},
        {"NETWORK", GEOPM_REGION_HINT_NETWORK},
        {"IO", GEOPM_REGION_HINT_IO},
        {"SERIAL", GEOPM_REGION_HINT_SERIAL},
        {"PARALLEL", GEOPM_REGION_HINT_PARALLEL},
        {"IGNORE", GEOPM_REGION_HINT_IGNORE},
    };
    auto it = result_map.find(name);
    if (it != result_map.end()) {
        result = it->second;
    }
    return result;
}

void MockApplicationSampler::inject_records(const std::string &record_trace)
{
    m_records.clear();
    m_time_0 = 0.0;
    m_time_1 = NAN;

    std::string header = "";
    std::vector<std::string> lines = geopm::string_split(record_trace, "\n");
    for (const auto &line : lines) {
        if (line.size() == 0 || line[0] == '#') {
            continue;
        }
        if (header == "") {
            header = line;
            continue;
        }
        std::vector<std::string> cols = geopm::string_split(line, "|");
        if (cols.size() != 4) {
            throw geopm::Exception("MockApplicationSampler::inject_records(): "
                                   " failed to parse record trace string; wrong num columns.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);

        }
        // columns match fields from m_record_s
        double time = std::stod(cols[0]);
        int process = std::stoi(cols[1]);
        int event = geopm::ApplicationSampler::event_type(cols[2]);
        uint64_t signal = 0;
        // assume hex is going to be most convenient
        switch (event) {
            case geopm::EVENT_REGION_ENTRY:
            case geopm::EVENT_REGION_EXIT:
                signal = std::stoull(cols[3], 0, 16);
                break;
            case geopm::EVENT_EPOCH_COUNT:
                signal = std::stoi(cols[3]);
                break;
            case geopm::EVENT_HINT:
                signal = name_to_hint(cols[3]);
                break;
        }
        m_records.push_back({time, process, event, signal});
    }
}
