/*
 * Copyright (c) 2017, Intel Corporation
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

#include <omp.h>
#include <ompt.h>

enum {
    ARRAY_LEN = 100,
    LOOP_COUNT = 4,
};

static int g_err = 0;

static void on_ompt_event_parallel_begin(ompt_task_id_t parent_task_id, ompt_frame_t *parent_task_frame,
                                         ompt_parallel_id_t parallel_id, uint32_t requested_team_size,
                                         void *parallel_function, ompt_invoker_t invoker)
{
     static unsigned is_once = 1;
     static ompt_parallel_id_t first_parallel_id;
     if (is_once) {
         first_parallel_id = parallel_id;
         is_once = 0;
     }
     if (g_err == 0 && parallel_id != first_parallel_id) {
          fprintf(stderr, "Begin: parallel ID is not the same: 0x%llx != 0x%llx\n\n",
                  (unsigned long long)(first_parallel_id),
                  (unsigned long long)(parallel_id));
          g_err = -1;
     }
}

static void on_ompt_event_parallel_end(ompt_parallel_id_t parallel_id, ompt_task_id_t task_id,
                                       ompt_invoker_t invoker)
{
     static unsigned is_once = 1;
     static ompt_parallel_id_t first_parallel_id;
     if (is_once) {
         first_parallel_id = parallel_id;
         is_once = 0;
     }
     if (g_err == 0 && parallel_id != first_parallel_id) {
          fprintf(stderr, "End: parallel ID is not the same: 0x%llx != 0x%llx\n\n",
                  (unsigned long long)(first_parallel_id),
                  (unsigned long long)(parallel_id));
          g_err = -2;
     }
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


int main(int argc, char **argv)
{
    int i, j;
    double a[ARRAY_LEN];
    double b[ARRAY_LEN] = {0};

    for (j = 0; j < ARRAY_LEN; ++j) {
        a[j] = ((double)random()) / RAND_MAX;
    }
    for (i = 0; i < LOOP_COUNT; ++i) {
#pragma omp parallel for
        for (j = 0; j < ARRAY_LEN; ++j) {
            b[j] += a[j];
        }
    }
    return g_err;
}
