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

#include "GlobalPolicy.hpp"

class MockGlobalPolicy : public geopm::IGlobalPolicy {
    public:
        MOCK_CONST_METHOD0(mode,
            int (void));
        MOCK_CONST_METHOD0(frequency_mhz,
            int (void));
        MOCK_CONST_METHOD0(tdp_percent,
            int (void));
        MOCK_CONST_METHOD0(budget_watts,
            int (void));
        MOCK_CONST_METHOD0(affinity,
            int (void));
        MOCK_CONST_METHOD0(goal,
            int (void));
        MOCK_CONST_METHOD0(num_max_perf,
            int (void));
        MOCK_CONST_METHOD0(tree_decider,
            const std::string &(void));
        MOCK_CONST_METHOD0(leaf_decider,
            const std::string &(void));
        MOCK_CONST_METHOD0(platform,
            const std::string &(void));
        MOCK_CONST_METHOD0(mode_string,
            std::string (void));
        MOCK_METHOD1(policy_message,
            void (struct geopm_policy_message_s &policy_message));
        MOCK_METHOD1(mode,
            void (int mode));
        MOCK_METHOD1(frequency_mhz,
            void (int frequency));
        MOCK_METHOD1(tdp_percent,
            void (int percentage));
        MOCK_METHOD1(budget_watts,
            void (int budget));
        MOCK_METHOD1(affinity,
            void (int cpu_affinity));
        MOCK_METHOD1(goal,
            void (int geo_goal));
        MOCK_METHOD1(num_max_perf,
            void (int num_big_cores));
        MOCK_METHOD1(tree_decider,
            void (const std::string &description));
        MOCK_METHOD1(leaf_decider,
            void (const std::string &description));
        MOCK_METHOD1(platform,
            void (const std::string &description));
        MOCK_METHOD0(write,
            void (void));
        MOCK_METHOD0(read,
            void (void));
        MOCK_METHOD0(enforce_static_mode,
            void (void));
        MOCK_CONST_METHOD0(header,
            std::string (void));
};
