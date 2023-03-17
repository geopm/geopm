/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "PlatformIOProf.hpp"

#include <string>
#ifdef GEOPM_DEBUG
#include <iostream>
#endif

#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "ProfileIOGroup.hpp"


namespace geopm
{
    PlatformIO &PlatformIOProf::platform_io(void)
    {
        static PlatformIOProf instance;
        return instance.m_platform_io;
    }

    PlatformIOProf::PlatformIOProf()
        : m_platform_io(geopm::platform_io())
    {
        try {
            m_platform_io.register_iogroup(
                ProfileIOGroup::make_plugin());
        }
        catch (const geopm::Exception &ex) {
            print_load_warning("ProfileIOGroup", ex.what());
        }
    }
    void PlatformIOProf::print_load_warning(const std::string &io_group_name,
                                            const std::string &what) const
    {
#ifdef GEOPM_DEBUG
        std::cerr << "Warning: <geopm> Failed to load " << io_group_name
                  << " IOGroup.  "
                  << "GEOPM may not work properly unless an alternate "
                  << "IOGroup plugin is loaded to provide signals/controls "
                  << "required."
                  << std::endl;
        std::cerr << "The error was: " << what << std::endl;
#endif
    }
}

