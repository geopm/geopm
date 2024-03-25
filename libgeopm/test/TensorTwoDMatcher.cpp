/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "TensorTwoD.hpp"
#include "TensorTwoDMatcher.hpp"

using geopm::TensorTwoD;

::testing::Matcher<const TensorTwoD&> TensorTwoDEqualTo(const TensorTwoD& expected) {
  return TensorTwoDMatcher(expected);
}
