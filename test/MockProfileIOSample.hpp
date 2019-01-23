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

#ifndef MOCKPROFILEIOSAMPLE_HPP_INCLUDE
#define MOCKPROFILEIOSAMPLE_HPP_INCLUDE

#include "ProfileIOSample.hpp"
#include "geopm.h"

class MockProfileIOSample : public geopm::IProfileIOSample {
    public:
        MOCK_METHOD0(finalize_unmarked_region,
                           void(void));
        MOCK_METHOD2(update,
                     void(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin, std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end));
        MOCK_METHOD1(update_thread,
                     void(const std::vector<double> &));
        MOCK_CONST_METHOD0(per_cpu_region_id,
                           std::vector<uint64_t>(void));
        MOCK_CONST_METHOD1(per_cpu_progress,
                           std::vector<double>(const struct geopm_time_s &extrapolation_time));
        MOCK_CONST_METHOD0(per_cpu_thread_progress,
                           std::vector<double>(void));
        MOCK_CONST_METHOD1(per_cpu_runtime,
                           std::vector<double>(uint64_t region_id));
        MOCK_CONST_METHOD1(per_rank_runtime,
                           std::vector<double>(uint64_t region_id));
        MOCK_CONST_METHOD0(total_app_runtime,
                           double(void));
        MOCK_CONST_METHOD0(cpu_rank,
                           std::vector<int>(void));
};

#endif
