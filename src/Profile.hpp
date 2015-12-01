/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef PROFILE_HPP_INCLUDE
#define PROFILE_HPP_INCLUDE

#include <stdint.h>
#include <string>
#include <list>
#include <forward_list>
#include <fstream>
#include <mpi.h>

#include "geopm_time.h"
#include "geopm_message.h"
#include "SharedMemory.hpp"
#include "LockingHashTable.hpp"

namespace geopm
{
    /// @brief Encapsulates the state and provides the interface for
    /// computational application profiling.
    ///
    /// The C++ implementation of the computational application side
    /// interface to the GEOPM profiler.  The class methods support
    /// the C interface defined for use with the geopm_prof_c
    /// structure and are named accordingly.  The geopm_prof_c
    /// structure is an opaque reference to the geopm::Profile class.
    class Profile
    {
        public:
            /// @brief Profile constructor.
            ///
            /// The Profile object is used by the application to
            /// instrument regions of code and post profile
            /// information to a shared memory region to be read by
            /// the Controller process.
            ///
            /// @param [in] prof_name Name associated with the
            ///        profile.  This name will be printed in the
            ///        header of the report.
            ///
            /// @param [in] table_size Size in bytes of shared memory
            ///        region that will be used for posting updates.
            ///        The Controller is responsible for creating the
            ///        shared memory region that the Profile object
            ///        attaches to.  The Controller is the consumer of
            ///        the posted data that the Profile produces.
            ///
            /// @param [in] shm_key_base String that is the base for
            ///        the POSIX shared memory keys that have been
            ///        created by the Controller.  There is one key
            ///        created for each MPI rank in the communicator
            ///        provided (comm), and each key is constructed by
            ///        appending an underscore followed by a string
            ///        representation of the integer MPI rank.
            ///
            /// @param [in] comm The application's MPI communicator.
            ///        Each rank of this communicator will report to a
            ///        separate shared memory region.  One controller
            ///        on each compute node will consume the output
            ///        from each rank running on the compute node.
            Profile(const std::string prof_name, size_t table_size, const std::string shm_key_base, MPI_Comm comm);
            /// @brief Profile destructor, virtual.
            virtual ~Profile();
            /// @brief Register a region of code to be profiled.
            ///
            /// The statistics gathered for each region are aggregated
            /// in the final report, and the power policy will be
            /// determined distinctly for each region.  The
            /// registration of a region is idempotent, and the first
            /// call will have more overhead than subsequent
            /// attempts to re-register the same region.
            ///
            /// @param [in] region_name Unique name that identifies
            ///             the region being profiled.  This name will
            ///             be printed next to the region statistics
            ///             in the report.
            ///
            /// @param [in] policy_hint Value from the
            ///             #geopm_policy_hint_e structure which is
            ///             used to derive a starting policy before
            ///             the application has been profiled.
            uint64_t region(const std::string region_name, long policy_hint);
            void enter(uint64_t region_id);
            void exit(uint64_t region_id);
            void progress(uint64_t region_id, double fraction);
            void outer_sync(void);
            void sample(uint64_t region_id);
            void enable(const std::string feature_name);
            void disable(const std::string feature_name);
            void print(const std::string file_name, int depth);
        protected:
            void name_set(const std::string file_name);
            void report(void);
            void shutdown(void);
            void init_cpu_list(void);
            std::string m_prof_name;
            uint64_t m_curr_region_id;
            int m_num_enter;
            int m_num_progress;
            double m_progress;
            void *m_table_buffer;
            SharedMemoryUser *m_ctl_shmem;
            struct geopm_ctl_message_s *m_ctl_msg;
            SharedMemoryUser *m_table_shmem;
            LockingHashTable<struct geopm_prof_message_s> *m_table;
            std::list<int> m_cpu_list;
            MPI_Comm m_shm_comm;
            int m_rank;
            int m_shm_rank;
    };

    class ProfileRankSampler
    {
        public:
            ProfileRankSampler(const std::string shm_key, size_t table_size);
            virtual ~ProfileRankSampler();
            void rank_sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content_begin, size_t &length);
            size_t capacity(void);
            void report(std::ofstream &file_desc);
            bool name_fill(void);
        protected:
            SharedMemory m_table_shmem;
            LockingHashTable<struct geopm_prof_message_s> m_table;
            std::map<uint64_t, struct geopm_sample_message_s> m_agg_stats;
            struct geopm_prof_message_s m_region_entry;
            std::string m_prof_name;
            std::string m_report_name;
            std::set<std::string> m_name_set;
            bool m_is_name_finished;
    };

    class ProfileSampler
    {
        public:
            ProfileSampler(const std::string shm_key_base, size_t table_size, MPI_Comm comm);
            virtual ~ProfileSampler(void);
            size_t capacity(void);
            void sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length);
            bool do_shutdown(void);
            void report(void);
            void initialize(void);
        protected:
            void name_set(std::string &file_name, std::string &prof_name, std::set<std::string> &key_name);
            void print(const std::string file_name, const std::set<std::string> &key_name);
            SharedMemory m_ctl_shmem;
            struct geopm_ctl_message_s *m_ctl_msg;
            std::forward_list<ProfileRankSampler *> m_rank_sampler;
            MPI_Comm m_comm;
            size_t m_table_size;
    };
}

#endif
