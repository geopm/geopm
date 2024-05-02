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
    class __attribute__((visibility("default"))) PlatformIOProf {
        public:
            static PlatformIO &platform_io(void);
            virtual ~PlatformIOProf() = default;
        private:
            __attribute__((visibility("hidden"))) PlatformIOProf();
            void __attribute__((visibility("hidden"))) print_load_warning(const std::string &io_group_name,
                                    const std::string &what) const;
            PlatformIO &m_platform_io;
    };
}

#endif
