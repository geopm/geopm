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

#include <stdlib.h>
#include <errno.h>
#include <functional>
#include "gtest/gtest.h"
#include "ELF.hpp"
#include "Helper.hpp"
#include "geopm_hash.h"

class ELFTest: public :: testing :: Test
{
    protected:
        void SetUp();
        std::string m_program_name;
};

void ELFTest::SetUp()
{
    m_program_name = program_invocation_name;
}

bool ELFTestFunction(void)
{
    return random() % 2;
}

extern "C" {
bool elf_test_function(void)
{
    return random() % 4;
}
}


TEST_F(ELFTest, symbols_exist)
{
    std::map<size_t, std::string> off_sym_map(geopm::elf_symbol_map(m_program_name));
    EXPECT_LT(0ULL, off_sym_map.size());
}

TEST_F(ELFTest, symbol_lookup)
{
    // Lookup a C++ symbol in the elf header
    std::pair<size_t, std::string> symbol = geopm::symbol_lookup((void*)ELFTestFunction);
    EXPECT_EQ((size_t)&ELFTestFunction, symbol.first);
    EXPECT_EQ("ELFTestFunction()", symbol.second);

    // Lookup a C symbol in the elf header
    symbol = geopm::symbol_lookup((void*)elf_test_function);
    EXPECT_EQ((size_t)&elf_test_function, symbol.first);
    EXPECT_EQ("elf_test_function", symbol.second);

    // Lookup a C symbol in the elf header offset by 8 bytes
    size_t fn_off = (size_t)(elf_test_function);
    fn_off += 8;
    symbol = geopm::symbol_lookup((void*)fn_off);
    EXPECT_EQ((size_t)&elf_test_function, symbol.first);
    EXPECT_EQ("elf_test_function", symbol.second);

    // Lookup a C++ symbol in the shared object table
    symbol = geopm::symbol_lookup((void*)geopm::string_format_double);
    EXPECT_TRUE(geopm::string_begins_with(symbol.second, "geopm::string_format_double")) << symbol.second;
    EXPECT_TRUE(geopm::string_ends_with(symbol.second, "(double)")) << symbol.second;

    // Lookup a C symbol in the shared object table
    symbol = geopm::symbol_lookup((void*)geopm_crc32_str);
    EXPECT_EQ("geopm_crc32_str", symbol.second);

    // Lookup a C symbol in the shared object table offset by 8 bytes
    fn_off = (size_t)geopm_crc32_str;
    fn_off += 8;
    symbol = geopm::symbol_lookup((void*)fn_off);
    EXPECT_EQ("geopm_crc32_str", symbol.second);
}
