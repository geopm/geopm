/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AGG_HPP_INCLUDE
#define AGG_HPP_INCLUDE

#include <vector>
#include <string>
#include <functional>

namespace geopm
{
    /// Aggregation functions available to be used by Agents and IOGroups to
    /// condense a vector of signal values into a single value.
    class Agg
    {
        public:
            enum m_type_e {
                M_SUM,
                M_AVERAGE,
                M_MEDIAN,
                M_INTEGER_BITWISE_OR,
                M_LOGICAL_AND,
                M_LOGICAL_OR,
                M_MIN,
                M_MAX,
                M_STDDEV,
                M_REGION_HASH,
                M_REGION_HINT,
                M_SELECT_FIRST,
                M_EXPECT_SAME,
                M_NUM_TYPE
            };
            /// @brief Returns the sum of the input operands.
            static double sum(const std::vector<double> &operand);
            /// @brief Returns the average of the input operands.
            static double average(const std::vector<double> &operand);
            /// @brief Returns the median of the input operands.
            static double median(const std::vector<double> &operand);
            /// @brief Returns the output of bitwise OR over all the
            ///        operands after casting them to int64_t where 0
            ///        is false and all positive numbers are true
            static double integer_bitwise_or(const std::vector<double> &operand);
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
            ///        GEOPM_REGION_HASH_UNMARKED.  This is intended for
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
            /// @brief Returns the corresponding agg function for a
            ///        given string name.  If the name does not match
            ///        a known function, it throws an error.
            static std::function<double(const std::vector<double> &)> name_to_function(const std::string &name);
            /// @brief Returns the corresponding agg function name for a
            ///        given std::function.  If the std::function does not match
            ///        a known function, it throws an error.
            static std::string function_to_name(std::function<double(const std::vector<double> &)> func);
            /// @brief Returns the corresponding agg function type for a
            ///        given std::function.  If the std::function does not match
            ///        a known function, it throws an error.
            static int function_to_type(std::function<double(const std::vector<double> &)> func);
            /// @brief Returns the corresponding agg function for one
            ///        of the Agg::m_type_e enum values.  If the
            ///        agg_type is out of range, it throws an error.
            static std::function<double(const std::vector<double> &)> type_to_function(int agg_type);
            /// @brief Returns the corresponding agg function name for
            ///        one of the Agg:m_type_e enum values.  If the
            ///        agg_type is out of range, it throws an error.
            static std::string type_to_name(int agg_type);
    };
}

#endif
