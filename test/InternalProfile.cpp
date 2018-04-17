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

#include <sstream>

#include "InternalProfile.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    InternalProfile &InternalProfile::internal_profile(void)
    {
        static InternalProfile instance;
        return instance;
    }

    void InternalProfile::enter(const std::string &region_name)
    {
        auto it = m_region_map.emplace(std::piecewise_construct,
                                       std::forward_as_tuple(region_name),
                                       std::forward_as_tuple(M_REGION_ZERO)).first;
        geopm_time(&(it->second.enter_time));
    }

    void InternalProfile::exit(const std::string &region_name)
    {
        struct geopm_time_s curr_time;
        geopm_time(&curr_time);
        auto it = m_region_map.find(region_name);
        if (it == m_region_map.end()) {
            throw Exception("InternalProfile::exit(): Region name has not been previously passed to the enter() method",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        it->second.total_time += geopm_time_diff(&(it->second.enter_time), &curr_time);
        ++(it->second.count);
    }

    std::string InternalProfile::report(void)
    {
        std::ostringstream result;
        for (auto it = m_region_map.begin(); it != m_region_map.end(); ++it) {
            result << it->first << " : " << it->second.total_time << " : " << it->second.count << "\n";
        }
        result << "\n";
        return result.str();
    }
}
