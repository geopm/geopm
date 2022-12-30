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

class GRPCPlatformService(service.PlatformService):
    def __init__(self):
        super().__init__()
        self._timeout = 0.1
        self._grpc_socket_path = os.path.join(system_files.GEOPM_SERVICE_RUN_PATH,
                                              'GRPC_SOCKET')
        self._session_socket_path = os.path.join(system_files.GEOPM_SERVICE_RUN_PATH,
                                                 'SESSION_SOCKET')
        for path in (self._grpc_socket_path, self._session_socket_path):
            try:
                os.unlink(path)
            except OSError:
                if os.path.exists(path):
                    raise
        self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self._socket.settimeout(0)
        self._socket.bind(self._session_socket_path)
        os.chmod(self._session_socket_path, 0o777)
        self._socket.listen()
        self._active_sessions = system_files.ActiveSessions()
        self._poll_term = select.poll()
        self._poll_socket = select.poll()
        self._poll_socket.register(self._socket.fileno())

    def _close_session_completely(self, client_pidfd):
        super()._close_session_completely(client_pidfd)
        self._poll_term.unregister(client_pidfd)
        os.close(client_pidfd)

    def _check_term(self):
        completed_sessions = self._poll_term.poll(0)
        for pidfd, event in completed_sessions:
            if event == poll.POLLIN:
                self._close_session_completely(pidfd)

    def _check_socket(self):
        sessions = self._poll_socket.poll(0)
        for socket_fd, event in sessions:
            pidfd = 0
            if event == poll.POLLIN:
                connection, address = self._socket.accept()
                data, ancdata, msg_flags, address = connection.recvmsg(4, socket.CMSG_SPACE())
                sys.stderr.write(f'\nDEBUG: message = {data}     {ancdata}     {msg_flags}     {address}\n')
                for ad in ancdata:
                    if ad[0] == socket.SOL_SOCKET and ad[1] == socket.SCM_RIGHTS:
                        pidfd = struct.unpack('i', ad[2])
                sys.stderr.write(f'DEBUG:  Creating session {pidfd}\n')
                if pidfd != 0:
                    create_session(pidfd)
                # TODO: Just sending pidfd as an int. Could be a uuid4
                # string in the future.  If not, we should change the
                # type of the session key from string to int
                connection.sendmsg(struct.pack('i', pidfd))

    def _watch_client(self, client_pidfd):
        self._poll_term.register(client_pidfd, poll.POLLIN)
        return None

    def _write_mode_pid(self, client_pidfd):
        client_pid = geopm_pidfd_to_pid(client_pidfd)
        if self._write_pidfd is None:
            client_sid = geopm_pidfd_get_sid(client_pidfd, client_pid)
            if client_sid != client_pid:
                self._write_pidfd = geopm_pidfd_open_ns(self._write_pidfd, client_sid)
                self._write_pid = client_sid
                self.open_session(self._write_sidfd)
            else:
                self._write_pidfd = client_pidfd
                self._write_pid = client_pid
            result = self._write_pid
        else:
            result = geopm_pidfd_get_sid(self._write_pidfd, client_pid)
        return result

    def run(self):
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        geopm_proxy = GEOPMServiceProxy()
        geopm_service_pb2_grpc.add_GEOPMServiceServicer_to_server(geopm_proxy, server)
        server.add_insecure_port(f'unix://{self._grpc_socket_path}')
        server.start()
        while server.wait_for_termination(self._timeout):
            try:
                pidfd = None
                self._check_term()
                self._check_socket()
            except RuntimeError as ex:
                if pidfd is not None:
                    os.close(pidfd)
                sys.stderr.write('Warning: {ex}\n')
