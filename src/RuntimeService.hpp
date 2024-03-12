/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pthread.h>
#include <string>
#include <memory>

namespace geopm
{
    class Stats;
    class Policy;

    struct policy_struct_s {
        pthread_mutex_t mutex;
        bool is_updated;
        std::shared_ptr<Policy> policy;
        std::shared_ptr<Stats> stats;
    };
    int rtd_main(const std::string &server_address);
}
