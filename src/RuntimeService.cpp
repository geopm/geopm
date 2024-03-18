/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "RuntimeService.hpp"

#include <cmath>
#include <memory>
#include <vector>
#include <string>
#include <map>

#include <grpcpp/grpcpp.h>
#include <pthread.h>

#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm_version.h"
#include "geopm/SharedMemoryScopedLock.hpp"
#include "Waiter.hpp"
#include "geopm_runtime.grpc.pb.h"


namespace geopm {
    class RuntimeStats;
    class RuntimePolicy;

    struct policy_struct_s {
        pthread_mutex_t mutex;
        bool is_updated;
        std::shared_ptr<RuntimePolicy> policy;
        std::shared_ptr<RuntimeStats> stats;
    };

    class RuntimeServiceImp final : public GEOPMRuntime::Service
    {
        public:
            static constexpr double POLICY_LATENCY = 5e-3; // 5 millisec sleep while waiting for a policy
            RuntimeServiceImp() = delete;
            RuntimeServiceImp(policy_struct_s &policy_struct);
            virtual ~RuntimeServiceImp() = default;
            ::grpc::Status SetPolicy(::grpc::ServerContext* context,
                                     const ::Policy* request,
                                     ::Policy* response) override;
            ::grpc::Status GetReport(::grpc::ServerContext* context,
                                     const ::ReportRequest* request,
                                     ::ReportList* response) override;
            ::grpc::Status AddChildHost(::grpc::ServerContext* context,
                                        const ::Url* request,
                                        ::TimeSpec* response) override;
            ::grpc::Status RemoveChildHost(::grpc::ServerContext* context,
                                           const ::Url* request,
                                           ::TimeSpec* response) override;
        private:
            policy_struct_s &m_policy_struct;
    };

    class RuntimePolicy
    {
        public:
            RuntimePolicy();
            RuntimePolicy(const std::string &agent, double period, const std::string &profile, const std::map<std::string, double> &params);
            RuntimePolicy(const RuntimePolicy &other);
            virtual ~RuntimePolicy() = default;
            const std::string m_agent;
            const double m_period;
            const std::string m_profile;
            const std::map<std::string, double> m_params;
    };

    RuntimePolicy::RuntimePolicy()
        : RuntimePolicy("", RuntimeServiceImp::POLICY_LATENCY, "", {})
    {

    }

    RuntimePolicy::RuntimePolicy(const std::string &agent, double period, const std::string &profile, const std::map<std::string, double> &params)
        : m_agent(agent)
        , m_period(period)
        , m_profile(profile)
        , m_params(params)
    {

    }

    RuntimePolicy::RuntimePolicy(const RuntimePolicy &other)
        : RuntimePolicy(other.m_agent, other.m_period, other.m_profile, other.m_params)
    {

    }

    class RuntimeStats
    {
        public:
            RuntimeStats() = default;
            RuntimeStats(const std::vector<std::string> &metric_names);
            virtual ~RuntimeStats() = default;
            int num_metric(void) const;
            std::string metric_name(int metric_idx) const;
            uint64_t count(int metric_idx) const;
            double first(int metric_idx) const;
            double last(int metric_idx) const;
            double min(int metric_idx) const;
            double max(int metric_idx) const;
            double mean(int metric_idx) const;
            double std(int metric_idx) const;
            double skew(int metric_idx) const;
            double kurt(int metric_idx) const;
            double lse_linear_0(int metric_idx) const;
            double lse_linear_1(int metric_idx) const;
            void reset(void);
            void update(const std::vector<double> &sample);
        private:
            void check_index(int metric_idx, const std::string &func, int line) const;
            struct stats_s {
                stats_s &operator=(const stats_s &other);
                uint64_t count;
                double first;
                double last;
                double min;
                double max;
                double m_1;
                double m_2;
                double m_3;
                double m_4;
            };
            const std::vector<std::string> m_metric_names;
            std::vector<stats_s> m_moments;
    };

    RuntimeStats::RuntimeStats(const std::vector<std::string> &metric_names)
        : m_metric_names(metric_names)
        , m_moments(m_metric_names.size())
    {
        reset();
    }

    int RuntimeStats::num_metric(void) const
    {
        return m_metric_names.size();
    }

    void RuntimeStats::check_index(int metric_idx, const std::string &func, int line) const
    {
        if (metric_idx < 0 || (size_t)metric_idx >= m_metric_names.size()) {
            throw Exception("RuntimeStats::" + func  + "(): metric_idx out of range: " + std::to_string(metric_idx),
                            GEOPM_ERROR_INVALID, __FILE__, line);
        }
    }

    std::string RuntimeStats::metric_name(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        return m_metric_names[metric_idx];
    }

    uint64_t RuntimeStats::count(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        return m_moments[metric_idx].count;
    }

    double RuntimeStats::first(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].first;
        }
        return result;
    }

    double RuntimeStats::last(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].last;
        }
        return result;
    }

    double RuntimeStats::min(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].min;
        }
        return result;
    }

    double RuntimeStats::max(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].max;
        }
        return result;
    }

    double RuntimeStats::mean(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].m_1 /
                     m_moments[metric_idx].count;
        }
        return result;
    }

    double RuntimeStats::std(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count > 1) {
            result = std::sqrt(
                         (m_moments[metric_idx].m_2 -
                          m_moments[metric_idx].m_1 *
                          m_moments[metric_idx].m_1) /
                         (m_moments[metric_idx].count - 1));
        }
        return result;
    }

    double RuntimeStats::skew(int metric_idx) const
    {
        throw Exception("RuntimeStats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double RuntimeStats::kurt(int metric_idx) const
    {
        throw Exception("RuntimeStats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double RuntimeStats::lse_linear_0(int metric_idx) const
    {
        throw Exception("RuntimeStats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double RuntimeStats::lse_linear_1(int metric_idx) const
    {
        throw Exception("RuntimeStats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void RuntimeStats::reset(void)
    {
        for (auto &it : m_moments) {
            it.count = 0;
            it.m_1 = 0.0;
            it.m_2 = 0.0;
            it.m_3 = 0.0;
            it.m_4 = 0.0;
        }
    }

    void RuntimeStats::update(const std::vector<double> &sample)
    {
        if (sample.size() != m_moments.size()) {
            throw Exception("RuntimeStats::update(): invalid input vector size: " + std::to_string(sample.size()),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto moments_it = m_moments.begin();
        for (const auto &ss : sample) {
            if (platform_io().is_valid_value(ss)) {
                moments_it->count += 1;
                if (moments_it->count == 1) {
                    moments_it->first = ss;
                    moments_it->min = ss;
                    moments_it->max = ss;
                }
                moments_it->last = ss;
                if (moments_it->min > ss) {
                    moments_it->min = ss;
                }
                if (moments_it->max < ss) {
                    moments_it->max = ss;
                }
                double mm = ss;
                moments_it->m_1 += mm;
                mm *= ss;
                moments_it->m_2 += mm;
                mm *= ss;
                moments_it->m_3 += mm;
                mm *= ss;
                moments_it->m_4 += mm;
                ++moments_it;
            }
        }
    }


    class RuntimeAgent
    {
        public:
            static std::unique_ptr<RuntimeStats> make_stats(const std::string &agent_name);
            static std::unique_ptr<RuntimeAgent> make_agent(const RuntimePolicy &policy);
            RuntimeAgent() = default;
            virtual ~RuntimeAgent() = default;
            virtual std::string name(void) const = 0;
            virtual double period(void) const = 0;
            virtual std::string profile(void) const = 0;
            virtual std::map<std::string, double> params(void) const = 0;
            virtual std::vector<double> update(void) = 0;
    };

    class NullRuntimeAgent : public RuntimeAgent
    {
        public:
            static std::vector<std::string> metric_names(void);
            NullRuntimeAgent() = delete;
            NullRuntimeAgent(const RuntimePolicy &policy);
            virtual ~NullRuntimeAgent() = default;
            std::string name(void) const override;
            double period(void) const override;
            std::string profile(void) const override;
            std::map<std::string, double> params(void) const override;
            std::vector<double> update(void) override;
        private:
            const RuntimePolicy m_policy;
    };

    std::vector<std::string> NullRuntimeAgent::metric_names(void)
    {
        return {};
    }


    NullRuntimeAgent::NullRuntimeAgent(const RuntimePolicy &policy)
        : m_policy(policy)
    {
        if (m_policy.m_agent != "") {
            throw Exception("NullRuntimeAgent: policy is defined for different agent: " + m_policy.m_agent,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_policy.m_params.empty()) {
            throw Exception("NullRuntimeAgent: policy parameters are not empty: ",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    std::string NullRuntimeAgent::name(void) const
    {
        return "";
    }

    double NullRuntimeAgent::period(void) const
    {
        return m_policy.m_period;
    }

    std::string NullRuntimeAgent::profile(void) const
    {
        return m_policy.m_profile;
    }

    std::map<std::string, double> NullRuntimeAgent::params(void) const
    {
        return {};
    }

    std::vector<double> NullRuntimeAgent::update(void)
    {
        return {};
    }



    class MonitorRuntimeAgent : public RuntimeAgent
    {
        public:
            static std::vector<std::string> metric_names(void);
            MonitorRuntimeAgent() = delete;
            MonitorRuntimeAgent(const RuntimePolicy &policy);
            virtual ~MonitorRuntimeAgent() = default;
            std::string name(void) const override;
            double period(void) const override;
            std::string profile(void) const override;
            std::map<std::string, double> params(void) const override;
            std::vector<double> update(void) override;
        private:
            const RuntimePolicy m_policy;
            std::vector<std::string> m_metric_names;
            std::vector<int> m_pio_idx;
    };

    std::vector<std::string> MonitorRuntimeAgent::metric_names(void)
    {
        return {"cpu-energy (J)",
                "gpu-energy (J)",
                "dram-energy (J)",
                "cpu-power (W)",
                "gpu-power (W)",
                "dram-power (W)",
                "cpu-frequency (Hz)",
                "cpu-frequency (%)",
                "gpu-frequency (Hz)",
                "gpu-frequency (%)"};
    }


    MonitorRuntimeAgent::MonitorRuntimeAgent(const RuntimePolicy &policy)
        : m_policy(policy)
        , m_metric_names(metric_names())
    {
        if (m_policy.m_agent != "monitor") {
            throw Exception("MonitorRuntimeAgent: policy is defined for different agent: " + m_policy.m_agent,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_policy.m_params.empty()) {
            throw Exception("MonitorRuntimeAgent: policy parameters are not empty: ",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_pio_idx.push_back(platform_io().push_signal("CPU_ENERGY", GEOPM_DOMAIN_BOARD, 0));
        // TODO add conditional logic for GPUs
        m_pio_idx.push_back(-1);
        m_pio_idx.push_back(platform_io().push_signal("DRAM_ENERGY", GEOPM_DOMAIN_BOARD, 0));
        m_pio_idx.push_back(platform_io().push_signal("CPU_POWER", GEOPM_DOMAIN_BOARD, 0));
        // TODO add conditional logic for GPUs
        m_pio_idx.push_back(-1);
        m_pio_idx.push_back(platform_io().push_signal("DRAM_POWER", GEOPM_DOMAIN_BOARD, 0));
        m_pio_idx.push_back(platform_io().push_signal("CPU_FREQUENCY_STATUS", GEOPM_DOMAIN_BOARD, 0));
        // TODO add logic for fraction of sticker and conditional logic for GPUs
        m_pio_idx.push_back(-1);
        m_pio_idx.push_back(-1);
        m_pio_idx.push_back(-1);
    }

    std::string MonitorRuntimeAgent::name(void) const
    {
        return "monitor";
    }

    double MonitorRuntimeAgent::period(void) const
    {
        return m_policy.m_period;
    }

    std::string MonitorRuntimeAgent::profile(void) const
    {
        return m_policy.m_profile;
    }

    std::map<std::string, double> MonitorRuntimeAgent::params(void) const
    {
        return {};
    }

    std::vector<double> MonitorRuntimeAgent::update(void)
    {
        std::vector<double> samples(m_pio_idx.size(), NAN);
        platform_io().read_batch();
        int sample_idx = 0;
        for (const auto &pio_idx : m_pio_idx) {
            if (pio_idx != -1) {
                samples[sample_idx] = platform_io().sample(pio_idx);
            }
            ++sample_idx;
        }
        return samples;
    }

    std::unique_ptr<RuntimeStats> RuntimeAgent::make_stats(const std::string &agent_name)
    {
        std::vector<std::string> metric_names;

        if (agent_name == "monitor") {
             metric_names = MonitorRuntimeAgent::metric_names();
        }
        else if (agent_name != "") {
            throw Exception("RuntimeAgent::make_stats(): Unknown agent name: " + agent_name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return std::make_unique<RuntimeStats>(metric_names);
    }


    std::unique_ptr<RuntimeAgent> RuntimeAgent::make_agent(const RuntimePolicy &policy)
    {
        geopm_pio_reset();
        if (policy.m_agent == "") {
            return std::make_unique<NullRuntimeAgent>(policy);
        }
        else if (policy.m_agent == "monitor") {
            return std::make_unique<MonitorRuntimeAgent>(policy);
        }
        else {
            throw Exception("RuntimeAgent::make_agent(): Unknown agent name: " + policy.m_agent,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    RuntimeServiceImp::RuntimeServiceImp(policy_struct_s &policy_struct)
        : m_policy_struct(policy_struct)
    {

    }

    ::grpc::Status RuntimeServiceImp::SetPolicy(::grpc::ServerContext* context,
                                                const ::Policy* request,
                                                ::Policy* response)
    {
        *response = *request;
        std::map<std::string, double> params;
        for (const auto &it : request->params()) {
            params[it.first] = it.second;
        }
        // TODO Create report and store before creating new stats object
        SharedMemoryScopedLock lock(&(m_policy_struct.mutex));
        m_policy_struct.policy = std::make_shared<RuntimePolicy>(request->agent(),
                                                                 request->period(),
                                                                 request->profile(),
                                                                 params);
        m_policy_struct.stats = RuntimeAgent::make_stats(request->agent());
        m_policy_struct.is_updated = true;
        return ::grpc::Status::OK;
    }

    ::grpc::Status RuntimeServiceImp::GetReport(::grpc::ServerContext* context,
                                                const ::ReportRequest* request,
                                                ::ReportList* response)
    {
        response->set_geopm_version(geopm_version());
        auto report = response->add_list();
        Url *host = new Url;
        host->set_url(geopm::hostname());
        report->set_allocated_host(host);
        SharedMemoryScopedLock lock(&(m_policy_struct.mutex));
        for (int metric_idx = 0; metric_idx != m_policy_struct.stats->num_metric(); ++metric_idx) {
            auto stats = report->add_stats();
            stats->set_name(m_policy_struct.stats->metric_name(metric_idx));
            stats->set_count(m_policy_struct.stats->count(metric_idx));
            stats->set_first(m_policy_struct.stats->first(metric_idx));
            stats->set_last(m_policy_struct.stats->last(metric_idx));
            stats->set_min(m_policy_struct.stats->min(metric_idx));
            stats->set_max(m_policy_struct.stats->max(metric_idx));
            stats->set_mean(m_policy_struct.stats->mean(metric_idx));
            stats->set_std(m_policy_struct.stats->std(metric_idx));
        }
        m_policy_struct.stats->reset();
        return ::grpc::Status::OK;
    }

    ::grpc::Status RuntimeServiceImp::AddChildHost(::grpc::ServerContext* context,
                                                   const ::Url* request,
                                                   ::TimeSpec* response)
    {
        ::grpc::Status result;
        return result;
    }

    ::grpc::Status RuntimeServiceImp::RemoveChildHost(::grpc::ServerContext* context,
                                                      const ::Url* request,
                                                      ::TimeSpec* response)
    {
        ::grpc::Status result;
        return result;
    }

    void rtd_run(policy_struct_s &policy_struct)
    {
        std::shared_ptr<RuntimeAgent> agent;
        std::unique_ptr<Waiter> waiter = Waiter::make_unique(RuntimeServiceImp::POLICY_LATENCY);
        bool do_loop = true;
        while(do_loop) {
            bool is_updated;
            std::unique_ptr<RuntimePolicy> policy;
            {
                SharedMemoryScopedLock lock(&(policy_struct.mutex));
                is_updated = (policy_struct.is_updated || agent == nullptr);
                if (is_updated) {
                    // TODO Create report if the policy has changed and store for next call to GetReports()
                    if (policy_struct.policy == nullptr) {
                        throw Exception("rtd_runt(): Thread data is invalid: NULL pointer",
                                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    }
                    policy = std::make_unique<RuntimePolicy>(*(policy_struct.policy));
                    policy_struct.is_updated = false;
                }
            }
            if (is_updated) {
                agent = RuntimeAgent::make_agent(*policy);
                waiter = Waiter::make_unique(agent->period());
            }
            if (agent->period() == 0) {
                do_loop = false;
                break;
            }
            auto sample = agent->update();
            if (sample.size() != 0) // TODO: avoid using sample when agent changes
            {
                SharedMemoryScopedLock lock(&(policy_struct.mutex));
                policy_struct.stats->update(sample);
            }
            waiter->wait();
        }
    }

    int rtd_main(const std::string &server_address)
    {
        int err = 0;
        policy_struct_s policy_struct {
            PTHREAD_MUTEX_INITIALIZER, true, std::make_shared<RuntimePolicy>(), std::make_shared<RuntimeStats>()
        };
        try {
            RuntimeServiceImp service(policy_struct);
            grpc::ServerBuilder builder;
            // TODO Use secure credentials for authentication
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&service);
            std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
            rtd_run(policy_struct);
            server->Shutdown();
        }
        catch (const geopm::Exception &ex) {
            err = ex.err_value();
            std::cerr << "Error: <geopmrtd>" << ex.what() << "\n\n";
        }
        return err;
    }

}
