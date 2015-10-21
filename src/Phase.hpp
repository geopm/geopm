/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef PHASE_HPP_INCLUDE
#define PHASE_HPP_INCLUDE

#include <stdint.h>
#include <string>

#include "Observation.hpp"
#include "Policy.hpp"

namespace geopm
{
    class Phase
    {
        public:
            Phase(const std::string &name, long identifier, int hint);
            virtual ~Phase();
            long identifier(void) const;
            void observation_insert(int buffer_index, double value);
            void name(std::string &name) const;
            int hint(void) const;
            void policy(Policy* policy);
            Policy* policy(void);
            Policy* last_policy(void);
            double observation_mean(int buffer_index) const;
            double observation_median(int buffer_index) const;
            double observation_stddev(int buffer_index) const;
            double observation_max(int buffer_index) const;
            double observation_min(int buffer_index) const;
            double observation_integrate_time(int buffer_index) const;
        protected:
            Observation m_obs;
            Policy m_policy;
            Policy m_last_policy;
            std::string m_name;
            long m_identifier;
            int m_hint;
    };
}

#endif
