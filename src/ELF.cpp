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

#include "config.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include <dlfcn.h>
#include <cxxabi.h>

#include "ELF.hpp"
#include "Exception.hpp"
#include "Helper.hpp"

namespace geopm
{
    std::map<size_t, std::string> elf_symbol_map(const std::string &file_path)
    {
        std::map<size_t, std::string> result;
        std::shared_ptr<ELF> elf_ptr = elf(file_path);
        do {
            do {
                if (elf_ptr->num_symbol()) {
                    do {
                        result[elf_ptr->symbol_offset()] = elf_ptr->symbol_name();
                    } while (elf_ptr->next_symbol());
                }
            } while (elf_ptr->next_data());
        } while (elf_ptr->next_section());
        return result;
    }

    std::pair<size_t, std::string> symbol_lookup(void *instruction_ptr)
    {
        std::pair<size_t, std::string> result(0, "");
        size_t target = (size_t)instruction_ptr;
        Dl_info info;
        bool dladdr_success = dladdr(instruction_ptr, &info);
        // "dladdr() returns 0 on error, and nonzero on success."
        if (dladdr_success != 0) {
            // dladdr() found the file
            if (info.dli_sname) {
                // dladdr() found the symbol
                result.first = (size_t)info.dli_saddr;
                result.second = info.dli_sname;
            }
            else {
                std::map<size_t, std::string> symbol_map(elf_symbol_map(info.dli_fname));
                auto symbol_it = symbol_map.upper_bound(target);
                if (symbol_it != symbol_map.begin()) {
                    --symbol_it;
                }
                if (symbol_it->first <= target) {
                    result = *symbol_it;
                }
            }
        }
        if (result.second.size()) {
            // Check for C++ mangling
            const char *demangled_name = abi::__cxa_demangle(result.second.c_str(), NULL, NULL, NULL);
            if (demangled_name != NULL) {
                result.second = demangled_name;
            }
            // End all C functions with ()
            if (!string_ends_with(result.second, ")")) {
                result.second += "()";
            }
        }
        return result;
    }

    class ELFImp : public ELF
    {
        public:
            ELFImp() = delete;
            ELFImp(const std::string &file_path);
            virtual ~ELFImp();
            size_t num_symbol(void) override;
            std::string symbol_name(void) override;
            size_t symbol_offset(void) override;
            bool next_section(void) override;
            bool next_data(void) override;
            bool next_symbol(void) override;
        private:
            int m_file_desc;
            struct Elf *m_elf_handle;
            struct Elf_Scn *m_section;
            std::unique_ptr<GElf_Shdr> m_section_header;
            Elf_Data *m_data;
            size_t m_symbol_idx;
            std::unique_ptr<GElf_Sym> m_symbol;
    };

    std::shared_ptr<ELF> elf(const std::string &file_path)
    {
        return std::make_shared<ELFImp>(file_path);
    }

    ELFImp::ELFImp(const std::string &file_path)
        : m_file_desc(-1)
        , m_elf_handle(nullptr)
        , m_section(nullptr)
        , m_section_header(geopm::make_unique<GElf_Shdr>())
        , m_data(nullptr)
        , m_symbol_idx(0)
        , m_symbol(geopm::make_unique<GElf_Sym>())
    {
        unsigned int version = elf_version(EV_CURRENT);
        if (version == EV_NONE) {
            throw Exception("ELFImp::ELFImp(): version unsupported",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_file_desc = open(file_path.c_str(), O_RDONLY, 0);
        if (m_file_desc < 0) {
            throw Exception("ELFImp::ELFImp(): file_path invalid",
                            errno ? errno : GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_elf_handle = elf_begin(m_file_desc, ELF_C_READ, nullptr);
        if (!m_elf_handle) {
            (void)close(m_file_desc);
            throw Exception("ELFImp::ELFImp(): libelf init failed on file",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_section = elf_nextscn(m_elf_handle, nullptr);
        if (m_section) {
            int err = (gelf_getshdr(m_section, m_section_header.get()) == nullptr);
            if (!err) {
                m_data = elf_getdata(m_section, nullptr);
                if (num_symbol()) {
                    err = (gelf_getsym(m_data, m_symbol_idx, m_symbol.get()) == nullptr);
                    if (err) {
                        throw Exception("ELFImp::ELFImp(): Error on first call to gelf_getsym()",
                                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    ELFImp::~ELFImp()
    {
        if (m_elf_handle) {
            (void)elf_end(m_elf_handle);
        }
        if (m_file_desc >= 0) {
            (void)close(m_file_desc);
        }
    }

    size_t ELFImp::num_symbol(void)
    {
        size_t result = 0;
        if (m_section && m_section_header->sh_type == SHT_SYMTAB) {
            result = m_section_header->sh_size / m_section_header->sh_entsize;
        }
        return result;
    }

    std::string ELFImp::symbol_name(void)
    {
        std::string result;
        if (m_section && m_data && m_symbol_idx < num_symbol()) {
            const char *result_cstr = elf_strptr(m_elf_handle,
                                                 m_section_header->sh_link,
                                                 m_symbol->st_name);
            if (result_cstr) {
                result = result_cstr;
            }
        }
        return result;
    }

    size_t ELFImp::symbol_offset(void)
    {
        size_t result = 0;
        if (m_data && m_symbol_idx < num_symbol()) {
           result = m_symbol->st_value;
        }
        return result;
    }

    bool ELFImp::next_section(void)
    {
        bool result = false;
        if (m_elf_handle && m_section) {
            m_section = elf_nextscn(m_elf_handle, m_section);
            if (m_section != nullptr) {
                int err = (gelf_getshdr(m_section, m_section_header.get()) == nullptr);
                if (!err) {
                    result = true;
                }
                m_data = elf_getdata(m_section, NULL);
            }
        }
        return result;
    }

    bool ELFImp::next_data(void)
    {
        bool result = false;
        if (m_section && m_data) {
            m_data = elf_getdata(m_section, m_data);
            if (m_data != nullptr) {
                result = true;
            }
        }
        return result;
    }

    bool ELFImp::next_symbol(void)
    {
        bool result = false;

        if (m_data && m_symbol_idx < num_symbol()) {
            ++m_symbol_idx;
            if (m_symbol_idx < num_symbol()) {
                int err = (gelf_getsym(m_data, m_symbol_idx, m_symbol.get()) == nullptr);
                if (err) {
                    throw Exception("ELFImp::next_symbol(): call to gelf_getsym() failed",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                result = true;
            }
        }
        return result;
    }
}
