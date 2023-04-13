/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "ApplicationRecordLog.hpp"
#include <unistd.h>
#include "geopm/SharedMemory.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"
#include "geopm_hint.h"
#include "geopm_hash.h"


namespace geopm
{
    std::unique_ptr<ApplicationRecordLog> ApplicationRecordLog::make_unique(std::shared_ptr<SharedMemory> shmem)
    {
        return geopm::make_unique<ApplicationRecordLogImp>(shmem);
    }

    size_t ApplicationRecordLog::buffer_size(void)
    {
        return M_LAYOUT_SIZE;
    }

    size_t ApplicationRecordLog::max_record(void)
    {
        return M_MAX_RECORD;
    }

    size_t ApplicationRecordLog::max_region(void)
    {
        return M_MAX_REGION;
    }

    ApplicationRecordLogImp::ApplicationRecordLogImp(std::shared_ptr<SharedMemory> shmem)
        : ApplicationRecordLogImp(shmem, getpid())
    {
    }

    ApplicationRecordLogImp::ApplicationRecordLogImp(std::shared_ptr<SharedMemory> shmem,
                                                     int process)
        : m_process(process)
        , m_shmem(shmem)
        , m_epoch_count(0)
        , m_entered_region_hash(GEOPM_REGION_HASH_INVALID)
    {
        if (m_shmem->size() < buffer_size()) {
            throw Exception("ApplicationRecordLog: Shared memory provided in constructor is too small",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void ApplicationRecordLogImp::enter(uint64_t hash, const geopm_time_s &time)
    {
        std::unique_ptr<SharedMemoryScopedLock> lock = m_shmem->get_scoped_lock();
        m_layout_s &layout = *((m_layout_s *)(m_shmem->pointer()));
        check_reset(layout);
        auto emplace_pair = m_hash_region_enter_map.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(hash),
            std::forward_as_tuple());
        bool is_new  = emplace_pair.second;
        m_region_enter_s &region_enter = emplace_pair.first->second;
        region_enter.enter_time = time;
        if (is_new) {
            region_enter.record_idx = layout.num_record;
            region_enter.region_idx = -1; // Not a short region yet
            region_enter.is_short = false;
            record_s enter_record = {
               .time = time,
               .process = m_process,
               .event = EVENT_REGION_ENTRY,
               .signal = hash,
            };
            append_record(layout, enter_record);
        }
        m_entered_region_hash = hash;
    }

    void ApplicationRecordLogImp::exit(uint64_t hash, const geopm_time_s &time)
    {
        std::unique_ptr<SharedMemoryScopedLock> lock = m_shmem->get_scoped_lock();
        m_layout_s &layout = *((m_layout_s *)(m_shmem->pointer()));
        check_reset(layout);

        auto region_it = m_hash_region_enter_map.find(hash);
        if (region_it == m_hash_region_enter_map.end()) {
            // No short region info; send a normal exit event
            record_s exit_record = {
               .time = time,
               .process = m_process,
               .event = EVENT_REGION_EXIT,
               .signal = hash,
            };
            append_record(layout, exit_record);
        }
        else {
            // This region was previous marked short or an entry
            // occurred in the same control loop.
            auto &enter_info = region_it->second;
            enter_info.is_short = true;
            if (enter_info.record_idx == -1) {
                GEOPM_DEBUG_ASSERT(enter_info.region_idx == -1,
                                   "Short region in list with no matching record");
                // There is a region entry from a previous control loop that
                // is not in records array yet.  This will be converted
                // to a short region record by the next block.
                record_s enter_record = {
                    .time = time,
                    .process = m_process,
                    .event = EVENT_REGION_ENTRY,
                    .signal = hash,
                };
                enter_info.record_idx = layout.num_record;
                append_record(layout, enter_record);
            }
            GEOPM_DEBUG_ASSERT(enter_info.record_idx >= 0 && enter_info.record_idx < layout.num_record,
                               "Invalid record index");

            // find or add the region in short regions array
            int region_idx = enter_info.region_idx;
            if (region_idx == -1) {
                region_idx = layout.num_region;
                enter_info.region_idx = region_idx;
                ++(layout.num_region);
                GEOPM_DEBUG_ASSERT(layout.num_region <= M_MAX_REGION,
                                   "ApplicationRecordLogImp::exit(): too many regions entered and exited within one control loop");
                // Add a new short region
                layout.region_table[region_idx] = {
                    .hash = hash,
                    .num_complete = 0,
                    .total_time = 0.0,
                };
                GEOPM_DEBUG_ASSERT(layout.record_table[enter_info.record_idx].event == EVENT_REGION_ENTRY,
                                   "ApplicationRegionLog::exit(): adding a new short region when existing was not an entry.");
                // Convert the region entry event into a short region event
                layout.record_table[enter_info.record_idx].event = EVENT_SHORT_REGION;
                layout.record_table[enter_info.record_idx].signal = region_idx;
            }
            GEOPM_DEBUG_ASSERT(region_idx >= 0 && region_idx < layout.num_region,
                               "Invalid region index");
            // Update the count and total time for the short region
            auto &region = layout.region_table[region_idx];
            ++(region.num_complete);
            region.total_time += geopm_time_diff(&(enter_info.enter_time), &time);
        }
        m_entered_region_hash = GEOPM_REGION_HASH_INVALID;
    }

    void ApplicationRecordLogImp::epoch(const geopm_time_s &time)
    {
        std::unique_ptr<SharedMemoryScopedLock> lock = m_shmem->get_scoped_lock();
        m_layout_s &layout = *((m_layout_s *)(m_shmem->pointer()));
        check_reset(layout);

        ++m_epoch_count;
        record_s epoch_record = {
           .time = time,
           .process = m_process,
           .event = EVENT_EPOCH_COUNT,
           .signal = m_epoch_count,
        };
        append_record(layout, epoch_record);
    }

    void ApplicationRecordLogImp::cpuset_changed(const geopm_time_s &time)
    {
        std::unique_ptr<SharedMemoryScopedLock> lock = m_shmem->get_scoped_lock();
        m_layout_s &layout = *((m_layout_s *)(m_shmem->pointer()));
        check_reset(layout);

        record_s affinity_record = {
           .time = time,
           .process = m_process,
           .event = EVENT_AFFINITY,
           .signal = (uint64_t)m_process, // Could be TID (not PID) in the future
        };
        append_record(layout, affinity_record);
    }

    void ApplicationRecordLogImp::dump(std::vector<record_s> &records,
                                       std::vector<short_region_s> &short_regions)
    {
        // this function should not do anything with m_hash_region_enter_map
        std::unique_ptr<SharedMemoryScopedLock> lock = m_shmem->get_scoped_lock();
        m_layout_s &layout = *((m_layout_s *)(m_shmem->pointer()));
        records.assign(layout.record_table, layout.record_table + layout.num_record);
        short_regions.assign(layout.region_table, layout.region_table + layout.num_region);
        layout.num_record = 0;
        layout.num_region = 0;
    }

    void ApplicationRecordLogImp::check_reset(m_layout_s &layout)
    {
        if (layout.num_record == 0) {
            // Other side has cleared the records.
            // If currently in a short region, keep track of any short region data.
            auto region_enter_it = m_hash_region_enter_map.find(m_entered_region_hash);
            if (region_enter_it != m_hash_region_enter_map.end()) {
                auto &entry_info = region_enter_it->second;
                if (entry_info.is_short) {
                    // the current region was previous marked as short;
                    // maintain entry to convert a future exit
                    entry_info.record_idx = -1;
                    entry_info.region_idx = -1;
                    m_hash_region_enter_map = {*region_enter_it};
                }
                else {
                    // never marked as short region so an exit event will be sent
                    m_hash_region_enter_map.clear();
                }
            }
            else {
                m_hash_region_enter_map.clear();
            }
        }
    }


    void ApplicationRecordLogImp::append_record(m_layout_s &layout, const record_s &record)
    {
        int record_idx = layout.num_record;
        // Don't overrun the buffer
        if (record_idx < M_MAX_RECORD) {
            layout.record_table[record_idx] = record;
            ++(layout.num_record);
        }
        else {
            throw Exception("ApplicationRecordLog: maximum number of records reached.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }
}
