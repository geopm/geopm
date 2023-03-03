/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <limits.h>
#include <string>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "geopm/Exception.hpp"


class ExceptionTest: public :: testing :: Test {};

TEST_F(ExceptionTest, hello)
{
    const std::string geopm_tag("<geopm> ");
    std::string what_str;

    geopm::Exception ex0("Hello world", GEOPM_ERROR_NO_AGENT, __FILE__, __LINE__);
    EXPECT_EQ(GEOPM_ERROR_NO_AGENT, ex0.err_value());
    what_str = std::string(ex0.what());
    EXPECT_TRUE(what_str.size() != 0);
    EXPECT_TRUE(what_str.compare(0, geopm_tag.length(), geopm_tag) == 0);
    EXPECT_TRUE(what_str.find("agent") != std::string::npos);
    EXPECT_TRUE(what_str.find("ExceptionTest.cpp") != std::string::npos);
    std::cerr << "Error value = " << ex0.err_value() << std::endl;
    try {
        throw ex0;
    }
    catch (...) {
        int err = geopm::exception_handler(std::current_exception());
        EXPECT_EQ(GEOPM_ERROR_NO_AGENT, err);
    }

    geopm::Exception ex1;
    EXPECT_EQ(GEOPM_ERROR_RUNTIME, ex1.err_value());
    what_str = std::string(ex1.what());
    EXPECT_TRUE(what_str.size() != 0);
    EXPECT_TRUE(what_str.compare(0, geopm_tag.length(), geopm_tag) == 0);
    EXPECT_TRUE(what_str.find("untime") != std::string::npos);
    std::cerr << "Error: " << ex1.what() << std::endl;
}

TEST_F(ExceptionTest, last_message)
{
    char message_cstr[PATH_MAX];
    std::string message;
    std::string expect("<geopm> Invalid argument: ExceptionTest: Detail: at ExceptionTest.cpp:1234");
    try {
        throw geopm::Exception("ExceptionTest: Detail", GEOPM_ERROR_INVALID, "ExceptionTest.cpp", 1234);
    }
    catch (...) {
        geopm::exception_handler(std::current_exception(), false);
    }
    geopm_error_message(GEOPM_ERROR_INVALID, message_cstr, 256);
    message = message_cstr;
    // Check basic use case
    EXPECT_EQ(expect, message);

    // Check that message truncation works
    geopm_error_message(GEOPM_ERROR_INVALID, message_cstr, 8);
    message = message_cstr;
    EXPECT_EQ(expect.substr(0, 7), message);

    // Make sure the message changes when a new exception is handled
    try {
        throw geopm::Exception("ExceptionTest: New message", GEOPM_ERROR_RUNTIME, "ExceptionTest.cpp", 1234);
    }
    catch (...) {
        geopm::exception_handler(std::current_exception(), false);
    }
    geopm_error_message(GEOPM_ERROR_RUNTIME, message_cstr, 256);
    message = message_cstr;
    std::string expect_new("<geopm> Runtime error: ExceptionTest: New message: at ExceptionTest.cpp:1234");
    EXPECT_EQ(expect_new, message);

    // Make sure long exception messages are handled and truncated properly
    std::string too_long(2 * NAME_MAX, 'X');
    try {
        throw geopm::Exception(too_long, GEOPM_ERROR_RUNTIME, "ExceptionTest.cpp", 1234);
    }
    catch (...) {
        geopm::exception_handler(std::current_exception(), false);
    }
    geopm_error_message(GEOPM_ERROR_RUNTIME, message_cstr, 256);
    message = message_cstr;
    std::string message_prefix ("<geopm> Runtime error: "); // Size = 24
    std::string expect_long(message_prefix + too_long.substr(0, 232)); // 232 + 24 = 256
    EXPECT_EQ(expect_long, message);

    // Make sure long exception messages are handled and fully returned if the size is adequate
    // geopm_error_message() will handle messages up to PATH_MAX (4096) in length
    // NAME_MAX is 255
    try {
        throw geopm::Exception(too_long, GEOPM_ERROR_RUNTIME, "ExceptionTest.cpp", 1234);
    }
    catch (...) {
        geopm::exception_handler(std::current_exception(), false);
    }
    geopm_error_message(GEOPM_ERROR_RUNTIME, message_cstr, PATH_MAX);
    message = message_cstr;
    expect_long = message_prefix + too_long + ": at ExceptionTest.cpp:1234";
    EXPECT_EQ(expect_long, message);

    // Check that we get the short message when no exception was thrown
    geopm_error_message(GEOPM_ERROR_LOGIC, message_cstr, 256);
    message = message_cstr;
    EXPECT_EQ("<geopm> Logic error", message);
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
