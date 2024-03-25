/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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

    InternalProfile::InternalProfile()
        : m_region_stack_colon(std::string::npos)
        , m_region_map_curr_it(m_region_map.end())

    {

    }

    void InternalProfile::enter(const std::string &region_name)
    {
        if (m_region_stack.size()) {
            m_region_stack_colon = m_region_stack.size() + 1;
            m_region_stack += ":" + region_name;
        }
        else {
            m_region_stack = region_name;
        }
        m_region_map_curr_it = m_region_map.emplace(std::piecewise_construct,
                                                    std::forward_as_tuple(m_region_stack),
                                                    std::forward_as_tuple(M_REGION_ZERO)).first;
        geopm_time(&(m_region_map_curr_it->second.enter_time));
    }

    void InternalProfile::exit(const std::string &region_name)
    {
        struct geopm_time_s curr_time;
        geopm_time(&curr_time);
        if (m_region_map_curr_it == m_region_map.end()) {
            throw Exception("InternalProfile::exit(): Region name has not been previously passed to the enter() method",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_region_map_curr_it->second.total_time += geopm_time_diff(&(m_region_map_curr_it->second.enter_time), &curr_time);
        ++(m_region_map_curr_it->second.count);
        if (m_region_stack_colon != std::string::npos) {
            m_region_stack = m_region_stack.substr(0, m_region_stack_colon - 1);
            m_region_stack_colon = m_region_stack.find(':');
            m_region_map_curr_it = m_region_map.find(m_region_stack);
        }
        else {
            m_region_stack = "";
            m_region_map_curr_it = m_region_map.end();
        }
    }

    std::string InternalProfile::report(void)
    {
        std::ostringstream result;
        result << "region-name | time | count \n";
        for (auto it = m_region_map.begin(); it != m_region_map.end(); ++it) {
            result << it->first << " | " << it->second.total_time << " | " << it->second.count << "\n";
        }
        result << "\n";
        return result.str();
    }
}
