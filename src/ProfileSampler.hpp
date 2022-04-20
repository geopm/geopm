/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PROFILESAMPLER_HPP_INCLUDE
#define PROFILESAMPLER_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <string>
#include <set>
#include <forward_list>
#include <memory>


namespace geopm
{
    class ProfileRankSampler
    {
        public:
            ProfileRankSampler() = default;
            ProfileRankSampler(const ProfileRankSampler &other) = default;
            ProfileRankSampler &operator=(const ProfileRankSampler &other) = default;
            virtual ~ProfileRankSampler() = default;
            /// @brief Retrieve region names from the application process.
            ///
            /// Coordinates with the application process to retrieve the
            /// profile name, region names, and the file name to write
            /// the report to.
            ///
            /// @return Returns true if finished retrieving names from the
            ///         application, else returns false.
            virtual bool name_fill(std::set<std::string> &name_set) = 0;
            virtual void report_name(std::string &report_str) const = 0;
            virtual void profile_name(std::string &prof_str) const = 0;
    };

    class Comm;

    class ProfileSampler
    {
        public:
            ProfileSampler() = default;
            ProfileSampler(const ProfileSampler &other) = default;
            ProfileSampler &operator=(const ProfileSampler &other) = default;
            virtual ~ProfileSampler() = default;
            /// @brief Check if the application is shutting down.
            ///
            /// Queries the control shared memory region to test if the
            /// application status in shutdown.
            ///
            /// @return Return true if application is shutting down, else
            ///         returns false.
            virtual bool do_shutdown(void) const = 0;
            /// @brief Generate a post-run report for a single node.
            ///
            /// Generates a post-run report by telling each ProfileRankSampler
            /// to dump its per-region statistics to a file descriptor.
            virtual bool do_report(void) const = 0;
            virtual void region_names(void) = 0;
            /// @brief Initialize shared memory regions.
            ///
            /// Coordinates with the application to initialize shared memory
            /// and create ProfileRankSamplers for each MPI application rank
            /// running on the local compute node.
            virtual void initialize(void) = 0;
            /// @brief Return the number of ranks per node.
            ///
            /// @return number of mpi ranks
            /// running on the node.
            virtual int rank_per_node(void) const = 0;
            /// @brief Retrieve a vector to the affinities of all
            ///        application ranks.
            ///
            /// Return vector is sized to number of Linux online CPUs
            /// in the system.  Each element of the vector is indexed
            /// by the Linux CPU ID, and the value assigned is the MPI
            /// rank running on the CPU (or -1 if no rank has been
            /// affinitized).
            ///
            /// @return Vector to be filled with the MPI rank for each
            ///         Linux CPU, set to -1 if no MPI rank is
            ///         affinitized.
            virtual std::vector<int> cpu_rank(void) const = 0;
            virtual std::set<std::string> name_set(void) const = 0;
            virtual std::string report_name(void) const = 0;
            virtual std::string profile_name(void) const = 0;
            /// @brief Signal to the application that the controller
            ///        is ready to begin receiving samples.
            virtual void controller_ready(void) = 0;
            /// @brief Signal application of failure.
            virtual void abort(void) = 0;

            virtual void check_sample_end(void) = 0;
    };


    class SharedMemory;
    class ControlMessage;
    class ProfileTable;


    /// @brief Retrieves sample data from a single application rank through
    ///        a shared memory interface.
    ///
    /// The ProfileRankSampler is the runtime side interface to the shared
    /// memory region for a single rank of the application. It can retrieve
    /// samples from the shared hash table for that rank.
    class ProfileRankSamplerImp : public ProfileRankSampler
    {
        public:
            /// @brief ProfileRankSamplerImp constructor.
            ///
            /// The ProfileRankSamplerImp constructor takes in a unique shared
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
            ProfileRankSamplerImp(const std::string &shm_key, size_t table_size);
            /// @brief ProfileRankSamplerImp destructor.
            ///
            /// Cleans up the hash table and shared memory region.
            virtual ~ProfileRankSamplerImp();
            bool name_fill(std::set<std::string> &name_set) override;
            void report_name(std::string &report_str) const override;
            void profile_name(std::string &prof_str) const override;
        private:
            /// Holds the shared memory region used for sampling from the
            /// application process.
            std::unique_ptr<SharedMemory> m_table_shmem;
            /// The hash table which stores application process samples.
            std::unique_ptr<ProfileTable> m_table;
            /// Holds the profile name string.
            std::string m_prof_name;
            /// Holds the file name for the post-process report.
            std::string m_report_name;
            /// Holds the set of region string names.
            std::set<std::string> m_name_set;
            /// Holds the status of the name_fill operation.
            bool m_is_name_finished;
    };

    class PlatformTopo;

    /// @brief Retrieves sample data from the set of application ranks on
    ///        a single node.
    ///
    /// The ProfileSampler class is the geopm runtime side interface to
    /// the GEOPM profiler. It retrieves samples from all application ranks
    /// on a single compute node. It is also the interface to the shared
    /// memory region used to coordinate between the geopm runtime and
    /// the MPI application.
    class ProfileSamplerImp : public ProfileSampler
    {
        public:
            /// @brief ProfileSamplerImp constructor.
            ///
            /// Constructs a shared memory region for coordination between
            /// the geopm runtime and the MPI application.
            ///
            /// @param [in] table_size The size of the hash table that will
            ///        be created for each application rank.
            ProfileSamplerImp(size_t table_size);
            /// @brief ProfileSamplerImp constructor.
            ///
            /// Constructs a shared memory region for coordination between
            /// the geopm runtime and the MPI application.
            ///
            /// @param [in] topo Reference to PlatformTopo singleton.
            ///
            /// @param [in] table_size The size of the hash table that will
            ///        be created for each application rank.
            ProfileSamplerImp(const PlatformTopo &topo, size_t table_size);
            /// @brief ProfileSamplerImp destructor.
            virtual ~ProfileSamplerImp();
            bool do_shutdown(void) const override;
            bool do_report(void) const override;
            void region_names(void) override;
            void initialize(void) override;
            int rank_per_node(void) const override;
            std::vector<int> cpu_rank(void) const override;
            std::set<std::string> name_set(void) const override;
            std::string report_name(void) const override;
            std::string profile_name(void) const override;
            void controller_ready(void) override;
            void abort(void) override;
            void check_sample_end(void) override;
        private:
            /// Holds the shared memory region used for application coordination
            /// and control.
            std::unique_ptr<SharedMemory> m_ctl_shmem;
            /// Pointer to the control structure used for application coordination
            /// and control.
            std::unique_ptr<ControlMessage> m_ctl_msg;
            /// List of per-rank samplers for each MPI application rank running
            /// on the local compute node.
            std::forward_list<std::unique_ptr<ProfileRankSampler> > m_rank_sampler;
            /// Size of the hash tables to create for each MPI application rank
            /// running on the local compute node.
            const size_t m_table_size;
            std::set<std::string> m_name_set;
            std::string m_report_name;
            std::string m_profile_name;
            bool m_do_report;
            int m_rank_per_node;
    };
}

#endif
