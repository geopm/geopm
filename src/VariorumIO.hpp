/*
 */

#ifndef VARIOUMIO_HPP_INCLUDE
#define VARIOUMIO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <memory>

namespace geopm
{
    class VariorumIO
    {
        public:
            VariorumIO() = default;
            virtual ~VariorumIO() = default;
            /// @brief Read from a single Variorum on a CPU.
            /// @brief [in] cpu_idx logical Linux CPU index to read
            ///        from.
            /// @param [in] offset The Variorum offset to read from.
            /// @return The raw encoded Variorum value read.
            virtual uint64_t read_msr(int cpu_idx,
                                      uint64_t offset) = 0;
            /// @brief Write to a single Variorum on a CPU.
            /// @param [in] cpu_idx logical Linux CPU index to write
            ///        to.
            /// @param [in] offset The Variorum offset to write to.
            /// @param [in] raw_value The raw encoded Variorum value to
            ///        write, only bits where the write_mask is set
            ///        will be written, other bits in the Variorum will be
            ///        unmodified.
            /// @param [in] write_mask The mask determines the bits of
            ///        the Variorum that will be modified.  An error will
            ///        occur if bits are set in the raw_value that are
            ///        not in the write mask.
            virtual void write_msr(int cpu_idx,
                                   uint64_t offset,
                                   uint64_t raw_value,
                                   uint64_t write_mask) = 0;
            /// @brief Extend the set of Variorums for batch read with a single offset.
            /// @param [in] cpu_idx logical Linux PU index to read from when
            ///         read_batch() method is called.
            /// @param [in] offset Variorum offset to be read when
            ///        read_batch() is called.
            /// @return The logical index that will be passed to sample().
            virtual int add_read(int cpu_idx, uint64_t offset) = 0;
            /// @brief Batch read a set of Variorums configured by a
            ///        previous call to the batch_config() method.
            ///        The memory used to store the result should have
            ///        been returned by add_read().
            virtual void read_batch(void) = 0;
            /// @brief Add another offset to the list of Variorums to be
            ///        written in batch.
            /// @param [in] cpu_idx logical Linux CPU index to write
            ///        to when write_batch() method is called.
            /// @param [in] offset Variorum offset to be written when
            ///        write_batch() method is called.
            /// @return The logical index that will be passed to
            ///         adjust().
            virtual int add_write(int cpu_idx, uint64_t offset) = 0;
            /// @brief Adjust a value that was previously added with
            ///        the add_write() method.
            virtual void adjust(int batch_idx,
                                uint64_t value,
                                uint64_t write_mask) = 0;
            /// @brief Read the full 64-bit value of the Variorum that was
            ///        previously added to the VariorumIO with add_read().
            ///        read_batch() must be called prior to calling
            ///        this function.
            virtual uint64_t sample(int batch_idx) const = 0;
            /// @brief Write all adjusted values.
            virtual void write_batch(void) = 0;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<VariorumIO> make_unique(void);
            /// @brief Returns a shared_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::shared_ptr<VariorumIO> make_shared(void);
    };
}

#endif
