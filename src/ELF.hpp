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

#ifndef ELF_HPP_INCLUDE
#define ELF_HPP_INCLUDE

#include <string>
#include <memory>
#include <map>

namespace geopm
{
    /// @brief Look up the nearest symbol lower than an instruction
    ///        address.
    /// @param instruction_ptr Address of an instruction or function.
    /// @return Pair of symbol location and symbol name.  If symbol
    ///         couldn't be found, location is zero and symbol name is
    ///         empty.
    std::pair<size_t, std::string> symbol_lookup(void *instruction_ptr);

    /// @brief Get a map from symbol location to symbol name for all
    ///        symbols in an ELF file.
    /// @param file_path Path to ELF encoded binary file.
    /// @return Map from symbol location to symbol name.
    std::map<size_t, std::string> elf_symbol_map(const std::string &file_path);

    /// @brief Class encapsulating interactions with ELF files.
    class ELF
    {
        public:
            ELF() = default;
            virtual ~ELF() = default;
            /// @brief Get the number of symbols in the current
            ///        section.
            /// @return Number of symbols.
            virtual size_t num_symbol(void) = 0;
            /// @brief Get the name of the current symbol.
            /// @return Current symbol name.  Will return an empty
            ///         string if all symbols in section have been
            ///         iterated over.
            virtual std::string symbol_name(void) = 0;
            /// @brief Get the offest of the current symbol.
            /// @return Current symbol offset.  Will return zero if
            ///         all symbols in section have been iterated
            ///         over.
            virtual size_t symbol_offset(void) = 0;
            /// @brief Iterate to the next section.
            /// @return True if next section exists, false when all
            ///         sections have been iterated over.
            virtual bool next_section(void) = 0;
            /// @brief Iterate to the next data descriptor in the
            ///        section.
            /// @return True if next data descriptor exists, false
            ///         when all data descriptors in the section have
            ///         been iterated over.
            virtual bool next_data(void) = 0;
            /// @brief Iterate to the next symbol in the section.
            /// @return True if next symbol in section exists, false
            ///         when all symbols in the section have been
            ///         iterated over.
            virtual bool next_symbol(void) = 0;
    };

    /// @brief Factory method to construct an ELF pointer from a path
    ///        to an ELF encoded file.
    std::shared_ptr<ELF> elf(const std::string &file_path);

}

#endif
