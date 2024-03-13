/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "RuntimeService.hpp"

#include <cmath>

#include "geopm/Exception.hpp"
#include <grpcpp/grpcpp.h>
#include "geopm_runtime.grpc.pb.h"


namespace geopm {
    void *rtd_run(void *policy_ptr);
    class Stats;
    class Policy;

    struct policy_struct_s {
        pthread_mutex_t mutex;
        bool is_updated;
        std::shared_ptr<Policy> policy;
        std::shared_ptr<Stats> stats;
    };

    class Policy
    {
        public:
            Policy() = delete;
            Policy(const std::string &agent, double period, const std::string &profile, std::vector<double> params);
            virtual ~Policy() = default;
            const std::string m_agent;
            const double m_period;
            const std::string m_profile;
            const std::vector<double> m_params;
    };

    Policy::Policy(const std::string &agent, double period, const std::string &profile, std::vector<double> params)
        : m_agent(agent)
        , m_period(period)
        , m_profile(profile)
        , m_params(params)
    {

    }

    class Stats
    {
        public:
            Stats(const std::vector<std::string> &metric_names);
            virtual ~Stats() = default;
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

    Stats::Stats(const std::vector<std::string> &metric_names)
        : m_metric_names(metric_names)
        , m_moments(m_metric_names.size())
    {
        reset();
    }

    int Stats::num_metric(void) const
    {
        return m_metric_names.size();
    }

    void Stats::check_index(int metric_idx, const std::string &func, int line) const
    {
        if (metric_idx < 0 || (size_t)metric_idx >= m_metric_names.size()) {
            throw Exception("Stats::" + func  + "(): metric_idx out of range: " + std::to_string(metric_idx),
                            GEOPM_ERROR_INVALID, __FILE__, line);
        }
    }

    std::string Stats::metric_name(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        return m_metric_names[metric_idx];
    }

    uint64_t Stats::count(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        return m_moments[metric_idx].count;
    }

    double Stats::first(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].first;
        }
        return result;
    }

    double Stats::last(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].last;
        }
        return result;
    }

    double Stats::min(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].min;
        }
        return result;
    }

    double Stats::max(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].max;
        }
        return result;
    }

    double Stats::mean(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].m_1 /
                     m_moments[metric_idx].count;
        }
        return result;
    }

    double Stats::std(int metric_idx) const
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

    double Stats::skew(int metric_idx) const
    {
        throw Exception("Stats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double Stats::kurt(int metric_idx) const
    {
        throw Exception("Stats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double Stats::lse_linear_0(int metric_idx) const
    {
        throw Exception("Stats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double Stats::lse_linear_1(int metric_idx) const
    {
        throw Exception("Stats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void Stats::reset(void)
    {
        for (auto &it : m_moments) {
            it.count = 0;
            it.m_1 = 0.0;
            it.m_2 = 0.0;
            it.m_3 = 0.0;
            it.m_4 = 0.0;
        }
    }

    void Stats::update(const std::vector<double> &sample)
    {
        if (sample.size() != m_moments.size()) {
            throw Exception("Stats::update(): invalid input vector size: " + std::to_string(sample.size()),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto moments_it = m_moments.begin();
        for (const auto &ss : sample) {
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

    class RuntimeServiceImp final : public GEOPMRuntime::Service
    {
        public:
            RuntimeServiceImp() = delete;
            RuntimeServiceImp(policy_struct_s &policy_struct);
            virtual ~RuntimeServiceImp() = default;
            ::grpc::Status SetPolicy(::grpc::ServerContext* context, const ::Policy* request, ::Policy* response) override;
            ::grpc::Status GetReport(::grpc::ServerContext* context, const ::Empty* request, ::ReportList* response) override;
            ::grpc::Status AddChildHost(::grpc::ServerContext* context, const ::Url* request, ::TimeSpec* response) override;
            ::grpc::Status RemoveChildHost(::grpc::ServerContext* context, const ::Url* request, ::TimeSpec* response) override;
        private:
            policy_struct_s &m_policy_struct;
    };

    RuntimeServiceImp::RuntimeServiceImp(policy_struct_s &policy_struct)
        : m_policy_struct(policy_struct)
    {

    }

    ::grpc::Status RuntimeServiceImp::SetPolicy(::grpc::ServerContext* context, const ::Policy* request, ::Policy* response)
    {
        ::grpc::Status result;
        return result;
    }

    ::grpc::Status RuntimeServiceImp::GetReport(::grpc::ServerContext* context, const ::Empty* request, ::ReportList* response)
    {
        ::grpc::Status result;
        return result;
    }

    ::grpc::Status RuntimeServiceImp::AddChildHost(::grpc::ServerContext* context, const ::Url* request, ::TimeSpec* response)
    {
        ::grpc::Status result;
        return result;
    }

    ::grpc::Status RuntimeServiceImp::RemoveChildHost(::grpc::ServerContext* context, const ::Url* request, ::TimeSpec* response)
    {
        ::grpc::Status result;
        return result;
    }

    void *rtd_run(void *policy_ptr)
    {
        // struct policy_struct_s *policy_struct = static_cast<policy_struct_s *>(policy_ptr);
        // TODO add event loop
        return nullptr;
    }

    int rtd_main(const std::string &server_address)
    {
        int err = 0;
        try {
            policy_struct_s policy_struct {
                PTHREAD_MUTEX_INITIALIZER, false, nullptr, nullptr
            };
            RuntimeServiceImp service(policy_struct);
            grpc::ServerBuilder builder;
            // TODO Use secure credentials for authentication
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&service);
            std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
            pthread_t rtd_run_thread;
            pthread_create(&rtd_run_thread, NULL, rtd_run, &policy_struct);
            server->Wait();
        }
        catch (const geopm::Exception &ex) {
            err = ex.err_value();
            std::cerr << "Error: <geopmrtd>" << ex.what() << "\n\n";
        }
        return err;
    }

}
