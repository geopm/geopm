/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKDOMAINNETMAP_HPP_INCLUDE
#define MOCKDOMAINNETMAP_HPP_INCLUDE

#include "gmock/gmock.h"
#include "DomainNetMap.hpp"

namespace geopm {

class MockDomainNetMap : public DomainNetMap {
 public:
  MOCK_METHOD(void, sample, (), (override));
  MOCK_METHOD(std::vector<std::string>, trace_names, (), (const, override));
  MOCK_METHOD(std::vector<double>, trace_values, (), (const, override));
  MOCK_METHOD((std::map<std::string, double>), last_output, (), (const, override));
};

}  // namespace geopm

#endif //MOCKDOMAINNETMAP_HPP_INCLUDE
