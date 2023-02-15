/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
