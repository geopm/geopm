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

#include "geopm_daemon.h"

#include <cstring>
#include <string>

#include "Environment.hpp"
#include "Exception.hpp"
#include "Helper.hpp"

extern "C"
{
int geopm_daemon_get_env_agent(size_t size, char *agent)
{
    int err = 0;
    try {
        std::strncpy(agent, geopm::environment().agent().c_str(), size);
    }
    catch(...) {
        err = geopm::exception_handler(std::current_exception(), false);
    }
    return err;
}

int geopm_daemon_set_host_policy(const char *hostname,
                                 const char *policy_file_path,
                                 const char *policy_json)
{
    int err = 0;
    try {
        const std::string policy_file {policy_file_path};
        // @todo: only works when run on the local host
        if (geopm::hostname() != std::string(hostname)) {
            throw geopm::Exception("Setting host policy for remote host not yet supported.",
                                   GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        geopm::write_file(policy_file, policy_json);
    }
    catch(...) {
        err = geopm::exception_handler(std::current_exception(), false);
    }
    return err;
}
}
