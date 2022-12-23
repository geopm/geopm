#  Copyright (c) 2015 - 2023, Intel Corporation
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
_loop = None

def term_handler(signum, frame):
    if signum == SIGTERM:
        stop()

def stop():
    global _bus
    if _bus is not None:
        _bus.disconnect()
        _bus = None
    if _loop is not None:
        _loop.quit()

def main_dbus():
    signal(SIGTERM, term_handler)
    global _bus, _loop
    _loop = EventLoop()
    _bus = SystemMessageBus()
    try:
        _bus.publish_object("/io/github/geopm", service.GEOPMService())
        _bus.register_service("io.github.geopm")
        _loop.run()
    finally:
        stop()

def main_grpc():
    grpc_service.run()

def main():
    if len(sys.argv) > 1 and sys.argv[1] == '--grpc':
        main_grpc()
    else:
        main_dbus()

if __name__ == '__main__':
    main()
