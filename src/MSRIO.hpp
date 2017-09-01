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
                                   uint64_t write_mask,
                                   uint64_t raw_value) = 0;
            /// @brief initialize internal data stuctures to batch
            ///        read/write from MSRs.
            /// @param [in] read_cpu_idx A vector of logical Linux CPU
            ///        indicies to read from when read_batch() method
            ///        is called.
            /// @param [in] read_offset A vector of the MSR offsets to
            ///        be read from when the read_batch() method is
            ///        called.
            /// @param [in] write_cpu_idx A vector of logical Linux
            ///        CPU indicies to write to when the write_batch()
            ///        method is called.
            /// @param [in] write_offset A vector of the MSR offset to
            ///        be written to when the write_batch() method is
            ///        called.
            /// @param [in] write_mask A vector of write masks that
            ///        will determine the bits of the MSRs to be
            ///        modified when the write_batch() method is s
            ///        called.
            virtual void config_batch(const std::vector<int> &read_cpu_idx,
                                      const std::vector<uint64_t> &read_offset,
                                      const std::vector<int> &write_cpu_idx,
                                      const std::vector<uint64_t> &write_offset,
                                      const std::vector<uint64_t> &write_mask) = 0;
            /// @brief Batch read a set of MSRs configured by a
            ///        previous call to the batch_config() method.
            /// @param [out] raw_value The raw encoded MSR values to
            ///        be read.
            virtual void read_batch(std::vector<uint64_t> &raw_value) = 0;
            /// @brief Batch write a set of MSRs configured by a
            ///        previous call to the batch_config() method.
            /// @param [in] raw_value The raw encoded MSR values to be
            ///        written.
            virtual void write_batch(const std::vector<uint64_t> &raw_value) = 0;
    };

    class MSRIO : public IMSRIO
    {
        public:
            MSRIO(int num_cpu);
            virtual ~MSRIO();
            uint64_t read_msr(int cpu_idx,
                              uint64_t offset);
            void write_msr(int cpu_idx,
                           uint64_t offset,
                           uint64_t write_mask,
                           uint64_t raw_value);
            void config_batch(const std::vector<int> &read_cpu_idx,
                              const std::vector<uint64_t> &read_offset,
                              const std::vector<int> &write_cpu_idx,
                              const std::vector<uint64_t> &write_offset,
                              const std::vector<uint64_t> &write_mask);
            void read_batch(std::vector<uint64_t> &raw_value);
            void write_batch(const std::vector<uint64_t> &raw_value);
        protected:
            struct m_msr_batch_op_s {
                uint16_t cpu;      /// @brief In: CPU to execute {rd/wr}msr ins.
                uint16_t isrdmsr;  /// @brief In: 0=wrmsr, non-zero=rdmsr
                int32_t err;       /// @brief Out: Error code from operation
                uint32_t msr;      /// @brief In: MSR Address to perform op
                uint64_t msrdata;  /// @brief In/Out: Input/Result to/from operation
                uint64_t wmask;    /// @brief Out: Write mask applied to wrmsr
            };

            struct m_msr_batch_array_s {
                uint32_t numops;               /// @brief In: # of operations in ops array
                struct m_msr_batch_op_s *ops;  /// @brief In: Array[numops] of operations
            };

            virtual int msr_desc(int cpu_idx);
            virtual int msr_batch_desc(void);
            virtual void msr_path(int cpu_idx,
                                  bool is_fallback,
                                  std::string &path);
            virtual void msr_batch_path(std::string &path);
            virtual void open_msr(int cpu_idx);
            virtual void open_msr_batch(void);
            virtual void close_msr(int cpu_idx);
            virtual void close_msr_batch(void);
            virtual void msr_ioctl(bool is_read);

            const int m_num_cpu;
            std::vector<int> m_file_desc;
            bool m_is_batch_enabled;
            struct m_msr_batch_array_s m_read_batch;
            struct m_msr_batch_array_s m_write_batch;
            std::vector<struct m_msr_batch_op_s> m_read_batch_op;
            std::vector<struct m_msr_batch_op_s> m_write_batch_op;
    };
}

#endif
