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
    std::string read_file(const std::string &path);

    /// @brief Read a file and return a double read from the file.
    /// @details If a double cannot be read from the file or the units reported
    ///          in the file do not match the expected units, an exception is
    ///          thrown.
    /// @param [in] path The path of the file to read.
    /// @param [in] expected_units Expected units to follow the double. Provide
    ///             an empty string if no units are expected.
    /// @return The value read from the file.
    double read_double_from_file(const std::string &path,
                                 const std::string &expected_units);

    /// @brief Writes a string to a file.  This will replace the file
    ///        if it exists or create it if it does not exist.
    /// @param [in] path The path to the file to write to.
    /// @param [in] contents The string to write to the file.
    void write_file(const std::string &path, const std::string &contents);

    /// @brief Splits a string according to a delimiter.
    /// @param [in] str The string to be split.
    /// @param [in] delim The delimiter to use to divide the string.
    ///        Cannot be empty.
    /// @return A vector of string pieces.
    std::vector<std::string> string_split(const std::string &str,
                                          const std::string &delim);

    /// @brief Returns the current hostname as a string.
    std::string hostname(void);

    /// @brief List all files in the given directory.
    /// @param [in] path Path to the directory.
    std::vector<std::string> list_directory_files(const std::string &path);

    /// @brief Returns whether one string begins with another.
    bool string_begins_with(const std::string &str, const std::string &key);

    /// @brief Returns whether one string ends with another.
    bool string_ends_with(std::string str, std::string key);

    /// @brief Format a string to best represent a signal encoding a
    ///        double precision floating point number.
    /// @param [in] signal A real number that requires many
    ///        significant digits to accurately represent.
    /// @return A well-formatted string representation of the signal.
    std::string string_format_double(double signal);

    /// @brief Format a string to best represent a signal encoding a
    ///        single precision floating point number.
    /// @param [in] signal A real number that requires a few
    ///        significant digits to accurately represent.
    /// @return A well formatted string representation of the signal.
    std::string string_format_float(double signal);

    /// @brief Format a string to best represent a signal encoding a
    ///        decimal integer.
    /// @param [in] signal An integer that is best represented as a
    ///        decimal number.
    /// @return A well formatted string representation of the signal.
    std::string string_format_integer(double signal);

    /// @brief Format a string to best represent a signal encoding an
    ///        unsigned hexadecimal integer.
    /// @param [in] signal An unsigned integer that is best
    ///        represented as a hexadecimal number and has been
    ///        assigned to a double precision number
    /// @return A well formatted string representation of the signal.
    std::string string_format_hex(double signal);

    /// @brief Format a string to represent the raw memory supporting
    ///        a signal as an unsigned hexadecimal integer.
    /// @param [in] signal A 64-bit unsigned integer that has been
    ///        byte-wise copied into the memory of signal.
    /// @return A well formatted string representation of the signal.
    std::string string_format_raw64(double signal);
}

#endif
