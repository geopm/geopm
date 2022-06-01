/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BATCHREQUEST_HPP_INCLUDE
#define BATCHREQUEST_HPP_INCLUDE

#include <vector>
#include <memory>
#include <string>

namespace geopm
{
    class PlatformIO;

    class BatchRequest
    {
        public:
            struct m_request_s {
                int domain_type;
                int domain_idx;
                std::string name;
            };
            BatchRequest() = default;
            virtual ~BatchRequest() = default;
            static std::unique_ptr<BatchRequest> make_unique(
                const std::string &request_str);
            virtual int num_requests(void) const = 0;
            virtual std::vector<m_request_s> requests(void) const = 0;
            virtual void push_signals(void) = 0;
            virtual std::vector<double> read(void) const = 0;
    };

    class BatchRequestImp : public BatchRequest
    {
        public:
            BatchRequestImp(const std::string &request_str);
            BatchRequestImp(const std::string &request_str,
                            PlatformIO &pio);
            virtual ~BatchRequestImp() = default;
            virtual int num_requests(void) const override;
            virtual std::vector<m_request_s> requests(void) const override;
            virtual void push_signals(void) override;
            virtual std::vector<double> read(void) const override;
            static std::vector<m_request_s> parse_request_string(
                const std::string &request_str);
        private:
            PlatformIO &m_pio;
            std::vector<m_request_s> m_requests;
            std::vector<int> m_batch_idx;
    };
}

#endif
