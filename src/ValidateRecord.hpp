/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef VALIDATERECORD_HPP_INCLUDE
#define VALIDATERECORD_HPP_INCLUDE

#include "ApplicationSampler.hpp"
#include "geopm_time.h"

namespace geopm
{
    /// @brief Checks validity and self consistency of a record stream
    ///        from a single process.  This check is applied by the
    ///        ApplicationSampler when updates are provided and after
    ///        the filter is applied.
    class ValidateRecord
    {
        public:
            ValidateRecord(void);
            /// @brief Default destructor.
            virtual ~ValidateRecord() = default;
            /// @brief Check that the record is valid and self
            ///        consistent with previously checked records.
            /// @param record [in] Application sampler record to be
            ///        validated.
            void check(const record_s &record);
        private:
            bool m_is_empty;
            geopm_time_s m_time;
            int m_process;
            uint64_t m_epoch_count;
            uint64_t m_region_hash;
    };
}

#endif
