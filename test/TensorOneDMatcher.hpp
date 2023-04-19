/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORONEDMATCHER_HPP_INCLUDE
#define TENSORONEDMATCHER_HPP_INCLUDE

#include "TensorOneD.hpp"

using geopm::TensorOneD;

class TensorOneDMatcher {
  public:
    using is_gtest_matcher = void;

    /* We store the data, not the instance. This helps
     * us avoid leaking the mock objects.
     */
    explicit TensorOneDMatcher(const TensorOneD& expected)
        : m_expected_values(expected.get_data()) {}

    bool MatchAndExplain(const TensorOneD& actual,
        std::ostream* /* listener */) const {
        return actual.get_data() == m_expected_values;
    }

    void DescribeTo(std::ostream* os) const {
        *os << "TensorOneD contents equal [";
        for (auto val : m_expected_values) {
            *os << val << " ";
        }
        *os << "]";
    }

    void DescribeNegationTo(std::ostream* os) const {
        *os << "TensorOneD contents do not equal [";
        for (auto val : m_expected_values) {
            *os << val << " ";
        }
        *os << "]";
    }

  private:
    const std::vector<double> m_expected_values;
};

::testing::Matcher<const TensorOneD&> TensorOneDEqualTo(const TensorOneD& expected);

#endif  // TENSORONEDMATCHER_HPP_INCLUDE
