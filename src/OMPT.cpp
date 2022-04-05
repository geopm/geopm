/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#include <cstdint>
#include <string>
#include <limits.h>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "geopm_prof.h"
#include "geopm_sched.h"
#include "geopm_error.h"
#include "Profile.hpp"
#include "ELF.hpp"
#include "Environment.hpp"
#include "geopm/Exception.hpp"
#include "OMPT.hpp"
#include "geopm_hash.h"
#include "geopm_hint.h"

#include "config.h"


extern "C"
{
    int geopm_is_pmpi_prof_enabled(void);
}

namespace geopm
{
    class OMPTImp : public OMPT
    {
        public:
            OMPTImp();
            OMPTImp(bool do_ompt);
            virtual ~OMPTImp() = default;
            bool is_enabled(void) override;
            void region_enter(const void *function_ptr) override;
            void region_exit(const void *function_ptr) override;
            uint64_t region_id(const void *function_ptr);
            std::string region_name(const void *function_ptr);
        private:
            /// Map from function address to geopm region ID
            std::map<size_t, uint64_t> m_function_region_id_map;
            bool m_do_ompt;
    };

    OMPT &OMPT::ompt(void)
    {
        static OMPTImp instance;
        return instance;
    }

    OMPTImp::OMPTImp()
        : OMPTImp(environment().do_ompt())
    {

    }

    OMPTImp::OMPTImp(bool do_ompt)
        : m_do_ompt(do_ompt)
    {

    }

    bool OMPTImp::is_enabled(void)
    {
        return m_do_ompt;
    }

    uint64_t OMPTImp::region_id(const void *parallel_function)
    {
        size_t target = (size_t) parallel_function;
        uint64_t result = GEOPM_REGION_HASH_UNMARKED;
        auto it = m_function_region_id_map.find(target);
        if (m_function_region_id_map.end() != it) {
            result = it->second;
        }
        else {
            std::string rn = region_name(parallel_function);
            int err = geopm_prof_region(rn.c_str(), GEOPM_REGION_HINT_UNKNOWN, &result);
            if (err) {
                result = GEOPM_REGION_HASH_UNMARKED;
            }
            else {
                m_function_region_id_map.insert(std::pair<size_t, uint64_t>(target, result));
            }
        }
        return result;
    }

    std::string OMPTImp::region_name(const void *parallel_function)
    {
        size_t target = (size_t) parallel_function;
        std::ostringstream name_stream;
        std::string symbol_name;
        std::string region_name;
        name_stream << "[OMPT]";
        std::pair<size_t, std::string> symbol = symbol_lookup(parallel_function);
        if (symbol.second.size()) {
            name_stream << symbol.second << "+0x" << std::hex << target - symbol.first;
        }
        else {
            // Set the name to the address if lookup failed
            name_stream << "0x" << std::setfill('0') << std::setw(16) << std::hex
                        << target;
        }
        region_name = name_stream.str();
        region_name.erase(std::remove(region_name.begin(), region_name.end(), ' '), region_name.end());
        return region_name;
    }

    void OMPTImp::region_enter(const void *parallel_function)
    {
        uint64_t rid = region_id(parallel_function);
        if (rid != GEOPM_REGION_HASH_UNMARKED) {
            geopm_prof_enter(rid);
        }
    }

    void OMPTImp::region_exit(const void *parallel_function)
    {
        uint64_t rid = region_id(parallel_function);
        if (rid != GEOPM_REGION_HASH_UNMARKED) {
            geopm_prof_exit(rid);
        }
    }
}
