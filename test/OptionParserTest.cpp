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

#include "gtest/gtest.h"

#include "OptionParser.hpp"
#include "geopm_test.hpp"

using geopm::OptionParser;
using geopm::Exception;

class OptionParserTest : public testing::Test
{
    protected:
        OptionParserTest();

        std::string m_name;
        std::ostringstream m_std_out;
        std::ostringstream m_err_out;
        OptionParser m_opt;
};

OptionParserTest::OptionParserTest()
    : m_name("option_parser_test")
    , m_opt(m_name, m_std_out, m_err_out, "")
{

}

TEST_F(OptionParserTest, get_invalid)
{
    EXPECT_THROW(m_opt.is_set("invalid"), Exception);

    EXPECT_THROW(m_opt.get_value("bad"), Exception);
}

TEST_F(OptionParserTest, parse_errors)
{
    m_opt.add_option("bool_a", 'a', "bool-a", false, "");
    m_opt.add_option("str_b", 'b', "str-b", "AA", "");

    // unknown option
    const char * const invalid[] = {"", "--unknown"};
    GEOPM_EXPECT_THROW_MESSAGE(m_opt.parse(2, invalid),
                               GEOPM_ERROR_INVALID,
                               "Error: invalid option");

    // missing required argument
    const char * const missing_args[] = {"", "--str-b"};
    GEOPM_EXPECT_THROW_MESSAGE(m_opt.parse(2, missing_args), GEOPM_ERROR_INVALID,
                               "Error: invalid option");

    // help string should be in error output
    std::string msg = m_err_out.str();
    EXPECT_NE(std::string::npos, msg.find("Usage: "));
}

TEST_F(OptionParserTest, add_option_errors)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_opt.add_option("hello", 'H', "help", false, ""),
                               GEOPM_ERROR_INVALID, "already assigned");
    GEOPM_EXPECT_THROW_MESSAGE(m_opt.add_option("very", 'V', "version", false, ""),
                               GEOPM_ERROR_INVALID, "already assigned");
    GEOPM_EXPECT_THROW_MESSAGE(m_opt.add_option("hello", 'h', "hello", false, ""),
                               GEOPM_ERROR_INVALID, "already assigned");
    GEOPM_EXPECT_THROW_MESSAGE(m_opt.add_option("very", 'v', "very", false, ""),
                               GEOPM_ERROR_INVALID, "already assigned");
    GEOPM_EXPECT_THROW_MESSAGE(m_opt.add_option("bad", '?', "bad", false, ""),
                               GEOPM_ERROR_INVALID, "cannot have ? as a short option");
    m_opt.add_option("str_a", 'a', "str-a", "AA", "");
    GEOPM_EXPECT_THROW_MESSAGE(m_opt.add_option("aaa", 'a', "aaa", "AA", ""),
                               GEOPM_ERROR_INVALID, "already assigned");
}

TEST_F(OptionParserTest, unset_bool_gets_default)
{
    // default false
    m_opt.add_option("bool_a", 'a', "bool-a", false, "");
    // default true
    m_opt.add_option("bool_b", 'b', "bool-b", true, "");

    EXPECT_FALSE(m_opt.is_set("bool_a"));
    EXPECT_TRUE(m_opt.is_set("bool_b"));
}

TEST_F(OptionParserTest, set_bool_flag)
{
    // default false, set true
    m_opt.add_option("bool_a", 'a', "bool-a", false, "");
    // default true, set false
    m_opt.add_option("bool_b", 'b', "bool-b", true, "");

    const char * const both_long[] = {"", "--bool-a", "--bool-b"};
    m_opt.parse(3, both_long);
    EXPECT_TRUE(m_opt.is_set("bool_a"));
    EXPECT_FALSE(m_opt.is_set("bool_b"));

    const char * const both_short[] = {"", "-b", "-a"};
    m_opt.parse(3, both_short);
    EXPECT_TRUE(m_opt.is_set("bool_a"));
    EXPECT_FALSE(m_opt.is_set("bool_b"));
}

TEST_F(OptionParserTest, unset_string_gets_default)
{
    m_opt.add_option("str_a", 'a', "str-a", "AA", "");

    EXPECT_EQ("AA", m_opt.get_value("str_a"));
}

TEST_F(OptionParserTest, set_string_value)
{
    m_opt.add_option("str_a", 'a', "str-a", "AA", "");

    const char * const long_args_eq[] = {"", "--str-a=one"};
    m_opt.parse(2, long_args_eq);
    EXPECT_EQ("one", m_opt.get_value("str_a"));
    const char * const long_args_sp[] = {"", "--str-a", "two"};
    m_opt.parse(3, long_args_sp);
    EXPECT_EQ("two", m_opt.get_value("str_a"));
    const char * const long_args_quote[] = {"", "--str-a", "th ree"};
    m_opt.parse(3, long_args_quote);
    EXPECT_EQ("th ree", m_opt.get_value("str_a"));

    const char * const short_args_sp[] = {"", "-a", "four"};
    m_opt.parse(3, short_args_sp);
    EXPECT_EQ("four", m_opt.get_value("str_a"));
    const char * const short_args_quote[] = {"", "-a", "fi ve"};
    m_opt.parse(3, short_args_quote);
    EXPECT_EQ("fi ve", m_opt.get_value("str_a"));
}

TEST_F(OptionParserTest, positional_args)
{
    m_opt.add_option("str_a", 'a', "str-a", "AA", "");

    const char * const long_args[] = {"", "--str-a=one", "two", "three"};
    m_opt.parse(4, long_args);

    std::vector<std::string> expected = {"two", "three"};
    EXPECT_EQ(expected, m_opt.get_positional_args());
}

TEST_F(OptionParserTest, help)
{
    // long form
    const char * const long_form[] = {"", "--help"};
    bool result = m_opt.parse(2, long_form);
    EXPECT_TRUE(result);
    std::string msg = m_std_out.str();

    EXPECT_NE(std::string::npos, msg.find("Usage: "));
    EXPECT_NE(std::string::npos, msg.find("help"));
    EXPECT_NE(std::string::npos, msg.find("version"));

    m_err_out.str("");

    // short form
    const char * const short_form[] = {"", "-h"};
    result = m_opt.parse(2, short_form);
    EXPECT_TRUE(result);
    msg = m_std_out.str();
    EXPECT_NE(std::string::npos, msg.find("Usage: "));
    // todo: test of formatting of options
    EXPECT_NE(std::string::npos, msg.find("help"));
    EXPECT_NE(std::string::npos, msg.find("version"));
}

TEST_F(OptionParserTest, version)
{
    // long form
    const char * const long_form[] = {"", "--version"};
    bool result = m_opt.parse(2, long_form);
    EXPECT_TRUE(result);
    std::string msg = m_std_out.str();
    EXPECT_NE(std::string::npos, msg.find("Intel Corporation"));

    m_err_out.str("");

    // short form
    const char * const short_form[] = {"", "-v"};
    result = m_opt.parse(2, short_form);
    EXPECT_TRUE(result);

    msg = m_std_out.str();
    EXPECT_NE(std::string::npos, msg.find("Intel Corporation"));
}

TEST_F(OptionParserTest, complex)
{
    m_opt.add_option("a", 'a', "ayy", "", "");
    m_opt.add_option("b", 'b', "bee", false, "");
    m_opt.add_option("c", 'c', "see", true, "");
    const char * const help_version[] = {"", "-a", "thing", "-h", "--bee", "--version", "-c"};
    bool result = m_opt.parse(7, help_version);
    EXPECT_TRUE(result);

    const char * const many[] = {"", "-a", "thing", "--bee", "-c"};
    result = m_opt.parse(5, many);
    EXPECT_FALSE(result);
    EXPECT_EQ("thing", m_opt.get_value("a"));
    EXPECT_TRUE(m_opt.is_set("b"));
    EXPECT_FALSE(m_opt.is_set("c"));
}

TEST_F(OptionParserTest, compact_short_options)
{
    m_opt.add_option("a", 'a', "ayy", "", "");
    m_opt.add_option("b", 'b', "bee", false, "");
    m_opt.add_option("c", 'c', "see", true, "");
    m_opt.add_option("mode", 'm', "mode", "open", "");

    const char * const compact[] = {"", "-bca", "stuff"};
    bool result = m_opt.parse(3, compact);
    EXPECT_FALSE(result);
    EXPECT_EQ("stuff", m_opt.get_value("a"));
    EXPECT_TRUE(m_opt.is_set("b"));
    EXPECT_FALSE(m_opt.is_set("c"));
    EXPECT_EQ("open", m_opt.get_value("mode"));
}

TEST_F(OptionParserTest, format_help)
{
    m_opt.add_option("agent", 'a', "agent", "", "specify the name of the agent");
    m_opt.add_option("policy", 'p', "policy", "NAN",
                     "values to be set for each policy in a comma-separated list");
    m_opt.add_option("cache", 'c', "cache", false,
                     "create geopm topo cache if it does not exist");
    m_opt.add_option("long", 'l', "very-long-option-needs-to-wrap", false,
                     "an option with a long name that forces the description to the next line. "
                     "the description is also long and wraps a lot of times.");
    m_opt.add_example_usage("[--cache]");
    m_opt.add_example_usage("[-a AGENT] [-p POLICY0,POLICY1,...]");

    const char * const usage = "\n"
                        "Usage: option_parser_test [--cache]\n"
                        "       option_parser_test [-a AGENT] [-p POLICY0,POLICY1,...]\n"
                        "       option_parser_test [--help] [--version]\n"
                        "\n"
                        "Mandatory arguments to long options are mandatory for short options too.\n"
                        "\n"
                        "  -a, --agent=AGENT         specify the name of the agent\n"
                        "  -p, --policy=POLICY       values to be set for each policy in a\n"
                        "                            comma-separated list\n"
                        "  -c, --cache               create geopm topo cache if it does not exist\n"
                        "  -l, --very-long-option-needs-to-wrap\n"
                        "                            an option with a long name that forces the\n"
                        "                            description to the next line. the description is\n"
                        "                            also long and wraps a lot of times.\n"
                        "  -h, --help                print brief summary of the command line usage\n"
                        "                            information, then exit\n"
                        "  -v, --version             print version of GEOPM to standard output, then exit\n"
                        "\n"
                        "Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation. All rights reserved.\n"
                        "\n";

    std::string result = m_opt.format_help();
    EXPECT_EQ(usage, result);
}

/*
TEST_F(OptionParserTest, geopmagent_help)
{
    OptionParser opt("geopmagent");
    const char * const usage = "\nUsage: geopmagent\n"
                        "       geopmagent [-a AGENT] [-p POLICY0,POLICY1,...]\n"
                        "       geopmagent [--help] [--version]\n"
                        "\n"
                        "Mandatory arguments to long options are mandatory for short options too.\n"
                        "\n"
                        "  -a, --agent=AGENT         specify the name of the agent\n"
                        "  -p, --policy=POLICY0,...  values to be set for each policy in a\n"
                        "                            comma-seperated list\n"
                        "  -h, --help                print  brief summary of the command line\n"
                        "                            usage information, then exit\n"
                        "  -v, --version             print version of GEOPM to standard output,\n"
                        "                            then exit\n"
                        "\n"
                        "Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation. All rights reserved.\n"
                        "\n";
    std::string result = opt.format_help();
    EXPECT_EQ(usage, result);
}

TEST_F(OptionParserTest, geopmread_help)
{
    OptionParser opt("geopmread");
    const char * const usage = "\nUsage:\n"
                        "       geopmread SIGNAL_NAME DOMAIN_TYPE DOMAIN_INDEX\n"
                        "       geopmread [--domain [SIGNAL_NAME]]\n"
                        "       geopmread [--info [SIGNAL_NAME]]\n"
                        "       geopmread [--help] [--version] [--cache]\n"
                        "\n"
                        "  SIGNAL_NAME:  name of the signal\n"
                        "  DOMAIN_TYPE:  name of the domain for which the signal should be read\n"
                        "  DOMAIN_INDEX: index of the domain, starting from 0\n"
                        "\n"
                        "  -d, --domain                     print domain of a signal\n"
                        "  -i, --info                       print longer description of a signal\n"
                        "  -c, --cache                      create geopm topo cache if it does not exist\n"
                        "  -h, --help                       print brief summary of the command line\n"
                        "                                   usage information, then exit\n"
                        "  -v, --version                    print version of GEOPM to standard output,\n"
                        "                                   then exit\n"
                        "\n"
                        "Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation. All rights reserved.\n"
                        "\n";
    std::string result = opt.format_help();
    EXPECT_EQ(usage, result);
}
*/
