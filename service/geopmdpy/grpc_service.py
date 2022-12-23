#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import socket
import select
import os
import sys
import pwd
import grpc
from concurrent import futures
import google.protobuf.empty_pb2
from . import geopm_service_pb2_grpc
from . import geopm_service_pb2
from . import service
from . import system_files

class GEOPMServiceProxy(geopm_service_pb2_grpc.GEOPMServiceServicer):
    def __init__(self):
        self._platform_service = service.PlatformService('grpc')
        self._topo_service = service.TopoService()

    def GetUserAccess(self, request, context):
        result = geopm_service_pb2.AccessLists()
        signals, controls = self._platform_service.get_user_access('')
        for ss in signals:
            result.signals.append(ss)
        for cc in controls:
            result.controls.append(cc)
        return result

    def GetSignalInfo(self, request, context):
        result = geopm_service_pb2.SignalInfoList()
        signal_info = self._platform_service.get_signal_info(request.names)
        for si in signal_info:
            element = geopm_service_pb2.SignalInfoList.SignalInfo()
            element.name = si[0]
            element.description = si[1]
            element.domain = si[2]
            element.aggregation = si[3]
            element.format_type = si[4]
            element.behavior = si[5]
            result.list.append(element)
        return result

    def GetControlInfo(self, request, context):
        result = geopm_service_pb2.ControlInfoList()
        control_info = self._platform_service.get_control_info(request.names)
        for ci in control_info:
            element = geopm_service_pb2.ControlInfoList.ControlInfo()
            element.name = ci[0]
            element.description = ci[1]
            element.domain = ci[2]
            result.list.append(element)
        return result

    def StartBatch(self, request, context):
        result = geopm_service_pb2.BatchKey()
        client_id = self._get_client_id(context)
        signal_config = []
        if request.signal_config:
            signal_config = [(int(sc.domain), int(sc.domain_idx), (str(sc.name)))
                             for sc in request.signal_config]
        control_config = []
        if request.control_config:
            control_config = [(int(cc.domain), int(cc.domain_idx, str(cc.name)))
                              for cc in request.control_config]
        server_pid, server_key = self._platform_service.start_batch(client_id,
                                                                    signal_config,
                                                                    control_config)
        result.batch_pid = server_pid
        result.shmem_key = server_key
        return result

    def StopBatch(self, request, context):
        client_id = self._get_client_id(context)
        self._platform_service.stop_batch(client_id, request.batch_pid)
        return google.protobuf.empty_pb2.Empty()

    def ReadSignal(self, request, context):
        result = geopm_service_pb2.Sample()
        client_id = self._get_client_id(context)
        platform_request = request.request
        result.sample = self._platform_service.read_signal(client_id,
                                                           platform_request.name,
                                                           platform_request.domain,
                                                           platform_request.domain_idx)
        return result

    def WriteControl(self, request, context):
        client_id = self._get_client_id(context)
        platform_request = request.request
        self._platform_service.write_control(client_id,
                                             platform_request.name,
                                             platform_request.domain,
                                             platform_request.domain_idx,
                                             request.setting)
        return google.protobuf.empty_pb2.Empty()

    def TopoGetCache(self, request, context):
        result = geopm_service_pb2.TopoCache()
        result.cache = self._topo_service.get_cache()
        return result

    def OpenSession(self, request, context):
        client_id = self._get_client_id(context)
        self._platform_service.open_session("", client_id)
        result = geopm_service_pb2.SessionKey()
        result.name = str(client_id)
        return result

    def CloseSession(self, request, context):
        client_id = self._get_client_id(context)
        self._platform_service.close_session(client_id)
        return google.protobuf.empty_pb2.Empty()

    def _get_client_id(self, context):
        return int(context.peer().split(':')[-1])


def run():
    grpc_port = 50051
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
    geopm_proxy = GEOPMServiceProxy()
    geopm_service_pb2_grpc.add_GEOPMServiceServicer_to_server(geopm_proxy, server)
    server.add_insecure_port(f'localhost:{grpc_port}')
    server.start()
    server.wait_for_termination()
