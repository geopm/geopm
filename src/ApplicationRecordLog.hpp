/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#ifndef APPLICATIONRECORDLOG_HPP_INCLUDE
#define APPLICATIONRECORDLOG_HPP_INCLUDE

#include <cstdint>
#include <cstddef>
#include <map>
#include <vector>
#include <memory>

#include "geopm_time.h"
#include "record.hpp"

namespace geopm
{
    class SharedMemory;
    class SharedMemoryScopedLock;

    /// @brief Provides an abstraction for a shared memory buffer that
    ///        can be used to pass entry, exit, epoch and short region
    ///        events from Profile to ApplicationSampler.
    ///
    /// This class provides a compression of short running regions to
    /// avoid overwhelming the controller with too many records.
    ///
    /// If the Profile calls entry() and then exit() for the same
    /// region hash within a control interval (between two calls to
    /// dump()) the entry event is converted into a short region
    /// event.  All future pairs of entry() and exit() using this hash
    /// but prior to the next dump() call will be aggregated into this
    /// short region event.
    ///
    /// If a region hash was observed as a short region during a
    /// control interval (it was both entered and exited), but the
    /// dump() call occurs prior to an exit() call that would close
    /// the region, the subsequent exit() call to this region will be
    /// classified as a short region event and the intervening
    /// interval of time will be recorded in this event created by the
    /// exit() call.
    ///
    /// There is a loss of information when a region is entered and
    /// exited within one control loop when compared to the case where
    /// a dump() call is made between an entry() and exit() call.  In
    /// the former case, the exact timestamp associated with all
    /// region exit and entry events for a particular hash that occur
    /// after the first enter() call for that hash and prior to the
    /// next dump() call are lost.  The short region event records the
    /// number of calls to the hashed region and the total amount of
    /// time in the region, but the exact sequence and timing of
    /// events following the first enter() is not recorded.
    class ApplicationRecordLog
    {
        public:
            /// @brief Factory constructor
            /// @param [in] shmem Shared memory object of at least the
            ///        size returned by buffer_size().
            static std::unique_ptr<ApplicationRecordLog> make_unique(std::shared_ptr<SharedMemory> shmem);
            /// @brief Destructor for pure virtual base class.
            virtual ~ApplicationRecordLog() = default;
            /// @brief Set the process identifier.
            ///
            /// Called by the Profile to set the process identifier
            /// that will be used to tag all control messages.  This
            /// method must be called prior to the enter(), exit() or
            /// epoch() methods.  The value provided is any integer
            /// unique to the compute node that identifies the caller
            /// of the profile interface.  Commonly this is the MPI
            /// rank, but could also be the linux process ID for a
            /// parent thread.
            ///
            /// @param [in] process The process identifier.
            virtual void set_process(int process) = 0;
            /// @brief Set the reference time.
            ///
            /// Called by the Profile to set the reference time that
            /// is used to construct the time element of a record_s.
            /// This method must be called prior to the enter(),
            /// exit() or epoch() methods.
            ///
            /// @param [in] time The timestamp when the profiled
            ///        process began.
            virtual void set_time_zero(const geopm_time_s &time) = 0;
            /// @brief Create a message in the log defining a region
            ///        entry.
            ///
            /// Called by the Profile object when a region is entered.
            /// This creates a record_s in the log indicating entry if
            /// the region is entered for the first time since the
            /// last dump() call, or it sets the start time for a
            /// short region if the region was entered and exited
            /// since the last call to dump().
            ///
            /// @param [in] hash The region hash that is entered.
            ///
            /// @param [in] time The timestamp when the entry event
            ///        occurred.
            virtual void enter(uint64_t hash, const geopm_time_s &time) = 0;
            /// @brief Create a message in the log defining a region
            ///        exit.
            ///
            /// Called by the Profile object when a region is exited.
            /// This creates a record_s in the log indicating exit if
            /// the matching entry for the region was called prior to
            /// the last dump() call.  If the region was entered since
            /// the last call to dump(), the first entry event will be
            /// converted into a short region event.  The call to
            /// exit() for short regions has the effect of updating
            /// the short region event time and count values.
            ///
            /// @param hash [in] The region hash that was exited.
            ///
            /// @param time [in] The timestamp when the exit event
            ///             occurred.
            virtual void exit(uint64_t hash, const geopm_time_s &time) = 0;
            /// @brief Create a message in the log defining an epoch
            ///        event.
            ///
            /// Called by the Profile object when an epoch event
            /// occurs.  This creates a record_s in the log indicating
            /// that an epoch event occurred.
            ///
            /// @param time [in] The timestamp when the epoch event
            ///             occurred.
            virtual void epoch(const geopm_time_s &time) = 0;
            /// @brief Get all events that have occurred since the last
            ///        call to dump().
            ///
            /// Called by the ApplicationSampler to gather all records
            /// that have been created by the Profile object since the
            /// last time the method was called.  The call effectively
            /// removes all of the records and short region data from
            /// the table.
            ///
            /// For optimal performance the user should reserve space
            /// in the output vectors using the max_record() and
            /// max_region() static methods:
            ///
            ///     records.reserve(ApplicationRecordLog::max_record());
            ///     short_regions.reserve(ApplicationRecordLog::max_region());
            ///
            /// Note that the "signal" in any sort region events in
            /// the records output vector is the index into the
            /// short_regions output vector, and the length of the
            /// short_regions vector will be equal to the number of
            /// events with type "EVENT_SHORT_REGION" in the records
            /// output vector.
            ///
            /// @param [out] records Vector of records written since
            ///        last dump().
            ///
            /// @param [out] short_regions Vector of short region data
            ///        about any short regions events in the records
            ///        output vector.
            virtual void dump(std::vector<record_s> &records,
                              std::vector<short_region_s> &short_regions) = 0;
            /// @brief Gets the shared memory size requirement.
            ///
            /// This method returns the value to use when sizing the
            /// SharedMemory object used to construct the
            /// ApplicationRecordLog.
            ///
            /// @return Size requirement for SharedMemory object.
            static size_t buffer_size(void);
            /// @brief Gets the maximum number of records.
            ///
            /// This method returns the value to use when reserving
            /// elements in the records vector passed to dump().
            ///
            /// @return The maximum length of the records vector after
            ///         a call to dump().
            static size_t max_record(void);
            /// @brief Gets the maximum number of short region events.
            ///
            /// This method returns the value to use when reserving
            /// elements in the short_regions vector passed to dump().
            ///
            /// @return The maximum length of the short_regions vector
            ///         after a call to dump().
            static size_t max_region(void);
        protected:
            ApplicationRecordLog() = default;
            static constexpr size_t M_LAYOUT_SIZE = 49192;
            static constexpr int M_MAX_RECORD = 1024;
            static constexpr int M_MAX_REGION = M_MAX_RECORD + 1;
    };
    class ApplicationRecordLogImp : public ApplicationRecordLog
    {
        public:
            ApplicationRecordLogImp(std::shared_ptr<SharedMemory> shmem);
            virtual ~ApplicationRecordLogImp() = default;
            void set_process(int process) override;
            void set_time_zero(const geopm_time_s &time) override;
            void enter(uint64_t hash, const geopm_time_s &time) override;
            void exit(uint64_t hash, const geopm_time_s &time) override;
            void epoch(const geopm_time_s &time) override;
            void dump(std::vector<record_s> &records,
                      std::vector<short_region_s> &short_regions) override;
        private:
            struct m_layout_s {
                int num_record;
                record_s record_table[M_MAX_RECORD];
                int num_region;
                short_region_s region_table[M_MAX_REGION];
            };
            static_assert(sizeof(m_layout_s) == M_LAYOUT_SIZE,
                          "Defined layout size does not match the actual layout size");

            struct m_region_enter_s {
                int record_idx;
                int region_idx;
                geopm_time_s enter_time;
                bool is_short;
            };
            void check_setup(void);
            void check_reset(m_layout_s &layout);
            void append_record(m_layout_s &layout, const record_s &record);
            int m_process;
            std::shared_ptr<SharedMemory> m_shmem;
            std::map<uint64_t, m_region_enter_s> m_hash_region_enter_map;
            geopm_time_s m_time_zero;
            bool m_is_setup;
            uint64_t m_epoch_count;
            uint64_t m_entered_region_hash;
    };
}

#endif
