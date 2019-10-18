/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef MOCKENVIRONMENT_HPP_INCLUDE
#define MOCKENVIRONMENT_HPP_INCLUDE

#include "Environment.hpp"

class MockEnvironment : public geopm::Environment
{
    public:
        MOCK_CONST_METHOD0(report,
                           std::string(void));
        MOCK_CONST_METHOD0(comm,
                           std::string(void));
        MOCK_CONST_METHOD0(policy,
                           std::string(void));
        MOCK_CONST_METHOD0(endpoint,
                           std::string(void));
        MOCK_CONST_METHOD0(shmkey,
                           std::string(void));
        MOCK_CONST_METHOD0(trace,
                           std::string(void));
        MOCK_CONST_METHOD0(trace_profile,
                           std::string(void));
        MOCK_CONST_METHOD0(trace_endpoint_policy,
                           std::string(void));
        MOCK_CONST_METHOD0(plugin_path,
                           std::string(void));
        MOCK_CONST_METHOD0(profile,
                           std::string(void));
        MOCK_CONST_METHOD0(frequency_map,
                           std::string(void));
        MOCK_CONST_METHOD0(agent,
                           std::string(void));
        MOCK_CONST_METHOD0(trace_signals,
                           std::string(void));
        MOCK_CONST_METHOD0(report_signals,
                           std::string(void));
        MOCK_CONST_METHOD0(max_fan_out,
                           int(void));
        MOCK_CONST_METHOD0(pmpi_ctl,
                           int(void));
        MOCK_CONST_METHOD0(do_policy,
                           bool(void));
        MOCK_CONST_METHOD0(do_endpoint,
                           bool(void));
        MOCK_CONST_METHOD0(do_region_barrier,
                           bool(void));
        MOCK_CONST_METHOD0(do_trace,
                           bool(void));
        MOCK_CONST_METHOD0(do_trace_profile,
                           bool(void));
        MOCK_CONST_METHOD0(do_trace_endpoint_policy,
                           bool(void));
        MOCK_CONST_METHOD0(do_profile,
                           bool());
        MOCK_CONST_METHOD0(timeout,
                           int(void));
        MOCK_CONST_METHOD0(debug_attach,
                           int(void));
};

#endif
