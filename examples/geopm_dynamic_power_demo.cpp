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

#include <iostream>
#include <fstream>

#include "geopm_time.h"
#include "geopm_version.h"
#include "geopm_endpoint.h"
#include "geopm_agent.h"
#include "geopm_topo.h"
#include "geopm_pio.h"
#include "geopm_error.h"
#include "Endpoint.hpp"
#include "Agent.hpp"
#include "Helper.hpp"
#include "PolicyStore.hpp"


// TODO list
// - Change to endpoint C api
// - break out functions
// - need a clear function for Endpoint

/// Helpers for printing
template <typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> vec)
{
    os << "{";
    for (int ii = 0; ii < vec.size(); ++ii) {
        os << vec[ii];
        if (ii < vec.size() - 1) {
            os << ", ";
        }
    }
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, geopm_time_s time)
{
    os << (double)(time.t.tv_sec + time.t.tv_nsec * 1.0E-9);
    return os;
}

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
        //std::shared_ptr<geopm::ShmemEndpoint> m_endpoint;
        struct geopm_endpoint_c *m_endpoint;
        //std::string m_agent;
        char m_agent[GEOPM_ENDPOINT_AGENT_NAME_MAX];
        double m_board_tdp;
        double m_range;   // how much to vary power caps over time
        int m_offset;
        struct geopm_time_s m_last_sample_time;
        std::vector<double> m_sample;
        std::ofstream m_log;
};

DynamicPolicyDemo::DynamicPolicyDemo()
    : m_range(30)
{
    //: m_endpoint("/geopm_endpoint_demo")
    //m_endpoint->open();
    int err = geopm_endpoint_create("/geopm_endpoint_demo", &m_endpoint);
    if (!err) {

    }
    err = geopm_endpoint_open(m_endpoint);

    /// @todo: should be board total, not per package
    int num_pkg = geopm_topo_num_domain(GEOPM_DOMAIN_PACKAGE);
    m_board_tdp = 0.0;
    int err = geopm_pio_read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_BOARD, 0, &result);
    if (!err) {
        std::cerr << "Error: failed to read TDP package power" << std::endl;
        exit(GEOPM_ERROR_RUNTIME);
    }
    board_tdp *= num_pkg;

    geopm_time(&m_last_sample_time);
}

DynamicPolicyDemo::~DynamicPolicyDemo()
{
    m_endpoint->close();
}

void DynamicPolicyDemo::wait_for_controller_attach(void)
{
    while (strlen(m_agent) == 0) {
        //m_agent = m_endpoint->get_agent();
    }
    std::cout << "Controller with agent " << m_agent << " attached." << std::endl;
    if (strncmp(m_agent, "power_governor", GEOPM_ENDPOINT_AGENT_NAME_MAX) == 0) {
        std::cout << "power_governor will use dynamic policy." << std::endl;
    }
    else {
        std::cerr << "Warning: demo not supported for agents other than power_governor.  "
                  << "No policy will be applied." << std::endl;
    }
    m_policy.resize(geopm::Agent::num_policy(geopm::agent_factory().dictionary(m_agent)));
    m_sample.resize(geopm::Agent::num_sample(geopm::agent_factory().dictionary(m_agent)));

    std::string prof = m_endpoint.get_profile_name();
    // todo: unique file names?
    m_log.open("endpoint_demo_" + prof + ".log");
}

bool DynamicPolicyDemo::is_attached(void)
{
    //m_agent = m_endpoint->get_agent();
    //return m_agent != "";
    char agent_name[GEOPM_ENDPOINT_AGENT_NAME_MAX];
    int err = geopm_endpoint_agent(m_endpoint, GEOPM_ENDPOINT_AGENT_NAME_MAX,
                                   agent_name);
    if (!err) {

    }
    return strnlen(agent_name, GEOPM_ENDPOINT_AGENT_NAME_MAX) > 0;
}

void DynamicPolicyDemo::write_next_policy(void)
{
    if (m_agent == "power_governor") {
        struct geopm_time_s current;
        geopm_time(&current);
        //m_endpoint->write_policy({m_board_tdp - m_range + m_offset});
        if (m_policy.size() < 1) {
            // error
        }
        m_policy[0] = m_board_tdp - m_range + m_offset;
        geopm_endpoint_write_policy(m_endpoint, m_policy.size(), m_policy.data());
        // todo: not portable
        m_offset = current.t.tv_sec % range;
    }
}

void DynamicPolicyDemo::get_sample_or_timeout(void)
{
    const geopm_time_s ZERO {{0, 0}};
    const double TIMEOUT = 3.0;

    geopm_time_s sample_time;
    geopm_time_s current_time;
    do {
        geopm_time(&current_time);
        geopm_endpoint_read_sample(
        sample_time = endpoint->read_sample(m_sample);
    }
    while (geopm_time_diff(&sample_time, &zero) == 0.0 ||
           (geopm_time_diff(&sample_time, &m_last_sample_time) == 0.0 &&
            geopm_time_diff(&m_last_sample_time, &current_time) < TIMEOUT));

    if (geopm_time_diff(&m_last_sample_time, &current_time) >= TIMEOUT) {
        std::cerr << "Timeout waiting for Controller sample." << std::endl;
        m_agent = "";
        // todo: need a clear function
        m_endpoint->close();
        m_endpoint->open();
    }
    else {
        m_last_sample_time = sample_time;
        m_log << sample_time << " " << m_sample << std::endl;
    }
}

int main(int argc, char **argv)
{
    struct sigaction act;
    act.sa_handler = handler;
    sigaction(SIGINT, &act, NULL);

    DynamicPolicyDemo demo;

    while (g_continue) {
        demo.wait_for_controller_attach();
        while (g_continue && demo.is_attached()) {
            demo.write_dynamic_power_policy();
            demo.get_sample_or_timeout();
        }
        std::cout << "Controller detached." << std::endl;
    }
    return 0;
}
