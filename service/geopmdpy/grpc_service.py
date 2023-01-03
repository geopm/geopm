#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import socket
import select
import os
import sys
import grpc
from concurrent import futures
import google.protobuf.empty_pb2
from . import geopm_service_pb2_grpc
from . import geopm_service_pb2
from . import service
from . import system_files

class GEOPMServiceProxy(geopm_service_pb2_grpc.GEOPMServiceServicer):
    def __init__(self):
        self._platform_service = GRPCPlatformService()
        self._topo_service = service.TopoService()
        self._serial_id = 42

    def GetUserAccess(self, request, context):
        result = geopm_service_pb2.AccessLists()
        result.allowed_signals, result.allowed_controls = self._platform_service.get_user_access(None)
        return result

    def GetSignalInfo(self, request, context):
        result = geopm_service_pb2.SignalInfoList()
        signal_info = self._platform_service.get_signal_info(request.signal_name)
        for si in signal_info:
            element = geopm_service_pb2.SignalInfoList.SignalInfo()
            element.name = si[0]
            element.description = si[1]
            element.domain_type = si[2]
            element.aggregation = si[3]
            element.format_type = si[4]
            element.behavior = si[5]
            result.list.append(element)
        return result

    def GetControlInfo(self, request, context):
        result = geopm_service_pb2.ControlInfoList()
        control_info = self._platform_service.get_control_info(request.control_name)
        for ci in control_info:
            element = geopm_service_pb2.ControlInfoList.ControlInfo()
            element.name = ci[0]
            element.description = ci[1]
            element.domain = ci[2]
            result.list.append(element)
        return result

    def StartBatch(self, request, context):
        result = geopm_service_pb2.BatchKey()
        result.session_key.name = request.session_key.name
        pidfd = int(request.session_key.name)
        signal_config = list()
        control_config = list()
        for sc in request.signal_config:
            element = tuple(sc.name, sc.domain, sc.domain_idx)
            signal_config.append(element)
        for cc in request.control_config:
            element = tuple(cc.name, cc.domain, cc.domain_idx)
            control_config.append(element)
        result.session_key.name = self._platform_service.start_batch(pidfd,
                                                                     signal_config,
                                                                     control_config)
        return result

    def StopBatch(self, request, context):
        result = google.protobuf.empty_pb2.Empty()
        pidfd = int(request.session_key.name)
        self._platform_service.stop_batch(pidfd, request.server_pid)
        return result

    def ReadSignal(self, request, context):
        result = geopm_service_pb2.Sample()
        pidfd = int(request.session_key.name)
        result.sample = self._platform_service.read_signal(pidfd, request.signal_name,
                                                           request.domain, request.domain_idx)
        return result

    def WriteControl(self, request, context):
        result = google.protobuf.empty_pb2.Empty()
        pidfd = int(request.session_key.name)
        self._platform_service.write_control(pidfd, request.control_name,
                                             request.domain, request.domain_idx,
                                             request.setting)
        return result

    def TopoGetCache(self, request, context):
        result = geopm_service_pb2.TopoCache()
        result.cache = self._topo_service.get_cache()
        return result

    def OpenSession(self, request, context):
        result = geopm_service_pb2.SessionKey()
        result.name = str(self._serial_id)
        self._platform_service.open_session("", self._serial_id)
        self._serial_id += 1

class GRPCPlatformService(service.PlatformService):
    def __init__(self):
        super().__init__()
        self._grpc_socket_path = os.path.join(system_files.GEOPM_SERVICE_RUN_PATH,
                                              'GRPC_SOCKET')
        self._timeout = 0.01

    def _watch_client(self, client_pidfd):
        return None

    def _write_mode_pid(self, client_pidfd):
        return None

    def run(self):
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        geopm_proxy = GEOPMServiceProxy()
        geopm_service_pb2_grpc.add_GEOPMServiceServicer_to_server(geopm_proxy, server)
        server.add_insecure_port(f'unix://{self._grpc_socket_path}')
        server.start()
        while server.wait_for_termination(self._timeout):
            pass
