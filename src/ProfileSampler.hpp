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

#ifndef PROFILE_SAMPLER_HPP_INCLUDE
#define PROFILE_SAMPLER_HPP_INCLUDE

#include <forward_list>

#include "ControlMessage.hpp"

namespace geopm
{
    class ISharedMemory;
    class IProfileTable;
    class IProfileThreadTable;

    /// @brief Retrieves sample data from the set of application ranks on ///        a single node.
    ///
    /// The ProfileSampler class is the geopm runtime side interface to
    /// the GEOPM profiler. It retrieves samples from all application ranks
    /// on a single compute node. It is also the interface to the shared
    /// memory region used to coordinate between the geopm runtime and
    /// the MPI application.
    class IProfileSampler
    {
        public:
            IProfileSampler() {}
            IProfileSampler(const IProfileSampler &other) {}
            virtual ~IProfileSampler() {}
            /// @brief Retrieve the maximum capacity of all the per-rank
            ///        hash tables.
            ///
            /// @return The maximum number of samples that can possibly
            ///         be returned.
            virtual size_t capacity(void) = 0;
            /// @brief Returns the samples present in all the per-rank
            ///        hash tables.
            ///
            /// Fills in a portion of a vector which is assumed to be already
            /// sized greater than or equal to the maximum number of samples
            /// we can return. This value can be queried with the capacity()
            /// method.
            ///
            /// @param [in] content Vector to be filled with per-node
            ///        sample messages.
            ///
            /// @param [out] length The number of samples that were inserted.
            virtual void sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length, IComm *comm) = 0;
            /// @brief Check if the application is shutting down.
            ///
            /// Queries the control shared memory region to test if the
            /// application status in shutdown.
            ///
            /// @return Return true if application is shutting down, else
            ///         returns false.
            virtual bool do_shutdown(void) = 0;
            /// @brief Generate a post-run report for a single node.
            ///
            /// Generates a post-run report by telling each ProfileRankSampler
            /// to dump its per-region statistics to a file descriptor.
            virtual bool do_report(void) = 0;
            virtual void region_names(void) = 0;
            /// @brief Initialize shared memory regions.
            ///
            /// Coordinates with the application to initialize shared memory
            /// and create ProfileRankSamplers for each MPI application rank
            /// running on the local compute node.
            ///
            /// @param [out] rank_per_node number of mpi ranks
            /// running on the node.
            virtual void initialize(int &rank_per_node) = 0;
            /// @brief Retrieve a vector to the affinities of all
            ///        application ranks.
            ///
            /// Resizes the input vector cpu_rank to number of Linux
            /// online CPUs in the system.  Each element of the vector
            /// is indexed by the Linux CPU ID, and the value assigned
            /// is the MPI rank running on the CPU (or -1 if no rank
            /// has been affinitized).
            ///
            /// @param [out] cpu_rank Vector to be filled with the MPI
            ///        rank for each Linux CPU, set to -1 if no MPI
            ///        rank is affinitized.
            virtual void cpu_rank(std::vector<int> &cpu_rank) = 0;
            virtual void name_set(std::set<std::string> &region_name) = 0;
            virtual void report_name(std::string &report_str) = 0;
            virtual void profile_name(std::string &prof_str) = 0;
            virtual IProfileThreadTable *tprof_table(void) = 0;
    };

    /// @brief Retrieves sample data from a single application rank through
    ///        a shared memory interface.
    ///
    /// The ProfileRankSampler is the runtime side interface to the shared
    /// memory region for a single rank of the application. It can retrieve
    /// samples from the shared hash table for that rank.
    class IProfileRankSampler
    {
        public:
            IProfileRankSampler() {}
            IProfileRankSampler(const IProfileRankSampler &other) {}
            virtual ~IProfileRankSampler() {}
            /// @brief Returns the samples present in the hash table.
            ///
            /// Fills in a portion of a vector specified by a vector iterator.
            /// It is assumed the vector is already sized greater than or
            /// equal to the maximum number of samples we can return. This value
            /// can be queried with the capacity() method. Internally the samples
            /// are aggregated for later reporting functionality.
            ///
            /// @param [in] content_begin Vector iterator at which to begin inserting
            ///        sample messages.
            ///
            /// @param [out] length The number of samples that were inserted.
            virtual void sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content_begin, size_t &length) = 0;
            /// @brief Retrieve the maximum capacity of the hash table.
            ///
            /// @return The maximum number of samples that can possibly
            ///         be returned.
            virtual size_t capacity(void) = 0;
            /// @brief Retrieve region names from the application process.
            ///
            /// Coordinates with the application process to retrieve the
            /// profile name, region names, and the file name to write
            /// the report to.
            ///
            /// @return Returns true if finished retrieving names from the
            ///         application, else returns false.
            virtual bool name_fill(std::set<std::string> &name_set) = 0;
            virtual void report_name(std::string &report_str) = 0;
            virtual void profile_name(std::string &prof_str) = 0;
    };

    class ProfileSampler : public IProfileSampler
    {
        public:
            /// @brief ProfileSampler constructor.
            ///
            /// Constructs a shared memory region for coordination between
            /// the geopm runtime and the MPI application.
            ///
            /// @param [in] table_size The size of the hash table that will
            ///        be created for each application rank.
            ProfileSampler(size_t table_size);
            /// @brief ProfileSampler destructor.
            virtual ~ProfileSampler();
            size_t capacity(void);
            void sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length, IComm *comm);
            bool do_shutdown(void);
            bool do_report(void);
            void region_names(void);
            void initialize(int &rank_per_node);
            void cpu_rank(std::vector<int> &cpu_rank);
            void name_set(std::set<std::string> &region_name);
            void report_name(std::string &report_str);
            void profile_name(std::string &prof_str);
            IProfileThreadTable *tprof_table(void);
        protected:
            /// Holds the shared memory region used for application coordination
            /// and control.
            ISharedMemory *m_ctl_shmem;
            /// Pointer to the control structure used for application coordination
            /// and control.
            IControlMessage *m_ctl_msg;
            /// List of per-rank samplers for each MPI application rank running
            /// on the local compute node.
            std::forward_list<IProfileRankSampler *> m_rank_sampler;
            /// Size of the hash tables to create for each MPI application rank
            /// running on the local compute node..
            const size_t m_table_size;
            std::set<std::string> m_name_set;
            std::string m_report_name;
            std::string m_profile_name;
            bool m_do_report;
            ISharedMemory *m_tprof_shmem;
            IProfileThreadTable *m_tprof_table;
    };

    class ProfileRankSampler : public IProfileRankSampler
    {
        public:
            /// @brief ProfileRankSampler constructor.
            ///
            /// The ProfileRankSampler constructor takes in a unique shared
            /// memory key for the rank as well as the size of the hash table
            /// to be shared with the application rank. It creates the shared
            /// memory region and the hash table that the application will
            /// attach to.
            ///
            /// @param [in] shm_key Shared memory key unique to a
            ///        specific rank.
            ///
            /// @param [in] table_size Size of the hash table to create in
            ///        the shared memory region.
            ProfileRankSampler(const std::string shm_key, size_t table_size);
            /// @brief ProfileRankSampler destructor.
            ///
            /// Cleans up the hash table and shared memory region.
            virtual ~ProfileRankSampler();
            void sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content_begin, size_t &length);
            size_t capacity(void);
            bool name_fill(std::set<std::string> &name_set);
            void report_name(std::string &report_str);
            void profile_name(std::string &prof_str);
            IProfileThreadTable *tprof_table(void);
        protected:
            /// Holds the shared memory region used for sampling from the
            /// application process.
            ISharedMemory *m_table_shmem;
            /// The hash table which stores application process samples.
            IProfileTable *m_table;
            ISharedMemory *m_tprof_shmem;
            IProfileThreadTable *m_tprof_table;
            /// Holds the initial state of the last region entered.
            struct geopm_prof_message_s m_region_entry;
            /// Holds the initial state of the last region entered.
            struct geopm_prof_message_s m_epoch_entry;
            /// Holds the profile name string.
            std::string m_prof_name;
            /// Holds the file name for the post-process report.
            std::string m_report_name;
            /// Holds the set of region string names.
            std::set<std::string> m_name_set;
            /// Holds the status of the name_fill operation.
            bool m_is_name_finished;
    };
}

#endif
