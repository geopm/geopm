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

#ifndef MOCKAPPLICATIONSAMPLER_HPP_INCLUDE
#define MOCKAPPLICATIONSAMPLER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ApplicationSampler.hpp"
#include "record.hpp"


class MockApplicationSampler : public geopm::ApplicationSampler
{
    public:
        MOCK_METHOD1(time_zero,
                     void(const geopm_time_s &start_time));
        MOCK_METHOD1(update,
                     void(const geopm_time_s &curr_time));
        MOCK_CONST_METHOD1(cpu_region_hash,
                           uint64_t(int cpu_idx));
        MOCK_CONST_METHOD1(cpu_hint,
                           uint64_t(int cpu_idx));
        MOCK_CONST_METHOD2(cpu_hint_time,
                           double(int cpu_idx, uint64_t hint));
        MOCK_CONST_METHOD1(cpu_progress,
                           double(int cpu_idx));
        MOCK_CONST_METHOD0(per_cpu_process,
                           std::vector<int>(void));
        MOCK_METHOD1(connect,
                     void(const std::string &shm_key));
        MOCK_METHOD1(set_sampler,
                     void(std::shared_ptr<geopm::ProfileSampler> sampler));
        MOCK_METHOD0(get_sampler,
                     std::shared_ptr<geopm::ProfileSampler>(void));
        MOCK_CONST_METHOD1(get_name_map,
                           std::map<uint64_t, std::string>(uint64_t name_key));
        MOCK_CONST_METHOD1(get_short_region,
                           geopm::short_region_s(uint64_t event_signal));
        std::vector<geopm::record_s> get_records(void) const override;
        /// Inject records to be used by next call to get_records()
        /// @todo: figure out input type for this
        void inject_records(const std::vector<geopm::record_s> &records);
        void inject_records(const std::string &record_trace);
        void update_time(double time);
    private:
        std::vector<geopm::record_s> m_records;
        double m_time_0;
        double m_time_1;
};

#endif
