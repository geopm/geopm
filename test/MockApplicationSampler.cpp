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

#include <map>

#include "MockApplicationSampler.hpp"

#include "Helper.hpp"
#include "Exception.hpp"

std::vector<geopm::ApplicationSampler::m_record_s> MockApplicationSampler::get_records(void) const
{
    return m_records;
}

void MockApplicationSampler::inject_records(const std::string &record_trace)
{
    m_records.clear();

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
        // assume hex is going to be most convenient
        uint64_t signal = std::stoull(cols[3], 0, 16);
        m_records.push_back({time, process, event, signal});
    }

}
