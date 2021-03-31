/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include "config.h"

#include "Daemon.hpp"
#include "DaemonImp.hpp"
#include "geopm_daemon.h"

#include "geopm_time.h"
#include "Exception.hpp"
#include "Endpoint.hpp"
#include "PolicyStore.hpp"
#include "Helper.hpp"

namespace geopm
{
    std::unique_ptr<Daemon> Daemon::make_unique(const std::string &endpoint_name,
                                                const std::string &db_path)
    {
        return geopm::make_unique<DaemonImp>(endpoint_name, db_path);
    }

    DaemonImp::DaemonImp(const std::string &endpoint_name,
                         const std::string &db_path)
        : DaemonImp(Endpoint::make_unique(endpoint_name),
                    PolicyStore::make_unique(db_path))
    {

    }

    DaemonImp::DaemonImp(std::shared_ptr<Endpoint> endpoint,
                         std::shared_ptr<const PolicyStore> policystore)
        : m_endpoint(endpoint)
        , m_policystore(policystore)
    {
        m_endpoint->open();
    }

    DaemonImp::~DaemonImp()
    {
        m_endpoint->close();
    }

    void DaemonImp::update_endpoint_from_policystore(double timeout)
    {
        m_endpoint->wait_for_agent_attach(timeout);
        auto agent = m_endpoint->get_agent();
        if (agent != "") {
            std::string profile_name = m_endpoint->get_profile_name();
            auto policy = m_policystore->get_best(agent, profile_name);
            m_endpoint->write_policy(policy);
        }
    }

    void DaemonImp::stop_wait_loop(void)
    {
        m_endpoint->stop_wait_loop();
    }

    void DaemonImp::reset_wait_loop(void)
    {
        m_endpoint->reset_wait_loop();
    }
}

int geopm_daemon_create(const char *endpoint_name,
                        const char *policystore_path,
                        struct geopm_daemon_c **daemon)
{
    int err = 0;
    try {
        *daemon = (struct geopm_daemon_c*)(new geopm::DaemonImp(endpoint_name,
                                                                policystore_path));
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_daemon_destroy(struct geopm_daemon_c *daemon)
{
    int err = 0;
    try {
        delete (geopm::DaemonImp*)daemon;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_daemon_update_endpoint_from_policystore(struct geopm_daemon_c *daemon,
                                                  double timeout)
{
    int err = 0;
    geopm::DaemonImp *dae = (geopm::DaemonImp*)daemon;
    try {
        dae->update_endpoint_from_policystore(timeout);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_daemon_stop_wait_loop(struct geopm_daemon_c *daemon)
{
    int err = 0;
    geopm::DaemonImp *dae = (geopm::DaemonImp*)daemon;
    try {
        dae->stop_wait_loop();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}

int geopm_daemon_reset_wait_loop(struct geopm_daemon_c *daemon)
{
    int err = 0;
    geopm::DaemonImp *dae = (geopm::DaemonImp*)daemon;
    try {
        dae->reset_wait_loop();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}
