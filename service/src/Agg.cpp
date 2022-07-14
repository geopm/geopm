/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "geopm/Agg.hpp"
#include "geopm_hint.h"
#include "geopm_hash.h"

#include <cmath>

#include <algorithm>
#include <numeric>
#include <map>

#include "geopm/Exception.hpp"

namespace geopm
{
    std::vector<double> nan_filter(const std::vector<double> &operand)
    {
        std::vector<double> result;
        std::copy_if(operand.begin(), operand.end(), std::back_inserter(result),
                     [](double x) -> bool { return !std::isnan(x); });
        return result;
    }

    double Agg::sum(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        if (filtered.size()) {
            result = std::accumulate(filtered.begin(), filtered.end(), 0.0);
        }
        return result;
    }

    double Agg::average(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        if (filtered.size()) {
            result = Agg::sum(filtered) / filtered.size();
        }
        return result;
    }

    double Agg::median(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        size_t num_op = filtered.size();
        if (num_op) {
            size_t mid_idx = num_op / 2;
            bool is_even = ((num_op % 2) == 0);
            std::vector<double> operand_sorted(filtered);
            std::sort(operand_sorted.begin(), operand_sorted.end());
            result = operand_sorted[mid_idx];
            if (is_even) {
                result += operand_sorted[mid_idx - 1];
                result /= 2.0;
            }
        }
        return result;
    }

    double Agg::bitwise_or(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        uint64_t agg_tmp = 0;
        if (filtered.size()) {
            for (auto it : filtered) {
                if (it >= 0) {
                    agg_tmp |= (uint64_t) it;
                }
            }
            result = (double) agg_tmp;
        }
        return result;
    }

    double Agg::logical_and(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        if (filtered.size()) {
            result = std::all_of(filtered.begin(), filtered.end(),
                                 [](double it) {return (it != 0.0);});
        }
        return result;
    }

    double Agg::logical_or(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        if (filtered.size()) {
            result = std::any_of(filtered.begin(), filtered.end(),
                                 [](double it) {return (it != 0.0);});
        }
        return result;
    }

    static double common_value(const std::vector<double> &operand, double no_match)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        if (filtered.size() != 0) {
            result = std::all_of(filtered.cbegin(), filtered.cend(),
                                [filtered](double x) {
                                    return x == filtered[0];
                                }) ?
                     filtered[0] : no_match;
        }
        return result;
    }

    double Agg::region_hash(const std::vector<double> &operand)
    {
        return common_value(operand, GEOPM_REGION_HASH_UNMARKED);
    }

    double Agg::region_hint(const std::vector<double> &operand)
    {
        return common_value(operand, GEOPM_REGION_HINT_UNKNOWN);
    }

    double Agg::min(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        if (filtered.size()) {
            result = *std::min_element(filtered.begin(), filtered.end());
        }
        return result;
    }

    double Agg::max(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        if (filtered.size()) {
            result = *std::max_element(filtered.begin(), filtered.end());
        }
        return result;
    }

    double Agg::stddev(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double result = NAN;
        if (filtered.size() > 1) {
            double sum_squared = Agg::sum(filtered);
            sum_squared *= sum_squared;
            std::vector<double> operand_squared(filtered);
            for (auto &it : operand_squared) {
                it *= it;
            }
            double sum_squares = Agg::sum(operand_squared);
            double aa = 1.0 / (filtered.size() - 1);
            double bb = aa / filtered.size();
            result = std::sqrt(aa * sum_squares - bb * sum_squared);
        }
        else if (filtered.size() == 1) {
            result = 0.0;
        }
        return result;
    }

    double Agg::select_first(const std::vector<double> &operand)
    {
        // do not filter out NAN in case we are dealing with 64-bit raw MSRs
        double result = 0.0;
        if (operand.size() > 0) {
            result = operand[0];
        }
        return result;
    }

    double Agg::expect_same(const std::vector<double> &operand)
    {
        auto filtered = nan_filter(operand);
        double value = NAN;
        if (filtered.size() > 0) {
            value = filtered[0];
        }
        for (auto vv : filtered) {
            if (vv != value) {
                value = NAN;
                break;
            }
        }
        return value;
    }

    std::function<double(const std::vector<double> &)> Agg::name_to_function(const std::string &name)
    {
        std::map<std::string, std::function<double(const std::vector<double> &)> > name_map = {
            {"sum", sum},
            {"average", average},
            {"median", median},
            {"logical_and", logical_and},
            {"logical_or", logical_or},
            {"region_hash", region_hash},
            {"region_hint", region_hint},
            {"min", min},
            {"max", max},
            {"stddev", stddev},
            {"select_first", select_first},
            {"expect_same", expect_same},
        };

        auto result = name_map.find(name);
        if (result == name_map.end()) {
            throw Exception("Agg::name_to_function(): unknown aggregation function: " + name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result->second;
    }

    std::string Agg::function_to_name(std::function<double(const std::vector<double> &)> func)
    {
        std::map<decltype(&sum), std::string> function_map = {
            {sum, "sum"},
            {average, "average"},
            {median, "median"},
            {logical_and, "logical_and"},
            {logical_or, "logical_or"},
            {region_hash, "region_hash"},
            {region_hint, "region_hint"},
            {min, "min"},
            {max, "max"},
            {stddev, "stddev"},
            {select_first, "select_first"},
            {expect_same, "expect_same"},
        };

        auto f_ref = *(func.target<decltype(&sum)>());
        auto result = function_map.find(f_ref);
        if (result == function_map.end()) {
            throw Exception("Agg::function_to_name(): unknown aggregation function.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result->second;
    }

    int Agg::function_to_type(std::function<double(const std::vector<double> &)> func)
    {
        std::map<decltype(&sum), int> function_map = {
            {sum, M_SUM},
            {average, M_AVERAGE},
            {median, M_MEDIAN},
            {logical_and, M_LOGICAL_AND},
            {logical_or, M_LOGICAL_OR},
            {region_hash, M_REGION_HASH},
            {region_hint, M_REGION_HINT},
            {min, M_MIN},
            {max, M_MAX},
            {stddev, M_STDDEV},
            {select_first, M_SELECT_FIRST},
            {expect_same, M_EXPECT_SAME},
        };

        auto f_ref = *(func.target<decltype(&sum)>());
        auto result = function_map.find(f_ref);
        if (result == function_map.end()) {
            throw Exception("Agg::function_to_name(): unknown aggregation function.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result->second;
    }

    std::function<double(const std::vector<double> &)> Agg::type_to_function(int agg_type)
    {
        std::map<int, std::function<double(const std::vector<double> &)> > type_map = {
            {M_SUM, sum},
            {M_AVERAGE, average},
            {M_MEDIAN, median},
            {M_LOGICAL_AND, logical_and},
            {M_LOGICAL_OR, logical_or},
            {M_REGION_HASH, region_hash},
            {M_REGION_HINT, region_hint},
            {M_MIN, min},
            {M_MAX, max},
            {M_STDDEV, stddev},
            {M_SELECT_FIRST, select_first},
            {M_EXPECT_SAME, expect_same},
        };
        auto result = type_map.find(agg_type);
        if (result == type_map.end()) {
            throw Exception("Agg::type_to_function(): agg_type out of range: " + std::to_string(agg_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result->second;
    }

    std::string Agg::type_to_name(int agg_type)
    {
        std::map<int, std::string> type_map = {
            {M_SUM, "sum"},
            {M_AVERAGE, "average"},
            {M_MEDIAN, "median"},
            {M_LOGICAL_AND, "logical_and"},
            {M_LOGICAL_OR, "logical_or"},
            {M_REGION_HASH, "region_hash"},
            {M_REGION_HINT, "region_hint"},
            {M_MIN, "min"},
            {M_MAX, "max"},
            {M_STDDEV, "stddev"},
            {M_SELECT_FIRST, "select_first"},
            {M_EXPECT_SAME, "expect_same"},
        };
        auto result = type_map.find(agg_type);
        if (result == type_map.end()) {
            throw Exception("Agg::type_to_function(): agg_type out of range: " + std::to_string(agg_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result->second;
    }
}
