/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORMIOPROF_HPP_INCLUDE
#define PLATFORMIOPROF_HPP_INCLUDE

#include <string>

namespace geopm
{
    class PlatformIO;
    class PlatformIOProf {
        public:
            static PlatformIO &platform_io(void);
            virtual ~PlatformIOProf() = default;
        private:
            PlatformIOProf();
            void print_load_warning(const std::string &io_group_name,
                                    const std::string &what) const;
            PlatformIO &m_platform_io;
    };
}

#endif
