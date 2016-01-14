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

#ifndef SAMPLEREGULATOR_HPP_INCLUDE
#define SAMPLEREGULATOR_HPP_INCLUDE

#include <vector>
#include <stack>
#include <set>
#include <map>

#include "CircularBuffer.hpp"
#include "geopm_message.h"

namespace geopm
{
    class SampleRegulator
    {
        public:
            SampleRegulator(const std::vector<int> &cpu_rank);
            virtual ~SampleRegulator();
            void sample(const struct geopm_time_s &platform_sample_time,
                        std::vector<double>::const_iterator platform_sample_begin,
                        std::vector<double>::const_iterator platform_sample_end,
                        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end,
                        const std::vector<double> &signal_domain_matrix,
                        std::stack<struct geopm_telemetry_message_s> &telemetry); // result list per domain of control
        protected:
            void insert(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end);
            void insert(std::vector<double>::const_iterator platform_sample_begin,
                        std::vector<double>::const_iterator platform_sample_end);
            void align(const struct geopm_time_s &timestamp);
            void transform(const std::vector<double> &signal_domain_matrix,
                           std::stack<struct geopm_telemetry_message_s> &telemetry);
            struct m_rank_sample_s {
                struct geopm_time_s timestamp;
                double progress;
                double runtime;
            };
            enum m_num_rank_sample_e {
                M_NUM_RANK_SIGNAL = 2,
            };
            enum m_interp_type_e {
                M_INTERP_TYPE_NONE = 0,
                M_INTERP_TYPE_NEAREST = 1,
                M_INTERP_TYPE_LINEAR = 2,
            };
            int m_num_rank;
            std::set<int> m_rank_set;
            std::map<int, int> m_rank_idx_map;
            uint64_t m_region_id_prev;
            // per rank record of last profile samples in m_region_id_prev
            std::vector<CircularBuffer<struct m_rank_sample_s> > m_rank_sample_prev;
            struct geopm_time_s m_aligned_time;
            // vector to multiply with signal_domain_matrix to project into control domains
            std::vector<double> m_aligned_signal;
            size_t m_num_platform_signal;
    };
}

#endif
