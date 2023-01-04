#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import os
from dasbus.loop import EventLoop
from dasbus.connection import SystemMessageBus
from dasbus.connection import AddressedMessageBus
from signal import signal
from signal import SIGTERM
from . import service

_bus = None
_dbus_daemon_pid = None

def term_handler(signum, frame):
    if signum == SIGTERM:
        stop()

def stop():
    if _bus is not None:
        _bus.disconnect()
    if _dbus_daemon_pid is not None:
        os.kill(_dbus_daemon_pid, 7)

def main():
    signal(SIGTERM, term_handler)
    is_session_bus = False
    if len(sys.argv) > 1 and sys.argv[1] == '--session-bus':
        is_session_bus = True
    loop = EventLoop()
    global _bus
    global _dbus_daemon_pid
    if is_session_bus:
        socket_path = "/run/geopm-service/SESSION_BUS_SOCKET"
        _dbus_daemon_pid = service.popen_dbus_server(socket_path)
        _bus = AddressedMessageBus(f'unix:path={socket_path}')
    else:
        _bus = SystemMessageBus()
    try:
        _bus.publish_object("/io/github/geopm", service.GEOPMService(_bus))
        _bus.register_service("io.github.geopm")
        loop.run()
    finally:
        stop()

if __name__ == '__main__':
    main()
