/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <cmath>
#include <map>

#include "MockApplicationSampler.hpp"
#include "record.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm_prof.h"

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
            if (it.time.t.tv_sec >= m_time_0 &&
                it.time.t.tv_sec < m_time_1) {
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
        geopm_time_s time = {{std::stoi(cols[0]), 0}};
        int process = std::stoi(cols[1]);
        int event = geopm::event_type(cols[2]);
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
        }
        m_records.push_back({time, process, event, signal});
    }
}
