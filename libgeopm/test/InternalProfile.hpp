/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INTERNALPROFILE_HPP_INCLUDE
#define INTERNALPROFILE_HPP_INCLUDE

#include <string>
#include <map>

#include "geopm_time.h"

namespace geopm
{
    class InternalProfile
    {
        public:
            static InternalProfile &internal_profile(void);
            virtual ~InternalProfile() = default;
            void enter(const std::string &region_name);
            void exit(const std::string &region_name);
            std::string report(void);
        protected:
            InternalProfile();
            struct m_region_s {
                struct geopm_time_s enter_time;
                double total_time;
                int count;
            };
            const struct m_region_s M_REGION_ZERO {{{0,0}}, 0.0, 0};
            std::map<std::string, m_region_s> m_region_map;
            std::string m_region_stack;
            size_t m_region_stack_colon;
            std::map<std::string, m_region_s>::iterator m_region_map_curr_it;
    };

    static inline void ip_enter(const std::string &region_name)
    {
        InternalProfile::internal_profile().enter(region_name);
    }

    static inline void ip_exit(const std::string &region_name)
    {
        InternalProfile::internal_profile().exit(region_name);
    }

    static inline std::string ip_report(void)
    {
        return InternalProfile::internal_profile().report();
    }
}

#endif
