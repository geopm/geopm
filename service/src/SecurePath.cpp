/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SecurePath.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <iostream>
#include <sstream>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

namespace geopm
{
    SecurePath::SecurePath(const std::string &orig_path)
        : SecurePath(orig_path,
                     (S_IWGRP | S_IWOTH), // 0o022
                     (geopm::has_cap_sys_admin()))
    {

    }

    SecurePath::SecurePath(const std::string &orig_path, mode_t umask, bool enforce)
        : m_fd(open(orig_path.c_str(), (O_RDONLY | O_NOFOLLOW)))
    {
        if (m_fd < 0) {
            throw Exception("SecurePath::" + std::string(__func__) +
                            "(): Failed to open file: ",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        if (enforce) {
            struct stat stat_struct;
            int err = fstat(m_fd, &stat_struct);
            if (err) {
                (void)!close(m_fd);
                throw Exception("SecurePath::" + std::string(__func__) + "(): ",
                                 errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            if (stat_struct.st_uid != getuid()) {
                (void)!close(m_fd);
                throw Exception("SecurePath::" + std::string(__func__) +
                                "(): File not owned by current user (id: " + std::to_string(getuid()) +
                                ") and will be ignored: " +
                                orig_path, GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            if (S_ISREG(stat_struct.st_mode) == 0) {
                (void)!close(m_fd);
                throw Exception("SecurePath::" + std::string(__func__) +
                                "(): File not a regular file and will be ignored: " +
                                orig_path, GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            mode_t perms_bits = stat_struct.st_mode & (~S_IFMT);
            if ((perms_bits & (~umask)) != perms_bits) {
                (void)!close(m_fd);
                std::ostringstream msg;
                msg << "SecurePath::" << std::string(__func__)
                    << "(): File has invalid permissions: " << orig_path << " :" << std::oct
                    << " Expected unset: 0o" << umask
                    << ", Actual: 0o" << perms_bits << std::dec;
                throw Exception(msg.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    SecurePath::~SecurePath()
    {
        (void)!close(m_fd);
    }

    std::string SecurePath::secure_path(void) const
    {
        return "/proc/self/fd/" + std::to_string(m_fd);
    }
}
