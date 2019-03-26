/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef CONTROLMESSAGE_HPP_INCLUDE
#define CONTROLMESSAGE_HPP_INCLUDE

#include <stdint.h>

enum geopm_ctl_message_e {
    GEOPM_MAX_NUM_CPU = 768
};

/// @brief Structure intended to be shared between
/// the GEOPM runtime and the application in
/// order to convey status and control information.
struct geopm_ctl_message_s {
    /// @brief Status of the GEOPM runtime.
    volatile uint32_t ctl_status;
    /// @brief Status of the application.
    volatile uint32_t app_status;
    /// @brief Holds affinities of all application ranks
    /// on the local compute node.
    int cpu_rank[GEOPM_MAX_NUM_CPU];
};

namespace geopm
{
    /// @brief Class for non-runtime messaging
    ///
    /// Interface for application and controller to communicate during
    /// the start up and shut down phases.
    class ControlMessage
    {
        public:
            ControlMessage() = default;
            virtual ~ControlMessage() = default;
            /// @brief Signal an advance to next phase in runtime.
            virtual void step(void) = 0;
            /// @brief Wait for message that other side has advanced
            /// to next phase in runtime.
            virtual void wait(void) = 0;
            virtual void abort(void) = 0;
            /// @brief Set the rank running on a logical CPU.
            ///
            /// @param [in] cpu_idx Linux logical CPU index.
            ///
            /// @param [in] rank MPI rank running on the given CPU.
            virtual void cpu_rank(int cpu_idx, int rank) = 0;
            /// @brief Get the rank running on a logical CPU.
            ///
            /// @param [in] cpu_idx Linux logical CPU index.
            ///
            /// @return Returns the MPI rank running on the given CPU.
            virtual int cpu_rank(int cpu_idx) const = 0;
            /// @brief Used by Controller to query if application has
            /// begun sampling.
            ///
            /// @return Returns true if application has begun putting
            /// samples in the table and false otherwise.
            virtual bool is_sample_begin(void) const = 0;
            /// @brief Used by Controller to query if application has
            /// stopped sampling.
            ///
            /// @return Returns true if application has stopped
            /// putting samples in the table and false otherwise.
            virtual bool is_sample_end(void) const = 0;
            /// @brief Used by Controller to query if application has
            /// begun sending region names.
            ///
            /// @return Returns true if application has begun sending
            /// region names across the table and false otherwise.
            virtual bool is_name_begin(void) const = 0;
            /// @brief Used by Controller to query if application is
            /// ready to shutdown.
            ///
            /// @return Returns true if application is ready to
            /// shutdown and false otherwise.
            virtual bool is_shutdown(void) const = 0;
            /// @brief Used to synchronize passing region names across
            /// the table.
            ///
            /// The application and controller call this interface to
            /// synchronize each trip through the buffering loop which
            /// is used to pass region names from the application to
            /// the controller at the end of an application run.
            virtual void loop_begin(void) = 0;
    };
}
#endif
