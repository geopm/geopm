/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <errno.h>
#include <functional>
#include "gtest/gtest.h"
#include "ELF.hpp"
#include "geopm/Helper.hpp"
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
