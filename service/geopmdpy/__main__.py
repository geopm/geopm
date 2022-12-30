#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys

from dasbus.loop import EventLoop
from dasbus.connection import SystemMessageBus
from signal import signal
from signal import SIGTERM

from . import service
from . import system_files
from . import grpc_service

_bus = None

def term_handler(signum, frame):
    if signum == SIGTERM:
        stop()

def stop():
    if _bus is not None:
        _bus.disconnect()
    exit(0)

def main_dbus():
    signal(SIGTERM, term_handler)
    loop = EventLoop()
    global _bus
    _bus = SystemMessageBus()
    try:
        _bus.publish_object("/io/github/geopm", service.GEOPMService())
        _bus.register_service("io.github.geopm")
        loop.run()
    finally:
        stop()

def main_grpc():
    server = grpc_service.GRPCPlatformService()
    server.run()

def main():
    if len(sys.argv) > 1 and sys.argv[1] == '--grpc':
        main_grpc()
    else:
        main_dbus()

if __name__ == '__main__':
    main()
