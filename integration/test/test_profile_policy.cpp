/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <signal.h>

#include <string>

#include "geopm_daemon.h"
#include "geopm/Exception.hpp"


struct geopm_daemon_c *g_daemon;
static void handler(int sig)
{
    if (g_daemon) {
        geopm_daemon_stop_wait_loop(g_daemon);
    }
}

int main(int argc, char* argv[])
{
    // Handle signals
    struct sigaction act;
    act.sa_handler = handler;
    sigaction(SIGINT, &act, NULL);

    std::string db_path = "policystore.db";
    std::string endpoint_name = "/geopm_endpoint_profile_policy_test";

    int err = 0;
    err = geopm_daemon_create(endpoint_name.c_str(),
                              db_path.c_str(),
                              &g_daemon);
    if (err) {
        throw geopm::Exception("geopm_daemon_create() failed",
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    err = geopm_daemon_update_endpoint_from_policystore(g_daemon, 10);
    if (err) {
        geopm_daemon_destroy(g_daemon);
        throw geopm::Exception("geopm_daemon_update_endpoint_from_policystore() failed",
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    err = geopm_daemon_destroy(g_daemon);
    if (err) {
        throw geopm::Exception("geopm_daemon_daemon() failed",
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }

    // cpp version: include "geopm/Daemon.hpp"
    //auto daemon = geopm::Daemon::make_unique(endpoint_name, db_path);
    //daemon->update_endpoint_from_policystore(10);

    return 0;
}
