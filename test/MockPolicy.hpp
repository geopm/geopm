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

class MockPolicy : public geopm::IPolicy {
    public:
        MOCK_METHOD0(num_domain,
                int(void));
        MOCK_METHOD1(region_id,
                void(std::vector<uint64_t> &region_id));
        MOCK_METHOD3(update,
                void(uint64_t region_id, int domain_idx, double target));
        MOCK_METHOD2(update,
                void(uint64_t region_id, const std::vector<double> &target));
        MOCK_METHOD1(mode,
                void(int new_mode));
        MOCK_METHOD1(policy_flags,
                void(unsigned long new_flags));
        MOCK_METHOD2(target,
                void(uint64_t region_id, std::vector<double> &target));
        MOCK_METHOD3(target,
                void(uint64_t region_id, int domain, double &target));
        MOCK_CONST_METHOD0(mode,
                int(void));
        MOCK_CONST_METHOD0(frequency_mhz,
                int(void));
        MOCK_CONST_METHOD0(tdp_percent,
                int(void));
        MOCK_CONST_METHOD0(affinity,
                int(void));
        MOCK_CONST_METHOD0(goal,
                int(void));
        MOCK_CONST_METHOD0(num_max_perf,
                int(void));
        MOCK_METHOD2(target_updated,
                void(uint64_t region_id, std::map<int, double> &target));
        MOCK_METHOD2(target_valid,
                void(uint64_t region_id, std::map<int, double> &target));
        MOCK_METHOD3(policy_message,
                void(uint64_t region_id, const struct geopm_policy_message_s &parent_msg,
                    std::vector<struct geopm_policy_message_s> &child_msg));
        MOCK_METHOD2(is_converged,
                void(uint64_t region_id, bool converged_state));
        MOCK_METHOD1(is_converged,
                bool(uint64_t region_id));
        MOCK_METHOD1(ctl_cpu_freq,
                void(std::vector<double> freq));
};
