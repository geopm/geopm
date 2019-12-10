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
#include <thread>
#include <chrono>

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
//#include "Endpoint.hpp"

using geopm::Exception;


static bool g_continue = true;
static void handler(int sig)
{
    g_continue = false;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> vec)
{
    for (int ii = 0; ii < vec.size(); ++ii) {
        os << vec[ii];
        if (ii < vec.size() - 1) {
            os << "|";
        }
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, struct geopm_time_s time)
{
    os << (double)(time.t.tv_sec + time.t.tv_nsec * 1.0E-9);
    return os;
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
        std::string m_endpoint_name;
        std::string m_sample_log_path;
        struct geopm_endpoint_c *m_endpoint;
        const double M_TIMEOUT;  // timeout used for both attach and sample
        char m_agent[GEOPM_ENDPOINT_AGENT_NAME_MAX];
        double m_board_tdp;
        int m_range;   // how much to vary power caps in W over time
        int m_offset;
        struct geopm_time_s m_last_sample_time;
        struct geopm_time_s m_start_time;
        std::vector<double> m_policy;
        std::vector<double> m_sample;
        std::ofstream m_log;
};

DynamicPolicyDemo::DynamicPolicyDemo()
    : m_endpoint_name("/geopm_test_dynamic_policy")
    , m_sample_log_path("test_dynamic_policy_sample.log")
    , M_TIMEOUT(10.0)
    , m_range(30)
    , m_offset(0)
{
    int err = geopm_endpoint_create(m_endpoint_name.c_str(), &m_endpoint);
    if (err) {
        throw Exception("geopm_endpoint_create() failed",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    err = geopm_endpoint_open(m_endpoint);
    if (err) {
        (void) geopm_endpoint_destroy(m_endpoint);
        throw Exception("geopm_endpoint_open() failed",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    m_board_tdp = 0.0;
    err = geopm_pio_read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_BOARD, 0, &m_board_tdp);
    if (err) {
        char msg[2048];
        geopm_error_message(err, msg, 2048);
        std::cerr << msg << std::endl;
        (void) geopm_endpoint_close(m_endpoint);
        (void) geopm_endpoint_destroy(m_endpoint);
        throw Exception("Failed to read TDP package power",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    memset(m_agent, 0, GEOPM_ENDPOINT_AGENT_NAME_MAX);
    geopm_time(&m_start_time);
    geopm_time(&m_last_sample_time);
}

DynamicPolicyDemo::~DynamicPolicyDemo()
{
    (void) geopm_endpoint_close(m_endpoint);
    (void) geopm_endpoint_destroy(m_endpoint);
}

void DynamicPolicyDemo::wait_for_controller_attach(void)
{
    int err = geopm_endpoint_wait_for_agent_attach(m_endpoint, M_TIMEOUT);
    if (err) {
        throw Exception("geopm_endpoint_wait_for_agent_attach() failed.",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    err = geopm_endpoint_agent(m_endpoint, GEOPM_ENDPOINT_AGENT_NAME_MAX, m_agent);
    if (err) {
        throw Exception("geopm_endpoint_agent() failed.",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    if (strlen(m_agent) == 0) {
        throw Exception("No agent attached; probably timed out.",
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
    m_policy.resize(geopm::Agent::num_policy(m_agent), NAN);
    m_sample.resize(geopm::Agent::num_sample(m_agent), NAN);

    char prof[GEOPM_ENDPOINT_PROFILE_NAME_MAX];
    err = geopm_endpoint_profile_name(m_endpoint, GEOPM_ENDPOINT_PROFILE_NAME_MAX, prof);

    m_log.open(m_sample_log_path);
    m_log << "TIME|" <<  geopm::Agent::sample_names(m_agent) << std::endl;
}

bool DynamicPolicyDemo::is_attached(void)
{
    char agent_name[GEOPM_ENDPOINT_AGENT_NAME_MAX];
    int err = geopm_endpoint_agent(m_endpoint, GEOPM_ENDPOINT_AGENT_NAME_MAX,
                                   agent_name);
    if (err) {
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
        // todo: not portable; add to initialization
        m_offset = current.t.tv_sec % m_range;
    }
}

void DynamicPolicyDemo::get_sample_or_timeout(void)
{
    double sample_age = 0.0;
    int err = geopm_endpoint_read_sample(m_endpoint, m_sample.size(), m_sample.data(), &sample_age);
    if (err) {
        throw Exception("geopm_endpoint_read_sample() failed.",
                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }

    if (sample_age >= M_TIMEOUT) {
        std::cerr << "Timeout waiting for Controller sample. age=" << sample_age << std::endl;
        memset(m_agent, 0, GEOPM_ENDPOINT_AGENT_NAME_MAX);
        g_continue = false;
        // todo: need a clear function; for now close then open
        err = geopm_endpoint_close(m_endpoint);
        if (err) {
            throw Exception("geopm_endpoint_close() failed.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }
    else if (sample_age == -1) {
        // TODO: special case for agent attached but no sample yet
        // just skip
        std::cout << "no sample ready yet" << std::endl;
    }
    else {
        // TODO: maybe use CSV class? not public interface
        m_log << geopm_time_since(&m_start_time) << "|" << m_sample << std::endl;
    }
}

int main(int argc, char **argv)
{
    struct sigaction act;
    act.sa_handler = handler;
    sigaction(SIGINT, &act, NULL);

    DynamicPolicyDemo demo;
    try {
        demo.wait_for_controller_attach();
        while (g_continue && demo.is_attached()) {
            demo.write_next_policy();
            demo.get_sample_or_timeout();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        std::cout << "Controller detached." << std::endl;
    }
    catch (geopm::Exception &ex) {
        std::cerr << ex.what() << std::endl;
        g_continue = false;
    }
    return 0;
}
