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
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sched.h>
#include <stdint.h>
#include "geopm_omp.h"
#include "geopm_error.h"
#include "config.h"

#ifdef _OPENMP
#include <omp.h>

#ifdef GEOPM_HAS_OMPT
#include "geopm.h"
#include <ompt.h>

static void on_ompt_event_parallel_begin(ompt_task_id_t parent_task_id, ompt_frame_t *parent_task_frame,
                                         ompt_parallel_id_t parallel_id, uint32_t requested_team_size,
                                         void *parallel_function, ompt_invoker_t invoker)
{
     uint64_t region_id;
     char parallel_id_str[NAME_MAX];
     snprintf(parallel_id_str, NAME_MAX, "0x%llx", (unsigned long long)(parallel_id));
     geopm_prof_region(parallel_id_str, GEOPM_REGION_HINT_UNKNOWN, &region_id);
     geopm_prof_enter(region_id);
}

static void on_ompt_event_parallel_end(ompt_parallel_id_t parallel_id, ompt_task_id_t task_id,
                                       ompt_invoker_t invoker)
{
     uint64_t region_id;
     char parallel_id_str[NAME_MAX];
     snprintf(parallel_id_str, NAME_MAX, "0x%llx", (unsigned long long)(parallel_id));
     geopm_prof_region(parallel_id_str, GEOPM_REGION_HINT_UNKNOWN, &region_id);
     geopm_prof_exit(region_id);
}


void ompt_initialize(ompt_function_lookup_t lookup, const char *runtime_version, unsigned int ompt_version)
{
    ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
    ompt_set_callback(ompt_event_parallel_begin, (ompt_callback_t) &on_ompt_event_parallel_begin);
    ompt_set_callback(ompt_event_parallel_end, (ompt_callback_t) &on_ompt_event_parallel_end);

}

ompt_initialize_t ompt_tool()
{
    return &ompt_initialize;
}

#endif // OMPT available

int geopm_no_omp_cpu(int num_cpu, cpu_set_t *no_omp)
{
    int err = 0;
    for (int i = 0; i < num_cpu; ++i) {
        CPU_SET(i, no_omp);
    }
    #pragma omp parallel default(shared)
    {
        #pragma omp critical
        {
            int cpu_index = sched_getcpu();
            if (cpu_index < num_cpu)
            {
                CPU_CLR(cpu_index, no_omp);
            }
            else {
                err = GEOPM_ERROR_RUNTIME;
            }
        } /* end pragma omp critical */
    } /* end pragma omp parallel */
    return err;
}
#else

int geopm_no_omp_cpu(int num_cpu, cpu_set_t *no_omp)
{
    return GEOPM_ERROR_OPENMP_UNSUPPORTED;
}

#endif
