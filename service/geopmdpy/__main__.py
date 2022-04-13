#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from dasbus.loop import EventLoop
from dasbus.connection import SystemMessageBus
from . import service


def main():
    loop = EventLoop()
    bus = SystemMessageBus()
    try:
        bus.publish_object("/io/github/geopm", service.GEOPMService())
        bus.register_service("io.github.geopm")
        loop.run()
    finally:
        bus.disconnect()

if __name__ == '__main__':
    main()
