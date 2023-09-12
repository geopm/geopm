/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SECUREPATH_HPP_INCLUDE
#define SECUREPATH_HPP_INCLUDE

#include "config.h"

#include <sys/stat.h>

#include <string>
#include <memory>

namespace geopm
{
    /// @brief Helper object to optionally verify that a file was created with a
    //         particular umask.  This object will call open() on a file, and
    //         provide a path to the open file under procfs for secure reading/writing.
    //         The file will be closed on destruction of this object.
    class SecurePath
    {
        public:
            SecurePath() = delete;
            SecurePath(const SecurePath &other) = delete;
            SecurePath &operator=(const SecurePath &other) = delete;

            /// @brief Verify if orig_path is secure.  This defaults to enforcing for
            //         the root user with a umask of (S_IWGRP | S_IWOTH) or 0o022.
            SecurePath(const std::string &orig_path);
            /// @brief Verify if orig_path is secure (optional)
            /// @param orig_path [in] Path to the file to examine
            /// @param umask [in] The umask with which to assert orig_path was created
            /// @param enforce [in] Whether or not to enforce ownership/permissions checks
            SecurePath(const std::string &orig_path, mode_t umask, bool enforce);

            virtual ~SecurePath();

            /// @brief Accessor for path under procfs
            std::string secure_path(void) const;

        private:
            int m_fd;
    };
}
#endif
