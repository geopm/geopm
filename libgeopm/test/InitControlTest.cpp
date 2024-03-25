/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <fstream>
#include <string>
#include <cerrno>

#include "geopm/PlatformTopo.hpp"
#include "InitControl.hpp"

#include "MockPlatformIO.hpp"
#include "geopm_test.hpp"

using testing::InSequence;
using testing::Throw;
using testing::StrictMock;
using testing::_;

using geopm::PlatformTopo;
using geopm::InitControl;
using geopm::InitControlImp;

class InitControlTest : public :: testing::Test
{
    protected:
        void SetUp();
        void TearDown();
        void WriteFile(const std::string &contents);
        StrictMock<MockPlatformIO> m_platform_io;
        std::shared_ptr<InitControl> m_init_control;
        std::string m_file_name = "test_file";
};

void InitControlTest::SetUp()
{

}

void InitControlTest::TearDown()
{
    (void)unlink(m_file_name.c_str());
}

void InitControlTest::WriteFile(const std::string &contents)
{
    std::ofstream input_file(m_file_name);
    input_file << contents;
    input_file.close();
}

TEST_F(InitControlTest, parse_valid_file)
{
    std::string contents = "# This is a comment\n"
                           "FAKE_CONTROL0 board 0 123     # Test comment 0\n"
                           "FAKE_CONTROL1 package 1 -7.77 # Test comment 1\n"
                           "\n"
                           "# This is another comment\n"
                           "FAKE_CONTROL2 gpu 3 0         # Test comment 2\n"
                           "FAKE_CONTROL3 package 0 1.3e-5\n"
                           "FAKE_CONTROL4 package 0 -1.3e-5\n"
                           "FAKE_CONTROL5 package 3 1e9\n"
                           "    #FAKE_CONTROL6 package 3 2e9\n"
                           "FAKE_CONTROL7 cpu 3 0xB33F\n"
                           "";
    WriteFile(contents);

    InSequence s;
    EXPECT_CALL(m_platform_io,
                write_control("FAKE_CONTROL0", PlatformTopo::domain_name_to_type("board"), 0, 123));
    EXPECT_CALL(m_platform_io,
                write_control("FAKE_CONTROL1", PlatformTopo::domain_name_to_type("package"), 1, -7.77));
    EXPECT_CALL(m_platform_io,
                write_control("FAKE_CONTROL2", PlatformTopo::domain_name_to_type("gpu"), 3, 0));
    EXPECT_CALL(m_platform_io,
                write_control("FAKE_CONTROL3", PlatformTopo::domain_name_to_type("package"), 0, 1.3e-5));
    EXPECT_CALL(m_platform_io,
                write_control("FAKE_CONTROL4", PlatformTopo::domain_name_to_type("package"), 0, -1.3e-5));
    EXPECT_CALL(m_platform_io,
                write_control("FAKE_CONTROL5", PlatformTopo::domain_name_to_type("package"), 3, 1e9));
    EXPECT_CALL(m_platform_io,
                write_control("FAKE_CONTROL7", PlatformTopo::domain_name_to_type("cpu"), 3, 0xB33F));

    m_init_control = std::make_shared<InitControlImp>(m_platform_io);
    m_init_control->parse_input(m_file_name);
    m_init_control->write_controls();
}

TEST_F(InitControlTest, parse_valid_file_2)
{
    std::string contents = "# Assign all cores to resource monitoring association ID 0\n"
                           "MSR::PQR_ASSOC:RMID board 0 0\n"
                           "# Assign the resource monitoring ID for QM Events to match ID 0\n"
                           "MSR::QM_EVTSEL:RMID board 0 0\n"
                           "# Select monitoring event ID 0x2 - Total Memory Bandwidth Monitoring\n"
                           "MSR::QM_EVTSEL:EVENT_ID board 0 2\n"
                           "# Set the uncore bounds to the min/max\n"
                           "CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 2400000000.0\n"
                           "CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 1200000000.0\n";
    WriteFile(contents);

    InSequence s;
    EXPECT_CALL(m_platform_io,
                write_control("MSR::PQR_ASSOC:RMID", PlatformTopo::domain_name_to_type("board"), 0, 0));
    EXPECT_CALL(m_platform_io,
                write_control("MSR::QM_EVTSEL:RMID", PlatformTopo::domain_name_to_type("board"), 0, 0));
    EXPECT_CALL(m_platform_io,
                write_control("MSR::QM_EVTSEL:EVENT_ID", PlatformTopo::domain_name_to_type("board"), 0, 2));
    EXPECT_CALL(m_platform_io,
                write_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL", PlatformTopo::domain_name_to_type("board"), 0, 2400000000.0));
    EXPECT_CALL(m_platform_io,
                write_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL", PlatformTopo::domain_name_to_type("board"), 0, 1200000000.0));

    m_init_control = std::make_shared<InitControlImp>(m_platform_io);
    m_init_control->parse_input(m_file_name);
    m_init_control->write_controls();
}

TEST_F(InitControlTest, parse_empty_file)
{
    // Helper::read_file() will throw an exception if the file has no contents
    std::string contents = "";
    WriteFile(contents);

    InSequence s;
    EXPECT_CALL(m_platform_io,
                write_control(_, _, _, _)).Times(0);

    m_init_control = std::make_shared<InitControlImp>(m_platform_io);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(m_file_name),
                               GEOPM_ERROR_INVALID, "input file invalid");
    m_init_control->write_controls();

    // This is the minimum required contents string to not generate an exception in
    // Helper::read_file()
    contents = " ";
    WriteFile(contents);
    m_init_control->parse_input(m_file_name);
    m_init_control->write_controls();

    // All comments, so effectively empty
    contents = "# This is a comment\n"
               "# FAKE_CONTROL0 board 0 123     # Test comment 0\n"
               "# FAKE_CONTROL1 package 1 -7.77 # Test comment 1\n"
               "# This is another comment\n"
               "# FAKE_CONTROL2 gpu 3 0         # Test comment 2\n"
               "# FAKE_CONTROL3 package 0 1.3e-5\n";
    WriteFile(contents);
    m_init_control->parse_input(m_file_name);
    m_init_control->write_controls();
}

TEST_F(InitControlTest, parse_empty_file_name)
{
    m_init_control = std::make_shared<InitControlImp>(m_platform_io);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(""),
                               ENOENT, "file \"\" could not be opened");
}

TEST_F(InitControlTest, throw_bad_input)
{
    m_init_control = std::make_shared<InitControlImp>(m_platform_io);

    std::string contents = "CPU_POWER_LIMIT package 0\n";
    WriteFile(contents);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(m_file_name),
                               GEOPM_ERROR_INVALID, "missing fields");

    contents = "CPU_POWER_LIMIT package 0 2 00\n";
    WriteFile(contents);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(m_file_name),
                               GEOPM_ERROR_INVALID, "Syntax error");

    contents = "CPU_POWER_LIMIT package 0 2#00\n";
    WriteFile(contents);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(m_file_name),
                               GEOPM_ERROR_INVALID, "bad input: #00");

    contents = "CPU_POWER_LIMIT package 0 seven\n";
    WriteFile(contents);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(m_file_name),
                               GEOPM_ERROR_INVALID, "Missing setting value");

    contents = "CPU_POWER_LIMIT package 0 200 CPU_POWER_LIMIT package 0 150";
    WriteFile(contents);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(m_file_name),
                               GEOPM_ERROR_INVALID, "Syntax error");

    contents = "CPU_POWER_LIMIT package one -7.77\n";
    WriteFile(contents);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(m_file_name),
                               GEOPM_ERROR_INVALID, "missing fields");

    contents = "CPU_POWER_LIMIT 1 1 -7.77\n";
    WriteFile(contents);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(m_file_name),
                               GEOPM_ERROR_INVALID, "unrecognized domain_name");

    contents = "CPU_POWER_LIMIT 1 1 0xZ123\n";
    WriteFile(contents);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->parse_input(m_file_name),
                               GEOPM_ERROR_INVALID, "bad input: xZ123");
}

TEST_F(InitControlTest, throw_invalid_write)
{
    std::string contents = "FAKE_CONTROL0 board 0 123\n"
                           "FAKE_CONTROL1 package 1 -7.77\n"
                           "FAKE_CONTROL2 gpu 3 0\n";
    WriteFile(contents);

    InSequence s;
    EXPECT_CALL(m_platform_io,
                write_control("FAKE_CONTROL0", PlatformTopo::domain_name_to_type("board"), 0, 123));
    EXPECT_CALL(m_platform_io,
                write_control("FAKE_CONTROL1", PlatformTopo::domain_name_to_type("package"), 1, -7.77))
        .WillOnce(Throw(geopm::Exception("Test-injected failure", GEOPM_ERROR_INVALID, __FILE__, __LINE__)));

    m_init_control = std::make_shared<InitControlImp>(m_platform_io);
    m_init_control->parse_input(m_file_name);
    GEOPM_EXPECT_THROW_MESSAGE(m_init_control->write_controls(),
                               GEOPM_ERROR_INVALID, "Test-injected failure");
}
