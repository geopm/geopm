#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from dasbus.loop import EventLoop
from dasbus.connection import SystemMessageBus
from signal import signal
from signal import SIGTERM
import sys
from . import service
from geopmdpy.restorable_file_writer import RestorableFileWriter

ALLOW_WRITES_PATH = '/sys/module/msr/parameters/allow_writes'
ALLOW_WRITES_BACKUP_PATH = '/run/geopm/msr-saved-allow-writes'

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
    with RestorableFileWriter(
        ALLOW_WRITES_PATH, ALLOW_WRITES_BACKUP_PATH,
        warning_handler=lambda warning: print('Warning <geopm-service>', warning,
                                              file=sys.stderr)) as writer:
        try:
            writer.backup_and_try_update('on\n')
            _bus.publish_object("/io/github/geopm", service.GEOPMService())
            _bus.register_service("io.github.geopm")
            _loop.run()
        finally:
            stop()

if __name__ == '__main__':
    main()
