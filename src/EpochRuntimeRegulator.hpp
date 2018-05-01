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
#include "geopm_message.h"

namespace geopm
{
    class IKruntimeRegulator;

    class IEpochRuntimeRegulator
    {
        public:
            IEpochRuntimeRegulator() = default;
            virtual ~IEpochRuntimeRegulator() = default;
            virtual void init_unmarked_region() = 0;
            virtual void epoch(int rank, struct geopm_time_s epoch_time) = 0;
            virtual void record_entry(uint64_t region_id, int rank, struct geopm_time_s entry_time) = 0;
            virtual void record_exit(uint64_t region_id, int rank, struct geopm_time_s exit_time) = 0;
            virtual const IKruntimeRegulator &region_regulator(uint64_t region_id) const = 0;
            virtual bool is_regulated(uint64_t region_id) const = 0;
            virtual std::vector<double> last_epoch_time() const = 0;
            virtual std::vector<double> epoch_count() const = 0;
            virtual double total_region_runtime(uint64_t region_id) const = 0;
            virtual double total_region_mpi_time(uint64_t region_id) const = 0;
            virtual double total_epoch_runtime(void) const = 0;
            virtual double total_app_mpi_time(void) const = 0;
            virtual double total_app_ignore_time(void) const = 0;
            virtual int total_count(uint64_t region_id) const = 0;
            /// @todo this level of pass through will go away once this class is
            /// merged with ApplicationIO
            virtual std::list<geopm_region_info_s> region_entry_exit(void) const = 0;
            virtual void clear_region_entry_exit(void) = 0;
    };

    class EpochRuntimeRegulator : public IEpochRuntimeRegulator
    {
        public:
            EpochRuntimeRegulator() = delete;
            EpochRuntimeRegulator(int rank_per_node);
            virtual ~EpochRuntimeRegulator();
            virtual void init_unmarked_region() override;
            void epoch(int rank, struct geopm_time_s epoch_time) override;
            void record_entry(uint64_t region_id, int rank, struct geopm_time_s entry_time) override;
            void record_exit(uint64_t region_id, int rank, struct geopm_time_s exit_time) override;
            const IKruntimeRegulator &region_regulator(uint64_t region_id) const override;
            bool is_regulated(uint64_t region_id) const override;
            std::vector<double> last_epoch_time() const override;
            std::vector<double> epoch_count() const;

            double total_region_runtime(uint64_t region_id) const override;
            double total_region_mpi_time(uint64_t region_id) const override;
            double total_epoch_runtime(void) const override;
            double total_app_mpi_time(void) const override;
            double total_app_ignore_time(void) const override;
            int total_count(uint64_t region_id) const override;
            std::list<geopm_region_info_s> region_entry_exit(void) const override;
            void clear_region_entry_exit(void) override;
        private:
            std::vector<double> per_rank_last_runtime(uint64_t region_id) const;
            int m_rank_per_node;
            std::map<uint64_t, std::unique_ptr<IKruntimeRegulator> > m_rid_regulator_map;
            std::vector<bool> m_seen_first_epoch;
            std::vector<double> m_curr_ignore_runtime;
            std::vector<double> m_agg_ignore_runtime;
            std::vector<double> m_curr_mpi_runtime;
            std::vector<double> m_agg_mpi_runtime;
            std::vector<double> m_last_epoch_runtime;
            std::vector<double> m_agg_runtime;
            std::vector<std::set<uint64_t> > m_pre_epoch_region;
            std::list<geopm_region_info_s> m_region_entry_exit;
    };
}

#endif
