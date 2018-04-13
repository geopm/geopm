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

#include <iostream>

#include "geopm_env.h"
#include "geopm_signal_handler.h"
#include "geopm_message.h"
#include "geopm_time.h"
#include "Kontroller.hpp"
#include "ApplicationIO.hpp"
#include "Reporter.hpp"
#include "Tracer.hpp"
#include "Exception.hpp"
#include "Comm.hpp"
#include "PlatformTopo.hpp"
#include "PlatformIO.hpp"
#include "Agent.hpp"
#include "TreeComm.hpp"
#include "ManagerIO.hpp"
#include "config.h"

extern "C"
{
    static void *geopm_threaded_run(void *args)
    {
        long err = 0;
        geopm::Kontroller *ctl = (geopm::Kontroller *)args;
        try {
            ctl->run();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return (void *)err;
    }
}

namespace geopm
{
    Kontroller::Kontroller(std::shared_ptr<IComm> ppn1_comm,
                           const std::string &global_policy_path)
        : Kontroller(ppn1_comm,
                     platform_topo(),
                     platform_io(),
                     geopm_env_agent(),
                     IAgent::num_policy(agent_factory().dictionary(geopm_env_agent())),
                     IAgent::num_sample(agent_factory().dictionary(geopm_env_agent())),
                     std::unique_ptr<ITreeComm>(new TreeComm(ppn1_comm,
                         IAgent::num_policy(agent_factory().dictionary(geopm_env_agent())),
                         IAgent::num_sample(agent_factory().dictionary(geopm_env_agent())))),
                     std::shared_ptr<IApplicationIO>(new ApplicationIO(geopm_env_shmkey())),
                     std::unique_ptr<IReporter>(new Reporter(geopm_env_report(), platform_io())),
                     std::unique_ptr<ITracer>(new Tracer(geopm_env_trace())),
                     std::vector<std::unique_ptr<IAgent> >{},
                     std::unique_ptr<IManagerIOSampler>(new ManagerIOSampler(global_policy_path, true)))
    {

    }

    Kontroller::Kontroller(std::shared_ptr<IComm> comm,
                           IPlatformTopo &plat_topo,
                           IPlatformIO &plat_io,
                           const std::string &agent_name,
                           int num_send_down,
                           int num_send_up,
                           std::unique_ptr<ITreeComm> tree_comm,
                           std::shared_ptr<IApplicationIO> application_io,
                           std::unique_ptr<IReporter> reporter,
                           std::unique_ptr<ITracer> tracer,
                           std::vector<std::unique_ptr<IAgent> > level_agent,
                           std::unique_ptr<IManagerIOSampler> manager_io_sampler)
        : m_comm(comm)
        , m_platform_topo(plat_topo)
        , m_platform_io(plat_io)
        , m_agent_name(agent_name)
        , m_num_send_down(num_send_down)
        , m_num_send_up(num_send_up)
        , m_tree_comm(std::move(tree_comm))
        , m_num_level_ctl(m_tree_comm->num_level_controlled())
        , m_max_level(m_num_level_ctl + 1)
        , m_root_level(m_tree_comm->root_level())
        , m_application_io(std::move(application_io))
        , m_reporter(std::move(reporter))
        , m_tracer(std::move(tracer))
        , m_agent(std::move(level_agent))
        , m_is_root(m_num_level_ctl == m_root_level)
        , m_in_policy(m_num_send_down)
        , m_out_policy(m_num_level_ctl)
        , m_in_sample(m_num_level_ctl)
        , m_out_sample(m_num_send_up)
        , m_manager_io_sampler(std::move(manager_io_sampler))
    {
        // Three dimensional vector over levels, children, and message
        // index.  These are used as temporary storage when passing
        // messages up and down the tree.
        for (int level = 0; level != m_num_level_ctl; ++level) {
            int num_children = m_tree_comm->level_size(level);
            m_out_policy[level] = std::vector<std::vector<double> >(num_children,
                                                                    std::vector<double>(m_num_send_down));
            m_in_sample[level] = std::vector<std::vector<double> >(num_children,
                                                                   std::vector<double>(m_num_send_up));
        }

        if (m_agent.size() == 0) {
            m_agent.push_back(agent_factory().make_plugin(m_agent_name));
            m_agent.back()->init(0);
            for (int level = 1; level < m_max_level; ++level) {
                m_agent.push_back(agent_factory().make_plugin(m_agent_name));
                m_agent.back()->init(level);
            }
        }

        /// @todo move somewhere else: need to happen after Agents are constructed
        // sanity checks
        if (m_agent.size() == 0) {
            throw Exception("Kontroller requires at least one Agent",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_max_level != (int)m_agent.size()) {
            throw Exception("Kontroller number of agents is incorrect",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    Kontroller::~Kontroller()
    {
        geopm_signal_handler_check();
        geopm_signal_handler_revert();
    }

    void Kontroller::run(void)
    {
        setup_trace();
        while (!m_application_io->do_shutdown()) {
            step();
        }
        generate();
    }

    void Kontroller::generate(void)
    {
        std::string agent_report_header;
        if (m_is_root) {
            agent_report_header = m_agent[m_root_level]->report_header();
        }
        /// @todo why get node reports from each level of the tree?
        std::ostringstream agent_node_report;
        for (int level = 0; level < m_max_level; ++level) {
            agent_node_report << m_agent[level]->report_node();
        }

        m_reporter->generate(m_agent_name,
                             agent_report_header,
                             agent_node_report.str(),
                             m_agent[0]->report_region(),
                             *m_application_io,
                             nullptr,  /// @todo get the IComm from somewhere
                             *m_tree_comm);
        m_tracer->flush();
    }

    void Kontroller::step(void)
    {
        walk_down();
        geopm_signal_handler_check();
        m_application_io->update(m_comm);

        walk_up();
        geopm_signal_handler_check();
        m_agent[0]->wait();
        geopm_signal_handler_check();
    }

    void Kontroller::walk_down(void)
    {
        if (m_is_root) {
            m_in_policy = m_manager_io_sampler->sample();
        }
        else {
            m_tree_comm->receive_down(m_num_level_ctl, m_in_policy);
        }
        for (int level = m_num_level_ctl - 1; level != -1; --level) {
            m_agent[level]->descend(m_in_policy, m_out_policy[level]);
            m_tree_comm->send_down(level, m_out_policy[level]);
            m_tree_comm->receive_down(level, m_in_policy);
        }
        m_agent[0]->adjust_platform(m_in_policy);
        m_platform_io.write_batch();
    }

    void Kontroller::walk_up(void)
    {
        m_platform_io.read_batch();
        m_tracer->update(true); // todo: what to do with is_epoch

        m_agent[0]->sample_platform(m_out_sample);
        for (int level = 0; level != m_num_level_ctl; ++level) {
            m_tree_comm->send_up(level, m_out_sample);
            m_tree_comm->receive_up(level, m_in_sample[level]);
            m_agent[level]->ascend(m_in_sample[level], m_out_sample);
        }
        if (!m_is_root) {
            m_tree_comm->send_up(m_num_level_ctl, m_out_sample);
        }
        else {
            /// @todo At the root of the tree, send signals up to the
            /// resource manager.
        }
    }

    void Kontroller::pthread(const pthread_attr_t *attr, pthread_t *thread)
    {
        int err = pthread_create(thread, attr, geopm_threaded_run, (void *)this);
        if (err) {
            throw Exception("Controller::pthread(): pthread_create() failed",
                            err, __FILE__, __LINE__);
        }
    }

    void Kontroller::setup_trace(void)
    {
        std::vector<IPlatformIO::m_request_s> columns({{"TIME", PlatformTopo::M_DOMAIN_BOARD, 0},
                                                       {"REGION_ID#", PlatformTopo::M_DOMAIN_BOARD, 0},
                                                       {"ENERGY_PACKAGE", PlatformTopo::M_DOMAIN_BOARD, 0},
                                                       {"POWER_PACKAGE", PlatformTopo::M_DOMAIN_BOARD, 0},
                                                       {"FREQUENCY", PlatformTopo::M_DOMAIN_BOARD, 0}});
        std::vector<IPlatformIO::m_request_s> agent_columns = m_agent[0]->trace_columns();
        columns.insert(columns.end(), agent_columns.begin(), agent_columns.end());
        m_tracer->columns(columns);
    }
}
