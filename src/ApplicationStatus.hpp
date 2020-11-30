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
    class PlatformTopo;

    class ApplicationStatus
    {
            /// TODO: check for num CPU
            // TODO: const for getters
            /// TODO: make sure everything stored in memory is 32 bits
        public:
            virtual ~ApplicationStatus() = default;

            virtual void set_hint(int cpu_idx, uint64_t hints) = 0;  // may need to shift to get 32bit int
            virtual uint64_t get_hint(int cpu_idx) = 0;
            virtual std::vector<uint64_t> get_hint(void) = 0;

            virtual void set_hash(int cpu_idx, uint64_t hash) = 0;
            virtual uint64_t get_hash(int cpu_idx) = 0;
            virtual std::vector<uint64_t> get_hash(void) = 0;

            virtual void set_total_work_units(int cpu_idx, int work_units) = 0;  // each cpu gets a fraction of what was passed to tprof_init()
            virtual void increment_work_unit(int cpu_idx) = 0;  // complete a work unit for this cpu
            virtual double get_work_progress(int cpu_idx) = 0;
            virtual std::vector<double> get_work_progress(void) = 0;
            // unique id of a process with its own memory being managed by
            // the geopm controller
            // this may be:
            // - COMM_WORLD MPI rank
            // - local MPI rank
            // - Linux parent process id for whole app
            // unique id for things being coordinated within a job
            // there will be one-to-one mapping of Profile object (AppRecordLog etc) per process
            // this is not *necessarily* a linux process id, but it could be
            // it just has to be self consistent and unique within a job
            // more notes:
            // there is one AppRecordLog per process on each side of the shmem
            // there is one ApplicationStatus per node (board) on each side of the shmem
            virtual void set_process(std::set<int> cpu_idx, int process) = 0;
            virtual int get_process(int cpu_idx) = 0;
            virtual std::vector<int> get_process(void) = 0;

            // ID of a thread within a process
            // - Linux child process id
            // this should be something that linux understands0
            //void set_thread_id(int cpu_idx, int thread_id);
            //int get_thread_id(int cpu_idx);
            //std::vector<int> get_thread_id(void);

            static std::unique_ptr<ApplicationStatus> make_unique(const PlatformTopo& topo,
                                                                  std::shared_ptr<SharedMemory> shmem);
            static size_t buffer_size(int num_cpu);

        protected:
            static constexpr size_t M_STATUS_SIZE = geopm::hardware_destructive_interference_size;
    };

    class ApplicationStatusImp : public ApplicationStatus
    {
        public:
            ApplicationStatusImp(const PlatformTopo& topo,
                                 std::shared_ptr<SharedMemory> shmem);
            virtual ~ApplicationStatusImp() = default;
            void set_hint(int cpu_idx, uint64_t hints) override;
            uint64_t get_hint(int cpu_idx) override;
            std::vector<uint64_t> get_hint(void) override;
            void set_hash(int cpu_idx, uint64_t hash) override;
            uint64_t get_hash(int cpu_idx) override;
            std::vector<uint64_t> get_hash(void) override;
            void set_total_work_units(int cpu_idx, int work_units) override;
            void increment_work_unit(int cpu_idx) override;
            double get_work_progress(int cpu_idx) override;
            std::vector<double> get_work_progress(void) override;
            void set_process(std::set<int> cpu_idx, int process) override;
            int get_process(int cpu_idx) override;
            std::vector<int> get_process(void) override;
        private:
            // These fields must all be 32-bit int
            struct m_app_status_s
            {
                uint32_t hints;
                uint32_t hash;
                int32_t process; // can be negative
                uint32_t total_work;
                uint32_t completed_work;
                char padding[44];
            };
            static_assert((sizeof(ApplicationStatusImp::m_app_status_s) % geopm::hardware_destructive_interference_size) == 0,
                          "m_app_status_s not aligned to cache lines");
            static_assert(sizeof(ApplicationStatusImp::m_app_status_s) == ApplicationStatus::M_STATUS_SIZE,
                          "M_STATUS_SIZE does not match size of m_app_status_s");

            const PlatformTopo &m_topo;
            int m_num_cpu;
            std::shared_ptr<SharedMemory> m_shmem;
            // TODO: this is an array; better way to handle?
            // TODO: want all hints, etc. next to each other since
            // we will frequently use the vector return
            m_app_status_s *m_buffer;
    };
}

#endif
