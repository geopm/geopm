/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#ifndef APPLICATIONSTATUS_HPP_INCLUDE
#define APPLICATIONSTATUS_HPP_INCLUDE

#include <cstdint>

#include <memory>
#include <vector>
#include <set>

#include "Helper.hpp"

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
            /// @param [in] hints Bitfield of hints to set for the
            ///             CPU.  Any existing hints will be
            ///             overwritten.
            virtual void set_hint(int cpu_idx, uint64_t hints) = 0;
            /// @brief Get the current hint bits for a CPU.
            /// @param [in] cpu_idx Index of the Linux logical CPU.
            /// @return The current hints for the given CPU.
            virtual uint64_t get_hint(int cpu_idx) const = 0;
            /// @brief Set the hash of the region currently running on
            ///        a CPU.
            virtual void set_hash(int cpu_idx, uint64_t hash) = 0;
            /// @brief Get the hash of the region currently running on
            ///        a CPU.
            virtual uint64_t get_hash(int cpu_idx) const = 0;
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
            /// @brief Assign a set of CPUs to a unique ID for a
            ///        process being coordinated within a job by the
            ///        GEOPM controller.  This may be: a COMM_WORLD or
            ///        node-local MPI rank; a Linux parent process ID
            ///        for an application; or some other concept of a
            ///        process with its own memory.  This ID should be
            ///        self-consistent and unique within a job.  There
            ///        will be one Profile object per process on the
            ///        application side, and one ApplicationRecordLog
            ///        per process on each side of the shared memory.
            /// @param [in] cpu_idx Set of Linux logical CPUs for the process
            virtual void set_process(const std::set<int> &cpu_idx, int process) = 0;
            /// @brief Get the process ID for the process the CPU is
            ///        currently assigned to.
            /// @param [in] cpu_idx Index of the Linux logical CPU
            /// @return ID of the process running on the given CPU.
            virtual int get_process(int cpu_idx) const = 0;
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
            void set_hint(int cpu_idx, uint64_t hints) override;
            uint64_t get_hint(int cpu_idx) const override;
            void set_hash(int cpu_idx, uint64_t hash) override;
            uint64_t get_hash(int cpu_idx) const override;
            void set_total_work_units(int cpu_idx, int work_units) override;
            void increment_work_unit(int cpu_idx) override;
            double get_progress_cpu(int cpu_idx) const override;
            void set_process(const std::set<int> &cpu_idx, int process) override;
            int get_process(int cpu_idx) const override;
            void update_cache(void) override;
        private:
            // These fields must all be 32-bit int
            struct m_app_status_s
            {
                int32_t process; // can be negative, indicating unset process
                uint32_t hints;
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
