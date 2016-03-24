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
        , m_region_id(GEOPM_REGION_ID_INVALID)
        , m_ctl_status(GEOPM_STATUS_UNDEFINED)
        , m_teardown(false)
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

            struct geopm_plugin_description_s plugin_desc;
            int world_rank;
            check_mpi(MPI_Comm_rank(ppn1_comm, &world_rank));
            if (!world_rank) { // We are the root of the tree
                plugin_desc.tree_decider[NAME_MAX - 1] = '\0';
                plugin_desc.leaf_decider[NAME_MAX - 1] = '\0';
                plugin_desc.platform[NAME_MAX - 1] = '\0';
                strncpy(plugin_desc.tree_decider, m_global_policy->tree_decider().c_str(), NAME_MAX -1);
                strncpy(plugin_desc.leaf_decider, m_global_policy->leaf_decider().c_str(), NAME_MAX -1);
                strncpy(plugin_desc.platform, m_global_policy->platform().c_str(), NAME_MAX -1);
            }
            check_mpi(MPI_Bcast(&plugin_desc, sizeof(plugin_desc), MPI_CHAR, 0, ppn1_comm));

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
            m_policy.resize(num_level);
            m_tree_decider.resize(num_level);
            m_last_policy_msg.resize(num_level);
            std::fill(m_last_policy_msg.begin(), m_last_policy_msg.end(), GEOPM_POLICY_UNKNOWN);

            m_platform_factory = new PlatformFactory;
            m_platform = m_platform_factory->platform(plugin_desc.platform);

            m_msr_sample.resize(m_platform->capacity());

            m_decider_factory = new DeciderFactory;
            m_leaf_decider = m_decider_factory->decider(std::string(plugin_desc.leaf_decider));

            int num_domain;
            for (int level = 0; level < num_level; ++level) {
                if (level == 0) {
                    num_domain = m_platform->num_control_domain();
                    m_telemetry_sample.resize(num_domain);
                }
                else {
                    num_domain = m_tree_comm->level_size(level - 1);
                }
                m_policy[level] = new Policy(num_domain);
                m_tree_decider[level] = m_decider_factory->decider(std::string(plugin_desc.tree_decider));
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
            m_ctl_status = GEOPM_STATUS_INITIALIZED;
        }
    }

    Controller::~Controller()
    {
        if (m_is_node_root) {
            m_teardown = true;
            while (m_ctl_status != GEOPM_STATUS_SHUTDOWN) {}
        }
        delete m_tree_comm;
        delete m_platform_factory;
        delete m_decider_factory;
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
        m_ctl_status = GEOPM_STATUS_ACTIVE;

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
        int do_shutdown = 0;
        struct geopm_policy_message_s policy_msg;
        std::vector<struct geopm_policy_message_s> child_policy_msg(m_max_fanout);

        level = m_tree_comm->num_level() - 1;
        m_tree_comm->get_policy(level, policy_msg);
        for (; policy_msg.mode != GEOPM_MODE_SHUTDOWN && level != 0; --level) {
            if (!geopm_is_policy_equal(&policy_msg, &(m_last_policy_msg[level]))) {
                m_tree_decider[level]->update_policy(policy_msg, *(m_policy[level]));
                m_policy[level]->policy_message(GEOPM_REGION_ID_OUTER, policy_msg, child_policy_msg);
                m_tree_comm->send_policy(level - 1, child_policy_msg);
                m_last_policy_msg[level] = policy_msg;
            }
            m_tree_comm->get_policy(level - 1, policy_msg);
        }
        if (policy_msg.mode == GEOPM_MODE_SHUTDOWN || m_teardown == true) {
            m_ctl_status = GEOPM_STATUS_SHUTDOWN;
            do_shutdown = 1;
        }
        else {
            // update the leaf level (0)
            if (!geopm_is_policy_equal(&policy_msg, &(m_last_policy_msg[level]))) {
                m_leaf_decider->update_policy(policy_msg, *(m_policy[level]));
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
        size_t length;

        for (level = 0; level < m_tree_comm->num_level(); ++level) {
            if (level) {
                try {
                    m_tree_comm->get_sample(level, child_sample);
                    // use .begin() because map has only one entry
                    auto it = m_region[level].begin();
                    (*it).second->insert(child_sample);
                    m_tree_decider[level]->update_policy(*((*it).second), *(m_policy[level]));
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
                std::vector<double> platform_sample(m_msr_sample.size());
                auto output_it = platform_sample.begin();
                for (auto input_it = m_msr_sample.begin(); input_it != m_msr_sample.end(); ++input_it) {
                    *output_it = (*input_it).signal;
                    ++output_it;
                }
                auto prof_begin = m_prof_sample.cbegin();
                auto prof_end = m_prof_sample.cbegin();
                uint64_t region = (*prof_end).first;
                while (prof_end != (m_prof_sample.cbegin() + length)) {
                    while (prof_end != (m_prof_sample.cbegin() + length) && (*(prof_end)).first == region) {
                        ++prof_end;
                    }
                    std::vector<double> aligned_signal;
                    (*m_sample_regulator)(m_msr_sample[0].timestamp,
                                          platform_sample.cbegin(), platform_sample.cend(),
                                          prof_begin, prof_end,
                                          aligned_signal);

                    m_platform->transform_rank_data((*prof_begin).first, m_msr_sample[0].timestamp, aligned_signal, m_telemetry_sample);

                    m_region_id = m_telemetry_sample[0].region_id;
                    auto it = m_region[level].find(m_region_id);
                    if (it != m_region[level].end()) {
                        Region *curr_region = (*it).second;
                        Policy *curr_policy = m_policy[level];
                        curr_region->insert(m_telemetry_sample);
                        if (m_leaf_decider->update_policy(*curr_region, *curr_policy) == true) {
                            m_platform->enforce_policy(m_region_id, *curr_policy);
                        }
                    }
                    else {
                        m_region[level].insert(std::pair<uint64_t, Region *>
                                               (m_region_id,
                                                new Region(m_region_id,
                                                           GEOPM_POLICY_HINT_UNKNOWN,
                                                           m_platform->num_control_domain(),
                                                           level)));
                        auto it = m_region[level].find(m_region_id);
                        Region *curr_region = (*it).second;
                        Policy *curr_policy = m_policy[level];
                        curr_region->insert(m_telemetry_sample);
                        if (m_leaf_decider->update_policy(*curr_region, *curr_policy) == true) {
                            m_platform->enforce_policy(m_region_id, *curr_policy);
                        }
                    }
                    if (prof_end != (m_prof_sample.cbegin() + length)) {
                        region = (*prof_end).first;
                        prof_begin = prof_end;
                    }
                }
                auto outer_it = m_region[level].find(GEOPM_REGION_ID_OUTER);
                if (outer_it != m_region[level].end()) {
                    (*outer_it).second->sample_message(sample_msg);
                    // Subtract mpi syncronization time from outer-sync
                    if (!level) {
                        auto mpi_it = m_region[level].find(GEOPM_REGION_ID_MPI);
                        if (mpi_it != m_region[level].end()) {
                            struct geopm_sample_message_s mpi_sample;
                            (*outer_it).second->sample_message(mpi_sample);
                            sample_msg.signal[GEOPM_SAMPLE_TYPE_RUNTIME] -= mpi_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME];
                        }
                    }
                }
                do_shutdown = m_sampler->do_shutdown();
            }
            if (level != m_tree_comm->root_level() &&
                m_policy[level]->is_converged(m_region_id)) {
                m_tree_comm->send_sample(level, sample_msg);
            }
        }
        if (policy_msg.mode == GEOPM_MODE_SHUTDOWN || m_teardown == true) {
            do_shutdown = 1;
        }
        if (do_shutdown) {
            if (m_sampler->do_report()) {
                generate_report();
            }
            m_ctl_status = GEOPM_STATUS_SHUTDOWN;
        }
        return do_shutdown;
    }

    void Controller::enforce_child_policy(int level, const Policy &policy)
    {
        std::vector<geopm_policy_message_s> child_msg(m_policy[level]->num_domain());
        if (level) {
            m_policy[level]->policy_message(GEOPM_REGION_ID_OUTER,
                                            m_last_policy_msg[level],
                                            child_msg);
        }
        else {
            m_policy[level]->policy_message(m_region_id, m_last_policy_msg[level], child_msg);
        }
        m_tree_comm->send_policy(level, child_msg);
    }

    void Controller::generate_report(void)
    {
        std::string report_name;
        std::string profile_name;
        std::set<std::string> region_name;
        std::map<uint64_t, std::string> region;
        std::ofstream report;
        char hostname[NAME_MAX];

        m_sampler->report_name(report_name);
        m_sampler->profile_name(profile_name);
        m_sampler->name_set(region_name);

        if (report_name.empty() || profile_name.empty() || region_name.empty()) {
            throw Exception("Controller::generate_report(): Invalid report data", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // create a map from region_id to name
        for (auto it = region_name.begin(); it != region_name.end(); ++it) {
            uint64_t region_id = geopm_crc32_str(0, (*it).c_str());
            region.insert(std::pair<uint64_t, std::string>(region_id, (*it)));
        }

        gethostname(hostname, NAME_MAX);
        report.open(report_name + "_" + std::string(hostname), std::ios_base::out);
        report << "Profile: " << profile_name << std::endl;
        for (auto it = m_region[0].begin(); it != m_region[0].end(); ++it) {
            uint64_t region_id = (*it).first;
            std::string name;
            if (region_id == GEOPM_REGION_ID_MPI) {
                name.assign("mpi-sync");
            }
            else if (region_id == GEOPM_REGION_ID_OUTER) {
                name.assign("outer-sync");
            }
            else if (region_id == 0) {
                name.assign("unmarked region");
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
            (*it).second->report(report, name);
        }
        report.close();
    }
}
