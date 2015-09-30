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

#ifndef PLATFORM_HPP_INCLUDE
#define PLATFORM_HPP_INCLUDE

#include <hwloc.h>

#include "Phase.hpp"
#include "Policy.hpp"
#include "TreeCommunicator.hpp"
#include "PlatformImp.hpp"
#include "PowerModel.hpp"

namespace geopm
{
    class Platform
    {
        public:
            Platform();
            virtual ~Platform();
            void set_implementation(PlatformImp* platform_imp);
            void phase_begin(Phase *phase);
            void phase_end(void);
            Phase *cur_phase(void) const;
            int num_domain(void) const;
            void domain_index(int domain_type, std::vector <int> &domain_index) const;
            int level(void) const;
            std::string name(void) const;
            void buffer_index(hwloc_obj_t domain,
                              const std::vector <std::string> &signal_names,
                              std::vector <int> &buffer_index) const;
            void observe(struct sample_message_s &sample) const;
            void observe(const std::vector <struct sample_message_s> &sample) const;
            PowerModel *power_model(int domain_type) const;
            void tdp_limit(int percentage) const;
            void manual_frequency(int frequency, int num_cpu_max_perf, int affinity) const;
            void save_msr_state(const char *path) const;
            void restore_msr_state(const char *path) const;
            virtual void observe(void) = 0;
            virtual bool model_supported(int platform_id) const = 0;
            virtual void sample(struct sample_message_s &sample) const = 0;
            virtual void enforce_policy(const Policy &policy) const = 0;
            const PlatformTopology topology() const;
        protected:
            PlatformImp *m_imp;
            Phase *m_cur_phase;
            std::map <int, PowerModel *> m_power_model;
            int m_num_domains;
            int m_window_size;
            int m_level;
    };
}

#endif
