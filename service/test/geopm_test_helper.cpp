/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "geopm_test.hpp"
#include "geopm_field.h"

#include "geopm_hint.h"
#include "geopm_hash.h"
#include "geopm/Agg.hpp"

bool is_format_double(std::function<std::string(double)> func)
{
    double value = (double)0x3FF00000000000ULL;
    std::string expected = "1.799680632343757e+16";
    return func(value) == expected;
}

bool is_format_float(std::function<std::string(double)> func)
{
    double value = (double)0x3FF00000000000ULL;
    std::string expected = "1.79968e+16";
    return func(value) == expected;
}

bool is_format_integer(std::function<std::string(double)> func)
{
    double value = (double)0x3FF00000000000ULL;
    std::string expected = "17996806323437568";
    return func(value) == expected;
}

bool is_format_hex(std::function<std::string(double)> func)
{
    double value = (double)0x3FF00000000000ULL;
    std::string expected = "0x003ff00000000000";
    return func(value) == expected;
}

bool is_format_raw64(std::function<std::string(double)> func)
{
    uint64_t value_i = 0x3FF00000000000ULL;
    double value = geopm_field_to_signal(value_i);
    std::string expected = "0x003ff00000000000";
    return func(value) == expected;
}

static std::vector<double> example_data = {1, 2, 4, 10};

bool is_agg_sum(std::function<double(const std::vector<double> &)> func)
{
    return func(example_data) == geopm::Agg::sum(example_data);
}

bool is_agg_average(std::function<double(const std::vector<double> &)> func)
{
    return func(example_data) == geopm::Agg::average(example_data);
}

bool is_agg_median(std::function<double(const std::vector<double> &)> func)
{
    return func(example_data) == geopm::Agg::median(example_data);
}

bool is_agg_integer_bitwise_or(std::function<double(const std::vector<double> &)> func)
{
    return func({1, 2, 3}) == 3.0 &&
           func({1, 0, 1}) == 1.0 &&
           func({0, 0, 0}) == 0.0 &&
           func({4, 2, 1}) == 7 &&
           std::isnan(func({}));
}

bool is_agg_logical_and(std::function<double(const std::vector<double> &)> func)
{
    return func({1, 1, 1}) == 1.0 &&
           func({1, 0, 1}) == 0.0 &&
           std::isnan(func({}));
}

bool is_agg_logical_or(std::function<double(const std::vector<double> &)> func)
{
    return func({1, 1, 1}) == 1.0 &&
           func({1, 0, 1}) == 1.0 &&
           func({0, 0, 0}) == 0.0 &&
           std::isnan(func({}));
}

bool is_agg_region_hash(std::function<double(const std::vector<double> &)> func)
{
    return func({33, 44, 33}) == GEOPM_REGION_HASH_UNMARKED &&
           func({44, 44, 44}) == 44;
}

bool is_agg_region_hint(std::function<double(const std::vector<double> &)> func)
{
    return func({1, 2, 3}) == GEOPM_REGION_HINT_UNKNOWN &&
           func({2, 2, 2}) == 2;
}

bool is_agg_min(std::function<double(const std::vector<double> &)> func)
{
    return func(example_data) == geopm::Agg::min(example_data);
}

bool is_agg_max(std::function<double(const std::vector<double> &)> func)
{
    return func(example_data) == geopm::Agg::max(example_data);
}

bool is_agg_stddev(std::function<double(const std::vector<double> &)> func)
{
    return func(example_data) == geopm::Agg::stddev(example_data);
}

bool is_agg_select_first(std::function<double(const std::vector<double> &)> func)
{
    return func(example_data) == geopm::Agg::select_first(example_data);
}

bool is_agg_expect_same(std::function<double(const std::vector<double> &)> func)
{
    return func({3.3, 3.3, 3.3}) == 3.3 &&
           std::isnan(func({4.4, 4.4, 3.3, 4.4}));
}
