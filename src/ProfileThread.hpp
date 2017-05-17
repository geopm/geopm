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

#include "Profile.hpp"

namespace geopm
{
    /// @brief ProfileThread class encapsulates helper functions to report
    /// per rank profile data within threaded loops.
    class ProfileThreadBase
    {
        public:
            ProfileThreadBase() {}
            ProfileThreadBase(const ProfileThreadBase &other) {}
            virtual ~ProfileThreadBase() {}
            virtual void increment(Profile &prof, uint64_t region_id, int thread_idx) = 0;
    };

    class ProfileThread : public ProfileThreadBase
    {
        public:
            ProfileThread(int num_thread, size_t num_iter);
            ProfileThread(int num_thread, size_t num_iter, size_t chunk_size);
            virtual ~ProfileThread();
            void increment(Profile &prof, uint64_t region_id, int thread_idx);
        protected:
            const size_t m_num_iter;
            const int m_num_thread;
            const size_t m_chunk_size;
            const size_t m_stride;
            uint32_t *m_progress;
            std::vector<double> m_norm;
    };
}

#endif
