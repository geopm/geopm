/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORTWODMATCHER_HPP_INCLUDE
#define TENSORTWODMATCHER_HPP_INCLUDE

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

using geopm::TensorTwoD;

class TensorTwoDMatcher {
  public:
    using is_gtest_matcher = void;

    /* We store the data, not the instance. This helps
     * us avoid leaking the mock objects.
     */
    explicit TensorTwoDMatcher(const TensorTwoD& expected)
      : m_expected_values(expected.get_rows())
    {
      for (size_t i = 0; i < expected.get_rows(); ++i) {
	m_expected_values[i] = std::vector<double>(expected[i].get_data());
      }
    }

    bool MatchAndExplain(const TensorTwoD& actual,
        std::ostream* /* listener */) const {
      bool rval = true;
      for (size_t i = 0; i < m_expected_values.size(); ++i) {
	if (actual[i].get_data() != m_expected_values[i]) {
	  rval = false;
	}
      }
      return rval;
    }

    void DescribeTo(std::ostream* os) const {
      *os << "TensorTwoD contents equal [";
      for (std::vector<double> row : m_expected_values) {
	for (double val : row) {
	  *os << val << " ";
	}
	*os << "; ";
      }
      *os << "]";
    }

    void DescribeNegationTo(std::ostream* os) const {
      *os << "TensorTwoD contents do not equal [";
      for (std::vector<double> row : m_expected_values) {
	for (double val : row) {
	  *os << val << " ";
	}
	*os << "; ";
      }
      *os << "]";
    }

  private:
    std::vector<std::vector<double>> m_expected_values;
};

::testing::Matcher<const TensorTwoD&> TensorTwoDEqualTo(const TensorTwoD& expected);

#endif  // TENSORTWODMATCHER_HPP_INCLUDE
