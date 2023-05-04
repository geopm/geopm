/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKREPORTER_HPP_INCLUDE
#define MOCKREPORTER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ApplicationIO.hpp"
#include "Comm.hpp"
#include "Reporter.hpp"
#include "TreeComm.hpp"

class MockReporter : public geopm::Reporter
{
    public:
        MOCK_METHOD(void, init, (), (override));
        MOCK_METHOD(void, update, (), (override));
        MOCK_METHOD(void, generate,
                    (const std::string &agent_name,
                     (const std::vector<std::pair<std::string, std::string> > &agent_report_header),
                     (const std::vector<std::pair<std::string, std::string> > &agent_host_report),
                     (const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report),
                     const geopm::ApplicationIO &application_io,
                     std::shared_ptr<geopm::Comm> comm,
                     const geopm::TreeComm &tree_comm),
                    (override));
        MOCK_METHOD(std::string, generate,
                    (const std::string &profile_name,
                     const std::string &agent_name,
                     (const std::vector<std::pair<std::string, std::string> > &agent_report_header),
                     (const std::vector<std::pair<std::string, std::string> > &agent_host_report),
                     (const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report)),
                    (override));
        MOCK_METHOD(void, total_time, (double total), (override));
        MOCK_METHOD(void, overhead, (double overhead_sec, double sample_delay), (override));
};

#endif
