/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ELF_HPP_INCLUDE
#define ELF_HPP_INCLUDE

#include <string>
#include <memory>
#include <map>

namespace geopm
{
    /// @brief Look up the nearest symbol lower than an instruction
    ///        address.
    /// @param [in] instruction_ptr Address of an instruction or function.
    /// @return Pair of symbol location and symbol name.  If symbol
    ///         couldn't be found, location is zero and symbol name is
    ///         empty.
    std::pair<size_t, std::string> symbol_lookup(const void *instruction_ptr);

    /// @brief Get a map from symbol location to symbol name for all
    ///        symbols in an ELF file.
    /// @param [in] file_path Path to ELF encoded binary file.
    /// @return Map from symbol location to symbol name.
    std::map<size_t, std::string> elf_symbol_map(const std::string &file_path);
}

#endif
