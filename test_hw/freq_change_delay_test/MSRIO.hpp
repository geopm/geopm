/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#ifndef MSRIO_HPP_INCLUDE
#define MSRIO_HPP_INCLUDE

#include <stdint.h>
#include <string>
#include <vector>

namespace geopm
{
    class IMSRIO
    {
        public:
            IMSRIO() {}
            virtual ~IMSRIO() {}
            /// @brief Read from a single MSR on a CPU.
            /// @brief [in] cpu_idx logical Linux CPU index to read
            ///        from.
            /// @param [in] offset The MSR offset to read from.
            /// @return The raw encoded MSR value read.
            virtual uint64_t read_msr(int cpu_idx,
                                      uint64_t offset) = 0;
            /// @brief Write to a single MSR on a CPU.
            /// @param [in] cpu_idx logical Linux CPU index to write
            ///        to.
            /// @param [in] offset The MSR offset to write to.
            /// @param [in] write_mask The mask determines the bits of
            ///        the MSR that will be modified.
            /// @param [in] raw_value The raw encoded MSR value to
            ///        write, only bits where the write_mask is set
            ///        will be written, other bits in the MSR will be
            ///        unmodified.
            virtual void write_msr(int cpu_idx,
                                   uint64_t offset,
                                   uint64_t raw_value,
                                   uint64_t write_mask) = 0;
    };

    class MSRIO : public IMSRIO
    {
        public:
            MSRIO();
            virtual ~MSRIO();
            uint64_t read_msr(int cpu_idx,
                              uint64_t offset);
            void write_msr(int cpu_idx,
                           uint64_t offset,
                           uint64_t raw_value,
                           uint64_t write_mask);
        protected:
            void open_msr(int cpu_idx);
            void open_msr_batch(void);
            void close_msr(int cpu_idx);
            int msr_desc(int cpu_idx);
            virtual void msr_path(int cpu_idx,
                                  bool is_fallback,
                                  std::string &path);

            const int m_num_cpu;
            std::vector<int> m_file_desc;
    };
}

#endif
