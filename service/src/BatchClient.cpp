/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
        if (m_signal_shmem != nullptr) {
            m_signal_shmem->unlink();
        }
        if (m_control_shmem != nullptr) {
            m_control_shmem->unlink();
        }
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
