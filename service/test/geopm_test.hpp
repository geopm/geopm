/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_TEST_HPP_INCLUDE
#define GEOPM_TEST_HPP_INCLUDE

#include <string>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm/Exception.hpp"

bool is_format_double(std::function<std::string(double)> func);
bool is_format_float(std::function<std::string(double)> func);
bool is_format_integer(std::function<std::string(double)> func);
bool is_format_hex(std::function<std::string(double)> func);
bool is_format_raw64(std::function<std::string(double)> func);

bool is_agg_sum(std::function<double(const std::vector<double> &)> func);
bool is_agg_average(std::function<double(const std::vector<double> &)> func);
bool is_agg_median(std::function<double(const std::vector<double> &)> func);
bool is_agg_integer_bitwise_or(std::function<double(const std::vector<double> &)> func);
bool is_agg_logical_and(std::function<double(const std::vector<double> &)> func);
bool is_agg_logical_or(std::function<double(const std::vector<double> &)> func);
bool is_agg_region_hash(std::function<double(const std::vector<double> &)> func);
bool is_agg_region_hint(std::function<double(const std::vector<double> &)> func);
bool is_agg_min(std::function<double(const std::vector<double> &)> func);
bool is_agg_max(std::function<double(const std::vector<double> &)> func);
bool is_agg_stddev(std::function<double(const std::vector<double> &)> func);
bool is_agg_select_first(std::function<double(const std::vector<double> &)> func);
bool is_agg_expect_same(std::function<double(const std::vector<double> &)> func);

/// @brief Skip decorator for gtest test fixtures
///
/// Add to the top of any test fixture which has a requirement that may not be
/// met in typical CI situations, like single CPU VMs that are frequently
/// interrupted.  This should be applied to tests that require more than one
/// active thread.  Tests that are sensitive to delays in execution due to
/// timing requirements should also be decorated.  To enable these tests,
/// export GEOPM_TEST_EXTENDED in the environment.
///
/// @param [in] reason The requirement that may not be met in all test cases
#define GEOPM_TEST_EXTENDED(reason) \
if (getenv("GEOPM_TEST_EXTENDED") == nullptr) \
GTEST_SKIP() << reason \
             << ", export GEOPM_TEST_EXTENDED=1 to enable\n";

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
