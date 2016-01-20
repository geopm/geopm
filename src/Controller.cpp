/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <vector>
#include <algorithm>
#include <libgen.h>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <stdexcept>
#include <string>
#include <json-c/json.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

#include "config.h"
#include "geopm.h"
#include "Controller.hpp"
#include "Exception.hpp"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

extern "C"
{
    static void *geopm_threaded_run(void *args)
    {
        long err = 0;
        struct geopm_ctl_c *ctl = (struct geopm_ctl_c *)args;

        err = geopm_ctl_run(ctl);

        return (void *)err;
    }


    int geopmctl_main(const char *policy_config, const char *policy_key, const char *sample_key, const char *report)
    {
        int err = 0;
        char profile_name[NAME_MAX] = {0};
        strncpy(profile_name, report, NAME_MAX);
        if (profile_name[NAME_MAX-1] != '\0') {
            err = EINVAL;
        }
        if (!err) {
            try {
                std::string policy_config_str(policy_config);
                std::string policy_key_str(policy_key);
                std::string sample_key_str(sample_key);
                std::string report_str(report);
                std::string profile_name_str(basename(profile_name));
                geopm::GlobalPolicy policy(policy_config_str, "");

                geopm::Controller ctl(&policy, sample_key_str, MPI_COMM_WORLD);
                ctl.run();
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception());
            }
        }
        return err;
    }

    int geopm_ctl_create(struct geopm_policy_c *policy, const char *sample_key, MPI_Comm comm, struct geopm_ctl_c **ctl)
    {
        int err = 0;
        try {
            geopm::GlobalPolicy *global_policy = (geopm::GlobalPolicy *)policy;
            if (sample_key == NULL) {
               throw geopm::Exception("geopm_ctl_create: the sample key is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            const std::string sample_key_str(sample_key);
            *ctl = (struct geopm_ctl_c *)(new geopm::Controller(global_policy, sample_key_str, comm));
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_ctl_destroy(struct geopm_ctl_c *ctl)
    {
        int err = 0;
        geopm::Controller *ctl_obj = (geopm::Controller *)ctl;
        try {
            delete ctl_obj;
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_ctl_run(struct geopm_ctl_c *ctl)
    {
        int err = 0;
        geopm::Controller *ctl_obj = (geopm::Controller *)ctl;
        try {
            ctl_obj->run();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }
    int geopm_ctl_step(struct geopm_ctl_c *ctl)
    {
        int err = 0;
        geopm::Controller *ctl_obj = (geopm::Controller *)ctl;
        try {
            ctl_obj->step();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }
    int geopm_ctl_pthread(struct geopm_ctl_c *ctl,
                          const pthread_attr_t *attr,
                          pthread_t *thread)
    {
        long err = 0;
        geopm::Controller *ctl_obj = (geopm::Controller *)ctl;
        try {
            ctl_obj->pthread(attr, thread);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }
}

namespace geopm
{
    Controller::Controller(const GlobalPolicy *global_policy, const std::string &shmem_base, MPI_Comm comm)
        : m_is_node_root(false)
        , m_max_fanout(0)
        , m_time_zero({{0,0}})
        , m_global_policy(global_policy)
        , m_tree_comm(NULL)
        , m_leaf_decider(NULL)
        , m_decider_factory(NULL)
        , m_platform_factory(NULL)
        , m_platform(NULL)
        , m_sampler(NULL)
        , m_sample_regulator(NULL)
    {
#ifndef GEOPM_DEBUG
        throw geopm::Exception("class Controller", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
#endif
        (void) geopm_time(&m_time_zero);

        MPI_Comm ppn1_comm;
        int err = 0;
        int num_nodes = 0;

        err = geopm_comm_split_ppn1(comm, &ppn1_comm);
        if (err) {
            throw geopm::Exception("geopm_comm_split_ppn1()", err, __FILE__, __LINE__);
        }
        // Only the root rank on each node will have a fully initialized controller
        if (ppn1_comm != MPI_COMM_NULL) {
            m_is_node_root = true;
            check_mpi(MPI_Comm_size(ppn1_comm, &num_nodes));
            m_sampler = new ProfileSampler(shmem_base, GEOPM_CONST_SHMEM_REGION_SIZE);

            int num_fan_out = 1;
            std::vector<int> fan_out(num_fan_out);
            fan_out[0] = num_nodes;

            while (fan_out[0] > M_MAX_FAN_OUT && fan_out[num_fan_out - 1] != 1) {
                ++num_fan_out;
                fan_out.resize(num_fan_out);
                check_mpi(MPI_Dims_create(num_nodes, num_fan_out, fan_out.data()));
            }

            if (num_fan_out > 1 && fan_out[num_fan_out - 1] == 1) {
                --num_fan_out;
                fan_out.resize(num_fan_out);
            }
            std::reverse(fan_out.begin(), fan_out.end());

            m_tree_comm = new TreeCommunicator(fan_out, global_policy, ppn1_comm);
            int num_level = m_tree_comm->num_level();
            m_region.resize(num_level);
            Region *region = new Region(GEOPM_GLOBAL_POLICY_IDENTIFIER,
                                        GEOPM_POLICY_HINT_UNKNOWN, fan_out[num_level - 1]);
            m_region[num_level - 1].insert(std::pair<long, Region *>(GEOPM_GLOBAL_POLICY_IDENTIFIER, region));

            m_platform_factory = new PlatformFactory;
            m_platform = m_platform_factory->platform();

            m_msr_sample.resize(m_platform->capacity());

            m_decider_factory = new DeciderFactory;
            /// @bug Do not hardcode leaf decider string.
            m_leaf_decider = m_decider_factory->decider("governing");
            /// @todo Need to create tree decider(s) here and we need
            /// to get the name strings from the GlobalPolicy object

            for (int level = 0; level < num_level; ++level) {
                if (m_tree_comm->level_size(level) > m_max_fanout) {
                    m_max_fanout = m_tree_comm->level_size(level);
                }
            }
        }
    }

    Controller::~Controller()
    {
        delete m_tree_comm;
        delete m_platform_factory;
        delete m_sampler;
    }

    void Controller::run(void)
    {
        int level;
        int err = 0;
        struct geopm_policy_message_s policy;

        m_sampler->initialize();
        m_prof_sample.resize(m_sampler->capacity());
        std::vector<int> cpu_rank;
        m_sampler->cpu_rank(cpu_rank);
        m_platform->init_transform(cpu_rank);
        m_sample_regulator = new SampleRegulator(cpu_rank);

        // Spin waiting for for first policy message
        level = m_tree_comm->num_level() - 1;
        do {
            try {
                m_tree_comm->get_policy(level, policy);
                err = 0;
            }
            catch (geopm::Exception ex) {
                if (ex.err_value() != GEOPM_ERROR_POLICY_UNKNOWN) {
                    throw ex;
                }
                err = 1;
            }
        }
        while (err);

        while (1) {
            if (walk_down()) {
                break;
            }
            if (walk_up()) {
                break;
            }
        }
    }

    void Controller::step(void)
    {
        int err = walk_down();
        if (!err) {
            err = walk_up();
        }
        if (err) {
            throw Exception("Controller::step(): Shutdown signaled", GEOPM_ERROR_SHUTDOWN, __FILE__, __LINE__);
        }
    }

    void Controller::pthread(const pthread_attr_t *attr, pthread_t *thread)
    {
        int err = 0;

        if (m_is_node_root) {
            err = pthread_create(thread, attr, geopm_threaded_run, (void *)this);
        }
        if (err) {
            throw Exception("Controller::pthread(): pthread_create() failed", err, __FILE__, __LINE__);
        }
    }

    int Controller::walk_down(void)
    {
        int level;
        uint64_t region_id;
        int do_shutdown = 0;
        struct geopm_policy_message_s policy_msg;
        Region *curr_region;

        /// @bug Do calls to geopm_is_policy_equal() below belong inside of the Decider?
        /// @bug Should m_last_policy be a Decider member variable?

        level = m_tree_comm->num_level() - 1;
        m_tree_comm->get_policy(level, policy_msg);
        region_id = policy_msg.region_id;
        for (; policy_msg.mode != GEOPM_MODE_SHUTDOWN && level > 0; --level) {
            curr_region = m_region[level].find(region_id)->second;
            if (!geopm_is_policy_equal(&policy_msg, curr_region->last_policy())) {
                /// @todo Commenting out code here as we have not yet implemented a tree decider
#if 0
                m_tree_decider[level]->split_policy(policy_msg, curr_region);
                m_tree_comm->send_policy(level - 1, *(curr_region->split_policy()));
#endif
                // @bug Temp code to get profiling working.
                std::vector<geopm_policy_message_s> msgs(m_tree_comm->level_size(level - 1));
                std::fill(msgs.begin(), msgs.end(), policy_msg);
                m_tree_comm->send_policy(level - 1, msgs);
            }
            m_tree_comm->get_policy(level - 1, policy_msg);
            auto it = m_region[level - 1].find(region_id);
            if (it == m_region[level - 1].end()) {
                m_region[level - 1].insert(std::pair<long, Region *>(policy_msg.region_id,
                                           new Region(policy_msg.region_id,
                                                      GEOPM_POLICY_HINT_UNKNOWN,
                                                      m_tree_comm->level_size(level - 1))));
            }

        }
        if (policy_msg.mode == GEOPM_MODE_SHUTDOWN) {
            do_shutdown = 1;
        }
        else {
            curr_region = m_region[0].find(region_id)->second;
            if (!geopm_is_policy_equal(&policy_msg, curr_region->last_policy())) {
                m_leaf_decider->update_policy(policy_msg, curr_region);
            }
        }
        return do_shutdown;
    }

    int Controller::walk_up(void)
    {
        int level;
        int do_shutdown = 0;
        struct geopm_policy_message_s policy_msg;
        struct geopm_sample_message_s sample_msg;
        std::vector<struct geopm_sample_message_s> child_sample(m_max_fanout);
        Policy policy;
        int region_id = -1;
        size_t length;

        for (level = 0; level < m_tree_comm->num_level(); ++level) {
            if (level) {
                try {
                    m_tree_comm->get_sample(level, child_sample);
                    region_id = child_sample[0].region_id;
                    /// @bug Process_samples has issues. ifdef out for now
#if 0
                    process_samples(level, child_sample);
#endif
                }
                catch (geopm::Exception ex) {
                    if (ex.err_value() != GEOPM_ERROR_SAMPLE_INCOMPLETE) {
                        throw ex;
                    }
                    break;
                }
                if (region_id != -1) {
                    /// @todo We need to calculate the new per-child power budget for this region
                    /// and send the new policies down to them.
#if 0
                    m_tree_decider[level]->get_policy(m_platform, policy);
                    enforce_child_policy(region_id, level, policy);
#endif
                }
            }
            else {
                // Sample from the application, sample from RAPL,
                // sample from the MSRs and fuse all this data into a
                // single sample using coherant time stamps to
                // calculate elapsed values. We then pass this to the
                // decider who will create a new per domain policy for
                // the current region. Then we can enforce the policy
                // by adjusting RAPL power domain limits.
                m_sampler->sample(m_prof_sample, length);
                m_platform->sample(m_msr_sample);
                std::vector<double> platform_sample(m_msr_sample.size());
                auto output_it = platform_sample.begin();
                for (auto input_it = m_msr_sample.begin(); input_it != m_msr_sample.end(); ++input_it) {
                    *output_it = (*input_it).signal;
                    ++output_it;
                }
                m_sample_regulator->sample(m_msr_sample[0].timestamp,
                                           platform_sample.cbegin(), platform_sample.cend(),
                                           m_prof_sample.cbegin(), m_prof_sample.cbegin() + length,
                                           *(m_platform->signal_domain_transform()),
                                           m_telemetry_sample);

                do_shutdown = m_sampler->do_shutdown();
            }
            if (level != m_tree_comm->root_level()) {
                if ((level && m_tree_decider[level]->is_converged()) || (!level && m_leaf_decider->is_converged())) {
                    /// @bug We should be getting fused samples from ???(TBD)
                    m_tree_comm->send_sample(level, sample_msg);
                }
            }
        }
        if (policy_msg.mode == GEOPM_MODE_SHUTDOWN) {
            do_shutdown = 1;
        }
        return do_shutdown;
    }

    void Controller::enforce_child_policy(const int region_id, const int level, const Policy &policy)
    {
        std::vector<geopm_policy_message_s> message;
        policy.policy_message(region_id, message);
        m_tree_comm->send_policy(level, message);
    }
}
