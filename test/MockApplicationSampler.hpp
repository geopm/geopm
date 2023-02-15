/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
        MOCK_METHOD((std::map<uint64_t, std::string>), get_name_map,
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
