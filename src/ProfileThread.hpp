/*
 * Copyright (c) 2016, Intel Corporation
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


#ifndef PROFILE_THREAD_HPP_INCLUDE
#define PROFILE_THREAD_HPP_INCLUDE

#include <stdlib.h>
#include <stdint.h>
#include <vector>

namespace geopm
{
    class IProfileThreadTable
    {
        public:
            IProfileThreadTable() {}
            IProfileThreadTable(const IProfileThreadTable &other) {}
            virtual ~IProfileThreadTable() {}
            virtual void enable(bool is_enabled) = 0;
            virtual void reset(int num_thread, size_t num_iter) = 0;
            virtual void reset(int num_thread, size_t num_iter, size_t chunk_size) = 0;
            virtual void reset(const std::vector<uint32_t> &num_work_unit) = 0;
            virtual void increment(void) = 0;
            virtual void dump(std::vector<double> &progress) = 0;
            virtual int num_cpu(void) = 0;
    };

    class ProfileThreadTable : public IProfileThreadTable
    {
        public:
            ProfileThreadTable(size_t buffer_size, void *buffer);
            ProfileThreadTable(const ProfileThreadTable &other);
            virtual ~ProfileThreadTable();
            void enable(bool is_enabled);
            void reset(int num_thread, size_t num_iter);
            void reset(int num_thread, size_t num_iter, size_t chunk_size);
            void reset(const std::vector<uint32_t> &num_work_unit);
            void increment(void);
            void dump(std::vector<double> &progress);
            int num_cpu(void);
            static int num_cpu_s(void);
        private:
            static int cpu_idx(void);
            uint32_t *m_buffer;
            uint32_t m_num_cpu;
            size_t m_stride;
            bool m_is_enabled;
    };
}

#endif
