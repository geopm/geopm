/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UNIQUEFD_HPP_INCLUDE
#define UNIQUEFD_HPP_INCLUDE

namespace geopm
{
    /// A wrapper to close a file descriptor when the descriptor goes out of scope.
    class UniqueFd
    {
        public:
            UniqueFd() = delete;

            // Disable copies since we only want one wrapper to close the fd
            UniqueFd(UniqueFd const&) = delete;
            UniqueFd& operator=(UniqueFd const&) = delete;

            /// Main entry point: wrap a raw file descriptor
            UniqueFd(int fd);

            /// Move constructor: make sure the source object no longer closes the wrapped fd
            UniqueFd(UniqueFd &&other);

            /// Close the wrapped fd if it is valid.
            ~UniqueFd();

            /// Get the wrapped raw fd (e.g., to call IO functions on it)
            int get();
        private:
            int m_fd;
    };
}

#endif
