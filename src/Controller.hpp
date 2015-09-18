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
#include "Configuration.hpp"

namespace geopm
{
    class Controller
    {
        public:
            Controller(std::string control, std::string report, MPI_Comm comm);
            virtual ~Controller();
            void run();
            void phase_register(std::string phase_name, long phase_id, int hint);
            void phase_enter(long phase_id);
            void phase_exit(void);
            void phase_progress(double fraction);
        protected:
            int walk_down(void);
            int walk_up(void);
            std::vector<int> m_factor;
            Configuration *m_config;
            std::string m_report;
            TreeCommunicator m_comm;
            TreeDecider *m_tree_decider;
            LeafDecider *m_leaf_decider;
            // Per-level platforms
            std::vector <Platform *> m_platform;
            // Per level vector of maps from phase identifier to phase object
            std::vector <std::map <long, Phase *> > m_phase;
            std::vector <struct geopm_policy_message_s> m_split_policy;
            std::vector <struct sample_message_s> m_child_sample;
            std::vector <struct geopm_policy_message_s> m_last_policy;
    };

}
#endif
