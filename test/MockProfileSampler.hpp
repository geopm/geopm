/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#ifndef MOCKPROFILESAMPLER_HPP_INCLUDE
#define MOCKPROFILESAMPLER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ProfileSampler.hpp"
#include "Comm.hpp"

class MockProfileSampler : public geopm::ProfileSampler
{
    public:
        MOCK_CONST_METHOD0(do_shutdown,
                     bool (void));
        MOCK_CONST_METHOD0(do_report,
                           bool (void));
        MOCK_METHOD0(region_names,
                     void (void));
        MOCK_METHOD0(initialize,
                     void (void));
        MOCK_CONST_METHOD0(rank_per_node,
                           int (void));
        MOCK_CONST_METHOD0(cpu_rank,
                           std::vector<int> (void));
        MOCK_CONST_METHOD0(name_set,
                           std::set<std::string> (void));
        MOCK_CONST_METHOD0(report_name,
                           std::string (void));
        MOCK_CONST_METHOD0(profile_name,
                           std::string (void));
        MOCK_METHOD0(controller_ready,
                     void(void));
        MOCK_METHOD0(abort,
                     void(void));
        MOCK_METHOD0(sample_cache,
                     std::vector<struct geopm_prof_message_s> (void));
        MOCK_METHOD0(check_sample_end,
                     void(void));
};

#endif
