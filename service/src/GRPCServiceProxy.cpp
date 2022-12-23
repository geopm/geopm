/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <grpcpp/grpcpp.h>
#include "GRPCServiceProxy.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm_service.pb.h"
#include "geopm_service.grpc.pb.h"
#include "grpcpp/security/credentials.h"
#include "grpc/grpc_security_constants.h"
#include <sstream>
#include <iostream>
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
        try {
            platform_close_session();
        }
        catch (...) {

        }
    }

    GRPCServiceProxy::GRPCServiceProxy()
        : m_grpc_socket("unix:///run/geopm-service/grpc.sock")
        , m_pidfd(-1)
        , m_client(std::make_shared<GEOPMPackage::GEOPMService::Stub>(
                   grpc::CreateChannel(m_grpc_socket,
                                       grpc::experimental::LocalCredentials(UDS))))
    {
        // TODO:  Session key should be provided by the server when the session is opened
        std::ostringstream id;
        id << getuid() << "," << getpid();
        m_session_key = id.str();
        // Throw at construction time if topo cache cannot be read
        try {
            platform_open_session();
        }
        catch (const Exception &ex) {
            throw Exception("GRPCServiceProxy: Failed to connect with gRPC server: " +
                            std::string(ex.what()),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void GRPCServiceProxy::platform_get_user_access(std::vector<std::string> &signal_names,
                                                    std::vector<std::string> &control_names)
    {
        grpc::ClientContext context;
        GEOPMPackage::SessionKey request;
        request.set_name(m_session_key);
        GEOPMPackage::AccessLists response;
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
        GEOPMPackage::InfoRequest request;
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        request.set_allocated_session_key(session_key);
        for (const auto &name : signal_names) {
            request.add_names(name);
        }
        GEOPMPackage::SignalInfoList response;
        grpc::ClientContext context;
        grpc::Status status = m_client->GetSignalInfo(&context,
                                                      request,
                                                      &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_get_signal_info(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        std::vector<signal_info_s> result;
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
        GEOPMPackage::InfoRequest request;
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        request.set_allocated_session_key(session_key);
        for (const auto &name : control_names) {
            request.add_names(name);
        }
        GEOPMPackage::ControlInfoList response;
        grpc::ClientContext context;
        grpc::Status status = m_client->GetControlInfo(&context,
                                                       request,
                                                       &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_get_control_info(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        std::vector<control_info_s> result;
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
        GEOPMPackage::SessionKey request;
        request.set_name(m_session_key);
        grpc::ClientContext context;
        GEOPMPackage::SessionKey response;
        grpc::Status status = m_client->OpenSession(&context,
                                                    request,
                                                    &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_open_session(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_session_key = response.name();
    }

    void GRPCServiceProxy::platform_close_session(void)
    {
        GEOPMPackage::SessionKey request;
        request.set_name(m_session_key);
        grpc::ClientContext context;
        GEOPMPackage::Empty response;
        grpc::Status status = m_client->CloseSession(&context,
                                                     request,
                                                     &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_close_session(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void GRPCServiceProxy::platform_restore_control(void)
    {
        GEOPMPackage::SessionKey request;
        request.set_name(m_session_key);
        grpc::ClientContext context;
        GEOPMPackage::Empty response;
        grpc::Status status = m_client->RestoreControl(&context,
                                                       request,
                                                       &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_restore_control(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void GRPCServiceProxy::platform_start_batch(const std::vector<struct geopm_request_s> &signal_config,
                                                const std::vector<struct geopm_request_s> &control_config,
                                                int &server_pid,
                                                std::string &server_key)
    {
        GEOPMPackage::BatchRequest request;
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        request.set_allocated_session_key(session_key);
        for (const auto &sc : signal_config) {
            GEOPMPackage::PlatformRequest *signal_config = request.add_signal_config();
            signal_config->set_name(sc.name);
            signal_config->set_domain(GEOPMPackage::Domain(sc.domain_type));
            signal_config->set_domain_idx(sc.domain_idx);
        }
        for (const auto &cc : control_config) {
            GEOPMPackage::PlatformRequest *control_config = request.add_control_config();
            control_config->set_name(cc.name);
            control_config->set_domain(GEOPMPackage::Domain(cc.domain_type));
            control_config->set_domain_idx(cc.domain_idx);
        }
        grpc::ClientContext context;
        GEOPMPackage::BatchKey response;
        grpc::Status status = m_client->StartBatch(&context,
                                                   request,
                                                   &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_start_batch(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        server_pid = response.batch_pid();
        server_key = response.shmem_key();
    }

    void GRPCServiceProxy::platform_stop_batch(int server_pid)
    {
        GEOPMPackage::BatchSession request;
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        request.set_allocated_session_key(session_key);
        GEOPMPackage::BatchKey *batch_key = new GEOPMPackage::BatchKey;
        batch_key->set_batch_pid(server_pid);
        request.set_allocated_batch_key(batch_key);
        grpc::ClientContext context;
        GEOPMPackage::Empty response;
        grpc::Status status = m_client->StopBatch(&context,
                                                  request,
                                                  &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_stop_batch(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    double GRPCServiceProxy::platform_read_signal(const std::string &signal_name,
                                                  int domain,
                                                  int domain_idx)
    {
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        GEOPMPackage::PlatformRequest *platform_request = new GEOPMPackage::PlatformRequest;
        platform_request->set_name(signal_name);
        platform_request->set_domain(GEOPMPackage::Domain(domain));
        platform_request->set_domain_idx(domain_idx);
        GEOPMPackage::ReadRequest request;
        request.set_allocated_session_key(session_key);
        request.set_allocated_request(platform_request);
        grpc::ClientContext context;
        GEOPMPackage::Sample response;
        grpc::Status status = m_client->ReadSignal(&context,
                                                   request,
                                                   &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_read_signal(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return response.sample();
    }

    void GRPCServiceProxy::platform_write_control(const std::string &control_name,
                                                  int domain,
                                                  int domain_idx,
                                                  double setting)
    {
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        GEOPMPackage::PlatformRequest *platform_request = new GEOPMPackage::PlatformRequest;
        platform_request->set_name(control_name);
        platform_request->set_domain(GEOPMPackage::Domain(domain));
        platform_request->set_domain_idx(domain_idx);
        GEOPMPackage::WriteRequest request;
        request.set_allocated_session_key(session_key);
        request.set_allocated_request(platform_request);
        request.set_setting(setting);
        grpc::ClientContext context;
        GEOPMPackage::Empty response;
        grpc::Status status = m_client->WriteControl(&context,
                                                     request,
                                                     &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_read_signal(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void GRPCServiceProxy::platform_start_profile(const std::string &profile_name)
    {
        GEOPMPackage::ProfileRequest request;
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        request.set_allocated_session_key(session_key);
        request.set_profile_name(profile_name);
        grpc::ClientContext context;
        GEOPMPackage::Empty response;
        grpc::Status status = m_client->StartProfile(&context,
                                                     request,
                                                     &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_start_profile(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void GRPCServiceProxy::platform_stop_profile(const std::vector<std::string> &region_names)
    {
        GEOPMPackage::ProfileRequest request;
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        request.set_allocated_session_key(session_key);
        for (const auto &name : region_names) {
            request.add_region_names(name);
        }
        grpc::ClientContext context;
        GEOPMPackage::Empty response;
        grpc::Status status = m_client->StopProfile(&context,
                                                    request,
                                                    &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::platform_stop_profile(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    std::vector<int> GRPCServiceProxy::platform_get_profile_pids(const std::string &profile_name)
    {
        std::vector<int> result;
        GEOPMPackage::ProfileRequest request;
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        request.set_allocated_session_key(session_key);
        request.set_profile_name(profile_name);
        grpc::ClientContext context;
        GEOPMPackage::PidList response;
        grpc::Status status = m_client->GetProfilePids(&context,
                                                       request,
                                                       &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::get_profile_pids(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        for (const auto &pid : response.pids()) {
            result.push_back(pid);
        }
        return result;
    }

    std::vector<std::string> GRPCServiceProxy::platform_pop_profile_region_names(const std::string &profile_name)
    {
        std::vector<std::string> result;
        GEOPMPackage::ProfileRequest request;
        GEOPMPackage::SessionKey *session_key = new GEOPMPackage::SessionKey;
        session_key->set_name(m_session_key);
        request.set_allocated_session_key(session_key);
        request.set_profile_name(profile_name);
        grpc::ClientContext context;
        GEOPMPackage::NameList response;
        grpc::Status status = m_client->PopProfileRegionNames(&context,
                                                              request,
                                                              &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::get_profile_pids(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        for (const auto &name : response.names()) {
            result.push_back(name);
        }
        return result;
    }

    std::string GRPCServiceProxy::topo_get_cache(void)
    {
        GEOPMPackage::Empty request;
        grpc::ClientContext context;
        GEOPMPackage::TopoCache response;
        grpc::Status status = m_client->TopoGetCache(&context,
                                                     request,
                                                     &response);
        if (!status.ok()) {
            throw Exception("GRPCSericeProxy::topo_get_cache(): " +
                            status.error_message(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return response.cache();
    }

}
