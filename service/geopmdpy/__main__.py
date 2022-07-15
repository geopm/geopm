#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from dasbus.loop import EventLoop
from dasbus.connection import SystemMessageBus
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
    loop = EventLoop()
    global _bus
    _bus = SystemMessageBus()
    try:
        _bus.publish_object("/io/github/geopm", service.GEOPMService())
        _bus.register_service("io.github.geopm")
        loop.run()
    finally:
        stop()

if __name__ == '__main__':
    main()
