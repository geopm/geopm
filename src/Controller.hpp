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

#ifndef CONTROLLER_HPP_INCLUDE
#define CONTROLLER_HPP_INCLUDE

#include <pthread.h>

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <set>

namespace geopm
{
    class Comm;
    class PlatformIO;
    class EndpointUser;
    class FilePolicy;
    class ApplicationIO;
    class Reporter;
    class Tracer;
    class TreeComm;
    class Agent;

    class Controller
    {
        public:
            Controller();
            /// @brief Standard contstructor for the Controller.
            ///
            /// @param [in] ppn1_comm The MPI communicator that supports
            ///        the control messages.
            Controller(std::shared_ptr<Comm> ppn1_comm);
            /// @brief Constructor for testing that allows injecting mocked
            ///        versions of internal objects.
            Controller(std::shared_ptr<Comm> comm,
                       PlatformIO &plat_io,
                       const std::string &agent_name,
                       int num_send_up,
                       int num_send_down,
                       std::unique_ptr<TreeComm> tree_comm,
                       std::shared_ptr<ApplicationIO> application_io,
                       std::unique_ptr<Reporter> reporter,
                       std::unique_ptr<Tracer> tracer,
                       std::vector<std::unique_ptr<Agent> > level_agent,
                       std::vector<std::string> policy_names,
                       std::unique_ptr<EndpointUser> endpoint,
                       const std::string &policy_path,
                       const std::string &endpoint_key);
            virtual ~Controller();
            /// @brief Run control algorithm.
            ///
            /// Steps the control algorithm continuously until the
            /// shutdown signal is received.  Since this is a blocking
            /// call that never returns, it is intended that profiling
            /// information is provided through POSIX shared memory.
            void run(void);
            /// @brief Run a single step of the control algorithm.
            ///
            /// One step consists of receiving policy information from
            /// the resource manager, sending them to every other
            /// controller that the node is a parent of, and reading
            /// hardware telemetry.
            void step(void);
            /// @brief Propagate policy information from the resource
            ///        manager at the root of the tree down to the
            ///        controllers on every node.
            ///
            /// At each level of the tree, Agents may modify policies
            /// before sending them to their children.
            void walk_down(void);
            /// @brief Read hardware telemetry and application data
            ///        and send the combined data up the tree to the
            ///        resource manager.
            ///
            /// At each level of the tree, Agents may modify and
            /// aggregate samples before sending them up up to their
            /// parents.
            void walk_up(void);
            /// @brief Write the report file and finalize the trace.
            void generate(void);
            /// @brief Run control algorithm as a separate thread.
            ///
            /// Creates a POSIX thread running the control algorithm
            /// continuously until the shutdown signal is received.
            /// This is intended to be called by the application under
            /// control.  With this method of launch the supporting
            /// MPI implementation must be enabled for
            /// MPI_THREAD_MULTIPLE using MPI_Init_thread().
            ///
            /// @param [in] attr The POSIX thread attributes applied
            ///        when thread is created.
            ///
            /// @param [out] thread Pointer to the POSIX thread that
            ///        is created.
            void pthread(const pthread_attr_t *attr, pthread_t *thread);
            /// @brief Configure the trace with custom columns from
            ///        the Agent.
            void setup_trace(void);
            /// @brief Called upon failure to facilitate graceful destruction
            ///        of the Controller and notify application.
            void abort(void);
            /// @brief Return the names of hosts active in the current
            ///        job.  Must be called by all controllers in the
            ///        tree or else a hang will occur.
            /// @param [in] hostname Hostname of the calling Controller.
            /// @return Returns set of hostnames of all connected
            ///         controllers if the calling controller is at
            ///         the root of the tree, otherwise returns an
            ///         empty set.
            std::set<std::string> get_hostnames(const std::string &hostname);
        private:
            /// @brief Construct Agents for every level.  Agents can
            ///        register new IOGroups, signals, and controls
            ///        but not push them.
            void create_agents(void);
            /// @brief Call init() on every agent.  Agents can push
            ///        signals and controls.
            void init_agents(void);

            std::shared_ptr<Comm> m_comm;
            PlatformIO &m_platform_io;
            std::string m_agent_name;
            const int m_num_send_down;
            const int m_num_send_up;
            std::unique_ptr<TreeComm> m_tree_comm;
            const int m_num_level_ctl;
            const int m_max_level;
            const int m_root_level;
            std::shared_ptr<ApplicationIO> m_application_io;
            std::unique_ptr<Reporter> m_reporter;
            std::unique_ptr<Tracer> m_tracer;
            std::vector<std::unique_ptr<Agent> > m_agent;
            const bool m_is_root;
            std::vector<double> m_in_policy;
            std::vector<std::vector<std::vector<double> > > m_out_policy;
            std::vector<std::vector<std::vector<double> > > m_in_sample;
            std::vector<double> m_out_sample;
            std::vector<double> m_trace_sample;

            std::unique_ptr<EndpointUser> m_endpoint;
            std::unique_ptr<FilePolicy> m_file_policy;
            std::string m_policy_path;
            bool m_is_dynamic_policy;

            std::vector<std::string> m_agent_policy_names;
            std::vector<std::string> m_agent_sample_names;
    };
}
#endif
