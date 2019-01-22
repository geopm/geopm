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

#ifndef GEOPM_TEST_HPP_INCLUDE
#define GEOPM_TEST_HPP_INCLUDE

#include <string>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Exception.hpp"

/// Checks that the given statement throws a geopm::Exception with the
/// right error code and message.  The message must be a substring of
/// the thrown Exception's what() string.  Additional details may be
/// added upon failure with the << operator, e.g.:
/// GEOPM_EXPECT_THROW_MESSAGE(...) << "more details";
/// This macro is based on gtest's EXPECT_THROW() macro.
#define GEOPM_EXPECT_THROW_MESSAGE(statement, expected_err, expected_message)   \
    std::string GTEST_CONCAT_TOKEN_(throw_message, __LINE__);                   \
    if (true) {  /* used with naked else to support << in failure case */       \
        bool geopm_expect_throw_do_fail = false;                                \
        try {                                                                   \
            statement;                                                          \
            GTEST_CONCAT_TOKEN_(throw_message, __LINE__) = "Expected to throw, but succeeded."; \
            geopm_expect_throw_do_fail = true;                                  \
        }                                                                       \
        catch (const geopm::Exception &ex) {                                    \
            EXPECT_EQ(expected_err, ex.err_value());                            \
            if (expected_err != ex.err_value()) {                               \
                geopm_expect_throw_do_fail = true;                              \
            }                                                                   \
            /* use std::string to support C/C++ strings for expected message */ \
            if (std::string(ex.what()).find(expected_message) == std::string::npos) { \
                GTEST_CONCAT_TOKEN_(throw_message, __LINE__) = std::string("Exception message was different from expected: ") + \
                                                           ex.what() + '\n';    \
                GTEST_CONCAT_TOKEN_(throw_message, __LINE__) += std::string("Expected message: ") + \
                                                            expected_message;   \
                geopm_expect_throw_do_fail = true;                              \
            }                                                                   \
        }                                                                       \
        catch (const std::exception &ex) {                                      \
            GTEST_CONCAT_TOKEN_(throw_message, __LINE__) = std::string("Threw a different type of exception: ") + ex.what(); \
            geopm_expect_throw_do_fail = true;                                  \
        }                                                                       \
        if (geopm_expect_throw_do_fail) {                                       \
            /* gtest wrapper to make goto label unique */                       \
            goto GTEST_CONCAT_TOKEN_(geopm_expect_throw_message_fail, __LINE__); \
        }                                                                       \
    }                                                                           \
    else                                                                        \
        GTEST_CONCAT_TOKEN_(geopm_expect_throw_message_fail, __LINE__):         \
            GTEST_NONFATAL_FAILURE_(GTEST_CONCAT_TOKEN_(throw_message, __LINE__).c_str())

#endif
