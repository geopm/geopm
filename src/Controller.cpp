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

#include "Controller.hpp"

extern "C"
{
    int geopm_ctl_run(int num_factor, const int *factor, const char *control, const char *report, MPI_Comm comm)
    {
        int err = 0;

        try {
            std::vector<int> factor_vec(num_factor);
            std::string control_str(control);
            std::string report_str(report);
            factor_vec.assign(factor, factor + num_factor);
            geopm::Controller ctl(factor_vec, control_str, report_str, comm);
            ctl.run();
        }
        catch (std::exception ex) {
            std::cerr << ex.what();
            err = -1;
        }
        return err;
    }
}

namespace geopm
{
    Controller::Controller(std::vector<int> factor, std::string control, std::string report, MPI_Comm comm)
        : m_factor(factor), m_control(control), m_report(report), m_comm(factor, control, comm), m_last_policy(m_comm.num_level())
    {
        int max_size = 0;
        for (int level = 0; level < m_comm.num_level(); ++level) {
            if (m_comm.level_size(level) > max_size) {
                max_size = m_comm.level_size(level);
            }
        }
        m_split_policy.resize(max_size);
        m_child_sample.resize(max_size);
        std::fill(m_last_policy.begin(), m_last_policy.end(), GEOPM_UNKNOWN_POLICY);
    }

    Controller::~Controller() {}

    void Controller::run()
    {
        int level;
        int err = 0;
        struct geopm_policy_message_s policy;

        // Spin waiting for for first policy message
        level = m_comm.num_level() - 1;
        do {
            try {
                m_comm.get_policy(level, policy);
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

        // FIXME Do calls to is_policy_equal() below belong inside of the Decider?
        // Should m_last_policy be a Decider member variable?

        level = m_comm.num_level() - 1;
        m_comm.get_policy(level, policy_msg);
        for (; policy_msg.mode != GEOPM_MODE_SHUTDOWN && level > 0; --level) {
            if (!is_policy_equal(&policy_msg, &(m_last_policy[level]))) {
                m_tree_decider->split_policy(policy_msg, m_split_policy);
                m_comm.send_policy(level - 1, m_split_policy);
                m_last_policy[level] = policy_msg;
            }
            m_comm.get_policy(level - 1, policy_msg);
        }
        if (policy_msg.mode == GEOPM_MODE_SHUTDOWN) {
            do_shutdown = 1;
        }
        else {
            if (!is_policy_equal(&policy_msg, &(m_last_policy[0]))) {
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

        m_comm.get_policy(0, policy_msg);
        for (level = 0; policy_msg.mode != GEOPM_MODE_SHUTDOWN && level < m_comm.num_level(); ++level) {
            if (level) {
                try {
                    m_comm.get_sample(level, m_child_sample);
                    m_platform[level]->observe(m_child_sample);
                }
                catch (incomplete_sample_error ex) {
                    break;
                }
                m_tree_decider->get_policy(m_platform[level], policy);
                m_platform[level]->enforce_policy(policy);
            }
            else {
                m_platform[0]->observe();
                m_leaf_decider->get_policy(m_platform[0], policy);
                m_platform[0]->enforce_policy(policy);
            }
            if (level && m_tree_decider->is_converged()) {
                m_platform[level]->sample(sample_msg);
                m_comm.send_sample(level, sample_msg);
            }
        }
        if (policy_msg.mode == GEOPM_MODE_SHUTDOWN) {
            do_shutdown = 1;
        }
        return do_shutdown;
    }
}
