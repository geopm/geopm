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

#ifndef PROCESSEPOCH_HPP_INCLUDE
#define PROCESSEPOCH_HPP_INCLUDE

#include <cstdint>
#include <memory>

#include "ApplicationSampler.hpp"


namespace geopm
{
    /// @brief Class that analyzes application records sent from a
    ///        single process to determine epoch related signals for
    ///        the EpochIOGroup.  These signals are:
    ///            {"EPOCH_RUNTIME",
    ///             "EPOCH_COUNT",
    ///             "EPOCH_RUNTIME_NETWORK",
    ///             "EPOCH_RUNTIME_IGNORE"}.
    class ProcessEpoch
    {
        public:
            /// @brief Make a ProcessEpoch object that will do an
            ///        analysis of the application sampler record
            ///        updates.
            ///
            ///  Factory method to create a unique pointer to a
            ///  ProcessEpoch object that uses the M_EVENT_EPOCH_COUNT
            ///  and M_EVENT_HINT events to determine the epoch
            ///  related signals.
            ///
            ///  @return Unique pointer to a ProcessEpoch base class
            ///          object that processes epoch events.
            static std::unique_ptr<ProcessEpoch> make_unique(void);
            /// @brief Base class constructor method for pure virtual
            ///        interface.
            ProcessEpoch() = default;
            /// @brief Base class destructor method for pure virtual
            ///        interface.
            virtual ~ProcessEpoch() = default;
            /// @brief Process application sampler record to update
            ///        signal data.
            ///
            /// @param record A reference to a record that was queued
            ///        by the process tracked by this object.  It is
            ///        the callers responsiblity to filter out records
            ///        that are sent from other processes.
            virtual void update(const ApplicationSampler::m_record_s &record) = 0;
            /// @brief The number of epoch events that have occured
            ///        for the process that is tracked by this object.
            ///
            /// @return Zero based counter of the number of epoch
            ///         events.
            virtual int epoch_count(void) const = 0;
            /// @brief The total runtime that elapsed between the last
            ///        two epoch events for the tracked process.
            ///
            /// @return Elapsed time in seconds.
            virtual double last_epoch_runtime(void) const = 0;
            /// @brief The portion of the runtime that elapsed between
            ///        the last two epochs while application indicated
            ///        the network hint.
            ///
            /// @return Elapsed time in seconds.
            virtual double last_epoch_runtime_network(void) const = 0;
            /// @brief The portion of the runtime that elapsed between
            ///        the last two epochs while application indicated
            ///        the ignore hint.
            ///
            /// @return Elapsed time in seconds.
            virtual double last_epoch_runtime_ignore(void) const = 0;
    };
}

#endif
