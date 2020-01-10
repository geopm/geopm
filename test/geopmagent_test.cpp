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

#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopmagent_helper.hpp"
#include "Agent.hpp"

class geopmagent_test : public ::testing::Test
{
    protected:
        void check_output(std::vector<const char *> args,
                          const std::string &expected);
        void check_error(std::vector<const char *>,
                          const std::string &expected);
    private:
        std::ostringstream m_stdout;
        std::ostringstream m_stderr;
};

void geopmagent_test::check_output(std::vector<const char *> args,
                                   const std::string &expected)
{
    args.insert(args.begin(), "");
    int err = geopm::geopmagent_helper(args.size(), args.data(), m_stdout, m_stderr);
    std::string result = m_stdout.str();
    EXPECT_EQ(expected, result);
    EXPECT_EQ(0, err);

    EXPECT_EQ("", m_stderr.str());

    m_stdout.str("");
    m_stderr.str("");
}

void geopmagent_test::check_error(std::vector<const char *> args,
                                  const std::string &expected)
{
    args.insert(args.begin(), "");
    int err = geopm::geopmagent_helper(args.size(), args.data(), m_stdout, m_stderr);
    std::string result = m_stderr.str();
    EXPECT_EQ(expected, result);
    EXPECT_NE(0, err);

    m_stdout.str("");
    m_stderr.str("");
}

TEST_F(geopmagent_test, help_text)
{
    const char *text = "\n"
        "Usage: geopmagent \n"
        "       geopmagent [-a AGENT] [-p POLICY0,POLICY1,...]\n"
        "       geopmagent [--help] [--version]\n"
        "\n"
        "Mandatory arguments to long options are mandatory for short options too.\n"
        "\n"
        "  -a, --agent=AGENT         specify the name of the agent\n"
        "  -p, --policy=POLICY       values to be set for each policy in a\n"
        "                            comma-separated list\n"
        "  -h, --help                print brief summary of the command line usage\n"
        "                            information, then exit\n"
        "  -v, --version             print version of GEOPM to standard output, then exit\n"
        "\n"
        "Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation. All rights reserved.\n"
        "\n";
    check_output({"--help"}, text);
}

TEST_F(geopmagent_test, show_all_agents)
{
    std::string expected;
    auto all_agents = geopm::agent_factory().plugin_names();
    for (const auto &agent : all_agents) {
        expected += agent + '\n';
    }
    check_output({}, expected);
}

TEST_F(geopmagent_test, monitor_policy_sample)
{
    // should have (none) for both
    std::string expected = "Policy: (none)\n"
                           "Sample: (none)\n";
    check_output({"--agent", "monitor"}, expected);
}

TEST_F(geopmagent_test, balancer_policy_sample)
{
    // should have names for both
    std::string expected =
        "Policy: POWER_PACKAGE_LIMIT_TOTAL,STEP_COUNT,MAX_EPOCH_RUNTIME,POWER_SLACK\n"
        "Sample: STEP_COUNT,MAX_EPOCH_RUNTIME,SUM_POWER_SLACK,MIN_POWER_HEADROOM\n";
    check_output({"--agent", "power_balancer"}, expected);
}

TEST_F(geopmagent_test, monitor_policy_generate)
{
    std::string expected = "{}\n";
    check_output({"--agent", "monitor", "--policy", "None"}, expected);
}

TEST_F(geopmagent_test, balancer_policy_generate)
{
    std::string expected = "{\"POWER_PACKAGE_LIMIT_TOTAL\": 180, \"STEP_COUNT\": \"NAN\"}\n";
    check_output({"--agent", "power_balancer", "--policy", "180,NAN"}, expected);
}

TEST_F(geopmagent_test, error_policy_without_agent)
{
    // -p without -a
    check_error({"-p", "NAN"}, "no agent!!!!!");
}

TEST_F(geopmagent_test, error_monitor_requires_none)
{
    // monitor -p requires 'none'
    check_error({"-a", "monitor", "-p", "NAN"},
                "Error: Must specify \"None\" for the parameter option if agent takes no parameters.\n");
}

TEST_F(geopmagent_test, error_invalid_float)
{
    // invalid floating point number
    std::string expected =
        "Error: invalid is not a valid floating point number"
        "use \"NAN\" to indicate default.\n";
    check_error({"-a", "power_balancer", "-p", "invalid"}, expected);
}

TEST_F(geopmagent_test, error_positional_args)
{
    check_error({"one", "two", "three"},
                "Error: The following positional argument(s) are in error:\n"
                "one\n"
                "two\n"
                "three\n");
}
