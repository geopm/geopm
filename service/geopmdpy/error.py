#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import sys
from . import gffi

gffi.gffi.cdef("""
enum geopm_error_e {
    GEOPM_ERROR_RUNTIME = -1,
    GEOPM_ERROR_LOGIC = -2,
    GEOPM_ERROR_INVALID = -3,
    GEOPM_ERROR_FILE_PARSE = -4,
    GEOPM_ERROR_LEVEL_RANGE = -5,
    GEOPM_ERROR_NOT_IMPLEMENTED = -6,
    GEOPM_ERROR_PLATFORM_UNSUPPORTED = -7,
    GEOPM_ERROR_MSR_OPEN = -8,
    GEOPM_ERROR_MSR_READ = -9,
    GEOPM_ERROR_MSR_WRITE = -10,
    GEOPM_ERROR_AGENT_UNSUPPORTED = -11,
    GEOPM_ERROR_AFFINITY = -12,
    GEOPM_ERROR_NO_AGENT = -13,
};

void geopm_error_message(int err, char *msg, size_t size);

""")
_dl = gffi.get_dl_geopmd()

ERROR_RUNTIME = _dl.GEOPM_ERROR_RUNTIME
ERROR_LOGIC = _dl.GEOPM_ERROR_LOGIC
ERROR_INVALID = _dl.GEOPM_ERROR_INVALID
ERROR_FILE_PARSE = _dl.GEOPM_ERROR_FILE_PARSE
ERROR_LEVEL_RANGE = _dl.GEOPM_ERROR_LEVEL_RANGE
ERROR_NOT_IMPLEMENTED = _dl.GEOPM_ERROR_NOT_IMPLEMENTED
ERROR_PLATFORM_UNSUPPORTED = _dl.GEOPM_ERROR_PLATFORM_UNSUPPORTED
ERROR_MSR_OPEN = _dl.GEOPM_ERROR_MSR_OPEN
ERROR_MSR_READ = _dl.GEOPM_ERROR_MSR_READ
ERROR_MSR_WRITE = _dl.GEOPM_ERROR_MSR_WRITE
ERROR_AGENT_UNSUPPORTED = _dl.GEOPM_ERROR_AGENT_UNSUPPORTED
ERROR_AFFINITY = _dl.GEOPM_ERROR_AFFINITY
ERROR_NO_AGENT = _dl.GEOPM_ERROR_NO_AGENT

def message(err_number):
    """Return the error message associated with the error code.  Positive
    error codes are interpreted as system error numbers, and
    negative error codes are interpreted as GEOPM error numbers.

    Args:
        err_number (int): Error code to be interpreted.

    Returns:
        str: Error message associated with error code.

    """
    global _dl

    name_max = 1024
    result_cstr = gffi.gffi.new("char[]", name_max)
    _dl.geopm_error_message(err_number, result_cstr, name_max)
    return gffi.gffi.string(result_cstr).decode()
