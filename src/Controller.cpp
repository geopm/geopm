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

#include "geopm.h"
#include "geopm_version.h"
#include "geopm_signal_handler.h"
#include "Controller.hpp"
#include "Exception.hpp"
#include "config.h"

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


    int geopmctl_main(const char *policy_config)
    {
        int err = 0;
        try {
            if (policy_config) {
                std::string policy_config_str(policy_config);
                geopm::GlobalPolicy policy(policy_config_str, "");
                geopm::Controller ctl(&policy, MPI_COMM_WORLD);
                err = geopm_ctl_run((struct geopm_ctl_c *)&ctl);
            }
            //The null case is for all nodes except rank 0.
            //These controllers should assume their policy from the master.
            else {
                geopm::Controller ctl(NULL, MPI_COMM_WORLD);
                err = geopm_ctl_run((struct geopm_ctl_c *)&ctl);
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_ctl_create(struct geopm_policy_c *policy, MPI_Comm comm, struct geopm_ctl_c **ctl)
    {
        int err = 0;
        try {
            geopm::GlobalPolicy *global_policy = (geopm::GlobalPolicy *)policy;
            *ctl = (struct geopm_ctl_c *)(new geopm::Controller(global_policy, comm));
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_ctl_create_f(struct geopm_policy_c *policy, int comm, struct geopm_ctl_c **ctl)
    {
        return geopm_ctl_create(policy, MPI_Comm_f2c(comm), ctl);
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
            ctl_obj->reset();
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
    Controller::Controller(GlobalPolicy *global_policy, MPI_Comm comm)
        : m_is_node_root(false)
        , m_max_fanout(0)
        , m_global_policy(global_policy)
        , m_tree_comm(NULL)
        , m_leaf_decider(NULL)
        , m_decider_factory(NULL)
        , m_platform_factory(NULL)
        , m_platform(NULL)
        , m_sampler(NULL)
        , m_sample_regulator(NULL)
        , m_tracer(NULL)
        , m_region_id_all(0)
        , m_do_shutdown(false)
        , m_is_connected(false)
        , m_rate_limit(0.0)
        , m_is_in_outer(false)
        , m_rank_per_node(0)
        , m_outer_sync_time(0.0)
        , m_mpi_sync_time(0.0)
        , m_is_outer_changed(false)
    {
        MPI_Comm ppn1_comm;
        int err = 0;
        int num_nodes = 0;

        err = geopm_comm_split_ppn1(comm, "ctl", &ppn1_comm);
        if (err) {
            throw geopm::Exception("geopm_comm_split_ppn1()", err, __FILE__, __LINE__);
        }
        // Only the root rank on each node will have a fully initialized controller
        if (ppn1_comm != MPI_COMM_NULL) {
            m_is_node_root = true;
            struct geopm_plugin_description_s plugin_desc;
            int rank;
            check_mpi(MPI_Comm_rank(ppn1_comm, &rank));
            if (!rank) { // We are the root of the tree
                plugin_desc.tree_decider[NAME_MAX - 1] = '\0';
                plugin_desc.leaf_decider[NAME_MAX - 1] = '\0';
                plugin_desc.platform[NAME_MAX - 1] = '\0';
                switch (m_global_policy->mode()) {
                    case GEOPM_POLICY_MODE_TDP_BALANCE_STATIC:
                    case GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC:
                    case GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC:
                    case GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC:
                    case GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC:
                        throw geopm::Exception("Controller::Controller()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
                        break;
                    case GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC:
                        snprintf(plugin_desc.tree_decider, NAME_MAX, "power_balancing");
                        snprintf(plugin_desc.leaf_decider, NAME_MAX, "power_governing");
                        snprintf(plugin_desc.platform, NAME_MAX, "rapl");
                        break;
                    case GEOPM_POLICY_MODE_STATIC:
                        snprintf(plugin_desc.tree_decider, NAME_MAX, "static_policy");
                        snprintf(plugin_desc.leaf_decider, NAME_MAX, "static_policy");
                        snprintf(plugin_desc.platform, NAME_MAX, "rapl");
                        break;
                    case GEOPM_POLICY_MODE_DYNAMIC:
                        strncpy(plugin_desc.tree_decider, m_global_policy->tree_decider().c_str(), NAME_MAX -1);
                        strncpy(plugin_desc.leaf_decider, m_global_policy->leaf_decider().c_str(), NAME_MAX -1);
                        strncpy(plugin_desc.platform, m_global_policy->platform().c_str(), NAME_MAX -1);
                        break;
                    default:
                        throw Exception("Controller::Controller(): Invalid mode", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            check_mpi(MPI_Bcast(&plugin_desc, sizeof(plugin_desc), MPI_CHAR, 0, ppn1_comm));

            check_mpi(MPI_Comm_size(ppn1_comm, &num_nodes));

            if (num_nodes > 1) {
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
            }
            else {
                m_tree_comm = new SingleTreeCommunicator(global_policy);
            }

            int num_level = m_tree_comm->num_level();
            m_region.resize(num_level);
            m_policy.resize(num_level);
            m_tree_decider.resize(num_level);
            std::fill(m_tree_decider.begin(), m_tree_decider.end(), (Decider *)NULL);
            m_last_policy_msg.resize(num_level);
            std::fill(m_last_policy_msg.begin(), m_last_policy_msg.end(), GEOPM_POLICY_UNKNOWN);

            m_last_sample_msg.resize(num_level);
            std::fill(m_last_sample_msg.begin(), m_last_sample_msg.end(), GEOPM_SAMPLE_INVALID);

            m_platform_factory = new PlatformFactory;
            m_platform = m_platform_factory->platform(plugin_desc.platform);
            double upper_bound;
            double lower_bound;
            m_platform->bound(upper_bound, lower_bound);
            // convert rate limit from ms to seconds
            m_rate_limit = m_platform->control_latency_ms() * 1E-3;
            geopm_time(&m_loop_t0);

            m_msr_sample.resize(m_platform->capacity());

            m_decider_factory = new DeciderFactory;
            m_leaf_decider = m_decider_factory->decider(std::string(plugin_desc.leaf_decider));
            m_leaf_decider->bound(upper_bound, lower_bound);

            int num_domain;
            for (int level = 0; level < num_level; ++level) {
                if (level == 0) {
                    num_domain = m_platform->num_control_domain();
                    m_telemetry_sample.resize(num_domain, {0, {{0, 0}}, {0}});
                    m_region[level].insert(std::pair<uint64_t, Region *>
                                           (GEOPM_REGION_ID_MPI,
                                            new Region(GEOPM_REGION_ID_MPI,
                                                       GEOPM_POLICY_HINT_UNKNOWN,
                                                       num_domain,
                                                       level)));

                }
                else {
                    num_domain = m_tree_comm->level_size(level - 1);
                }
                m_policy[level] = new Policy(num_domain);
                if (m_platform->control_domain() == GEOPM_CONTROL_DOMAIN_POWER && level == 1) {
                    upper_bound *= m_platform->num_control_domain();
                    lower_bound *= m_platform->num_control_domain();
                    if (level > 1) {
                        int i = level - 1;
                        while (i >= 0) {
                            upper_bound *= m_tree_comm->level_size(i);
                            --i;
                        }
                    }
                }
                m_tree_decider[level] = m_decider_factory->decider(std::string(plugin_desc.tree_decider));
                m_tree_decider[level]->bound(upper_bound, lower_bound);
                m_region[level].insert(std::pair<uint64_t, Region *>
                                       (GEOPM_REGION_ID_OUTER,
                                        new Region(GEOPM_REGION_ID_OUTER,
                                                   GEOPM_POLICY_HINT_UNKNOWN,
                                                   num_domain,
                                                   level)));
                if (m_tree_comm->level_size(level) > m_max_fanout) {
                    m_max_fanout = m_tree_comm->level_size(level);
                }
            }

            // Synchronize the ranks so time zero is uniform.
            MPI_Barrier(ppn1_comm);
            m_tracer = new Tracer();
            m_sampler = new ProfileSampler(M_SHMEM_REGION_SIZE);
        }
    }

    Controller::~Controller()
    {
        if (!m_is_node_root) {
            return;
        }

        m_do_shutdown = true;

        delete m_tracer;
        for (int level = 0; level < m_tree_comm->num_level(); ++level) {
            for (auto it = m_region[level].begin(); it != m_region[level].end(); ++it) {
                delete (*it).second;
            }
            delete m_policy[level];
        }
        for (auto it = m_tree_decider.begin(); it != m_tree_decider.end(); ++it) {
           delete (*it);
        }
        delete m_leaf_decider;
        delete m_decider_factory;
        delete m_platform_factory;
        delete m_tree_comm;
        delete m_sampler;
        delete m_sample_regulator;
    }


    void Controller::connect(void)
    {
        if (!m_is_node_root) {
            return;
        }

        if (!m_is_connected) {
            m_sampler->initialize(m_rank_per_node);
            m_prof_sample.resize(m_sampler->capacity());
            std::vector<int> cpu_rank;
            m_sampler->cpu_rank(cpu_rank);
            m_platform->init_transform(cpu_rank);
            m_sample_regulator = new SampleRegulator(cpu_rank);
            m_is_connected = true;
        }
    }

    void Controller::run(void)
    {
        if (!m_is_node_root) {
            return;
        }

        geopm_signal_handler_register();

        connect();

        geopm_signal_handler_check();

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
            catch (Exception ex) {
                if (ex.err_value() != GEOPM_ERROR_POLICY_UNKNOWN) {
                    throw ex;
                }
                err = 1;
            }
            geopm_signal_handler_check();
        }
        while (err);

        while (!m_do_shutdown) {
            walk_down();
            geopm_signal_handler_check();
            walk_up();
            geopm_signal_handler_check();
        }
        geopm_signal_handler_check();

        reset();
    }

    void Controller::step(void)
    {
        if (!m_is_node_root) {
            return;
        }

        connect();

        walk_down();
        geopm_signal_handler_check();
        walk_up();
        geopm_signal_handler_check();

        if (m_do_shutdown) {
            throw Exception("Controller::step(): Shutdown signaled", GEOPM_ERROR_SHUTDOWN, __FILE__, __LINE__);
        }
    }

    void Controller::pthread(const pthread_attr_t *attr, pthread_t *thread)
    {
        if (!m_is_node_root) {
            return;
        }

        int err = pthread_create(thread, attr, geopm_threaded_run, (void *)this);
        if (err) {
            throw Exception("Controller::pthread(): pthread_create() failed", err, __FILE__, __LINE__);
        }
    }

    void Controller::walk_down(void)
    {
        int level;
        struct geopm_policy_message_s policy_msg;
        std::vector<struct geopm_policy_message_s> child_policy_msg(m_max_fanout);

        level = m_tree_comm->num_level() - 1;
        m_tree_comm->get_policy(level, policy_msg);
        for (; policy_msg.mode != GEOPM_POLICY_MODE_SHUTDOWN && level != 0; --level) {
            if (!geopm_is_policy_equal(&policy_msg, &(m_last_policy_msg[level]))) {
                m_tree_decider[level]->update_policy(policy_msg, *(m_policy[level]));
                m_policy[level]->policy_message(GEOPM_REGION_ID_OUTER, policy_msg, child_policy_msg);
                m_tree_comm->send_policy(level - 1, child_policy_msg);
                m_last_policy_msg[level] = policy_msg;
            }
            m_tree_comm->get_policy(level - 1, policy_msg);
        }
        if (policy_msg.mode == GEOPM_POLICY_MODE_SHUTDOWN) {
            m_do_shutdown = true;
        }
        else {
            // update the leaf level (0)
            if (!geopm_is_policy_equal(&policy_msg, &(m_last_policy_msg[level]))) {
                m_leaf_decider->update_policy(policy_msg, *(m_policy[level]));
                m_tracer->update(policy_msg);
            }
        }
    }

    void Controller::walk_up(void)
    {
        int level;
        struct geopm_sample_message_s sample_msg;
        std::vector<struct geopm_sample_message_s> child_sample(m_max_fanout);
        std::vector<struct geopm_policy_message_s> child_policy_msg(m_max_fanout);
        size_t length;
        struct geopm_time_s loop_t1;

        do {
            geopm_time(&loop_t1);
            geopm_signal_handler_check();
        }
        while (geopm_time_diff(&m_loop_t0, &loop_t1) < m_rate_limit);
        m_loop_t0 = loop_t1;

        for (level = 0; !m_do_shutdown && level < m_tree_comm->num_level(); ++level) {
            if (level) {
                try {
                    m_tree_comm->get_sample(level, child_sample);
                    // use .begin() because map has only one entry
                    auto it = m_region[level].begin();
                    (*it).second->insert(child_sample);
                    if (m_tree_decider[level]->update_policy(*((*it).second), *(m_policy[level]))) {
                       m_policy[level]->policy_message(GEOPM_REGION_ID_OUTER, m_last_policy_msg[level], child_policy_msg);
                       m_tree_comm->send_policy(level - 1, child_policy_msg);
                    }
                    (*it).second->sample_message(sample_msg);
                }
                catch (geopm::Exception ex) {
                    if (ex.err_value() != GEOPM_ERROR_SAMPLE_INCOMPLETE) {
                        throw ex;
                    }
                    break;
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
                // Insert MSR data into platform sample
                std::vector<double> platform_sample(m_msr_sample.size());
                auto output_it = platform_sample.begin();
                for (auto input_it = m_msr_sample.begin(); input_it != m_msr_sample.end(); ++input_it) {
                    *output_it = (*input_it).signal;
                    ++output_it;
                }

                std::vector<double> aligned_signal;
                bool is_outer_found = false;
                // Catch outer sync regions and region entries
                for (auto sample_it = m_prof_sample.cbegin();
                     sample_it != m_prof_sample.cbegin() + length;
                     ++sample_it) {
                    if ((*sample_it).second.progress == 0.0) {
                        auto region_it = m_region[level].find((*sample_it).second.region_id);
                        if (region_it == m_region[level].end()) {
                            auto tmp_it = m_region[level].insert(
                                          std::pair<uint64_t, Region *> ((*sample_it).second.region_id,
                                          new Region((*sample_it).second.region_id,
                                                     GEOPM_POLICY_HINT_UNKNOWN,
                                                     m_platform->num_control_domain(),
                                                     level)));
                            region_it = tmp_it.first;
                        }
                        (*region_it).second->entry();
                    }

                    if (!is_outer_found &&
                        (*sample_it).second.region_id == GEOPM_REGION_ID_OUTER) {
                        uint64_t region_id_all_tmp = m_region_id_all;
                        m_region_id_all = GEOPM_REGION_ID_OUTER;
                        (*m_sample_regulator)(m_msr_sample[0].timestamp,
                                              platform_sample.cbegin(), platform_sample.cend(),
                                              m_prof_sample.cbegin(), m_prof_sample.cbegin(),
                                              aligned_signal,
                                              m_region_id);
                        m_platform->transform_rank_data(m_region_id_all, m_msr_sample[0].timestamp, aligned_signal, m_telemetry_sample);
                        if (m_is_in_outer) {
                            override_telemetry(1.0);
                            update_region();
                            m_tracer->update(m_telemetry_sample);
                        }
                        m_is_in_outer = true;
                        override_telemetry(0.0);
                        update_region();
                        m_tracer->update(m_telemetry_sample);
                        m_region_id_all = region_id_all_tmp;
                        is_outer_found = true;
                    }
                }

                // Align profile data
                (*m_sample_regulator)(m_msr_sample[0].timestamp,
                                      platform_sample.cbegin(), platform_sample.cend(),
                                      m_prof_sample.cbegin(), m_prof_sample.cbegin() + length,
                                      aligned_signal,
                                      m_region_id);

                // Determine if all ranks were last sampled from the same region
                uint64_t region_id_all = m_region_id[0];
                for (auto it = m_region_id.begin(); it != m_region_id.end(); ++it) {
                    if (region_id_all != (*it)) {
                        region_id_all = 0;
                        break;
                    }
                }
                m_platform->transform_rank_data(region_id_all, m_msr_sample[0].timestamp, aligned_signal, m_telemetry_sample);

                bool do_accumulate_mpi = false;
                if (m_region_id_all && !region_id_all) {
                    override_telemetry(1.0);
                    update_region();
                    m_tracer->update(m_telemetry_sample);
                    if (m_region_id_all == GEOPM_REGION_ID_MPI) {
                        do_accumulate_mpi = true;
                    }
                    m_region_id_all = 0;
                    std::fill(m_region_id.begin(), m_region_id.end(), 0);
                }
                else if (!m_region_id_all && region_id_all) {
                    m_region_id_all = region_id_all;
                    override_telemetry(0.0);
                    update_region();
                    m_tracer->update(m_telemetry_sample);
                }
                else if (m_region_id_all && region_id_all &&
                         m_region_id_all != region_id_all) {
                    override_telemetry(1.0);
                    update_region();
                    m_tracer->update(m_telemetry_sample);
                    if (m_region_id_all == GEOPM_REGION_ID_MPI) {
                        do_accumulate_mpi = true;
                    }
                    m_region_id_all = region_id_all;
                    override_telemetry(0.0);
                    std::fill(m_region_id.begin(), m_region_id.end(), m_region_id_all);
                    update_region();
                    m_tracer->update(m_telemetry_sample);
                }
                else { // No entries or exits
                    update_region();
                    m_tracer->update(m_telemetry_sample);
                }
                if (do_accumulate_mpi) {
                    double max_runtime = 0;
                    Region *region_ptr = (*(m_region[level].find(GEOPM_REGION_ID_MPI))).second;
                    for (int i = 0; i < m_platform->num_control_domain(); ++i) {
                         double runtime = region_ptr->signal(i, GEOPM_SAMPLE_TYPE_RUNTIME);
                         max_runtime = max_runtime > runtime ?
                                       max_runtime : runtime;
                    }
                    m_mpi_sync_time += max_runtime;
                }
                // GEOPM_REGION_ID_OUTER is inserted at construction
                auto outer_it = m_region[level].find(GEOPM_REGION_ID_OUTER);
                (*outer_it).second->sample_message(sample_msg);
                // Subtract mpi syncronization time from outer-sync
                if (sample_msg.signal[GEOPM_SAMPLE_TYPE_RUNTIME] != m_outer_sync_time) {
                    m_outer_sync_time = sample_msg.signal[GEOPM_SAMPLE_TYPE_RUNTIME];
                    m_is_outer_changed = true;
                    sample_msg.signal[GEOPM_SAMPLE_TYPE_RUNTIME] -= m_mpi_sync_time;
                }
                if (is_outer_found) {
                    m_mpi_sync_time = 0.0;
                }
                m_do_shutdown = m_sampler->do_shutdown();
            }
            if (level != m_tree_comm->root_level() &&
                m_policy[level]->is_converged(m_region_id_all) &&
                m_is_outer_changed) {
                m_tree_comm->send_sample(level, sample_msg);
                m_last_sample_msg[level] = sample_msg;
                m_is_outer_changed = false;
            }
        }
        if (m_do_shutdown && m_sampler->do_report()) {
            generate_report();
        }
    }

    void Controller::override_telemetry(double progress)
    {
        for (auto it = m_telemetry_sample.begin(); it != m_telemetry_sample.end(); ++it) {
            (*it).region_id = m_region_id_all;
            (*it).signal[GEOPM_TELEMETRY_TYPE_PROGRESS] = progress;
            (*it).signal[GEOPM_TELEMETRY_TYPE_RUNTIME] = 0.0;
        }
    }

    void Controller::update_region(void)
    {
        if (m_region_id_all) {
            int level = 0; // Called only at the leaf
            auto it = m_region[level].find(m_region_id_all);
            if (it == m_region[level].end()) {
                auto tmp_it = m_region[level].insert(
                                  std::pair<uint64_t, Region *> (m_region_id_all,
                                          new Region(m_region_id_all,
                                                     GEOPM_POLICY_HINT_UNKNOWN,
                                                     m_platform->num_control_domain(),
                                                     level)));
                it = tmp_it.first;
            }
            Region *curr_region = (*it).second;
            Policy *curr_policy = m_policy[level];
            curr_region->insert(m_telemetry_sample);
            if (m_region_id_all != GEOPM_REGION_ID_OUTER &&
                m_leaf_decider->update_policy(*curr_region, *curr_policy) == true) {
                m_platform->enforce_policy(m_region_id_all, *curr_policy);
            }
        }
    }

    void Controller::enforce_child_policy(int level, const Policy &policy) /// @todo this method is *never* called
    {
        if (!m_is_node_root) {
            return;
        }

        std::vector<geopm_policy_message_s> child_msg(m_policy[level]->num_domain());
        if (level) {
            m_policy[level]->policy_message(GEOPM_REGION_ID_OUTER,
                                            m_last_policy_msg[level],
                                            child_msg);
        }
        else {
            m_policy[level]->policy_message(m_region_id_all, m_last_policy_msg[level], child_msg);
        }
        m_tree_comm->send_policy(level, child_msg);
    }

    void Controller::generate_report(void)
    {
        if (!m_is_node_root || !m_is_connected) {
            return;
        }

        if (m_is_in_outer) {
            m_region_id_all = GEOPM_REGION_ID_OUTER;
            override_telemetry(1.0);
            update_region();
            m_tracer->update(m_telemetry_sample);
        }

        std::string report_name;
        std::string profile_name;
        std::set<std::string> region_name;
        std::map<uint64_t, std::string> region;
        std::ofstream report;
        char hostname[NAME_MAX];

        m_sampler->report_name(report_name);
        m_sampler->profile_name(profile_name);
        m_sampler->name_set(region_name);

        if (report_name.empty() || profile_name.empty()) {
            throw Exception("Controller::generate_report(): Invalid report data", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // create a map from region_id to name
        for (auto it = region_name.begin(); it != region_name.end(); ++it) {
            uint64_t region_id = geopm_crc32_str(0, (*it).c_str());
            region.insert(std::pair<uint64_t, std::string>(region_id, (*it)));
        }

        gethostname(hostname, NAME_MAX);
        report.open(report_name + "-" + std::string(hostname), std::ios_base::out);
        report << "##### geopm " << geopm_version() << " #####" << std::endl << std::endl;
        report << "Profile: " << profile_name << std::endl;
        for (auto it = m_region[0].begin(); it != m_region[0].end(); ++it) {
            uint64_t region_id = (*it).first;
            std::string name;
            if (region_id == GEOPM_REGION_ID_MPI) {
                name = "mpi-sync";
            }
            else if (region_id == GEOPM_REGION_ID_OUTER) {
                name = "outer-sync";
            }
            else if (region_id == 0) {
                name = "unmarked region";
            }
            else {
                auto region_it = region.find(region_id);
                if (region_it != region.end()) {
                    name = (*region_it).second;
                }
                else {
                    throw Exception("Controller::generate_report(): Invalid region", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            (*it).second->report(report, name, m_rank_per_node);
        }
        report.close();
    }

    void Controller::reset(void)
    {
        geopm_error_destroy_shmem();
        m_platform->revert_msr_state();
    }
}
