/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef EPOCHRUNTIMEREGULATOR_HPP_INCLUDE
#define EPOCHRUNTIMEREGULATOR_HPP_INCLUDE

#include <vector>
#include <string>
#include <set>
#include <map>
#include <memory>
#include <list>

#include "geopm_time.h"
#include "geopm_internal.h"

namespace geopm
{
    class IRuntimeRegulator;

    class IEpochRuntimeRegulator
    {
        public:
            IEpochRuntimeRegulator() = default;
            virtual ~IEpochRuntimeRegulator() = default;
            /// @brief Handle the initial entry into the unmarked
            ///        region when the application starts.
            virtual void init_unmarked_region() = 0;
            /// @brief Record a transition between epochs with
            ///        entry/exit into the epoch region.
            virtual void epoch(int rank, struct geopm_time_s epoch_time) = 0;
            /// @brief Record entry into a region for one rank at the
            ///        given time.
            /// @param [in] region_id The ID of the region.
            /// @param [in] rank Rank tht entered the region.
            /// @param [in] entry_time Time of entry.
            virtual void record_entry(uint64_t region_id, int rank, struct geopm_time_s entry_time) = 0;
            /// @brief Record exit from a region for one rank at the
            ///        given time.  Also aggregates totals for the
            ///        epoch region.
            /// @param [in] region_id The ID of the region.
            /// @param [in] rank Rank that exited the region.
            /// @param [in] exit_time Time of exit.
            virtual void record_exit(uint64_t region_id, int rank, struct geopm_time_s exit_time) = 0;
            /// @brief Returns a reference to the RuntimeRegulator for
            ///        a given region.  This method is intended for
            ///        internal use by the ApplicationIO.
            /// @param [in] region_id The ID of the region.
            virtual const IRuntimeRegulator &region_regulator(uint64_t region_id) const = 0;
            /// @brief Returns whether or not the region is being
            ///        tracked by the EpochRuntimeRegulator.
            /// @param [in] region_id The ID of the region.
            virtual bool is_regulated(uint64_t region_id) const = 0;
            /// @brief Returns the total runtime of the last iteration
            ///        of the epoch for each rank.
            virtual std::vector<double> last_epoch_runtime() const = 0;
            /// @brief Returns the total mpi runtime of the last iteration
            ///        of the epoch for each rank.
            virtual std::vector<double> last_epoch_runtime_mpi() const = 0;
            /// @brief Returns the total ignore runtime of the last iteration
            ///        of the epoch for each rank.
            virtual std::vector<double> last_epoch_runtime_ignore() const = 0;
            /// @brief Returns the number of epoch calls seen by each
            ///        rank.
            virtual std::vector<double> epoch_count() const = 0;
            /// @brief Returns the total runtime recorded for the
            ///        given region.
            /// @param [in] region_id The ID of the region.
            virtual double total_region_runtime(uint64_t region_id) const = 0;
            /// @brief Returns the total time spent in MPI for a given
            ///        region.
            /// @param [in] region_id The ID of the region.
            virtual double total_region_runtime_mpi(uint64_t region_id) const = 0;
            /// @brief Returns the total runtime since the first epoch
            ///        call.  This total does not include MPI time or
            ///        ignore time.
            virtual double total_epoch_runtime(void) const = 0;
            /// @brief Returns the total time spent in MPI after the
            ///        first epoch call.
            virtual double total_epoch_runtime_mpi(void) const = 0;
            /// @brief Returns the total time spent in regions marked
            ///        with GEOPM_REGION_ID_HINT_IGNORE after the
            ///        first epoch call.
            virtual double total_epoch_runtime_ignore(void) const = 0;
            /// @brief Returns the total package energy since the
            ///        first epoch call.
            virtual double total_epoch_energy_pkg(void) const = 0;
            /// @brief Returns the total dram energy since the first
            ///        epoch call.
            virtual double total_epoch_energy_dram(void) const = 0;
            /// @brief Returns the total time spent in MPI calls since
            ///        the start of the application.
            virtual double total_app_runtime_mpi(void) const = 0;
            /// @brief Returns the total number of times a region was
            ///        entered and exited.
            /// @param [in] region_id The ID of the region.
            virtual int total_count(uint64_t region_id) const = 0;
            /// @todo this level of pass through will go away once
            ///       this class is merged with ApplicationIO
            /// @brief Returns the list of all regions entered or
            ///        exited since the last call to
            ///        clear_region_info().
            virtual std::list<geopm_region_info_s> region_info(void) const = 0;
            /// @brief Resets the internal list of region entries and
            ///        exits.
            virtual void clear_region_info(void) = 0;
    };

    class IPlatformIO;
    class IPlatformTopo;

    class EpochRuntimeRegulator : public IEpochRuntimeRegulator
    {
        public:
            EpochRuntimeRegulator() = delete;
            EpochRuntimeRegulator(int rank_per_node, IPlatformIO &platform_io,
                                  IPlatformTopo &platform_topo);
            virtual ~EpochRuntimeRegulator();
            virtual void init_unmarked_region() override;
            void epoch(int rank, struct geopm_time_s epoch_time) override;
            void record_entry(uint64_t region_id, int rank, struct geopm_time_s entry_time) override;
            void record_exit(uint64_t region_id, int rank, struct geopm_time_s exit_time) override;
            const IRuntimeRegulator &region_regulator(uint64_t region_id) const override;
            bool is_regulated(uint64_t region_id) const override;
            std::vector<double> last_epoch_runtime() const override;
            std::vector<double> last_epoch_runtime_mpi() const override;
            std::vector<double> last_epoch_runtime_ignore() const override;
            std::vector<double> epoch_count() const override;
            double total_region_runtime(uint64_t region_id) const override;
            double total_region_runtime_mpi(uint64_t region_id) const override;
            double total_epoch_runtime(void) const override;
            double total_epoch_runtime_mpi(void) const override;
            double total_epoch_runtime_ignore(void) const override;
            double total_epoch_energy_pkg(void) const override;
            double total_epoch_energy_dram(void) const override;
            double total_app_runtime_mpi(void) const override;
            int total_count(uint64_t region_id) const override;
            std::list<geopm_region_info_s> region_info(void) const override;
            void clear_region_info(void) override;
        private:
            std::vector<double> per_rank_last_runtime(uint64_t region_id) const;
            double current_energy_pkg(void) const;
            double current_energy_dram(void) const;
            int m_rank_per_node;
            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
            std::map<uint64_t, std::unique_ptr<IRuntimeRegulator> > m_rid_regulator_map;
            std::vector<bool> m_seen_first_epoch;
            std::vector<double> m_curr_runtime_ignore;
            std::vector<double> m_agg_epoch_runtime_ignore;
            std::vector<double> m_curr_runtime_mpi;
            std::vector<double> m_agg_epoch_runtime_mpi;
            std::vector<double> m_agg_runtime_mpi;
            std::vector<double> m_last_epoch_runtime;
            std::vector<double> m_last_epoch_runtime_mpi;
            std::vector<double> m_last_epoch_runtime_ignore;
            std::vector<double> m_agg_epoch_runtime;
            std::vector<std::set<uint64_t> > m_pre_epoch_region;
            std::list<geopm_region_info_s> m_region_info;
            double m_epoch_start_energy_pkg;
            double m_epoch_start_energy_dram;
            double m_epoch_total_energy_pkg;
            double m_epoch_total_energy_dram;
            std::map<uint64_t, int> m_region_rank_count;
    };
}

#endif
