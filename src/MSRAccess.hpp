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

#ifndef MSRACCESS_HPP_INCLUDE
#define MSRACCESS_HPP_INCLUDE

#ifndef X86_IOC_MSR_BATCH
#define  X86_IOC_MSR_BATCH _IOWR('c', 0xA2, m_msr_batch_array)
#endif

#include <sys/types.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <limits.h>
#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

#include "PlatformTopology.hpp"

namespace geopm
{
    class IMSRAccess
    {
        public:
            struct m_msr_signal_entry {
                off_t offset;
                uint64_t write_mask;
                int size;
                int lshift_mod;
                int rshift_mod;
                uint64_t mask_mod;
                double multiply_mod;
            };

            IMSRAccess() {}
            virtual ~IMSRAccess() {};
            virtual off_t offset(const std::string &msr_name) = 0;
            virtual uint64_t write_mask(const std::string &msr_name) = 0;
            virtual uint64_t read(int cpu_id, uint64_t offset) = 0;
            virtual void write(int cpu_id, uint64_t offset, uint64_t write_mask, uint64_t raw_value) = 0;
            virtual void config_batch_read(const std::vector<int> &cpu, const std::vector<uint64_t> &read_offset) = 0;
            virtual void config_batch_write(const std::vector<int> &cpu, const std::vector<uint64_t> &write_offset, const std::vector<uint64_t> &write_mask) = 0;
            virtual void read_batch(std::vector<uint64_t> &raw_value) = 0;
            virtual void write_batch(const std::vector<uint64_t> &raw_value) = 0;
            virtual size_t num_raw_signal(void) = 0;
    };

    class MSRAccess : public IMSRAccess
    {
        public:
            MSRAccess(const std::map<std::string, struct m_msr_signal_entry> *signal_map,
                      const std::map<std::string, std::pair<off_t, uint64_t> > *control_map,
                      const PlatformTopology &topo);
            virtual ~MSRAccess();
            off_t offset(const std::string &msr_name);
            uint64_t write_mask(const std::string &msr_name);
            uint64_t read(int cpu_id, uint64_t offset);
            void write(int cpu_id, uint64_t offset, uint64_t write_mask, uint64_t raw_value);
            void config_batch_read(const std::vector<int> &cpu, const std::vector<uint64_t> &read_offset);
            void config_batch_write(const std::vector<int> &cpu, const std::vector<uint64_t> &write_offset, const std::vector<uint64_t> &write_mask);
            void read_batch(std::vector<uint64_t> &raw_value);
            void write_batch(const std::vector<uint64_t> &raw_value);
            size_t num_raw_signal(void);

        protected:
            virtual void descriptor_path(int cpu_num);
            void msr_open(int cpu);
            void msr_close(int cpu);
            struct m_msr_batch_op {
                uint16_t cpu;      /// @brief In: CPU to execute {rd/wr}msr ins.
                uint16_t isrdmsr;  /// @brief In: 0=wrmsr, non-zero=rdmsr
                int32_t err;       /// @brief Out: Error code from operation
                uint32_t msr;      /// @brief In: MSR Address to perform op
                uint64_t msrdata;  /// @brief In/Out: Input/Result to/from operation
                uint64_t wmask;    /// @brief Out: Write mask applied to wrmsr
            };

            struct m_msr_batch_array {
                uint32_t numops;            /// @brief In: # of operations in ops array
                struct m_msr_batch_op *ops;   /// @brief In: Array[numops] of operations
            };

            char m_msr_path[NAME_MAX];
            std::vector<int> m_cpu_file_desc;
            int m_msr_batch_desc;
            bool m_is_batch_enabled;
            struct m_msr_batch_array m_read_batch;
            struct m_msr_batch_array m_write_batch;
            std::vector<struct m_msr_batch_op> m_read_batch_op;
            std::vector<struct m_msr_batch_op> m_write_batch_op;
            int m_num_logical_cpu;
            int m_num_package;
            const std::map<std::string, struct m_msr_signal_entry> *m_msr_signal_map_ptr;
            const std::map<std::string, std::pair<off_t, uint64_t> > *m_msr_control_map_ptr;
    };
}

#endif
