/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <cpuid.h>

#include "geopm.h"
#include "XeonPlatformImp.hpp"
#include "Profile.hpp"
#include "Exception.hpp"

void decide(geopm::PlatformImp *plat, std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &sample, size_t sample_length);

int read_cpuid()
{
    uint32_t key = 1; //processor features
    uint32_t proc_info = 0;
    uint32_t model;
    uint32_t family;
    uint32_t ext_model;
    uint32_t ext_family;
    uint32_t ebx, ecx, edx;
    const uint32_t model_mask = 0xF0;
    const uint32_t family_mask = 0xF00;
    const uint32_t extended_model_mask = 0xF0000;
    const uint32_t extended_family_mask = 0xFF00000;

    __get_cpuid(key, &proc_info, &ebx, &ecx, &edx);

    model = (proc_info & model_mask) >> 4;
    family = (proc_info & family_mask) >> 8;
    ext_model = (proc_info & extended_model_mask) >> 16;
    ext_family = (proc_info & extended_family_mask)>> 20;

    if (family == 6) {
        model+=(ext_model << 4);
    }
    else if (family == 15) {
        model+=(ext_model << 4);
        family+=ext_family;
    }

    return ((family << 8) + model);
}

int main(int argc, char** argv)
{
    int rank;
    int cpu_id;
    double timeout;
    int rank_per_node = 0;
    geopm::PlatformImp *plat = NULL;
    geopm::ProfileSampler *sampler = NULL;;
    geopm_time_s start, stop;
    std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > sample;
    size_t sample_length;
    const double LOOP_TIMEOUT = 8E-6; //8 ms loop

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (!rank) {
        // TODO: wrap this in a PlatformImp factory.
        cpu_id = read_cpuid();
        if (cpu_id == 0x62D || cpu_id == 0x63E) {
            plat = (geopm::PlatformImp *)(new geopm::IVTPlatformImp);
        }
        else if (cpu_id == 0x63F) {
            plat = (geopm::PlatformImp *)(new geopm::HSXPlatformImp);
        }
        else {
            throw geopm::Exception("cpuid: " + std::to_string(cpu_id), GEOPM_ERROR_PLATFORM_UNSUPPORTED, __FILE__, __LINE__);
        }
        plat->initialize();
        sampler = new geopm::ProfileSampler(4096);
        sampler->initialize(rank_per_node);
        sample.resize(sampler->capacity());

        while (!sampler->do_shutdown()) {
            geopm_time(&start);
            sampler->sample(sample, sample_length, MPI_COMM_WORLD);
            decide(plat, sample, sample_length);

            timeout = 0.0;
            while (timeout < LOOP_TIMEOUT) {
                geopm_time(&stop);
                timeout = geopm_time_diff(&start, &stop);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (!rank) {
        delete sampler;
        delete plat;
    }

    return 0;
}

void decide(geopm::PlatformImp *plat, std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &sample, size_t sample_length)
{
    //Your code goes here
}
