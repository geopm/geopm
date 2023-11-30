/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SysfsDriver.hpp"

namespace geopm
{
    std::map<std::string, SysfsDriver::properties_s> SysfsDriver::parse_properties_json(std::string properties_json)
    {
        std::map<std::string, SysfsDriver::properties_s> result;
        return result;
    }
}
