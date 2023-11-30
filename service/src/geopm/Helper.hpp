/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HELPER_HPP_INCLUDE
#define HELPER_HPP_INCLUDE

#include <stdint.h>
#include <sched.h>

#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <functional>
#include <set>

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

    /// @brief Joins a vector of strings together with a delimiter.
    /// @param [in] string_list The list of strings to be joined.
    /// @param [in] delim The delimiter to use to join the strings.
    /// @return The joined string.
    std::string string_join(const std::vector<std::string> &string_list,
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

    enum string_format_e {
        STRING_FORMAT_DOUBLE,
        STRING_FORMAT_INTEGER,
        STRING_FORMAT_HEX,
        STRING_FORMAT_RAW64,
    };
    /// @brief Convert a format type enum string_format_e to a format function
    std::function<std::string(double)> string_format_type_to_function(int format_type);
    /// @brief Convert a format function to a format name to a format function
    std::function<std::string(double)> string_format_name_to_function(const std::string &format_name);
    /// @brief Convert a format function to a format type enum string_format_e
    int string_format_function_to_type(std::function<std::string(double)> format_function);
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

    /// @brief Cache line size used to properly align structs to avoid
    ///        false sharing between threads.
    /// @todo  Replace with C++17 standard library equivalent.
    static constexpr int hardware_destructive_interference_size = 64;

    /// @brief Read an environment variable.
    /// @param [in] The name of the environment variable to read.
    /// @return The contents of the variable if present, otherwise an empty string.
    std::string get_env(const std::string &name);

    /// @brief Query for the user id associated with the process id.
    /// @param [in] pid The process id to query.
    /// @return The user id.
    unsigned int pid_to_uid(const int pid);

    /// @brief Query for the group id associated with the process id.
    /// @param [in] pid The process id to query.
    /// @return The group id.
    unsigned int pid_to_gid(const int pid);

    /// @brief Wrapper around CPU_ALLOC and CPU_FREE
    /// @param [in] num_cpu The number of CPUs to allocate the CPU set
    /// @param [in] cpu_enabled The CPUs to be included in the CPU set
    /// @return A std::unique_ptr to a cpu_set_t configured to use CPU_FREE as
    ///         the deleter.
    std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> >
        make_cpu_set(int num_cpu, const std::set<int> &cpu_enabled);

    /// @brief Check if the caller has effective capability CAP_SYS_ADMIN
    /// @return True if the PID has CAP_SYS_ADMIN
    bool has_cap_sys_admin(void);

    /// @brief Check if the pid has effective capability CAP_SYS_ADMIN
    /// @param [in] pid Linux PID to check
    /// @return True if the PID has CAP_SYS_ADMIN
    bool has_cap_sys_admin(int pid);
}

#endif
