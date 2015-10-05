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

#ifndef RAPLPLATFORM_HPP_INCLUDE
#define RAPLPLATFORM_HPP_INCLUDE

#include "Phase.hpp"
#include "Policy.hpp"
#include "TreeCommunicator.hpp"
#include "Platform.hpp"

namespace geopm
{
    struct buffer_index_s {
        int package0_pkg_energy;
        int package1_pkg_energy;
        int package0_pp0_energy;
        int package1_pp0_energy;
        int package0_dram_energy;
        int package1_dram_energy;
        int inst_retired_any_base;
        int clk_unhalted_core_base;
        int clk_unhalted_ref_base;
        int llc_victims_base;
        int num_slot;
    };

    class RAPLPlatform : public Platform
    {
        public:
            RAPLPlatform();
            virtual ~RAPLPlatform();
            virtual void set_implementation(PlatformImp* platform_imp);
            virtual bool model_supported(int platform_id) const;
            virtual void observe(void);
            virtual void sample(struct sample_message_s &sample) const;
            virtual void enforce_policy(const Policy &policy) const;
        protected:
            struct buffer_index_s m_buffer_index;
            std::vector<off_t> m_observe_msr_offsets;
            std::vector<off_t> m_enforce_msr_offsets;
            int m_num_cpu;
            int m_num_package;
            Phase* m_curr_phase;
    };
}

#endif
