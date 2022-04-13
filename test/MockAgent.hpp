/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKAGENT_HPP_INCLUDE
#define MOCKAGENT_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Agent.hpp"
#include "geopm/PlatformIO.hpp"

class MockAgent : public geopm::Agent
{
    public:
        MOCK_METHOD(void, init,
                    (int level, const std::vector<int> &fan_in, bool is_level_root),
                    (override));
        MOCK_METHOD(void, validate_policy, (std::vector<double> & policy),
                    (const, override));
        MOCK_METHOD(void, split_policy,
                    (const std::vector<double> &in_policy,
                     std::vector<std::vector<double> > &out_policy),
                    (override));
        MOCK_METHOD(bool, do_send_policy, (), (const, override));
        MOCK_METHOD(void, aggregate_sample,
                    (const std::vector<std::vector<double> > &in_signal,
                     std::vector<double> &out_signal),
                    (override));
        MOCK_METHOD(bool, do_send_sample, (), (const, override));
        MOCK_METHOD(void, adjust_platform,
                    (const std::vector<double> &in_policy), (override));
        MOCK_METHOD(bool, do_write_batch, (), (const, override));
        MOCK_METHOD(void, sample_platform, (std::vector<double> & out_sample),
                    (override));
        MOCK_METHOD(void, wait, (), (override));
        MOCK_METHOD((std::vector<std::pair<std::string, std::string> >),
                    report_header, (), (const, override));
        MOCK_METHOD((std::vector<std::pair<std::string, std::string> >),
                    report_host, (), (const, override));
        MOCK_METHOD((std::map<uint64_t, std::vector<std::pair<std::string, std::string> > >),
                    report_region, (), (const, override));
        MOCK_METHOD(std::vector<std::string>, trace_names, (), (const, override));
        MOCK_METHOD((std::vector<std::function<std::string(double)> >), trace_formats, (),
                    (const, override));
        MOCK_METHOD(void, trace_values, (std::vector<double> & values), (override));
};

#endif
