/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SDBus.hpp"
#include "SDBusMessage.hpp"
#include "geopm/Exception.hpp"

namespace geopm
{
    const char SDBusMessage::M_MESSAGE_TYPE_STRUCT = -1;
    const char SDBusMessage::M_MESSAGE_TYPE_ARRAY = -1;

    std::unique_ptr<SDBus> SDBus::make_unique(void)
    {
        throw Exception("SDBus is not enabled in this build, configure without --disable-systemd",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return nullptr;
    }
}
