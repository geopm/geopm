/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APPLICATIONSTATUS_HPP_INCLUDE
#define APPLICATIONSTATUS_HPP_INCLUDE

#include <cstdint>

#include <memory>
#include <vector>
#include <set>

#include "geopm/Helper.hpp"

namespace geopm
{
    class SharedMemory;

    /// @brief Object that encapsulates application process
    ///        information such as the process ID, region hash, or
    ///        region hint.  There will be one ApplicationStatus for
    ///        the node (board domain) on each side of the shared
    ///        memory.
    class ApplicationStatus
    {
        public:
            virtual ~ApplicationStatus() = default;

            /// @brief Set the current hint bits for a CPU.
            /// @param [in] cpu_idx Index of the Linux logical CPU
            /// @param [in] hint Bitfield of hint to set for the
            ///             CPU.  Any existing hint will be
            ///             overwritten.
            virtual void set_hint(int cpu_idx, uint64_t hint) = 0;
            /// @brief Get the current hint bits for a CPU.
            /// @param [in] cpu_idx Index of the Linux logical CPU.
            /// @return The current hint for the given CPU.
            virtual uint64_t get_hint(int cpu_idx) const = 0;
            /// @brief Set the hash and hint of the region currently
            ///        running on a CPU.
            virtual void set_hash(int cpu_idx, uint64_t hash, uint64_t hint) = 0;
            /// @brief Get the hash of the region currently running on
            ///        a CPU.
            virtual uint64_t get_hash(int cpu_idx) const = 0;
            virtual void reset_work_units(int cpu_idx) = 0;
            /// @brief Reset the total work units for all threads to
            ///        be completed as part of a parallel region.
            ///        Calling this method also resets the work
            ///        completed for the CPU.
            /// @param [in] cpu_idx Index of the Linux logical CPU
            virtual void set_total_work_units(int cpu_idx, int work_units) = 0;
            /// @brief Mark a unit of work completed for this CPU.
            /// @param [in] cpu_idx Index of the Linux logical CPU
            virtual void increment_work_unit(int cpu_idx) = 0;
            /// @brief Get the current progress for this CPU.
            ///        Progress is the fraction of the total work
            ///        units that have been completed.
            /// @param [in] cpu_idx Index of the Linux logical CPU.
            /// @return Fraction of the total work completed by this
            ///         CPU.
            virtual double get_progress_cpu(int cpu_idx) const = 0;
            virtual void set_valid_cpu(const std::set<int> &cpu_idx) = 0;
            /// @brief Updates the local memory with the latest values from
            ///        the shared memory.  Any calls to get methods will use
            ///        these values until the cache is updated again.
            virtual void update_cache(void) = 0;

            /// @brief Create an ApplicationStatus object using the
            ///        given SharedMemory.  The caller is responsible
            ///        for calling `buffer_size()` when creating the
            ///        shared memory, or attaching to an existing
            ///        shared memory region before passing the object
            ///        to this method.
            /// @return A unique_ptr to the new ApplicationStatus
            ///         object.
            static std::unique_ptr<ApplicationStatus> make_unique(int num_cpu,
                                                                  std::shared_ptr<SharedMemory> shmem);
            /// @brief Return the required size of the shared memory
            ///        region used by the ApplicationStatus for the
            ///        given number of CPUs.
            /// @return Minimum buffer size required for the
            ///         SharedMemory used by ApplicationStatus.
            static size_t buffer_size(int num_cpu);

        protected:
            static constexpr size_t M_STATUS_SIZE = geopm::hardware_destructive_interference_size;
    };

    class ApplicationStatusImp : public ApplicationStatus
    {
        public:
            ApplicationStatusImp(int num_cpu,
                                 std::shared_ptr<SharedMemory> shmem);
            virtual ~ApplicationStatusImp() = default;
            void set_hint(int cpu_idx, uint64_t hint) override;
            uint64_t get_hint(int cpu_idx) const override;
            void set_hash(int cpu_idx, uint64_t hash, uint64_t hint) override;
            uint64_t get_hash(int cpu_idx) const override;
            void reset_work_units(int cpu_idx) override;
            void set_total_work_units(int cpu_idx, int work_units) override;
            void increment_work_unit(int cpu_idx) override;
            double get_progress_cpu(int cpu_idx) const override;
            void set_valid_cpu(const std::set<int> &cpu_idx) override;
            void update_cache(void) override;
        private:
            // These fields must all be 32-bit int
            struct m_app_status_s
            {
                int32_t process; // can be negative, indicating unset process
                uint32_t hint;
                uint32_t hash;
                uint32_t total_work;
                uint32_t completed_work;
                char padding[44];
            };
            static_assert((sizeof(ApplicationStatusImp::m_app_status_s) % geopm::hardware_destructive_interference_size) == 0,
                          "m_app_status_s not aligned to cache lines");
            static_assert(sizeof(ApplicationStatusImp::m_app_status_s) == ApplicationStatus::M_STATUS_SIZE,
                          "M_STATUS_SIZE does not match size of m_app_status_s");

            int m_num_cpu;
            std::shared_ptr<SharedMemory> m_shmem;
            m_app_status_s *m_buffer;
            std::vector<m_app_status_s> m_cache;
    };
}

#endif
