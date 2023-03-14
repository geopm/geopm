/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include "gtest/gtest.h"
#include "geopm_error.h"
#include "geopm_test.hpp"
#include "geopm/Helper.hpp"
#include "CSV.hpp"
#include "geopm_version.h"
#include "geopm_hash.h"
#include "geopm_field.h"
#include "config.h"


class CSVTest: public :: testing :: Test
{
    protected:
        void SetUp(void);
        std::string m_host_name;
        std::string m_start_time;
        size_t m_buffer_size;
};

void CSVTest::SetUp(void)
{
    m_host_name = "csv-test-host";
    m_start_time = "Mon Jul  1 11:10:08 PDT 2019";
    m_buffer_size = 256;
}

TEST_F(CSVTest, header)
{
    std::string output_path = "CSVTest-header-output";
    {
        std::unique_ptr<geopm::CSV> tmp = geopm::make_unique<geopm::CSVImp>(output_path, m_host_name, m_start_time, m_buffer_size);
    }
#ifdef ENABLE_MPI
    output_path += "-" + m_host_name;
#endif

    std::string output_string = geopm::read_file(output_path);
    std::vector<std::string> output_lines = geopm::string_split(output_string, "\n");
    EXPECT_TRUE(geopm::string_begins_with(output_lines[0], "# geopm_version:"));
    EXPECT_TRUE(geopm::string_begins_with(output_lines[1], "# start_time:"));
    EXPECT_TRUE(geopm::string_begins_with(output_lines[2], "# profile_name:"));
    EXPECT_TRUE(geopm::string_begins_with(output_lines[3], "# node_name:"));
    EXPECT_TRUE(geopm::string_begins_with(output_lines[4], "# agent:"));
    EXPECT_EQ(geopm_version(), geopm::string_split(output_lines[0], ": ")[1]);
    EXPECT_EQ(m_start_time, geopm::string_split(output_lines[1], ": ")[1]);
    EXPECT_EQ(m_host_name, geopm::string_split(output_lines[3], ": ")[1]);
    unlink(output_path.c_str());
}

TEST_F(CSVTest, columns)
{
    std::vector<std::string> column_names {"COLUMN_DOUBLE",
                                           "COLUMN_FLOAT",
                                           "COLUMN_INTEGER",
                                           "COLUMN_HEX",
                                           "COLUMN_RAW64",
                                           "COLUMN_DEFAULT"};
    std::ostringstream expect_legend_stream;
    std::string sep;
    for (const auto &cn : column_names) {
        expect_legend_stream << sep << cn;
        sep = "|";
    }
    std::string expect_legend(expect_legend_stream.str());
    std::string expect_values = "0.000244140625|0.5|1024|0x20000000000000|0xffffffffffffffff|0.000244140625";
    std::string output_path = "CSVTest-columns-output";
    {
        std::unique_ptr<geopm::CSV> csv = geopm::make_unique<geopm::CSVImp>(output_path, m_host_name, m_start_time, m_buffer_size);
        csv->add_column(column_names[0], "double");
        csv->add_column(column_names[1], "float");
        csv->add_column(column_names[2], "integer");
        csv->add_column(column_names[3], "hex");
        csv->add_column(column_names[4], "raw64");
        csv->add_column(column_names[5]);
        double small = 0.000244140625;
        double half = 0.5;
        double big = 1024;
        double huge = 0x20000000000000ULL;
        double all_one = geopm_field_to_signal(0xFFFFFFFFFFFFFFFFULL);
        std::vector<double> sample = {small,
                                      half,
                                      big,
                                      huge,
                                      all_one,
                                      small};
        csv->activate();
        csv->update(sample);
    }
#ifdef ENABLE_MPI
    output_path += "-" + m_host_name;
#endif
    std::string output_string = geopm::read_file(output_path);
    std::vector<std::string> output_lines = geopm::string_split(output_string, "\n");
    ASSERT_TRUE(output_lines.size() == 8);
    for (size_t line_idx = 0; line_idx < 5; ++line_idx) {
        EXPECT_TRUE(geopm::string_begins_with(output_lines[line_idx], "# "));
    }
    EXPECT_EQ(expect_legend, output_lines[5]);
    EXPECT_EQ(expect_values, output_lines[6]);
    ASSERT_EQ("", output_lines[7]);
    unlink(output_path.c_str());
}

TEST_F(CSVTest, buffer)
{
    std::vector<std::string> column_names {"COLUMN_DOUBLE",
                                           "COLUMN_FLOAT",
                                           "COLUMN_INTEGER",
                                           "COLUMN_HEX",
                                           "COLUMN_RAW64",
                                           "COLUMN_DEFAULT"};
    std::ostringstream expect_legend_stream;
    std::string sep;
    for (const auto &cn : column_names) {
        expect_legend_stream << sep << cn;
        sep = "|";
    }
    std::string expect_legend(expect_legend_stream.str());
    std::string expect_values = "6.103515625e-05|0.5|1024|0x20000000000000|0xffffffffffffffff|6.103515625e-05";
    std::string output_path = "CSVTest-buffer-output";
    {
        std::unique_ptr<geopm::CSV> csv = geopm::make_unique<geopm::CSVImp>(output_path, "", m_start_time, m_buffer_size);
        csv->add_column(column_names[0], "double");
        csv->add_column(column_names[1], "float");
        csv->add_column(column_names[2], "integer");
        csv->add_column(column_names[3], "hex");
        csv->add_column(column_names[4], "raw64");
        csv->add_column(column_names[5]);
        double small = 6.103515625e-05;
        double half = 0.5;
        double big = 1024;
        double huge = 0x20000000000000ULL;
        double all_one = geopm_field_to_signal(0xFFFFFFFFFFFFFFFFULL);
        std::vector<double> sample = {small,
                                      half,
                                      big,
                                      huge,
                                      all_one,
                                      small};
        csv->activate();
        // Flush the buffer many times
        for (size_t count = 0; count != m_buffer_size; ++count) {
            csv->update(sample);
        }
    }
    std::string output_string = geopm::read_file(output_path);
    std::vector<std::string> output_lines = geopm::string_split(output_string, "\n");
    ASSERT_EQ(7 + m_buffer_size, output_lines.size());
    for (size_t line_idx = 0; line_idx != output_lines.size(); ++line_idx) {
        if (line_idx < 5) {
            EXPECT_TRUE(geopm::string_begins_with(output_lines[line_idx], "# "));
        }
        else if (line_idx == 5) {
            EXPECT_EQ(expect_legend, output_lines[line_idx]);
        }
        else if (line_idx != output_lines.size() - 1) {
            EXPECT_EQ(expect_values, output_lines[line_idx]);
        }
        else {
            EXPECT_EQ("", output_lines[line_idx]);
        }
    }
    ASSERT_EQ("", output_lines.back());
    unlink(output_path.c_str());
}

TEST_F(CSVTest, negative)
{
    std::string output_path = "CSVTest-negative-output";
    GEOPM_EXPECT_THROW_MESSAGE(geopm::make_unique<geopm::CSVImp>("/path/does/not/exist",
                                                                 "", m_start_time, m_buffer_size),
                               ENOENT, "Unable to open");

    std::unique_ptr<geopm::CSV> csv =
        geopm::make_unique<geopm::CSVImp>(output_path, "", m_start_time, m_buffer_size);

    GEOPM_EXPECT_THROW_MESSAGE(csv->add_column("name", "bad-format"),
                               GEOPM_ERROR_INVALID, "format is unknown");
    csv->add_column("name");
    GEOPM_EXPECT_THROW_MESSAGE(csv->update({1.0}),
                               GEOPM_ERROR_INVALID, "activate() must be called prior");
    csv->activate();
    GEOPM_EXPECT_THROW_MESSAGE(csv->add_column("another"),
                               GEOPM_ERROR_INVALID, "cannot be called after activate");
    GEOPM_EXPECT_THROW_MESSAGE(csv->update({1.0, 2.0}),
                               GEOPM_ERROR_INVALID, "incorrectly sized");
    csv->update({1.0});
    unlink(output_path.c_str());
}
