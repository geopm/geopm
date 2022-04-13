/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "OMPT.hpp"
#include "geopm_prof.h"
#include <omp-tools.h>


extern "C"
{

    static void on_ompt_event_parallel_begin(ompt_data_t *encountering_task_data,
                                             const ompt_frame_t *encountering_task_frame,
                                             ompt_data_t *parallel_data,
                                             unsigned int requested_parallelism,
                                             int flags,
                                             const void *parallel_function)
    {
        geopm::OMPT::ompt().region_enter(parallel_function);
    }


    static void on_ompt_event_parallel_end(ompt_data_t *parallel_data,
                                           ompt_data_t *encountering_task_data,
                                           int flags,
                                           const void *parallel_function)
    {
        geopm::OMPT::ompt().region_exit(parallel_function);
    }

    static void on_ompt_event_work(ompt_work_t wstype,
                                   ompt_scope_endpoint_t endpoint,
                                   ompt_data_t *parallel_data,
                                   ompt_data_t *task_data,
                                   uint64_t count,
                                   const void *parallel_function)
    {
        // Understanding based on inpection of the values passed by the
        // intel compiler implementation when running a test:
        //
        // - The omp team leader calls this function with the "count"
        //   set to the number of work units that will be executed by
        //   the team.
        //
        // - The omp non-lead threads call this function with "count"
        //   set to zero.
        //
        // - When independent work (e.g. not in a "#pragma omp for"
        //   section) is executed in a parallel section this function
        //   is called with count == 1.
        geopm_tprof_init(count);
    }

    int ompt_initialize(ompt_function_lookup_t lookup,
                        int initial_device_num,
                        ompt_data_t *tool_data)
    {
        if (geopm::OMPT::ompt().is_enabled()) {
            ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
            ompt_set_callback(ompt_callback_parallel_begin, (ompt_callback_t) &on_ompt_event_parallel_begin);
            ompt_set_callback(ompt_callback_parallel_end, (ompt_callback_t) &on_ompt_event_parallel_end);
            ompt_set_callback(ompt_callback_work, (ompt_callback_t) &on_ompt_event_work);
        }
        // OpenMP 5.0 standard says return non-zero on success!?!?!
        return 1;
    }

    void ompt_finalize(ompt_data_t *data)
    {

    }

    ompt_start_tool_result_t *ompt_start_tool(unsigned int omp_version, const char *runtime_version)
    {
        static ompt_start_tool_result_t ompt_start_tool_result = {&ompt_initialize,
                                                                  &ompt_finalize,
                                                                  {}};
        return &ompt_start_tool_result;
    }
}
