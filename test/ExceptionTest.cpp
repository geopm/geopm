/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#include <errno.h>
#include <string>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "Exception.hpp"


class ExceptionTest: public :: testing :: Test {};

TEST_F(ExceptionTest, hello)
{
    const std::string geopm_tag("<geopm> ");
    std::string what_str;

    geopm::Exception ex0(GEOPM_ERROR_INVALID);
    EXPECT_EQ(GEOPM_ERROR_INVALID, ex0.err_value());
    what_str = std::string(ex0.what());
    EXPECT_TRUE(what_str.size() != 0);
    EXPECT_TRUE(what_str.compare(0, geopm_tag.length(), geopm_tag) == 0);
    EXPECT_TRUE(what_str.find("argument") != std::string::npos);
    std::cerr << "Error: " << ex0.what() << std::endl;

    geopm::Exception ex1("Hello world", GEOPM_ERROR_LOGIC);
    EXPECT_EQ(GEOPM_ERROR_LOGIC, ex1.err_value());
    what_str = std::string(ex1.what());
    EXPECT_TRUE(what_str.size() != 0);
    EXPECT_TRUE(what_str.compare(0, geopm_tag.length(), geopm_tag) == 0);
    EXPECT_TRUE(what_str.find("Hello world") != std::string::npos);
    std::cerr << "Error: " << ex1.what() << std::endl;

    geopm::Exception ex2(GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
    EXPECT_EQ(GEOPM_ERROR_FILE_PARSE, ex2.err_value());
    what_str = std::string(ex2.what());
    EXPECT_TRUE(what_str.size() != 0);
    EXPECT_TRUE(what_str.compare(0, geopm_tag.length(), geopm_tag) == 0);
    EXPECT_TRUE(what_str.find("parse") != std::string::npos);
    EXPECT_TRUE(what_str.find("ExceptionTest.cpp") != std::string::npos);
    std::cerr << "Error: " << ex2.what() << std::endl;

    geopm::Exception ex3("Hello world", GEOPM_ERROR_NO_AGENT, __FILE__, __LINE__);
    EXPECT_EQ(GEOPM_ERROR_NO_AGENT, ex3.err_value());
    what_str = std::string(ex3.what());
    EXPECT_TRUE(what_str.size() != 0);
    EXPECT_TRUE(what_str.compare(0, geopm_tag.length(), geopm_tag) == 0);
    EXPECT_TRUE(what_str.find("agent") != std::string::npos);
    EXPECT_TRUE(what_str.find("ExceptionTest.cpp") != std::string::npos);
    std::cerr << "Error value = " << ex3.err_value() << std::endl;
    try {
        throw ex3;
    }
    catch (...) {
        int err = geopm::exception_handler(std::current_exception());
        EXPECT_EQ(GEOPM_ERROR_NO_AGENT, err);
    }

    geopm::Exception ex4(0);
    EXPECT_EQ(GEOPM_ERROR_RUNTIME, ex4.err_value());
    what_str = std::string(ex4.what());
    EXPECT_TRUE(what_str.size() != 0);
    EXPECT_TRUE(what_str.compare(0, geopm_tag.length(), geopm_tag) == 0);
    EXPECT_TRUE(what_str.find("untime") != std::string::npos);
    std::cerr << "Error: " << ex4.what() << std::endl;

    geopm::Exception ex5;
    EXPECT_EQ(GEOPM_ERROR_RUNTIME, ex5.err_value());
    what_str = std::string(ex5.what());
    EXPECT_TRUE(what_str.size() != 0);
    EXPECT_TRUE(what_str.compare(0, geopm_tag.length(), geopm_tag) == 0);
    EXPECT_TRUE(what_str.find("untime") != std::string::npos);
    std::cerr << "Error: " << ex5.what() << std::endl;
}

TEST_F(ExceptionTest, check_ronn)
{
    // Make sure output of geopm_print_error --ronn matches what is in
    // geopm_error.3.ronn
    EXPECT_EQ(0, system(
        "./examples/geopm_print_error --ronn > tmp.txt && "
        "diff tmp.txt ronn/geopm_error.3.ronn | grep '^<'; err=$?; "
        "rm -f tmp.txt; if [ \"$err\" -eq 0 ]; then false; else true; fi"));
}
