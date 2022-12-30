/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <grpcpp/grpcpp.h>
#include "GRPCServiceProxy.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_service.pb.h"
#include "geopm_service.grpc.pb.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace geopm
{

    GRPCServiceProxy::~GRPCServiceProxy()
    {
        if (m_pidfd != -1) {
            (void)close(m_pidfd);
            errno = 0;
        }
    }

    GRPCServiceProxy::GRPCServiceProxy()
        : m_grpc_socket("unix:///run/geopm-service/GRPC_SOCKET")
        , m_session_socket("/run/geopm-service/SESSION_SOCKET")
        , m_session_key("")
        , m_pidfd(-1)
        , m_client(std::make_shared<GEOPMPackage::GEOPMService::Stub>(
                   grpc::CreateChannel(m_grpc_socket,
                                       grpc::InsecureChannelCredentials())))
    {
        platform_open_session();
    }


    void GRPCServiceProxy::platform_get_user_access(std::vector<std::string> &signal_names,
                                                    std::vector<std::string> &control_names)
    {
        grpc::ClientContext context;
        GEOPMPackage::SessionKey request;
        GEOPMPackage::AccessLists response;
        request.set_name(m_session_key);
        grpc::Status status = m_client->GetUserAccess(&context,
                                                      request,
                                                      &response);
        if (!status.ok()) {
            throw Exception("GRPCServiceProxy::platform_get_user_access(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        signal_names.clear();
        control_names.clear();
        for (const auto &sn : response.signals()) {
            signal_names.push_back(sn);
        }
        for (const auto &cn : response.controls()) {
            control_names.push_back(cn);
        }
    }

    std::vector<signal_info_s> GRPCServiceProxy::platform_get_signal_info(
        const std::vector<std::string> &signal_names)
    {
        std::vector<signal_info_s> result;
        grpc::ClientContext context;
        GEOPMPackage::InfoRequest request;
        GEOPMPackage::SignalInfoList response;
        grpc::Status status = m_client->GetSignalInfo(&context,
                                                      request,
                                                      &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_get_signal_info(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        for (const auto &si : response.list()) {
            struct signal_info_s element;
            element.name = si.name();
            element.description = si.description();
            element.domain = si.domain();
            element.aggregation = si.aggregation();
            element.string_format = si.format_type();
            element.behavior = si.behavior();
            result.push_back(element);
        }
        return result;
    }

    std::vector<control_info_s> GRPCServiceProxy::platform_get_control_info(
        const std::vector<std::string> &control_names)
    {
        std::vector<control_info_s> result;
        grpc::ClientContext context;
        GEOPMPackage::InfoRequest request;
        GEOPMPackage::ControlInfoList response;
        grpc::Status status = m_client->GetControlInfo(&context,
                                                       request,
                                                       &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_get_control_info(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        for (const auto &ci : response.list()) {
            struct control_info_s element;
            element.name = ci.name();
            element.description = ci.description();
            element.domain = ci.domain();
            result.push_back(element);
        }
        return result;
    }

    void GRPCServiceProxy::platform_open_session(void)
    {
        int socket_fd = -1;
        try {
            socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (socket_fd == -1) {
                throw Exception("GRPCServiceProxy::platform_open_session(): Failed to create socket",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);

            }
            m_pidfd = geopm::pidfd_open(getpid(), 0);
            if (m_pidfd == -1) {
                throw Exception("GRPCServiceProxy::platform_open_session(): Failed to create pidfd",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            struct sockaddr_un address;
            memset(&address, 0, sizeof(struct sockaddr_un));
            address.sun_family = AF_UNIX;
            strncpy(address.sun_path, m_session_socket.c_str(), sizeof(address.sun_path) - 1);
            int err = connect(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un));
            if (err == -1) {
                throw Exception("GRPCServiceProxy::platform_open_session(): Failed to connect(2) to GEOPM Service session socket",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            struct msghdr msg = {};
            struct cmsghdr *control_msg;
            std::vector<char> control_msg_buffer(CMSG_SPACE(sizeof(int)), '\0');
            std::vector<char> iov_buffer(4, '\0');
            struct iovec io;
            io.iov_base = iov_buffer.data();
            io.iov_len = iov_buffer.size();
            msg.msg_iov = &io;
            msg.msg_iovlen = 1;
            msg.msg_control = control_msg_buffer.data();
            msg.msg_controllen = control_msg_buffer.size();
            control_msg = CMSG_FIRSTHDR(&msg);
            control_msg->cmsg_level = SOL_SOCKET;
            control_msg->cmsg_type = SCM_RIGHTS;
            control_msg->cmsg_len = CMSG_LEN(sizeof(int));
            memcpy((int *)CMSG_DATA(control_msg), &m_pidfd, sizeof(int));
            err = sendmsg(socket_fd, &msg, 0);
            if (err == -1) {
                throw Exception("GRPCServiceProxy::platform_open_session(): Failed to sendmsg(2) over GEOPM Service session socket",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            err = recvmsg(socket_fd, &msg, 0);
            char int_buffer[4];
            for (int bb = 0; bb != 4; ++bb) {
                int_buffer[3 - bb] = iov_buffer[bb];
            }
            int key = *((int *)int_buffer);
            m_session_key = std::to_string(key);
            err = close(socket_fd);
            if (err == -1) {
                throw Exception("GRPCServiceProxy::platform_open_session(): Failed to close(2) session socket",
                                errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        catch (...) {
            if (socket_fd != -1) {
                (void)close(socket_fd);
                errno = 0;
            }
            if (m_pidfd != -1) {
                (void)close(m_pidfd);
                errno = 0;
                m_pidfd = -1;
            }
            throw;
        }
    }

    void GRPCServiceProxy::platform_close_session(void)
    {
        // TODO: Push server_key through the session
        //       socket to close session
    }

    void GRPCServiceProxy::platform_start_batch(const std::vector<struct geopm_request_s> &signal_config,
                                                const std::vector<struct geopm_request_s> &control_config,
                                                int &server_pid,
                                                std::string &server_key)
    {
        // TODO
    }

    void GRPCServiceProxy::platform_stop_batch(int server_pid)
    {
        // TODO
    }

    double GRPCServiceProxy::platform_read_signal(const std::string &signal_name,
                                                  int domain,
                                                  int domain_idx)
    {
        double result = 0.0;
        grpc::ClientContext context;
        GEOPMPackage::Sample response;
        GEOPMPackage::SessionKey session_key;
        session_key.set_name(m_session_key);
        GEOPMPackage::PlatformRequest platform_request;
        platform_request.set_name(signal_name);
        platform_request.set_domain(GEOPMPackage::Domain(domain));
        platform_request.set_domain_idx(domain_idx);
        GEOPMPackage::ReadRequest request;
        request.set_allocated_session_key(&session_key);
        request.set_allocated_request(&platform_request);
        grpc::Status status = m_client->ReadSignal(&context,
                                                   request,
                                                   &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_read_signal(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        result = response.sample();
        return result;
    }

    void GRPCServiceProxy::platform_write_control(const std::string &control_name,
                                                  int domain,
                                                  int domain_idx,
                                                  double setting)
    {
        // TODO
    }
}
