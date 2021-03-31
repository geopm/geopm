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

#ifndef MOCKREPORTER_HPP_INCLUDE
#define MOCKREPORTER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Reporter.hpp"
#include "ApplicationIO.hpp"
#include "Comm.hpp"
#include "TreeComm.hpp"

class MockReporter : public geopm::Reporter
{
    public:
        MOCK_METHOD0(init, void(void));
        MOCK_METHOD0(update, void(void));
        MOCK_METHOD7(generate,
                     void(const std::string &agent_name,
                          const std::vector<std::pair<std::string, std::string> > &agent_report_header,
                          const std::vector<std::pair<std::string, std::string> > &agent_node_report,
                          const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report,
                          const geopm::ApplicationIO &application_io,
                          std::shared_ptr<geopm::Comm> comm,
                          const geopm::TreeComm &tree_comm));
};

#endif
