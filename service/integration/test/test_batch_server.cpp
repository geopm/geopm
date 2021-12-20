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
#include <iostream>
#include <unistd.h>
#include <vector>

#include "BatchServer.hpp"
#include "BatchClient.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm_topo.h"

using geopm::BatchServer;
using geopm::BatchClient;

void run(void)
{
    int client_pid = getpid();
    geopm_request_s request = {GEOPM_DOMAIN_CPU, 0, "TIME"};
    std::shared_ptr<BatchServer> batch_server =
        BatchServer::make_unique(client_pid, {request}, {});
    std::string server_key = batch_server->server_key();
    std::shared_ptr<BatchClient> batch_client =
        BatchClient::make_unique(server_key, 1.0, 1, 0);

    for (int idx = 0; idx < 10; ++idx) {
        std::vector<double> sample = batch_client->read_batch();
        std::cout << sample.at(0) << "\n";
        sleep(1);
    }
    batch_client->stop_batch();
}


int main (int argc, char **argv)
{
    run();
    return 0;
}
