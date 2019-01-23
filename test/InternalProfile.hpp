/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
            const struct m_region_s M_REGION_ZERO {GEOPM_TIME_REF, 0.0, 0};
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
