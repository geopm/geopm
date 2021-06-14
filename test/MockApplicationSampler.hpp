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
        MOCK_METHOD(void, time_zero, (const geopm_time_s &start_time), (override));
        MOCK_METHOD(void, update, (const geopm_time_s &curr_time), (override));
        MOCK_METHOD(uint64_t, cpu_region_hash, (int cpu_idx), (const, override));
        MOCK_METHOD(uint64_t, cpu_hint, (int cpu_idx), (const, override));
        MOCK_METHOD(double, cpu_hint_time, (int cpu_idx, uint64_t hint),
                    (const, override));
        MOCK_METHOD(double, cpu_progress, (int cpu_idx), (const, override));
        MOCK_METHOD(std::vector<int>, per_cpu_process, (), (const, override));
        MOCK_METHOD(void, connect, (const std::string &shm_key), (override));
        MOCK_METHOD(void, set_sampler,
                    (std::shared_ptr<geopm::ProfileSampler> sampler), (override));
        MOCK_METHOD(std::shared_ptr<geopm::ProfileSampler>, get_sampler, (),
                    (override));
        MOCK_METHOD(std::map<uint64_t, std::string>, get_name_map,
                    (uint64_t name_key), (const, override));
        MOCK_METHOD(geopm::short_region_s, get_short_region,
                    (uint64_t event_signal), (const, override));
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
