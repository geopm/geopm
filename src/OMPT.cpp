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

#include <cstdint>
#include <string>
#include <limits.h>
#include <map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>


#include "geopm.h"
#include "geopm_sched.h"
#include "geopm_error.h"
#include "ELF.hpp"
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
            OMPT() = default;
            virtual ~OMPT() = default;
            uint64_t region_id(void *parallel_function);
            void region_name(void *parallel_function, std::string &name);
        private:
            /// Map from function address to geopm region ID
            std::map<size_t, uint64_t> m_function_region_id_map;
    };

    static OMPT &ompt(void)
    {
        static OMPT instance;
        return instance;
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
        size_t target = (size_t) parallel_function;
        std::ostringstream name_stream;
        std::string symbol_name;
        name_stream << "[OMPT] ";
        std::pair<size_t, std::string> symbol = symbol_lookup(parallel_function);
        if (symbol.second.size()) {
            name_stream << symbol.second << "+" << target - symbol.first;
        }
        else {
            // Set the name to the address if lookup failed
            name_stream << "0x" << std::setfill('0') << std::setw(16) << std::hex
                        << target;
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
