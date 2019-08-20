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

#include "OMPT.hpp"

#include <cstdint>
#include <string>
#include <limits.h>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/wait.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>

#include "geopm.h"
#include "geopm_sched.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "config.h"

#ifdef GEOPM_ENABLE_OMPT

#include <ompt.h>

extern "C"
{
    int geopm_is_pmpi_prof_enabled(void);
}

namespace geopm
{
    class OMPT
    {
        public:
            OMPT();
            OMPT(const std::string &map_path);
            virtual ~OMPT() = default;
            uint64_t region_id(void *parallel_function);
            void region_name(void *parallel_function, std::string &name);
            void region_name_pretty(std::string &name);
        private:
            /// Map from <virtual_address, is_end> pair representing
            /// half of a virtual address range to the object file
            /// asigned to the address range.
            std::map<std::pair<size_t, bool>, std::string> m_range_object_map;
            /// Map from function address to geopm region ID
            std::map<size_t, uint64_t> m_function_region_id_map;
    };

    static OMPT &ompt(void)
    {
        static OMPT instance;
        return instance;
    }

    OMPT::OMPT()
        : OMPT("/proc/self/maps")
    {

    }

    OMPT::OMPT(const std::string &map_path)
    {
        std::ifstream maps_stream(map_path);
        while (maps_stream.good()) {
            std::string line;
            std::getline(maps_stream, line);
            if (line.length() == 0) {
                continue;
            }
            size_t addr_begin, addr_end;
            int n_scan = sscanf(line.c_str(), "%zx-%zx", &addr_begin, &addr_end);
            if (n_scan != 2) {
                continue;
            }

            std::string object;
            size_t object_loc = line.rfind(' ') + 1;
            if (object_loc == std::string::npos) {
                continue;
            }
            object = line.substr(object_loc);
            if (line.find(" r-xp ") != line.find(' ')) {
                continue;
            }
            std::pair<size_t, bool> aa(addr_begin, false);
            std::pair<size_t, bool> bb(addr_end, true);
            std::pair<std::pair<size_t, bool>, std::string> cc(aa, object);
            std::pair<std::pair<size_t, bool>, std::string> dd(bb, object);
            auto it0 = m_range_object_map.insert(m_range_object_map.begin(), cc);
            auto it1 = m_range_object_map.insert(it0, dd);
            ++it0;
            if (it0 != it1) {
                throw Exception("Error parsing /proc/self/maps, overlapping address ranges.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
        }
    }

    uint64_t OMPT::region_id(void *parallel_function)
    {
        uint64_t result = GEOPM_REGION_HASH_UNMARKED;
        auto it = m_function_region_id_map.find((size_t)parallel_function);
        if (m_function_region_id_map.end() != it) {
            result = it->second;
        }
        else {
            std::string rn;
            region_name(parallel_function, rn);
            int err = geopm_prof_region(rn.c_str(), GEOPM_REGION_HINT_UNKNOWN, &result);
            if (err) {
                result = GEOPM_REGION_HASH_UNMARKED;
            }
            else {
                m_function_region_id_map.insert(std::pair<size_t, uint64_t>((size_t)parallel_function, result));
            }
        }
        return result;
    }

    void OMPT::region_name(void *parallel_function, std::string &name)
    {
        Dl_info info;
        std::stringstream name_stream;
        std::string symbol_name;
        size_t min_distance = ~0ULL;
        name_stream << "[OMPT] ";
        bool is_found = false;
        bool dladdr_success = dladdr(parallel_function, &info);
        // "dladdr() returns 0 on error, and nonzero on success."
        if (dladdr_success != 0) {
            // dladdr() found the file
            if (info.dli_sname) {
                // dladdr() found the symbol
                min_distance = (uint64_t)parallel_function - (uint64_t)info.dli_saddr;
                symbol_name = info.dli_sname;
                is_found = true;
            }
            else {
                ELF elf(info.dli_fname);
                do {
                   do {
                       if (elf.num_symbol()) {
                           do {
                                // Check if the symbol is before the target
                                size_t offset = elf.symbol_offset();
                                size_t distance = target - offset;
                                if (target > offset &&
                                    distance < min_distance) {
                                    min_distance = distance;
                                    // Store the closest symbol before the target
                                    symbol_name = elf.symbol_name();
                                    if (symbol_name.size()) {
                                        is_found = true;
                                    }
                               }
                           } while (elf.next_symbol())
                       }
                   } while (elf.next_data())
                } while (elf.next_section())
            }
        }
        if (is_found) {
            // Check for C++ mangling
            const char *demangled_name = abi::__cxa_demangle(symbol_name, NULL, NULL, NULL);
            if (demangled_name != NULL) {
                symbol_name = demangled_name;
            }
            name_stream << symbol_name << "+" << min_distance;
        }
        else {
            // Set the name to the address if lookup failed
            name_stream << "0x" << std::setfill('0') << std::setw(16) << std::hex
                        << (size_t)parallel_function;
        }
        name = name_stream.str();
    }
}


extern "C"
{
    static void *g_curr_parallel_function = NULL;
    static ompt_parallel_id_t g_curr_parallel_id;
    static uint64_t g_curr_region_id = GEOPM_REGION_HASH_UNMARKED;

    static void on_ompt_event_parallel_begin(ompt_task_id_t parent_task_id,
                                             ompt_frame_t *parent_task_frame,
                                             ompt_parallel_id_t parallel_id,
                                             uint32_t requested_team_size,
                                             void *parallel_function,
                                             ompt_invoker_t invoker)
    {
        if (geopm_is_pmpi_prof_enabled() &&
            g_curr_parallel_function != parallel_function) {
            g_curr_parallel_function = parallel_function;
            g_curr_parallel_id = parallel_id;
            g_curr_region_id = geopm::ompt().region_id(parallel_function);
        }
        if (g_curr_region_id != GEOPM_REGION_HASH_UNMARKED) {
            geopm_prof_enter(g_curr_region_id);
        }
    }

    static void on_ompt_event_parallel_end(ompt_parallel_id_t parallel_id,
                                           ompt_task_id_t task_id,
                                           ompt_invoker_t invoker)
    {
        if (geopm_is_pmpi_prof_enabled() &&
            g_curr_region_id != GEOPM_REGION_HASH_UNMARKED &&
            g_curr_parallel_id == parallel_id) {
            geopm_prof_exit(g_curr_region_id);
        }
    }


    void ompt_initialize(ompt_function_lookup_t lookup,
                         const char *runtime_version,
                         unsigned int ompt_version)
    {
        ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
        ompt_set_callback(ompt_event_parallel_begin, (ompt_callback_t) &on_ompt_event_parallel_begin);
        ompt_set_callback(ompt_event_parallel_end, (ompt_callback_t) &on_ompt_event_parallel_end);

    }

    ompt_initialize_t ompt_tool()
    {
        return &ompt_initialize;
    }
}

#endif // GEOPM_ENABLE_OMPT defined
