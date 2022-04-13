/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MSRIOIMP_HPP_INCLUDE
#define MSRIOIMP_HPP_INCLUDE

#include <string>
#include <map>

#include "MSRIO.hpp"

namespace geopm
{
    class MSRPath;

    class MSRIOImp : public MSRIO
    {
        public:
            MSRIOImp();
            MSRIOImp(int num_cpu, std::shared_ptr<MSRPath> path);
            virtual ~MSRIOImp();
            uint64_t read_msr(int cpu_idx,
                              uint64_t offset) override;
            void write_msr(int cpu_idx,
                           uint64_t offset,
                           uint64_t raw_value,
                           uint64_t write_mask) override;
            int add_read(int cpu_idx, uint64_t offset) override;
            void read_batch(void) override;
            uint64_t sample(int batch_idx) const override;
            void write_batch(void) override;
            int add_write(int cpu_idx, uint64_t offset) override;
            void adjust(int batch_idx, uint64_t value, uint64_t write_mask) override;
        private:
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

            void open_all(void);
            void open_msr(int cpu_idx);
            void open_msr_batch(void);
            void close_all(void);
            void close_msr(int cpu_idx);
            void close_msr_batch(void);
            int msr_desc(int cpu_idx);
            int msr_batch_desc(void);
            void msr_ioctl(struct m_msr_batch_array_s &batch);
            void msr_ioctl_read(void);
            void msr_ioctl_write(void);
            uint64_t system_write_mask(uint64_t offset);

            const int m_num_cpu;
            std::vector<int> m_file_desc;
            bool m_is_batch_enabled;
            struct m_msr_batch_array_s m_read_batch;
            struct m_msr_batch_array_s m_write_batch;
            std::vector<struct m_msr_batch_op_s> m_read_batch_op;
            std::vector<struct m_msr_batch_op_s> m_write_batch_op;
            bool m_is_batch_read;
            std::vector<std::map<uint64_t, int> > m_read_batch_idx_map;
            std::vector<std::map<uint64_t, int> > m_write_batch_idx_map;
            std::map<uint64_t, uint64_t> m_offset_mask_map;
            std::vector<uint64_t> m_write_val;
            std::vector<uint64_t> m_write_mask;
            bool m_is_open;
            std::shared_ptr<MSRPath> m_path;
    };
}

#endif
