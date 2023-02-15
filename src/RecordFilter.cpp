/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "RecordFilter.hpp"
#include "ProxyEpochRecordFilter.hpp"
#include "EditDistEpochRecordFilter.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"

namespace geopm
{
    std::unique_ptr<RecordFilter> RecordFilter::make_unique(const std::string &name)
    {
        std::unique_ptr<RecordFilter> result;
        if (string_begins_with(name, "proxy_epoch")) {
            result = geopm::make_unique<ProxyEpochRecordFilter>(name);
        }
        else if (string_begins_with(name, "edit_distance")) {
            result = geopm::make_unique<EditDistEpochRecordFilter>(name);
        }
        else {
            throw Exception("RecordFilter::make_unique(): Unable to parse name: " + name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }
}
