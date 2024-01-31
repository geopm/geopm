/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include "IOUring.hpp"

#include "IOUringFallback.hpp"

#include <iostream>
#ifdef GEOPM_HAS_IO_URING
#include "IOUringImp.hpp"
#endif

namespace geopm
{
#ifdef GEOPM_HAS_IO_URING
    static void emit_missing_support_warning()
    {
#ifdef GEOPM_DEBUG
        static bool do_emit_warning = true;
        if (do_emit_warning) {
            do_emit_warning = false;
            std::cerr << "Warning: <geopm> GEOPM was built with liburing "
                         "enabled, but the system does not support all uring operations "
                         "needed by GEOPM. Using non-uring IO instead."
                      << std::endl;
        }
#endif
    }
#endif

    std::unique_ptr<IOUring> IOUring::make_unique(unsigned entries)
    {
#ifdef GEOPM_HAS_IO_URING
        if (IOUringImp::is_supported()) {
            return IOUringImp::make_unique(entries);
        }
        emit_missing_support_warning();
#endif
        return IOUringFallback::make_unique(entries);
    }
}
