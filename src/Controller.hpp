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

#ifndef CONTROLLER_HPP_INCLUDE
#define CONTROLLER_HPP_INCLUDE

#include <vector>
#include <string>
#include <mpi.h>

#include "TreeCommunicator.hpp"
#include "Platform.hpp"
#include "Decider.hpp"
#include "Phase.hpp"
#include "GlobalPolicy.hpp"
#include "Profile.hpp"


namespace geopm
{
    class Controller
    {
        public:
            Controller(const GlobalPolicy *global_policy, struct geopm_sample_shmem_s *shm, MPI_Comm comm);
            virtual ~Controller();
            void run(void);
            void step(void);
            void pthread(const pthread_attr_t *attr, pthread_t *thread);
            void spawn(void);
            int num_level(void);
            const
            void leaf_decider(const LeafDecider *leaf_decider);
            void tree_decider(int level, const TreeDecider *tree_decider);
            void process_samples(const int level, const std::vector<struct geopm_sample_message_s> &sample);
            void enforce_child_policy(const int phase_id, const int level, const Policy &policy);
        protected:
            int walk_down(void);
            int walk_up(void);
            int m_max_fanout;
            std::vector<int> m_fan_out;
            const GlobalPolicy *m_global_policy;
            struct geopm_sample_shmem_s *m_shm;
            TreeCommunicator *m_tree_comm;
            std::vector <TreeDecider *> m_tree_decider;
            LeafDecider *m_leaf_decider;
            // Per-level platforms
            std::vector <Platform *> m_platform;
            // Per level vector of maps from phase identifier to phase object
            std::vector <std::map <long, Phase *> > m_phase;
    };

}

#endif
