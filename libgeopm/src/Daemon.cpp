/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "geopm/Daemon.hpp"
#include "DaemonImp.hpp"
#include "geopm_daemon.h"

#include "geopm_time.h"
#include "geopm/Exception.hpp"
#include "geopm/Endpoint.hpp"
#include "PolicyStore.hpp"
#include "geopm/Helper.hpp"

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
        : m_endpoint(std::move(endpoint))
        , m_policystore(std::move(policystore))
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
