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

#include "geopm.h"
#include "geopm_policy_message.h"
#include "Controller.hpp"
#include "Profile.hpp"
#include "Exception.hpp"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

extern "C"
{
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
                geopm::Profile profile(profile_name_str, sample_key_str, 0); //FIXME how should the last parameter be determined?
                geopm::Controller ctl(&policy, &profile, MPI_COMM_WORLD);
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
            geopm::Profile *profile = (geopm::Profile *)prof;
            *ctl = (struct geopm_ctl_c *)(new geopm::Controller(global_policy, profile, comm));
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
}

namespace geopm
{
    Controller::Controller(const GlobalPolicy *global_policy, const Profile *profile, MPI_Comm comm)
        : m_global_policy(global_policy)
        , m_profile(profile)
    {
        int num_level = GEOPM_CONST_DEFAULT_CTL_NUM_LEVEL;
        std::vector<int> fan_out(num_level);
        int num_nodes;

        int err = geopm_num_nodes(comm, &num_nodes);
        if (!err) {
            err = MPI_Dims_create(num_nodes, num_level, fan_out.data());
        }
        if (!err) {
            std::reverse(fan_out.begin(), fan_out.end());
        }

        m_tree_comm = new TreeCommunicator(fan_out, global_policy, comm);

        int max_size = 0;
        for (int level = 0; level < m_tree_comm->num_level(); ++level) {
            if (m_tree_comm->level_size(level) > max_size) {
                max_size = m_tree_comm->level_size(level);
            }
        }
        m_split_policy.resize(max_size);
        m_child_sample.resize(max_size);
        std::fill(m_last_policy.begin(), m_last_policy.end(), GEOPM_UNKNOWN_POLICY);
    }

    Controller::~Controller()
    {
        delete m_tree_comm;
    }

    void Controller::run()
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
            catch (unknown_policy_error ex) {
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


    int Controller::walk_down(void)
    {
        int level;
        int do_shutdown = 0;
        struct geopm_policy_message_s policy_msg;

        // FIXME Do calls to geopm_is_policy_equal() below belong inside of the Decider?
        // Should m_last_policy be a Decider member variable?

        level = m_tree_comm->num_level() - 1;
        m_tree_comm->get_policy(level, policy_msg);
        for (; policy_msg.mode != GEOPM_MODE_SHUTDOWN && level > 0; --level) {
            if (!geopm_is_policy_equal(&policy_msg, &(m_last_policy[level]))) {
                m_tree_decider[level]->split_policy(policy_msg, m_split_policy);
                m_tree_comm->send_policy(level - 1, m_split_policy);
                m_last_policy[level] = policy_msg;
            }
            m_tree_comm->get_policy(level - 1, policy_msg);
        }
        if (policy_msg.mode == GEOPM_MODE_SHUTDOWN) {
            do_shutdown = 1;
        }
        else {
            if (!geopm_is_policy_equal(&policy_msg, &(m_last_policy[0]))) {
                m_leaf_decider->update_policy(policy_msg);
            }
        }
        return do_shutdown;
    }

    int Controller::walk_up(void)
    {
        int level;
        int do_shutdown = 0;
        struct geopm_policy_message_s policy_msg;
        struct sample_message_s sample_msg;
        Policy policy;

        m_tree_comm->get_policy(0, policy_msg);
        for (level = 0; policy_msg.mode != GEOPM_MODE_SHUTDOWN && level < m_tree_comm->num_level(); ++level) {
            if (level) {
                try {
                    m_tree_comm->get_sample(level, m_child_sample);
                    m_platform[level]->observe(m_child_sample);
                }
                catch (incomplete_sample_error ex) {
                    break;
                }
                m_tree_decider[level]->get_policy(m_platform[level], policy);
                m_platform[level]->enforce_policy(policy);
            }
            else {
                m_platform[0]->observe();
                m_leaf_decider->get_policy(m_platform[0], policy);
                m_platform[0]->enforce_policy(policy);
            }
            if (level && m_tree_decider[level]->is_converged()) {
                m_platform[level]->sample(sample_msg);
                m_tree_comm->send_sample(level, sample_msg);
            }
        }
        if (policy_msg.mode == GEOPM_MODE_SHUTDOWN) {
            do_shutdown = 1;
        }
        return do_shutdown;
    }
}
