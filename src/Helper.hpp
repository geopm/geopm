/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef HELPER_HPP_INCLUDE
#define HELPER_HPP_INCLUDE

#include <string>
#include <memory>
#include <utility>
#include <vector>

namespace geopm
{
    /// @brief Implementation of std::make_unique (C++14) for C++11.
    ///        Note that this version will only work for non-array
    ///        types.
    template <class Type, class ...Args>
    std::unique_ptr<Type> make_unique(Args &&...args)
    {
        return std::unique_ptr<Type>(new Type(std::forward<Args>(args)...));
    }

    /// @brief Reads the specified file and returns the contents in a string.
    /// @param [in] path The path of the file to read.
    /// @return The contents of the file at path.
    std::string read_file(const std::string& path);

    /// @brief Splits a string according to a delimiter.
    /// @param [in] str The string to be split.
    /// @param [in] delim The delimiter to use to divide the string.
    ///        Cannot be empty.
    /// @return A vector of string pieces.
    std::vector<std::string> split_string(const std::string &str,
                                          const std::string &delim);
}

#endif
