/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "EpochIOGroup.hpp"


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
        try {
            m_platform_io.register_iogroup(
                EpochIOGroup::make_plugin());
        }
        catch (const geopm::Exception &ex) {
            print_load_warning("EpochIOGroup", ex.what());
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

