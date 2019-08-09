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

#include <iostream>
#include <fstream>

#include "geopm_time.h"
#include "Endpoint.hpp"
#include "Helper.hpp"

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
    os << (int)(time.t.tv_sec + time.t.tv_nsec * 1.0E-9);
    return os;
}

/// Compute node daemon.  Only handle one job at a time.
/// Eventually, the policy path should be set in the environment override
/// and the daemon should use the policy store to figure out which policy
/// to send.  For now, policy path is hardcoded and this program assumes
/// PowerGovernor is the Agent.
int main(int argc, char *argv[])
{
    std::string shmem_prefix = "/geopmd_endpoint_test";
    geopm::ShmemEndpoint endpoint(shmem_prefix);
    while (true) {
        // check for agent
        std::string agent = "";
        while (agent == "") {
            agent = endpoint.get_agent();
        }
        std::cout << "Controller with agent " << agent << " attached." << std::endl;
        if (agent != "power_governor") {
            std::cerr << "Warning: not expected to work with agents other than power_governor" << std::endl;
        }
        std::ofstream log_file("endpoint_test.log");
        std::vector<double> sample;

        int offset = 0;
        geopm_time_s start_time;
        geopm_time(&start_time);
        geopm_time_s last_sample_time = start_time;
        while (agent != "") {
            endpoint.write_policy({11.0 + offset});
            offset = (int)geopm_time_since(&start_time) % 60;

            // get sample or timeout
            geopm_time_s sample_time;
            geopm_time_s current_time;
            do {
                geopm_time(&current_time);
                sample_time = endpoint.read_sample(sample);
            }
            while (geopm_time_diff(&sample_time, &last_sample_time) == 0.0 &&
                   geopm_time_diff(&current_time, &last_sample_time) < 10.0 /*timeout*/);

            if (geopm_time_diff(&current_time, &last_sample_time) >= 10.0) {
                std::cerr << "Timeout waiting for Controller sample." << std::endl;

                /// @todo: cleanup the shared memory
                agent = "";
            }
            else {
                last_sample_time = sample_time;
                log_file << sample_time << " " << sample << std::endl;
                agent = endpoint.get_agent();
                //usleep(0.01);
            }
        }
        std::cout << "Controller detached." << std::endl;
    }


    // TODO: handle SIGINT so that endpoint can cleanup
    unlink((shmem_prefix + "-policy").c_str());
    unlink((shmem_prefix + "-sample").c_str());
}
