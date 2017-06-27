/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include "Comm.hpp"
#include "geopm_message.h"
#include "ProfileThread.hpp"
#include "ProfileSampler.hpp"

class MockProfileSampler : public geopm::IProfileSampler {
    public:
        MOCK_METHOD0(capacity,
            size_t (void));
        MOCK_METHOD3(sample,
            void (std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length, geopm::IComm *comm));
        MOCK_METHOD0(do_shutdown,
            bool (void));
        MOCK_METHOD0(do_report,
            bool (void));
        MOCK_METHOD0(region_names,
            void (void));
        MOCK_METHOD1(initialize,
            void (int &rank_per_node));
        MOCK_METHOD1(cpu_rank,
            void (std::vector<int> &cpu_rank));
        MOCK_METHOD1(name_set,
            void (std::set<std::string> &region_name));
        MOCK_METHOD1(report_name,
            void (std::string &report_str));
        MOCK_METHOD1(profile_name,
            void (std::string &prof_str));
        MOCK_METHOD0(tprof_table,
            geopm::IProfileThreadTable *(void));
};
