/*
 * Copyright (c) 2015, Intel Corporation
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <json-c/json.h>

#include "geopm.h"

int main(int argc, char** argv)
{

    policy = json_object_new_object();
    options = json_object_new_object();

    switch (mode) {
        case MODE_NONE:
            break;
        case MODE_TDP_STATIC:
            json_object_object_add(policy,"mode",json_object_new_string("tdp_balanced_static"));
            json_object_object_add(options,"percent_tdp",json_object_new_int(percentage));
            json_object_object_add(policy,"options",options);
            break;
        case MODE_UNIFORM_STATIC:
            json_object_object_add(policy,"mode",json_object_new_string("freq_uniform_static"));
            json_object_object_add(options,"cpu_mhz",json_object_new_int(frequency));
            json_object_object_add(policy,"options",options);
            break;
        case MODE_HYBRID_STATIC:
            json_object_object_add(policy,"mode",json_object_new_string("freq_hybrid_static"));
            json_object_object_add(options,"cpu_mhz",json_object_new_int(frequency));
            json_object_object_add(options,"num_cpu_max_perf",json_object_new_int(num_cpus));
            json_object_object_add(options,"affinity",json_object_new_string(affinity));
            json_object_object_add(policy,"options",options);
            break;
        case MODE_PERF_DYNAMIC:
            json_object_object_add(policy,"mode",json_object_new_string("perf_balanced_dynamic"));
            json_object_object_add(options,"power_budget",json_object_new_int(budget));
            json_object_object_add(policy,"options",options);
            break;
        case MODE_UNIFORM_DYNAMIC:
            json_object_object_add(policy,"mode",json_object_new_string("freq_uniform_dynamic"));
            json_object_object_add(options,"power_budget",json_object_new_int(budget));
            json_object_object_add(policy,"options",options);
            break;
        case MODE_HYBRID_DYNAMIC:
            json_object_object_add(policy,"mode",json_object_new_string("freq_hybrid_dynamic"));
            json_object_object_add(options,"power_budget",json_object_new_int(budget));
            json_object_object_add(options,"num_cpu_max_perf",json_object_new_int(num_cpus));
            json_object_object_add(options,"affinity",json_object_new_string(affinity));
            json_object_object_add(policy,"options",options);
            break;
        default:
            slurm_error("energy-mode: invalid mode: %d", mode);
            return ESPANK_BAD_ARG;
    }
    fprintf(config_file, "%s", json_object_to_json_string(policy));

    return 0;
}
