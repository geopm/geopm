/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "Controller.hpp"

#include <cmath>
#include <climits>

#include <algorithm>

#include "ApplicationIO.hpp"
#include "Environment.hpp"
#include "Reporter.hpp"
#include "Tracer.hpp"
#include "EndpointPolicyTracer.hpp"
#include "geopm/Exception.hpp"
#include "Comm.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PlatformIO.hpp"
#include "Agent.hpp"
#include "TreeComm.hpp"
#include "EndpointUser.hpp"
#include "FilePolicy.hpp"
#include "geopm/Helper.hpp"
#include "ProfileTracer.hpp"
#include "ApplicationSampler.hpp"
#include "record.hpp"
#include "PlatformIOProf.hpp"

#include "ProfileIOGroup.hpp"

extern "C"
{
    static int geopm_run_imp(struct geopm_ctl_c *ctl);
    /// forward declaration instead of include geopm_ctl.h to avoid mpi.h inclusion
    int geopm_ctl_run(struct geopm_ctl_c *ctl);

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
            ctl_obj->abort();
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }

    static void *geopm_threaded_run(void *args)
    {
        return (void *) (long) geopm_run_imp((struct geopm_ctl_c *)args);
    }

    int geopmctl_main(void)
    {
        int err = 0;
        try {
            geopm::Controller ctl;
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
        geopm::Controller *ctl_obj = (geopm::Controller *)ctl;
        try {
            delete ctl_obj;
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }

    static int geopm_run_imp(struct geopm_ctl_c *ctl)
    {
        int err = 0;
        geopm::Controller *ctl_obj = (geopm::Controller *)ctl;
        try {
            ctl_obj->run();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }

    int geopm_ctl_run(struct geopm_ctl_c *ctl)
    {
        return geopm_run_imp(ctl);
    }

    int geopm_agent_enforce_policy(void)
    {
        int err = 0;
        try {
            std::string agent_name = geopm::environment().agent();
            std::shared_ptr<geopm::Agent> agent(geopm::Agent::make_unique(agent_name));
            std::vector<double> policy(geopm::Agent::num_policy(agent_name));
            std::string policy_path = geopm::environment().policy();
            geopm::FilePolicy file_policy(policy_path,
                                          geopm::Agent::policy_names(agent_name));
            policy = file_policy.get_policy();
            agent->validate_policy(policy);
            agent->enforce_policy(policy);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }
}

namespace geopm
{
    static std::string get_start_time()
    {
        static bool once = true;
        static std::string ret;

        if (once) {
            const int buf_size = 64;
            char time_buff[buf_size];
            geopm_time_string(buf_size, time_buff);
            std::string tmp(time_buff);
            tmp.erase(std::remove(tmp.begin(), tmp.end(), '\n'), tmp.end());
            ret = tmp;
            once = false;
        }
        return ret;
    }

    Controller::Controller()
        : Controller(Comm::make_unique())
    {

    }

    Controller::Controller(std::shared_ptr<Comm> ppn1_comm)
        : Controller(ppn1_comm,
                     PlatformIOProf::platform_io(),
                     environment().agent(),
                     Agent::num_policy(environment().agent()),
                     Agent::num_sample(environment().agent()),
                     geopm::make_unique<TreeCommImp>(
                         ppn1_comm,
                         Agent::num_policy(environment().agent()),
                         Agent::num_sample(environment().agent())),
                     ApplicationSampler::application_sampler(),
                     std::make_shared<ApplicationIOImp>(),
                     geopm::make_unique<ReporterImp>(
                         get_start_time(),
                         environment().report(),
                         PlatformIOProf::platform_io(),
                         platform_topo(),
                         ppn1_comm->rank()),
                     nullptr,
                     std::unique_ptr<EndpointPolicyTracer>(nullptr),
                     ProfileTracer::make_unique(get_start_time()),
                     std::vector<std::unique_ptr<Agent> >{},
                     Agent::policy_names(environment().agent()),
                     environment().policy(),
                     environment().do_policy(),
                     nullptr,
                     environment().endpoint(),
                     environment().do_endpoint(),
                     ApplicationSampler::default_shmkey())
    {

    }
    Controller::Controller(std::shared_ptr<Comm> comm,
                           PlatformIO &plat_io,
                           const std::string &agent_name,
                           int num_send_down,
                           int num_send_up,
                           std::unique_ptr<TreeComm> tree_comm,
                           ApplicationSampler &application_sampler,
                           std::shared_ptr<ApplicationIO> application_io,
                           std::unique_ptr<Reporter> reporter,
                           std::unique_ptr<Tracer> tracer,
                           std::unique_ptr<EndpointPolicyTracer> policy_tracer,
                           std::shared_ptr<ProfileTracer> profile_tracer,
                           std::vector<std::unique_ptr<Agent> > level_agent,
                           std::vector<std::string> policy_names,
                           const std::string &policy_path,
                           bool do_policy,
                           std::unique_ptr<EndpointUser> endpoint,
                           const std::string &endpoint_path,
                           bool do_endpoint,
                           const std::string &shm_key)
        : m_comm(comm)
        , m_platform_io(plat_io)
        , m_agent_name(agent_name)
        , m_num_send_down(num_send_down)
        , m_num_send_up(num_send_up)
        , m_tree_comm(std::move(tree_comm))
        , m_num_level_ctl(m_tree_comm->num_level_controlled())
        , m_max_level(m_num_level_ctl + 1)
        , m_root_level(m_tree_comm->root_level())
        , m_application_sampler(application_sampler)
        , m_application_io(std::move(application_io))
        , m_reporter(std::move(reporter))
        , m_tracer(std::move(tracer))
        , m_policy_tracer(std::move(policy_tracer))
        , m_profile_tracer(std::move(profile_tracer))
        , m_agent(std::move(level_agent))
        , m_is_root(m_num_level_ctl == m_root_level)
        , m_in_policy(m_num_send_down, NAN)
        , m_last_policy(m_num_send_down, NAN)
        , m_out_policy(m_num_level_ctl)
        , m_in_sample(m_num_level_ctl)
        , m_out_sample(m_num_send_up, NAN)
        , m_endpoint(std::move(endpoint))
        , m_do_endpoint(do_endpoint)
        , m_do_policy(do_policy)
        , m_shm_key(shm_key)
    {
        if (m_num_send_down > 0 && !(m_do_policy || m_do_endpoint)) {
            throw Exception("Controller(): at least one of policy or endpoint path"
                            " must be provided.", GEOPM_ERROR_INVALID,
                            __FILE__, __LINE__);
        }
        // Three dimensional vector over levels, children, and message
        // index.  These are used as temporary storage when passing
        // messages up and down the tree.
        for (int level = 0; level != m_num_level_ctl; ++level) {
            int num_children = m_tree_comm->level_size(level);
            m_out_policy[level] = std::vector<std::vector<double> >(num_children,
                                                                    std::vector<double>(m_num_send_down, NAN));
            m_in_sample[level] = std::vector<std::vector<double> >(num_children,
                                                                   std::vector<double>(m_num_send_up, NAN));
        }
        if (m_do_endpoint && m_endpoint == nullptr) {
            m_endpoint = EndpointUser::make_unique(endpoint_path, get_hostnames(hostname()));
        }
        else if (m_do_policy && !m_do_endpoint) {
            m_file_policy = geopm::make_unique<FilePolicy>(policy_path, policy_names);
            m_in_policy = m_file_policy->get_policy();
        }
        if (m_do_endpoint && m_policy_tracer == nullptr) {
            m_policy_tracer = EndpointPolicyTracer::make_unique();
        }
    }

    Controller::~Controller()
    {
        m_platform_io.restore_control();
    }

    void Controller::create_agents(void)
    {
        if (m_agent.size() == 0) {
            for (int level = 0; level < m_max_level; ++level) {
                m_agent.push_back(Agent::make_unique(m_agent_name));
            }
        }
#ifdef GEOPM_DEBUG
        if (m_agent.size() == 0) {
            throw Exception("Controller requires at least one Agent",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (m_max_level != (int)m_agent.size()) {
            throw Exception("Controller number of agents is incorrect",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
    }

    void Controller::init_agents(void)
    {
#ifdef GEOPM_DEBUG
        if (m_agent.size() != (size_t)m_max_level) {
            throw Exception("Controller must call create_agents() before init_agents().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        std::vector<int> fan_in(m_tree_comm->root_level());
        int level = 0;
        for (auto &it : fan_in) {
            it = m_tree_comm->level_size(level);
            ++level;
        }
        for (level = 0; level < m_max_level; ++level) {
            m_agent[level]->init(level, fan_in, (level < m_tree_comm->num_level_controlled()));
        }
    }

    std::set<std::string> Controller::get_hostnames(const std::string &hostname)
    {
        std::set<std::string> hostnames;
        int num_rank = m_comm->num_rank();
        int rank = m_comm->rank();
        // resize hostname string to fixed size buffer
        std::string temp = hostname;
        temp.resize(NAME_MAX, 0);
        std::vector<char> name_buffer(num_rank * NAME_MAX, 0);
        m_comm->gather((void*)temp.c_str(), NAME_MAX,
                     (void*)name_buffer.data(), NAME_MAX, 0);
        if (rank == 0) {
            auto ind = name_buffer.begin();
            for (int rr = 0; rr < num_rank; ++rr) {
                auto term = std::find(ind, ind + NAME_MAX, '\0');
                std::string host(ind, term);
                hostnames.insert(host);
                ind += NAME_MAX;
            }
        }
        return hostnames;
    }

    void Controller::run(void)
    {
        m_application_io->connect();
        m_application_sampler.connect(m_shm_key);

        create_agents();
        m_platform_io.save_control();
        init_agents();
        m_reporter->init();
        setup_trace();
        m_application_io->controller_ready();
        geopm_time_s curr_time;
        geopm_time(&curr_time);
        m_application_sampler.update(curr_time);
        m_platform_io.read_batch();
        m_reporter->update();
        m_tracer->update(m_trace_sample);
        m_profile_tracer->update(m_application_sampler.get_records());

        while (!m_application_io->do_shutdown()) {
            step();
        }
        geopm_time(&curr_time);
        m_application_sampler.update(curr_time);
        m_platform_io.read_batch();
        m_reporter->update();
        m_tracer->update(m_trace_sample);
        m_profile_tracer->update(m_application_sampler.get_records());
        generate();
        m_platform_io.restore_control();
    }

    void Controller::generate(void)
    {
        std::vector<std::pair<std::string, std::string> > agent_report_header;
        if (m_is_root) {
            agent_report_header = m_agent[m_root_level]->report_header();
        }

        auto agent_host_report = m_agent[0]->report_host();

        m_reporter->generate(m_agent_name,
                             agent_report_header,
                             agent_host_report,
                             m_agent[0]->report_region(),
                             *m_application_io,
                             m_comm,
                             *m_tree_comm);
        m_tracer->flush();
    }

    void Controller::step(void)
    {
        walk_down();
        m_agent[0]->wait();
        walk_up();
    }

    void Controller::walk_down(void)
    {
        bool do_send = false;
        if (m_is_root) {
            if (m_do_endpoint) {
                (void) m_endpoint->read_policy(m_in_policy);
                bool equal = std::equal(m_in_policy.begin(), m_in_policy.end(),
                                        m_last_policy.begin(),
                                        [] (double a, double b) -> bool {
                                            if (std::isnan(a) && std::isnan(b)) {
                                                return true;
                                            }
                                            return a == b;
                                        });
                if (!equal) {
                    m_policy_tracer->update(m_in_policy);
                    m_last_policy = m_in_policy;
                    do_send = true;
                }
                else {
                    do_send = false;
                }
            }
            else if (m_do_policy) {
                m_in_policy = m_file_policy->get_policy();
                do_send = true;
            }
        }
        else {
            do_send = m_tree_comm->receive_down(m_num_level_ctl, m_in_policy);
        }
        for (int level = m_num_level_ctl - 1; level > -1; --level) {
            if (do_send) {
                m_agent[level + 1]->validate_policy(m_in_policy);
                m_agent[level + 1]->split_policy(m_in_policy, m_out_policy[level]);
                do_send = m_agent[level + 1]->do_send_policy();
            }
            if (do_send) {
                m_tree_comm->send_down(level, m_out_policy[level]);
            }
            do_send = m_tree_comm->receive_down(level, m_in_policy);
        }
        m_agent[0]->validate_policy(m_in_policy);
        m_agent[0]->adjust_platform(m_in_policy);
        if (m_agent[0]->do_write_batch()) {
            m_platform_io.write_batch();
        }
    }

    void Controller::walk_up(void)
    {
        geopm_time_s curr_time;
        geopm_time(&curr_time);
        m_application_sampler.update(curr_time);
        m_platform_io.read_batch();
        m_agent[0]->sample_platform(m_out_sample);
        bool do_send = m_agent[0]->do_send_sample();
        m_reporter->update();
        m_agent[0]->trace_values(m_trace_sample);
        m_tracer->update(m_trace_sample);
        m_profile_tracer->update(m_application_sampler.get_records());

        for (int level = 0; level < m_num_level_ctl; ++level) {
            if (do_send) {
                m_tree_comm->send_up(level, m_out_sample);
            }
            do_send = m_tree_comm->receive_up(level, m_in_sample[level]);
            if (do_send) {
                m_agent[level + 1]->aggregate_sample(m_in_sample[level], m_out_sample);
                do_send = m_agent[level + 1]->do_send_sample();
            }
        }
        if (do_send) {
            if (!m_is_root) {
                m_tree_comm->send_up(m_num_level_ctl, m_out_sample);
            }
            else {
                if (m_do_endpoint) {
                    m_endpoint->write_sample(m_out_sample);
                }
            }
        }
    }

    void Controller::pthread(const pthread_attr_t *attr, pthread_t *thread)
    {
        int err = pthread_create(thread, attr, geopm_threaded_run, (void *)this);
        if (err) {
            throw Exception("Controller::pthread(): pthread_create() failed",
                            err, __FILE__, __LINE__);
        }
    }

    void Controller::setup_trace(void)
    {
        if (m_tracer == nullptr) {
            m_tracer = geopm::make_unique<TracerImp>(get_start_time());
        }
        std::vector<std::string> agent_cols = m_agent[0]->trace_names();
        std::vector<std::function<std::string(double)> > agent_formats = m_agent[0]->trace_formats();
        m_tracer->columns(agent_cols, agent_formats);
        m_trace_sample.resize(agent_cols.size());
    }

    void Controller::abort(void)
    {
        m_application_io->abort();
        m_platform_io.restore_control();
    }
}
