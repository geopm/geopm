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

#ifndef RECORDFILTER_HPP_INCLUDE
#define RECORDFILTER_HPP_INCLUDE

#include <vector>

#include "ApplicationSampler.hpp"

namespace geopm
{
    /// @brief Base class for filters that can be applied to
    ///        ApplicationSampler record streams produced by a single
    ///        process.
    class RecordFilter
    {
        public:
            /// @brief Default constructor for pure virtual interface.
            RecordFilter() = default;
            /// @brief Default destructor for pure virtual interface.
            virtual ~RecordFilter() = default;
            /// @brief Apply a filter to a stream of records.
            ///
            /// This method is called repeatedly by a user to update a
            /// filtered time stream with a new record.  The input
            /// record is used to update the state of the filter and
            /// the method returns a vector containing any filtered
            /// values resulting from the update.
            ///
            /// @param record The update value to be filtered.
            ///
            /// @return Vector of zero or more records to update the
            ///         filtered stream.
            virtual std::vector<ApplicationSampler::m_record_s> filter(const ApplicationSampler::m_record_s &record) = 0;
    };
}

#endif
