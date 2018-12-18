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

#ifndef AGG_HPP_INCLUDE
#define AGG_HPP_INCLUDE

#include <vector>

namespace geopm
{
    /// Aggregation functions available to be used by Agents and IOGroups to
    /// condense a vector of signal values into a single value.
    class Agg
    {
        public:
            /// @brief Returns the sum of the input operands.
            static double sum(const std::vector<double> &operand);
            /// @brief Returns the average of the input operands.
            static double average(const std::vector<double> &operand);
            /// @brief Returns the median of the input operands.
            static double median(const std::vector<double> &operand);
            /// @brief Returns the output of logical AND over all the
            ///        operands where 0.0 is false and all other
            ///        values are true.
            static double logical_and(const std::vector<double> &operand);
            /// @brief Returns the output of logical OR over all the
            ///        operands where 0.0 is false and all other
            ///        values are true.
            static double logical_or(const std::vector<double> &operand);
            /// @brief Returns the minimum value from the input
            ///        operands.
            static double min(const std::vector<double> &operand);
            /// @brief Returns the maximum value from the input
            ///        operands.
            static double max(const std::vector<double> &operand);
            /// @brief Returns the standard deviation of the input
            ///        operands.
            static double stddev(const std::vector<double> &operand);
            /// @brief If all operands are the same, returns the
            ///        common value.  Otherwise, returns
            ///        GEOPM_HASH_REGION_UNMARKED.  This is intended for
            ///        situations where all ranks in a domain must be
            ///        in the same region to exert control for that
            ///        region.
            static double region_hash(const std::vector<double> &operand);
            /// @brief If all operands are the same, returns the
            ///        common value.  Otherwise, returns
            ///        GEOPM_REGION_HINT_UNKNOWN.  This is intended for
            ///        situations where all ranks in a domain must be
            ///        in the same region to exert control for that
            ///        region.
            static double region_hint(const std::vector<double> &operand);
            /// @brief Returns the first value in the vector and
            ///        ignores other values.  If the vector is empty,
            ///        returns 0.0.
            static double select_first(const std::vector<double> &operand);
            /// @brief Returns the common value if all values are the same,
            ///        or NAN otherwise.  This function should not be used
            ///        to aggregate values that may be interpreted as NAN
            ///        such as raw register values.
            static double expect_same(const std::vector<double> &operand);
    };
}

#endif
