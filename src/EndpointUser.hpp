/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENDPOINTUSER_HPP_INCLUDE
#define ENDPOINTUSER_HPP_INCLUDE

#include <cstddef>

#include <vector>
#include <string>
#include <set>
#include <memory>

namespace geopm
{
    class EndpointUser
    {
        public:
            EndpointUser() = default;
            virtual ~EndpointUser() = default;
            /// @brief Read the latest policy values.  All NAN indicates
            ///        that a policy has not been written yet.
            /// @param [out] policy The policy values read. The order
            ///        is specified by the Agent.
            /// @return The age of the policy in seconds.
            virtual double read_policy(std::vector<double> &policy) = 0;
            /// @brief Write sample values and update the sample age.
            /// @param [in] sample The values to write.  The order is
            ///        specified by the Agent.
            virtual void write_sample(const std::vector<double> &sample) = 0;
            /// @brief Factory method for the EndpointUser receiving
            ///        the policy.
            static std::unique_ptr<EndpointUser> make_unique(const std::string &policy_path,
                                                             const std::set<std::string> &hosts);
    };

    class SharedMemory;

    class EndpointUserImp : public EndpointUser
    {
        public:
            EndpointUserImp() = delete;
            EndpointUserImp &operator=(const EndpointUserImp &other) = delete;
            EndpointUserImp(const EndpointUserImp &other) = delete;
            EndpointUserImp(const std::string &data_path,
                              const std::set<std::string> &hosts);
            EndpointUserImp(const std::string &data_path,
                            std::unique_ptr<SharedMemory> policy_shmem,
                            std::unique_ptr<SharedMemory> sample_shmem,
                            const std::string &agent_name,
                            int num_sample,
                            const std::string &profile_name,
                            const std::string &hostlist_path,
                            const std::set<std::string> &hosts);
            virtual ~EndpointUserImp();
            double read_policy(std::vector<double> &policy) override;
            void write_sample(const std::vector<double> &sample) override;
        private:
            std::string m_path;
            std::unique_ptr<SharedMemory> m_policy_shmem;
            std::unique_ptr<SharedMemory> m_sample_shmem;
            std::string m_hostlist_path;
            size_t m_num_sample;
    };
}

#endif
