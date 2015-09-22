/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef GLOBALPOLICY_HPP_INCLUDE
#define GLOBALPOLICY_HPP_INCLUDE

#include <string>
#include <fstream>
#include "geopm_policy_message.h"

namespace geopm
{
    class GlobalPolicy
    {
        public:
            GlobalPolicy(const std::string in_config, const std::string out_config);
            virtual ~GlobalPolicy();
            int mode(void) const;
            int frequency_mhz(void) const;
            int percent_tdp(void) const;
            int budget_watts(void) const;
            int affinity(void) const;
            int goal(void) const;
            int num_max_perf(void) const;
            long int flags(void) const;
            void policy_message(struct geopm_policy_message_s &policy_message) const;
            void mode(int mode);
            void frequency_mhz(int frequency);
            void percent_tdp(int percentage);
            void budget_watts(int budget);
            void affinity(int cpu_affinity);
            void goal(int geo_goal);
            void num_max_perf(int num_big_cores);
            void write(void);
            void read(void);
        protected:
            void affinity_string(int value, std::string &name);
            std::string m_in_config;
            std::string m_out_config;
            int m_mode;
            int m_power_budget_watts;
            long int m_flags;
            bool is_shm_in;
            bool is_shm_out;
            bool do_read;
            bool do_write;
            struct geopm_policy_shmem_s *m_policy_shmem_in;
            struct geopm_policy_shmem_s *m_policy_shmem_out;
            std::ifstream m_config_file_in;
            std::ofstream m_config_file_out;
    };

}
#endif
