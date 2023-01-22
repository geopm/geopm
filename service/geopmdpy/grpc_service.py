#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys
import pwd
import grpc
import subprocess # nosec
from concurrent import futures
from . import geopm_service_pb2_grpc
from . import geopm_service_pb2
from . import service
from . import system_files

class GEOPMServiceProxy(geopm_service_pb2_grpc.GEOPMServiceServicer):
    def __init__(self):
        self._platform_service = service.PlatformService()
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
        client_id = self._get_client_id(request.session_key, context)
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
        client_id = self._get_client_id(request.session_key, context)
        self._platform_service.stop_batch(client_id, request.batch_key.batch_pid)
        return geopm_service_pb2.Empty()

    def ReadSignal(self, request, context):
        result = geopm_service_pb2.Sample()
        client_id = self._get_client_id(request.session_key, context)
        platform_request = request.request
        result.sample = self._platform_service.read_signal(client_id,
                                                           platform_request.name,
                                                           platform_request.domain,
                                                           platform_request.domain_idx)
        return result

    def WriteControl(self, request, context):
        client_id = self._get_client_id(request.session_key, context)
        platform_request = request.request
        self._platform_service.write_control(client_id,
                                             platform_request.name,
                                             platform_request.domain,
                                             platform_request.domain_idx,
                                             request.setting)
        return geopm_service_pb2.Empty()

    def TopoGetCache(self, request, context):
        result = geopm_service_pb2.TopoCache()
        result.cache = self._topo_service.get_cache()
        return result

    def OpenSession(self, request, context):
        result = geopm_service_pb2.SessionKey()
        client_id = self._get_client_id(request, context)
        self._platform_service.open_session(self._get_user(client_id), client_id)
        result.name = request.name
        return result

    def CloseSession(self, request, context):
        client_id = self._get_client_id(request, context)
        self._platform_service.close_session(client_id)
        return geopm_service_pb2.Empty()

    def _get_client_id(self, session_key, context):
        pid_str = session_key.name.split(',')[1]
        return int(pid_str)

    def _get_user(self, client_id):
        uid = os.stat(f'/proc/{client_id}/status').st_uid
        return pwd.getpwuid(uid).pw_name


def run():
    grpc_socket_path = os.path.join(system_files.GEOPM_SERVICE_RUN_PATH,
                                    'GRPC_SOCKET_PRIVATE')
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
    geopm_proxy = GEOPMServiceProxy()
    geopm_service_pb2_grpc.add_GEOPMServiceServicer_to_server(geopm_proxy, server)
    server_credentials = grpc.local_server_credentials(grpc.LocalConnectionType.UDS)
    server.add_secure_port(f'unix://{grpc_socket_path}', server_credentials)
    server.start()
    with subprocess.Popen('geopmd-proxy') as proxy:
        server.wait_for_termination()
        proxy.terminate()
