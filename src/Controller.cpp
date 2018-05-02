/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <streambuf>
#include <string>
#include <stdexcept>
#include <system_error>
#include <vector>

#include "geopm.h"
#include "geopm_env.h"
#include "geopm_version.h"
#include "geopm_signal_handler.h"
#include "geopm_hash.h"
#include "Comm.hpp"
#include "Exception.hpp"
#include "SampleRegulator.hpp"
#include "PlatformIO.hpp"
#include "ProfileSampler.hpp"
#include "Tracer.hpp"
#include "OMPT.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "RuntimeRegulator.hpp"
#include "ProfileIOGroup.hpp"
#include "ProfileIORuntime.hpp"
#include "ProfileIOSample.hpp"
#include "Helper.hpp"
#include "Kontroller.hpp"
#include "config.h"

extern "C"
{
    int geopm_ctl_run(struct geopm_ctl_c *ctl);

    static void *geopm_threaded_run(void *args)
    {
        long err = 0;
        struct geopm_ctl_c *ctl = (struct geopm_ctl_c *)args;

        err = geopm_ctl_run(ctl);

        return (void *)err;
    }

    int geopmctl_main(const char *policy_config)
    {
        int err = 0;
        try {
            auto tmp_comm = geopm::comm_factory().make_plugin(geopm_env_comm());
            geopm::Kontroller ctl(std::move(tmp_comm), geopm_env_policy());
            err = geopm_ctl_run((struct geopm_ctl_c *)&ctl);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }

    int geopm_ctl_destroy(struct geopm_ctl_c *ctl)
    {
        int err = 0;
        geopm::Kontroller *ctl_obj = (geopm::Kontroller *)ctl;
        try {
            delete ctl_obj;
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }

    int geopm_ctl_run(struct geopm_ctl_c *ctl)
    {
        int err = 0;
        geopm::Kontroller *ctl_obj = (geopm::Kontroller *)ctl;
        try {
            ctl_obj->run();
        }
        catch (...) {
            ctl_obj->abort();
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }

    int geopm_ctl_pthread(struct geopm_ctl_c *ctl,
                          const pthread_attr_t *attr,
                          pthread_t *thread)
    {
        long err = 0;
        geopm::Kontroller *ctl_obj = (geopm::Kontroller *)ctl;
        try {
            ctl_obj->pthread(attr, thread);
        }
        catch (...) {
            ctl_obj->abort();
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }
}
