/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fstream>
#include <sstream>
#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm/Helper.hpp"
#include "Tracer.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm_hint.h"
#include "geopm_hash.h"
#include "geopm_version.h"
#include "geopm_test.hpp"
#include "config.h"

using geopm::TracerImp;
using geopm::PlatformIO;
using geopm::PlatformTopo;
using testing::_;
using testing::Return;
using testing::HasSubstr;

class TracerTest : public ::testing::Test
{
    protected:
        struct m_request_s {
            std::string name;
            int domain_type;
            int domain_idx;
            std::function<std::string(double)> format;
        };
        void SetUp(void);
        void TearDown(void);
        void remove_files(void);
        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        std::string m_path = "test.trace";
        std::string m_hostname = "myhost";
#ifdef GEOPM_ENABLE_MPI
        std::string m_file_path = m_path + "-" + m_hostname;
#else
        std::string m_file_path = m_path;
#endif
        std::string m_start_time = "Tue Nov  6 08:00:00 2018";
        std::vector<struct m_request_s> m_default_cols;
        const int m_num_extra_cols = 3;
        std::unique_ptr<geopm::TracerImp> m_tracer;
};

void TracerTest::remove_files(void)
{
    std::remove(m_path.c_str());
    std::remove(m_file_path.c_str());
}

void TracerTest::SetUp(void)
{
    remove_files();
    EXPECT_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillOnce(Return(2));
    EXPECT_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillRepeatedly(Return(1));

    m_default_cols = {
        {"TIME", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_double},
        {"EPOCH_COUNT", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_integer},
        {"REGION_HASH", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_hex},
        {"REGION_HINT", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_hex},
        {"REGION_PROGRESS", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_float},
        {"CPU_ENERGY", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_double},
        {"DRAM_ENERGY", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_double},
        {"CPU_POWER", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_double},
        {"DRAM_POWER", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_double},
        {"CPU_FREQUENCY_STATUS", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_double},
        {"CPU_CYCLES_THREAD", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_integer},
        {"CPU_CYCLES_REFERENCE", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_integer},
        {"CPU_CORE_TEMPERATURE", GEOPM_DOMAIN_BOARD, 0, geopm::string_format_double},
    };
    const std::vector<std::pair<std::string, int> > env_signals = {
        {"EXTRA", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"EXTRA_SPECIAL", geopm_domain_e::GEOPM_DOMAIN_CPU}
    };

    int idx = 0;
    for (auto cc : m_default_cols) {
        EXPECT_CALL(m_platform_io, push_signal(cc.name, cc.domain_type, cc.domain_idx))
            .WillOnce(Return(idx));
        ++idx;
    }
    EXPECT_CALL(m_platform_io, push_signal("EXTRA", GEOPM_DOMAIN_BOARD, 0))
        .WillOnce(Return(idx));
    ++idx;
    EXPECT_CALL(m_platform_io, push_signal("EXTRA_SPECIAL", GEOPM_DOMAIN_CPU, 0))
        .WillOnce(Return(idx));
    ++idx;
    EXPECT_CALL(m_platform_io, push_signal("EXTRA_SPECIAL", GEOPM_DOMAIN_CPU, 1))
        .WillOnce(Return(idx));

    EXPECT_CALL(m_platform_io, format_function("EXTRA"))
        .WillOnce(Return(geopm::string_format_double));

    EXPECT_CALL(m_platform_io, format_function("EXTRA_SPECIAL"))
        .WillOnce(Return(geopm::string_format_double));

    for (auto column : m_default_cols) {
        EXPECT_CALL(m_platform_io, format_function(column.name))
            .WillOnce(Return(column.format));
    }

    m_tracer = geopm::make_unique<TracerImp>(m_start_time, m_path, m_hostname, true,
                                             m_platform_io, m_platform_topo, env_signals);
}

void TracerTest::TearDown(void)
{
    remove_files();
}

void check_trace(std::istream &expected, std::istream &result);

TEST_F(TracerTest, columns)
{
    // columns from agent will be printed as-is
    std::vector<std::string> agent_cols {"col1", "col2"};

    m_tracer->columns(agent_cols, {});
    m_tracer->flush();
    std::string version(geopm_version());
    std::string expected_header = "# geopm_version:\n"
                                  "# start_time: " + m_start_time + "\n"
                                  "# profile_name:\n"
                                  "# node_name: " + m_hostname + "\n"
                                  "# agent:\n";
    std::string expected_str = expected_header +
        "TIME|EPOCH_COUNT|REGION_HASH|REGION_HINT|REGION_PROGRESS|CPU_ENERGY|DRAM_ENERGY|"
        "CPU_POWER|DRAM_POWER|CPU_FREQUENCY_STATUS|CPU_CYCLES_THREAD|CPU_CYCLES_REFERENCE|CPU_CORE_TEMPERATURE|"
        "EXTRA|EXTRA_SPECIAL-cpu-0|EXTRA_SPECIAL-cpu-1|"
        "col1|col2\n";
    std::istringstream expected(expected_str);
    std::ifstream result(m_file_path);
    ASSERT_TRUE(result.good()) << strerror(errno);
    check_trace(expected, result);
}

TEST_F(TracerTest, update_samples)
{
    int idx = 0;
    for (auto cc : m_default_cols) {
        EXPECT_CALL(m_platform_io, sample(idx))
            .WillOnce(Return(idx + 0.5));
        ++idx;
    }

    for (int count = 0; count < m_num_extra_cols; ++count) {
        EXPECT_CALL(m_platform_io, sample(idx))
            .WillOnce(Return(idx + 0.7));
        ++idx;
    }

    std::vector<std::string> agent_cols {"col1", "col2"};
    std::vector<double> agent_vals {88.8, 77.7};

    m_tracer->columns(agent_cols, {});
    m_tracer->update(agent_vals);
    m_tracer->flush();

    std::string expected_str = "\n\n\n\n\n\n"
        "0.5|1|0x00000002|0x00000003|4.5|5.5|6.5|7.5|8.5|9.5|10|11|12.5|13.7|14.7|15.7|88.8|77.7\n";
    std::istringstream expected(expected_str);
    std::ifstream result(m_file_path);
    ASSERT_TRUE(result.good()) << strerror(errno);
    check_trace(expected, result);
}

TEST_F(TracerTest, region_entry_exit)
{
    EXPECT_CALL(m_platform_io, sample(_)).Times(m_default_cols.size() + m_num_extra_cols)
        .WillOnce(Return(2.2))                        // time
        .WillOnce(Return(0.0))                        // epoch_count
        .WillOnce(Return(0x123))                      // region hash
        .WillOnce(Return(GEOPM_REGION_HINT_UNKNOWN))  // region hint
        .WillOnce(Return(0.0))  // progress; should cause one region entry to be skipped
        .WillOnce(Return(0.0))
        .WillRepeatedly(Return(2.2));

    std::vector<std::string> agent_cols {"col1", "col2"};
    std::vector<double> agent_vals {88.8, 77.7};

    m_tracer->columns(agent_cols, {geopm::string_format_integer,
                                   geopm::string_format_integer});
    m_tracer->update(agent_vals);
    m_tracer->flush();
    std::string expected_str ="\n\n\n\n\n"
        "\n" // header
        "2.2|0|0x00000123|0x00000001|0|0|2.2|2.2|2.2|2.2|2|2|2.2|2.2|2.2|2.2|88|77\n";

    std::istringstream expected(expected_str);
    std::ifstream result(m_file_path);
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
