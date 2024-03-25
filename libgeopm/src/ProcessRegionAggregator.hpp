/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PROCESSREGIONAGGREGATOR_HPP_INCLUDE
#define PROCESSREGIONAGGREGATOR_HPP_INCLUDE

#include <cstdint>

#include <map>
#include <memory>

namespace geopm
{
    /// @brief Class responsible for reading records from the
    ///        ApplicationSampler and calculating the per-process
    ///        runtimes within each region.
    class ProcessRegionAggregator
    {
        public:
            virtual ~ProcessRegionAggregator() = default;
            /// @brief Gets the latest set of records from
            ///        ApplicationSampler.
            virtual void update(void) = 0;
            /// @brief Returns the average total time spent in a
            ///        region across all processes
            ///
            /// @param [in] region_hash Hash of the region.
            virtual double get_runtime_average(uint64_t region_hash) const = 0;
            /// @brief Returns the average number of entries into a
            ///        region across all processes.
            ///
            /// @param [in] region_hash Hash of the region.
            virtual double get_count_average(uint64_t region_hash) const = 0;
            static std::unique_ptr<ProcessRegionAggregator> make_unique(void);
    };

    class ApplicationSampler;

    class ProcessRegionAggregatorImp : public ProcessRegionAggregator
    {
        public:
            ProcessRegionAggregatorImp();
            ProcessRegionAggregatorImp(ApplicationSampler &sampler);
            virtual ~ProcessRegionAggregatorImp() = default;
            void update(void) override;
            double get_runtime_average(uint64_t region_hash) const override;
            double get_count_average(uint64_t region_hash) const override;
        private:
            ApplicationSampler &m_app_sampler;
            int m_num_process;

            // Records will be coming in sorted by process.  An
            // optimization might be to keep an iterator around
            // pointing to the most recent process's map.  The lookup
            // by region hash will happen less frequently but requires
            // iteration over all the process maps.  Build a cache and
            // invalidate it if update() is called.
            struct region_info_s {
                double total_runtime;
                int total_count;
                double last_entry_time;
            };
            std::map<int, std::map<uint64_t, region_info_s> > m_region_info;
    };
}

#endif
