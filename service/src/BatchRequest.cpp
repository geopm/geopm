/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "BatchRequest.hpp"

#include <limits.h>
#include <sstream>

#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm_batch_request.h"


int geopm_batch_request_create(const char *file_path,
                               struct geopm_batch_request_s **request)
{
    int err = 0;
    try {
        std::string contents = geopm::read_file(file_path);
        geopm::BatchRequest *request_ptr = new geopm::BatchRequestImp(contents);
        request_ptr->push_signals();
        *request = (geopm_batch_request_s *)request_ptr;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}

int geopm_batch_request_destroy(struct geopm_batch_request_s *request)
{
    int err = 0;
    try {
        geopm::BatchRequest *request_ptr = (geopm::BatchRequest*)(request);
        delete request_ptr;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}

int geopm_batch_request_num_sample(const struct geopm_batch_request_s *request, int *num_sample)
{
    int err = 0;
    try {
        geopm::BatchRequest *request_ptr = (geopm::BatchRequest*)(request);
        *num_sample = request_ptr->num_requests();

    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}

int geopm_batch_request_read(struct geopm_batch_request_s *request,
                             size_t num_sample,
                             double *sample)
{
    int err = 0;
    try
    {
        geopm::BatchRequest *request_ptr = (geopm::BatchRequest*)(request);
        std::vector<double> result = request_ptr->read();
        if (num_sample != result.size()) {
            std::string msg = "geopm_batch_request_read(): "
                "Output vector is not sized appropriately";
            throw geopm::Exception(msg, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::copy(sample, sample + num_sample, result.begin());
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}


namespace geopm
{
    std::unique_ptr<BatchRequest> BatchRequest::make_unique(
        const std::string &request_str)
    {
        return geopm::make_unique<BatchRequestImp>(request_str);
    }


    BatchRequestImp::BatchRequestImp(const std::string &request_str)
        : BatchRequestImp(request_str, platform_io())
    {

    }

    BatchRequestImp::BatchRequestImp(const std::string &request_str,
                                     PlatformIO &pio)
        : m_pio(pio)
        , m_requests(parse_request_string(request_str))
    {

    }

    int BatchRequestImp::num_requests(void) const
    {
        return m_requests.size();
    }

    std::vector<BatchRequest::m_request_s> BatchRequestImp::requests(void) const
    {
        return m_requests;
    }

    void BatchRequestImp::push_signals(void)
    {
        if (m_batch_idx.size() == 0) {
            for (const auto &req : m_requests) {
                m_batch_idx.push_back(m_pio.push_signal(req.name,
                                                        req.domain_type,
                                                        req.domain_idx));
            }
        }
    }

    std::vector<double> BatchRequestImp::read(void) const
    {
        std::vector<double> result;
        m_pio.read_batch();
        for (auto batch_idx : m_batch_idx) {
            result.push_back(m_pio.sample(batch_idx));
        }
        return result;
    }

    std::vector<BatchRequest::m_request_s> BatchRequestImp::parse_request_string(
        const std::string &request_str)
    {
        std::vector<m_request_s> result;
        for (const auto &line : geopm::string_split(request_str, "\n")) {
            std::istringstream line_stream(line);
            std::string signal_name;
            std::string domain_str;
            int domain_idx;
            line_stream >> signal_name >> domain_str >> domain_idx;
            if (signal_name.size() >= NAME_MAX) {
                throw Exception("Signal name is too long: " + signal_name,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            int domain_type = geopm::PlatformTopo::domain_name_to_type(domain_str);
            result.push_back({domain_type, domain_idx, signal_name});
        }
        return result;
    }
}
