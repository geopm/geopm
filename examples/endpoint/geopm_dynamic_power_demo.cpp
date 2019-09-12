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

#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <climits>

#include <iostream>
#include <fstream>

#include "geopm_time.h"
#include "geopm_version.h"
#include "geopm_endpoint.h"
#include "geopm_agent.h"
#include "geopm_topo.h"
#include "geopm_pio.h"
#include "geopm_error.h"
#include "Agent.hpp"
#include "Helper.hpp"
#include "PolicyStore.hpp"
#include "geopm_endpoint_demo.hpp"

using geopm::Exception;
// TODO list
// - break out functions
// - need a clear function for Endpoint

static bool g_continue = true;
static void handler(int sig)
{
    g_continue = false;
}

class DynamicPolicyDemo
{
    public:
        DynamicPolicyDemo();
        virtual ~DynamicPolicyDemo();
        void wait_for_controller_attach(void);
        bool is_attached(void);
        void write_next_policy(void);
        void get_sample_or_timeout(void);
    private:
        struct geopm_endpoint_c *m_endpoint;
        char m_agent[GEOPM_ENDPOINT_AGENT_NAME_MAX];
        double m_board_tdp;
        int m_range;   // how much to vary power caps over time
        int m_offset;
        struct geopm_time_s m_last_sample_time;
        std::vector<double> m_policy;
        std::vector<double> m_sample;
        std::ofstream m_log;
};

DynamicPolicyDemo::DynamicPolicyDemo()
    : m_range(30)
{
    int err = geopm_endpoint_create("/geopm_endpoint_demo", &m_endpoint);
    if (!err) {
        throw Exception("geopm_endpoint_create() failed: ",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    err = geopm_endpoint_open(m_endpoint);
    if (err) {
        (void) geopm_endpoint_destroy(m_endpoint);
        throw Exception("geopm_endpoint_open() failed: ",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }

    /// @todo: should be board total, not per package
    int num_pkg = geopm_topo_num_domain(GEOPM_DOMAIN_PACKAGE);
    m_board_tdp = 0.0;
    err = geopm_pio_read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_BOARD, 0, &m_board_tdp);
    if (!err) {
        (void) geopm_endpoint_close(m_endpoint);
        (void) geopm_endpoint_destroy(m_endpoint);
        throw Exception("Failed to read TDP package power",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    m_board_tdp *= num_pkg;
    memset(m_agent, 0, GEOPM_ENDPOINT_AGENT_NAME_MAX);
    geopm_time(&m_last_sample_time);
}

DynamicPolicyDemo::~DynamicPolicyDemo()
{
    (void) geopm_endpoint_close(m_endpoint);
    (void) geopm_endpoint_destroy(m_endpoint);
}

void DynamicPolicyDemo::wait_for_controller_attach(void)
{
    int err = 0;
    while (!err && strlen(m_agent) == 0) {
        err = geopm_endpoint_agent(m_endpoint, GEOPM_ENDPOINT_AGENT_NAME_MAX, m_agent);
    }
    if (err) {
        throw Exception("geopm_endpoint_agent() failed.",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    std::cout << "Controller with agent " << m_agent << " attached." << std::endl;
    std::cout << "Nodes: ";
    int num_nodes = 0;
    err = geopm_endpoint_num_node(m_endpoint, &num_nodes);
    char node_name[NAME_MAX];
    for (int ii = 0; !err && ii < num_nodes; ++ii) {
        err = geopm_endpoint_node_name(m_endpoint, ii, NAME_MAX, node_name);
        std::cout << node_name << " ";
    }
    std::cout << std::endl;
    if (strncmp(m_agent, "power_governor", GEOPM_ENDPOINT_AGENT_NAME_MAX) == 0) {
        std::cout << "power_governor will use dynamic policy." << std::endl;
    }
    else {
        std::cerr << "Warning: demo not supported for agents other than power_governor.  "
                  << "No policy will be applied." << std::endl;
    }
    m_policy.resize(geopm::Agent::num_policy(geopm::agent_factory().dictionary(m_agent)), NAN);
    m_sample.resize(geopm::Agent::num_sample(geopm::agent_factory().dictionary(m_agent)), NAN);

    char prof[GEOPM_ENDPOINT_PROFILE_NAME_MAX];
    err = geopm_endpoint_profile_name(m_endpoint, GEOPM_ENDPOINT_PROFILE_NAME_MAX, prof);

    // todo: unique file names?
    m_log.open("endpoint_demo_" + std::string(prof) + ".log");
}

bool DynamicPolicyDemo::is_attached(void)
{
    char agent_name[GEOPM_ENDPOINT_AGENT_NAME_MAX];
    int err = geopm_endpoint_agent(m_endpoint, GEOPM_ENDPOINT_AGENT_NAME_MAX,
                                   agent_name);
    if (!err) {
        throw Exception("geopm_endpoint_agent() failed.",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    return strnlen(agent_name, GEOPM_ENDPOINT_AGENT_NAME_MAX) > 0;
}

void DynamicPolicyDemo::write_next_policy(void)
{
    if (strncmp(m_agent, "power_governor", GEOPM_ENDPOINT_AGENT_NAME_MAX) == 0) {
        struct geopm_time_s current;
        geopm_time(&current);
        m_policy[0] = m_board_tdp - m_range + m_offset;
        int err = geopm_endpoint_write_policy(m_endpoint, m_policy.size(), m_policy.data());
        if (err) {
            throw Exception("geopm_endpoint_write_policy() failed",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        // todo: not portable
        m_offset = current.t.tv_sec % m_range;
    }
}

void DynamicPolicyDemo::get_sample_or_timeout(void)
{
    int err = 0;
    const double TIMEOUT = 3.0;

    double sample_age = 0.0;
    do {
        err = geopm_endpoint_read_sample(m_endpoint, m_sample.size(), m_sample.data(), &sample_age);
    }
    while (!err && sample_age < TIMEOUT);

    if (err) {
        throw Exception("geopm_endpoint_read_sample() failed.",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }

    if (sample_age >= TIMEOUT) {
        std::cerr << "Timeout waiting for Controller sample." << std::endl;
        memset(m_agent, 0, GEOPM_ENDPOINT_AGENT_NAME_MAX);
        // todo: need a clear function; for now close then open
        err = geopm_endpoint_close(m_endpoint);
        if (err) {
            throw Exception("geopm_endpoint_close() failed.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = geopm_endpoint_open(m_endpoint);
        if (err) {
            throw Exception("geopm_endpoint_open() failed.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }
    else {
        geopm_time_s current_time;
        geopm_time(&current_time);
        // TODO: subtract age from current time
        m_log << current_time << " " << m_sample << std::endl;
    }
}

int main(int argc, char **argv)
{
    struct sigaction act;
    act.sa_handler = handler;
    sigaction(SIGINT, &act, NULL);

    DynamicPolicyDemo demo;

    while (g_continue) {
        try {
            demo.wait_for_controller_attach();
            while (g_continue && demo.is_attached()) {
                demo.write_next_policy();
                demo.get_sample_or_timeout();
            }
            std::cout << "Controller detached." << std::endl;
        }
        catch (geopm::Exception &ex) {
            std::cerr << ex.what() << std::endl;
            g_continue = false;
        }
    }
    return 0;
}
