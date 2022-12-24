#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
from dasbus.loop import EventLoop
from dasbus.connection import SystemMessageBus
from dasbus.connection import AddressedMessageBus
from signal import signal
from signal import SIGTERM
from . import service

_bus = None

def term_handler(signum, frame):
    if signum == SIGTERM:
        stop()

def stop():
    if _bus is not None:
        _bus.disconnect()
    exit(0)

def main():
    signal(SIGTERM, term_handler)
    is_anonymous = False
    if len(sys.argv) > 1 and sys.argv[1] == '--anonymous':
        is_anonymous = True
    loop = EventLoop()
    global _bus
    if is_anonymous:
        socket_path = "/run/geopm-service/SESSION_BUS_SOCKET"
        _bus = AddressedMessageBus(f'unix:path={socket_path}')
    else:
        _bus = SystemMessageBus()
    try:
        _bus.publish_object("/io/github/geopm", service.GEOPMService(is_anonymous))
        _bus.register_service("io.github.geopm")
        loop.run()
    finally:
        stop()

if __name__ == '__main__':
    main()
