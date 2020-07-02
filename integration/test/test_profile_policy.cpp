/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include <signal.h>

#include <string>

#include "geopm_daemon.h"
#include "Exception.hpp"


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

    // cpp version: include "Daemon.hpp"
    //auto daemon = geopm::Daemon::make_unique(endpoint_name, db_path);
    //daemon->update_endpoint_from_policystore(10);

    return 0;
}
