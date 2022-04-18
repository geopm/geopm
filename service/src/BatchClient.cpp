/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "BatchClient.hpp"

#include <signal.h>
#include <unistd.h>

#include "geopm/Helper.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "BatchServer.hpp"
#include "BatchStatus.hpp"


namespace geopm
{
    std::unique_ptr<BatchClient> BatchClient::make_unique(const std::string &server_key,
                                                          double timeout,
                                                          int num_signal,
                                                          int num_control)
    {
        return geopm::make_unique<BatchClientImp>(server_key, timeout,
                                                  num_signal, num_control);
    }

    BatchClientImp::BatchClientImp(const std::string &server_key, double timeout,
                                   int num_signal, int num_control)
        : BatchClientImp(num_signal, num_control,
                         BatchStatus::make_unique_client(server_key),
                         num_signal == 0 ? nullptr :
                            SharedMemory::make_unique_user(
                                BatchServer::get_signal_shmem_key(
                                    server_key), timeout),
                         num_control == 0 ? nullptr :
                            SharedMemory::make_unique_user(
                                BatchServer::get_control_shmem_key(
                                    server_key), timeout))
    {

    }


    BatchClientImp::BatchClientImp(int num_signal, int num_control,
                                   std::shared_ptr<BatchStatus> batch_status,
                                   std::shared_ptr<SharedMemory> signal_shmem,
                                   std::shared_ptr<SharedMemory> control_shmem)
        : m_num_signal(num_signal)
        , m_num_control(num_control)
        , m_batch_status(batch_status)
        , m_signal_shmem(signal_shmem)
        , m_control_shmem(control_shmem)
    {

    }

    std::vector<double> BatchClientImp::read_batch(void)
    {
        if (m_num_signal == 0) {
            return {};
        }
        m_batch_status->send_message(BatchStatus::M_MESSAGE_READ);
        m_batch_status->receive_message(BatchStatus::M_MESSAGE_CONTINUE);
        double *buffer = (double *)m_signal_shmem->pointer();
        std::vector<double> result(buffer, buffer + m_num_signal);
        return result;
    }

    void BatchClientImp::write_batch(std::vector<double> settings)
    {
        if (settings.size() != (size_t)m_num_control) {
            throw Exception("BatchClientImp::write_batch(): settings vector length does not match the number of configured controls",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_num_control == 0) {
            return;
        }
        double *buffer = (double *)m_control_shmem->pointer();
        std::copy(settings.begin(), settings.end(), buffer);
        m_batch_status->send_message(BatchStatus::M_MESSAGE_WRITE);
        m_batch_status->receive_message(BatchStatus::M_MESSAGE_CONTINUE);
    }

    void BatchClientImp::stop_batch(void)
    {
        // Note that all requests sent to the batch server block on
        // the client side until the server has completed the
        // request. This is even true for the request to quit.
        m_batch_status->send_message(BatchStatus::M_MESSAGE_QUIT);
        m_batch_status->receive_message(BatchStatus::M_MESSAGE_QUIT);
    }
}
