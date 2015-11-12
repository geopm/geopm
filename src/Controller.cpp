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

#include "geopm.h"
#include "Controller.hpp"
#include "Exception.hpp"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif
#define GEOPM_GLOBAL_POLICY_IDENTIFIER -1

extern "C"
{

    enum geopm_controller_cont_e {
        GEOPM_CTL_MAX_FAN_OUT = 16,
    };


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

    int geopm_ctl_create(struct geopm_policy_c *policy, struct geopm_prof_c *prof, MPI_Comm comm, struct geopm_ctl_c **ctl)
    {
        int err = 0;
        try {
            geopm::GlobalPolicy *global_policy = (geopm::GlobalPolicy *)policy;
            const std::string sample_key("/geopm_shmem_key");

            *ctl = (struct geopm_ctl_c *)(new geopm::Controller(global_policy, sample_key, comm));
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
}

namespace geopm
{
    Controller::Controller(const GlobalPolicy *global_policy, const std::string &shmem_base, MPI_Comm comm)
        : m_max_fanout(0)
        , m_global_policy(global_policy)
        , m_sampler(shmem_base, GEOPM_CONST_SHMEM_REGION_SIZE, comm)
    {
        throw geopm::Exception("class Controller", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);

        (void) geopm_time(&m_time_zero);

        int num_nodes = 0;
        int err = geopm_num_nodes(comm, &num_nodes);
        if (err) {
            throw geopm::Exception("geopm_num_nodes()", err, __FILE__, __LINE__);
        }
        int num_level = 1;
        std::vector<int> fan_out(num_level);
        fan_out[0] = num_nodes;
        while (fan_out[0] > GEOPM_CTL_MAX_FAN_OUT && fan_out[num_level - 1] != 1) {
            ++num_level;
            fan_out.resize(num_level);
            check_mpi(MPI_Dims_create(num_nodes, num_level, fan_out.data()));
        }
        if (num_level > 1 && fan_out[num_level - 1] == 1) {
            --num_level;
            fan_out.resize(num_level);
        }
        std::reverse(fan_out.begin(), fan_out.end());

        m_tree_comm = new TreeCommunicator(fan_out, global_policy, comm);
        m_region.resize(m_tree_comm->num_level());
        m_region[0].insert(std::pair<long, Region *>(GEOPM_GLOBAL_POLICY_IDENTIFIER,
                          new Region("global", GEOPM_GLOBAL_POLICY_IDENTIFIER,
                                    GEOPM_POLICY_HINT_UNKNOWN, m_platform->num_domain())));

        m_platform_factory = new PlatformFactory;
        m_platform = m_platform_factory->platform();

        for (int level = 0; level < m_tree_comm->num_level(); ++level) {
            if (m_tree_comm->level_size(level) > m_max_fanout) {
                m_max_fanout = m_tree_comm->level_size(level);
            }
            // default global region before application regions have been registered
            // holds the global power policy
            if(level) {
                m_region[level].insert(std::pair<long, Region *>(GEOPM_GLOBAL_POLICY_IDENTIFIER,
                                      new Region("global", GEOPM_GLOBAL_POLICY_IDENTIFIER,
                                                GEOPM_POLICY_HINT_UNKNOWN, m_tree_comm->level_size(level - 1))));
            }
        }
    }

    Controller::~Controller()
    {
        delete m_tree_comm;
        delete m_platform_factory;
    }

    void Controller::run(void)
    {
        int level;
        int err = 0;
        struct geopm_policy_message_s policy;

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


    int Controller::walk_down(void)
    {
        int level;
        uint64_t region_id;
        int do_shutdown = 0;
        struct geopm_policy_message_s policy_msg;
        Region *curr_region;

        // FIXME Do calls to geopm_is_policy_equal() below belong inside of the Decider?
        // Should m_last_policy be a Decider member variable?

        level = m_tree_comm->num_level() - 1;
        m_tree_comm->get_policy(level, policy_msg);
        region_id = policy_msg.region_id;
        for (; policy_msg.mode != GEOPM_MODE_SHUTDOWN && level > 0; --level) {
            curr_region = m_region[level].find(region_id)->second;
            if (!geopm_is_policy_equal(&policy_msg, curr_region->last_policy())) {
                m_tree_decider[level]->split_policy(policy_msg, curr_region);
                m_tree_comm->send_policy(level - 1, *(curr_region->split_policy()));
            }
            m_tree_comm->get_policy(level - 1, policy_msg);
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
        std::vector<struct geopm_sample_message_s> child_sample;
        Policy policy;
        int region_id = -1;

        for (level = 0; level < m_tree_comm->num_level(); ++level) {
            if (level) {
                try {
                    m_tree_comm->get_sample(level, child_sample);
                    region_id = child_sample[0].region_id;
                    process_samples(level, child_sample);
                }
                catch (geopm::Exception ex) {
                    if (ex.err_value() != GEOPM_ERROR_SAMPLE_INCOMPLETE) {
                        throw ex;
                    }
                    break;
                }
                if (region_id != -1) {
                    m_tree_decider[level]->get_policy(m_platform, policy);
                    enforce_child_policy(region_id, level, policy);
                }
            }
            else {
                m_platform->observe();
                m_leaf_decider->get_policy(m_platform, policy);
                m_platform->enforce_policy(policy);
            }
            if (level && m_tree_decider[level]->is_converged()) {
                m_platform->sample(sample_msg);
                m_tree_comm->send_sample(level, sample_msg);
            }
        }
        if (policy_msg.mode == GEOPM_MODE_SHUTDOWN) {
            do_shutdown = 1;
        }
        return do_shutdown;
    }

    void Controller::process_samples(const int level, const std::vector<struct geopm_sample_message_s> &sample)
    {
        uint64_t region_id = sample[0].region_id;
        Region *curr_region;
        auto iter = m_region[level].find(region_id);
        int num_domains;

        if (iter == m_region[level].end()) {
            //Region not found. Create a new one.
            if (level) {
                num_domains =  m_tree_comm->level_size(level - 1);
            }
            else {
                num_domains = m_platform->num_domain();
            }
            curr_region = new Region("some_name", region_id, GEOPM_POLICY_HINT_UNKNOWN, num_domains);
            //set it's policy equal to the global policy for this level.
            *(curr_region->policy()) = *(m_region[level].find(GEOPM_GLOBAL_POLICY_IDENTIFIER)->second->policy());
            m_region[level].insert(std::pair<long, Region *>(region_id, curr_region));
        }
        else {
            curr_region = iter->second;
        }

        struct geopm_time_s time;
        geopm_time(&time);
        double timestamp = geopm_time_diff(&m_time_zero, &time);

        for (auto sample_it = sample.begin(); sample_it < sample.end(); ++sample_it) {
            if (sample_it->region_id != region_id) {
                throw geopm::Exception("class Controller", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
            curr_region->observation_insert(GEOPM_INDEX_TIMESTAMP, timestamp);
            curr_region->observation_insert(GEOPM_INDEX_RUNTIME, sample_it->runtime);
            curr_region->observation_insert(GEOPM_INDEX_PROGRESS, sample_it->progress);
            curr_region->observation_insert(GEOPM_INDEX_ENERGY, sample_it->energy);
            curr_region->observation_insert(GEOPM_INDEX_FREQUENCY, sample_it->frequency);
        }
    }

    void Controller::enforce_child_policy(const int region_id, const int level, const Policy &policy)
    {
        std::vector<geopm_policy_message_s> message;
        policy.policy_message(region_id, message);
        m_tree_comm->send_policy(level, message);
    }
}
