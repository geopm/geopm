/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MSRIO_HPP_INCLUDE
#define MSRIO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <memory>

namespace geopm
{
    class MSRIO
    {
        public:
            enum m_driver_e {
                M_DRIVER_MSRSAFE,
                M_DRIVER_MSR,
                M_NUM_DRIVER,
            };

            MSRIO() = default;
            virtual ~MSRIO() = default;
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
            /// @param [in] raw_value The raw encoded MSR value to
            ///        write, only bits where the write_mask is set
            ///        will be written, other bits in the MSR will be
            ///        unmodified.
            /// @param [in] write_mask The mask determines the bits of
            ///        the MSR that will be modified.  An error will
            ///        occur if bits are set in the raw_value that are
            ///        not in the write mask.
            virtual void write_msr(int cpu_idx,
                                   uint64_t offset,
                                   uint64_t raw_value,
                                   uint64_t write_mask) = 0;
            /// @brief Add a new batch context to the set of managed contexts.
            /// @return The context index that can be passed to future batch
            ///         methods to refer to the added context.
            virtual int create_batch_context(void) = 0;
            /// @brief Extend the set of MSRs for batch read with a single offset.
            ///        Note: uses the default batch context.
            /// @param [in] cpu_idx logical Linux CPU index to read from when
            ///         read_batch() method is called.
            /// @param [in] offset MSR offset to be read when
            ///        read_batch() is called.
            /// @return The logical index that will be passed to sample().
            virtual int add_read(int cpu_idx, uint64_t offset) = 0;
            /// @brief Extend the set of MSRs for batch read with a single offset.
            /// @param [in] cpu_idx logical Linux CPU index to read from when
            ///         read_batch() method is called.
            /// @param [in] offset MSR offset to be read when
            ///        read_batch() is called.
            /// @param [in] batch_ctx batch context index where the read will be added.
            /// @return The logical index that will be passed to sample().
            virtual int add_read(int cpu_idx, uint64_t offset, int batch_ctx) = 0;
            /// @brief Batch read a set of MSRs configured by a
            ///        previous call to the batch_config() method.
            ///        The memory used to store the result should have
            ///        been returned by add_read().
            ///        Note: uses the default batch context.
            virtual void read_batch(void) = 0;
            /// @brief Batch read a set of MSRs configured by a
            ///        previous call to the batch_config() method.
            ///        The memory used to store the result should have
            ///        been returned by add_read().
            /// @param [in] batch_ctx index for batch context to use for the read.
            virtual void read_batch(int batch_ctx) = 0;
            /// @brief Add another offset to the list of MSRs to be
            ///        written in batch.
            ///        Note: uses the default batch context.
            /// @param [in] cpu_idx logical Linux CPU index to write
            ///        to when write_batch() method is called.
            /// @param [in] offset MSR offset to be written when
            ///        write_batch() method is called.
            /// @return The logical index that will be passed to
            ///         adjust().
            virtual int add_write(int cpu_idx, uint64_t offset) = 0;
            /// @brief Add another offset to the list of MSRs to be
            ///        written in batch.
            /// @param [in] cpu_idx logical Linux CPU index to write
            ///        to when write_batch() method is called.
            /// @param [in] offset MSR offset to be written when
            ///        write_batch() method is called.
            /// @return The logical index that will be passed to
            ///         adjust().
            /// @param [in] batch_ctx batch context index where the write will be added.
            virtual int add_write(int cpu_idx, uint64_t offset, int batch_ctx) = 0;
            /// @brief Adjust a value that was previously added with
            ///        the add_write() method.
            ///        Note: uses the default batch context.
            virtual void adjust(int batch_idx,
                                uint64_t value,
                                uint64_t write_mask) = 0;
            /// @brief Adjust a value that was previously added with
            ///        the add_write() method.
            /// @param [in] batch_ctx index of batch context where value will be
            ///        adjusted.
            virtual void adjust(int batch_idx,
                                uint64_t value,
                                uint64_t write_mask,
                                int batch_ctx) = 0;
            /// @brief Read the full 64-bit value of the MSR that was
            ///        previously added to the MSRIO with add_read().
            ///        read_batch() must be called prior to calling
            ///        this function.
            ///        Note: uses the default batch context.
            virtual uint64_t sample(int batch_idx) const = 0;
            /// @brief Read the full 64-bit value of the MSR that was
            ///        previously added to the MSRIO with add_read().
            ///        read_batch() must be called prior to calling
            ///        this function.
            /// @param [in] batch_ctx index of batch context from which the value
            ///         will be read.
            virtual uint64_t sample(int batch_idx, int batch_ctx) const = 0;
            /// @brief Write all adjusted values.
            ///        Note: uses the default batch context.
            virtual void write_batch(void) = 0;
            /// @brief Write all adjusted values.
            /// @param [in] batch_ctx index for batch context to use for the write.
            virtual void write_batch(int batch_ctx) = 0;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<MSRIO> make_unique(int driver_type);
    };
}

#endif
