/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "geopm_hint.h"

#include <string>

#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"

namespace geopm {

    void check_hint(uint64_t hint)
    {
        if (hint >= GEOPM_NUM_REGION_HINT) {
            throw geopm::Exception("Helper::" + std::string(__func__) +
                            "(): hint out of range: " +
                            geopm::string_format_hex(hint),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

}
