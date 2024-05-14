/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "geopm_version.h"

#include <stdexcept>

#include "geopm_debug.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"


const char *geopm_version(void)
{
    return PACKAGE_VERSION;
}

namespace geopm
{
    std::string version(void)
    {
        return PACKAGE_VERSION;
    }

    std::vector<int> shared_object_version(void)
    {
        geopm::Exception logic_error("geopm::plugin_load(): Could not parse GEOPM_ABI_VERSION: " GEOPM_ABI_VERSION,
                                     GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        std::vector<int> abi_nums;
        try {
            for (const auto &abi_str : geopm::string_split(GEOPM_ABI_VERSION, ":")) {
                abi_nums.push_back(std::stoi(abi_str));
            }
        }
        catch (const std::invalid_argument &ex) {
            throw logic_error;
        }
        catch (const std::out_of_range &ex) {
            throw logic_error;
        }
        GEOPM_DEBUG_ASSERT(abi_nums.size() == 3, logic_error.what());
        return {abi_nums.at(0) - abi_nums.at(2), abi_nums.at(2), abi_nums.at(1)};
    }
}
