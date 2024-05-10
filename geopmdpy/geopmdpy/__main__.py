#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from dasbus.loop import EventLoop
from dasbus.connection import SystemMessageBus
from signal import signal
from signal import SIGTERM
import sys
import os
from . import service
from . import system_files
from . import __version_str__
from geopmdpy.restorable_file_writer import RestorableFileWriter

ALLOW_WRITES_PATH = '/sys/module/msr/parameters/allow_writes'
ALLOW_WRITES_BACKUP_PATH = os.path.join(system_files.GEOPM_SERVICE_RUN_PATH, 'msr-saved-allow-writes')

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
    if len(sys.argv) > 1 and sys.argv[1] == '--version':
        print(__version_str__)
        return 0
    signal(SIGTERM, term_handler)
    global _bus, _loop
    system_files.secure_make_dirs(system_files.GEOPM_SERVICE_RUN_PATH,
                                  perm_mode=system_files.GEOPM_SERVICE_RUN_PATH_PERM)
    _loop = EventLoop()
    _bus = SystemMessageBus()
    with RestorableFileWriter(
        ALLOW_WRITES_PATH, ALLOW_WRITES_BACKUP_PATH,
        warning_handler=lambda warning: print('Warning <geopm-service>', warning,
                                              file=sys.stderr)) as writer:
        try:
            if not os.path.exists('/dev/cpu/msr_batch'):
                writer.backup_and_try_update('on\n')
            geopm_service = service.GEOPMService()
            geopm_service.topo_rm_cache()
            _bus.publish_object("/io/github/geopm", geopm_service)
            _bus.register_service("io.github.geopm")
            _loop.run()
        finally:
            stop()

if __name__ == '__main__':
    main()
