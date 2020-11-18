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
    class SharedMemoryUser;
    class SharedMemoryScopedLock;

    class ApplicationRecordLog
    {
        public:
            static std::unique_ptr<ApplicationRecordLog> make_unique(std::shared_ptr<SharedMemory> shmem);
            static std::unique_ptr<ApplicationRecordLog> make_unique(std::shared_ptr<SharedMemoryUser> shmem);
            virtual ~ApplicationRecordLog() = default;
            virtual void set_process(int process) = 0;
            virtual void set_time_zero(const geopm_time_s &time) = 0;
            virtual void enter(uint64_t hash, const geopm_time_s &time) = 0;
            virtual void exit(uint64_t hash, const geopm_time_s &time) = 0;
            virtual void epoch(const geopm_time_s &time) = 0;
            virtual void dump(std::vector<record_s> &records,
                              std::vector<short_region_s> &short_regions) = 0;
            static size_t buffer_size(void);
        protected:
            ApplicationRecordLog() = default;
            static constexpr int M_MAX_RECORD = 1024;
            static constexpr int M_MAX_REGION = M_MAX_RECORD + 1;
            struct m_layout_s {
                int num_record;
                record_s record_table[M_MAX_RECORD];
                int num_region;
                short_region_s region_table[M_MAX_REGION];
            };
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
            struct m_region_enter_s {
                int record_idx;
                int region_idx;
                geopm_time_s enter_time;
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
