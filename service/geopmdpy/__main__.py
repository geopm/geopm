#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from dasbus.loop import EventLoop
from dasbus.connection import SystemMessageBus
from signal import signal
from signal import SIGTERM
from . import service

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

def main():
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

if __name__ == '__main__':
    main()
