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

#include "Agg.hpp"

#include <cmath>

#include <algorithm>
#include <numeric>

#include "geopm_internal.h"
#include "geopm_hash.h"

#include "config.h"

namespace geopm
{
    double Agg::sum(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = std::accumulate(operand.begin(), operand.end(), 0.0);
        }
        return result;
    }

    double Agg::average(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = Agg::sum(operand) / operand.size();
        }
        return result;
    }

    double Agg::median(const std::vector<double> &operand)
    {
        double result = NAN;
        size_t num_op = operand.size();
        if (num_op) {
            size_t mid_idx = num_op / 2;
            bool is_even = ((num_op % 2) == 0);
            std::vector<double> operand_sorted(operand);
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
        double result = NAN;
        if (operand.size()) {
            result = std::all_of(operand.begin(), operand.end(),
                                 [](double it) {return (it != 0.0);});
        }
        return result;
    }

    double Agg::logical_or(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = std::any_of(operand.begin(), operand.end(),
                                 [](double it) {return (it != 0.0);});
        }
        return result;
    }

    double Agg::region_id(const std::vector<double> &operand)
    {
        uint64_t common_rid = GEOPM_REGION_ID_UNMARKED;
        if (operand.size()) {
            for (const auto &it : operand) {
                uint64_t it_rid = geopm_signal_to_field(it);
                if (it_rid != GEOPM_REGION_ID_UNMARKED &&
                    common_rid == GEOPM_REGION_ID_UNMARKED) {
                    common_rid = it_rid;
                }
                if (common_rid != GEOPM_REGION_ID_UNMARKED &&
                    it_rid != GEOPM_REGION_ID_UNMARKED &&
                    it_rid != common_rid) {
                    common_rid = GEOPM_REGION_ID_UNMARKED;
                    break;
                }
            }
        }
        return geopm_field_to_signal(common_rid);
    }

    static double common_value(const std::vector<double> &operand, double no_match)
    {
        return (operand.size() &&
                std::all_of(operand.cbegin(), operand.cend(),
                            [operand](double x) {
                                return x == operand[0];
                            })) ?
               operand[0] : no_match;
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
        double result = NAN;
        if (operand.size()) {
            result = *std::min_element(operand.begin(), operand.end());
        }
        return result;
    }

    double Agg::max(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size()) {
            result = *std::max_element(operand.begin(), operand.end());
        }
        return result;
    }

    double Agg::stddev(const std::vector<double> &operand)
    {
        double result = NAN;
        if (operand.size() > 1) {
            double sum_squared = Agg::sum(operand);
            sum_squared *= sum_squared;
            std::vector<double> operand_squared(operand);
            for (auto &it : operand_squared) {
                it *= it;
            }
            double sum_squares = Agg::sum(operand_squared);
            double aa = 1.0 / (operand.size() - 1);
            double bb = aa / operand.size();
            result = std::sqrt(aa * sum_squares - bb * sum_squared);
        }
        else if (operand.size() == 1) {
            result = 0.0;
        }
        return result;
    }

    double Agg::select_first(const std::vector<double> &operand)
    {
        double result = 0.0;
        if (operand.size() > 0) {
            result = operand[0];
        }
        return result;
    }

    double Agg::expect_same(const std::vector<double> &operand)
    {
        double value = NAN;
        if (operand.size() > 0) {
            value = operand[0];
        }
        for (auto vv : operand) {
            if (vv != value) {
                value = NAN;
                break;
            }
        }
        return value;
    }
}
