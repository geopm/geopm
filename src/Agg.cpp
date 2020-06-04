/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "Agg.hpp"

#include <cmath>

#include <algorithm>
#include <numeric>

#include "geopm_internal.h"
#include "geopm_hash.h"

#include "config.h"

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
        return (filtered.size() &&
                std::all_of(filtered.cbegin(), filtered.cend(),
                            [filtered](double x) {
                                return x == filtered[0];
                            })) ?
               filtered[0] : no_match;
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
            std::vector<double> filtered_squared(filtered);
            for (auto &it : filtered_squared) {
                it *= it;
            }
            double sum_squares = Agg::sum(filtered_squared);
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
}
