/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef KONTROLLER_HPP_INCLUDE
#define KONTROLLER_HPP_INCLUDE

#include <pthread.h>

#include <string>
#include <memory>
#include <vector>
#include <map>

namespace geopm
{
    class IComm;
    class IPlatformTopo;
    class IPlatformIO;
    class IKontrollerIO;
    class IManagerIO;
    class IManagerIOSampler;
    class IApplicationIO;
    class IReporter;
    class ITracer;
    class ITreeComm;
    class IAgent;

    class Kontroller
    {
        public:
            /// @brief Standard contstructor for the Kontroller.
            Kontroller(std::shared_ptr<IComm> ppn1_comm,
                       const std::string &global_policy_path);
            /// @brief Constructor for testing
            Kontroller(std::shared_ptr<IComm> comm,
                       IPlatformTopo &plat_topo,
                       IPlatformIO &plat_io,
                       const std::string &agent_name,
                       int num_send_up,
                       int num_send_down,
                       std::unique_ptr<ITreeComm> tree_comm,
                       std::shared_ptr<IApplicationIO> application_io,
                       std::unique_ptr<IReporter> reporter,
                       std::unique_ptr<ITracer> tracer,
                       std::vector<std::unique_ptr<IAgent> > level_agent,
                       std::unique_ptr<IManagerIOSampler> manager_io_sampler);
            virtual ~Kontroller();
            void run(void);
            void step(void);
            void walk_down(void);
            void walk_up(void);
            void generate(void);
            void pthread(const pthread_attr_t *attr, pthread_t *thread);
            void setup_trace(void);
        private:
            std::shared_ptr<IComm> m_comm;
            IPlatformTopo &m_platform_topo;
            IPlatformIO &m_platform_io;
            std::string m_agent_name;
            const int m_num_send_down;
            const int m_num_send_up;
            std::unique_ptr<ITreeComm> m_tree_comm;
            const int m_num_level_ctl;
            const int m_max_level;
            const int m_root_level;
            std::shared_ptr<IApplicationIO> m_application_io;
            std::unique_ptr<IReporter> m_reporter;
            std::unique_ptr<ITracer> m_tracer;
            std::vector<std::unique_ptr<IAgent> > m_agent;
            const bool m_is_root;
            std::vector<double> m_in_policy;
            std::vector<std::vector<std::vector<double> > > m_out_policy;
            std::vector<std::vector<std::vector<double> > > m_in_sample;
            std::vector<double> m_out_sample;

            std::unique_ptr<IManagerIOSampler> m_manager_io_sampler;

            std::vector<int> m_tracer_sample_idx;
            std::vector<double> m_tracer_sample;

            std::vector<std::string> m_agent_policy_names;
            std::vector<std::string> m_agent_sample_names;
    };
}
#endif
