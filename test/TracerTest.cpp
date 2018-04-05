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

#include <fstream>
#include <sstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Tracer.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "MockPlatformIO.hpp"
#include "geopm_test.hpp"

using geopm::Tracer;
using geopm::IPlatformIO;
using geopm::IPlatformTopo;
using testing::_;
using testing::Return;
using testing::HasSubstr;

class TracerTest : public ::testing::Test
{
    protected:
        void TearDown(void);
        MockPlatformIO m_platform_io;
        std::string m_path = "test.trace";
        std::string m_hostname = "myhost";
};

void TracerTest::TearDown(void)
{
    std::remove(m_path.c_str());
}

void check_trace(std::istream &expected, std::istream &result);

TEST_F(TracerTest, columns)
{
    Tracer tracer(m_path, m_hostname, true, m_platform_io);
    std::vector<IPlatformIO::m_request_s> cols = {
        {"TIME", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 0},
        {"ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 1}
    };

    for (auto cc : cols) {
        EXPECT_CALL(m_platform_io, push_signal(cc.name, cc.domain_type, cc.domain_idx));
    }

    tracer.columns(cols);
    tracer.flush();

    std::string expected_str = "HEADER\ntime-board-0|energy-package-0|energy-package-1\n";
    std::istringstream expected(expected_str);
    std::ifstream result(m_path + "-" + m_hostname);
    ASSERT_TRUE(result.good()) << strerror(errno);
    check_trace(expected, result);
}

TEST_F(TracerTest, update_samples)
{
    Tracer tracer(m_path, m_hostname, true, m_platform_io);
    std::vector<IPlatformIO::m_request_s> cols = {
        {"TIME", IPlatformTopo::M_DOMAIN_BOARD, 0},
        {"ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 0},
        {"ENERGY", IPlatformTopo::M_DOMAIN_PACKAGE, 1}
    };
    int idx = 0;
    for (auto cc : cols) {
        EXPECT_CALL(m_platform_io, push_signal(cc.name, cc.domain_type, cc.domain_idx))
            .WillOnce(Return(idx));

        EXPECT_CALL(m_platform_io, sample(idx))
            .WillOnce(Return(idx + 0.5));
        ++idx;
    }

    tracer.columns(cols);
    tracer.update(true);  //todo: how to use is_epoch arg
    tracer.flush();

    std::string expected_str = "\n\n0.5|1.5|2.5\n";
    std::istringstream expected(expected_str);
    std::ifstream result(m_path + "-" + m_hostname);
    ASSERT_TRUE(result.good()) << strerror(errno);
    check_trace(expected, result);

}

/// @todo This is shared with ReporterTest; can be put in common file
void check_trace(std::istream &expected, std::istream &result)
{
    char exp_line[1024];
    char res_line[1024];
    expected.getline(exp_line, 1024);
    result.getline(res_line, 1024);
    int line = 1;
    while (expected.good() && result.good()) {
        ASSERT_THAT(std::string(res_line), HasSubstr(exp_line)) << " on line " << line;
        expected.getline(exp_line, 1024);
        result.getline(res_line, 1024);
        ++line;
    }
    if (expected.good() != result.good()) {
        std::ostringstream message;
        message << "Different length strings." << std::endl;
        message << "Remaining expected:" << std::endl;
        message << "--------" << std::endl;
        while (expected.good()) {
            expected.getline(exp_line, 1024);
            message << exp_line << std::endl;
        }
        message << "--------" << std::endl;
        message << "Remaining result:" << std::endl;
        message << "--------" << std::endl;
        while (result.good()) {
            result.getline(res_line, 1024);
            message << res_line << std::endl;
        }
        message << "--------" << std::endl;
        FAIL() << message.str();
    }
}
