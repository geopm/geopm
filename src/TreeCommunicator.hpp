/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef TREECOMMUNICATOR_HPP_INCLUDE
#define TREECOMMUNICATOR_HPP_INCLUDE

#include <vector>
#include <mpi.h>
#include <pthread.h>

#include "geopm_message.h"

namespace geopm
{
    void check_mpi(int err);

    class TreeCommunicatorRoot;
    class TreeCommunicatorLevel;
    class GlobalPolicy;

    class TreeCommunicator
    {
        public:
            /// TreeCommunicator constructor.
            TreeCommunicator(const std::vector<int> &fan_out, const GlobalPolicy *global_policy, const MPI_Comm &comm);
            /// TreeCommunicator destructor.
            ~TreeCommunicator();
            /// Returns the number of levels of which the calling process is a
            /// member.
            int num_level(void) const;
            /// Returns the level of root (max level for any rank).
            int root_level(void) const;
            /// Returns rank of the calling process in the level.
            int level_rank(int level) const;
            /// Number of ranks that participate in the level.
            int level_size(int level) const;
            /// Send sample to root of the level.  If no recieve has been
            /// posted samples are not sent and no exception is thrown.
            void send_sample(int level, const struct geopm_sample_message_s &sample);
            /// Called only by root process of the level.  Send policy to each
            /// member of the level.  If no recieve has been posted then the
            /// policy is not sent and no exception is thrown.
            void send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy);
            /// Called only by root process of the level.  Returns samples
            /// from each member of the level.  Throws geopm::Exception with
            /// err_value() of GEOPM_ERROR_SAMPLE_INCOMPLETE if message has
            /// not been received by all members of the level since last call.
            void get_sample(int level, std::vector<struct geopm_sample_message_s> &sample);
            /// Record current policy for calling process rank on the level.
            /// Will post another recieve for the next update if the root of
            /// the level has sent an update since last call.  otherwise
            /// returns cached policy.  If no policy has been sent since
            /// startup throws A geopm::Exception with err_value() of
            /// GEOPM_ERROR_POLICY_UNKNOWN.
            void get_policy(int level, struct geopm_policy_message_s &policy);
        protected:
            void mpi_type_create(void);
            void comm_create(const MPI_Comm &comm);
            void level_create(void);
            void level_destroy(void);
            void comm_destroy(void);
            void mpi_type_destroy(void);
            /// Number of levels this rank participates in
            int m_num_level;
            /// Tree fan out from root to leaf. Note levels go from leaf to root
            std::vector<int> m_fan_out;
            /// Vector of communicators for each level (MPI_COMM_NULL for
            /// levels this rank does not participate in).
            std::vector<MPI_Comm> m_comm;
            /// GlobalPolicy object defining the policy
            const GlobalPolicy *m_global_policy;
            /// Intermediate levels
            std::vector<TreeCommunicatorLevel *> m_level;
            /// MPI data type for sample message
            MPI_Datatype m_sample_mpi_type;
            /// MPI data type for policy message
            MPI_Datatype m_policy_mpi_type;
    };

}

#endif
