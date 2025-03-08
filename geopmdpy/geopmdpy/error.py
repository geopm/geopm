#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import sys
from . import gffi

ERROR_RUNTIME = gffi.dl_geopmd.GEOPM_ERROR_RUNTIME
ERROR_LOGIC = gffi.dl_geopmd.GEOPM_ERROR_LOGIC
ERROR_INVALID = gffi.dl_geopmd.GEOPM_ERROR_INVALID
ERROR_FILE_PARSE = gffi.dl_geopmd.GEOPM_ERROR_FILE_PARSE
ERROR_LEVEL_RANGE = gffi.dl_geopmd.GEOPM_ERROR_LEVEL_RANGE
ERROR_NOT_IMPLEMENTED = gffi.dl_geopmd.GEOPM_ERROR_NOT_IMPLEMENTED
ERROR_PLATFORM_UNSUPPORTED = gffi.dl_geopmd.GEOPM_ERROR_PLATFORM_UNSUPPORTED
ERROR_MSR_OPEN = gffi.dl_geopmd.GEOPM_ERROR_MSR_OPEN
ERROR_MSR_READ = gffi.dl_geopmd.GEOPM_ERROR_MSR_READ
ERROR_MSR_WRITE = gffi.dl_geopmd.GEOPM_ERROR_MSR_WRITE
ERROR_AGENT_UNSUPPORTED = gffi.dl_geopmd.GEOPM_ERROR_AGENT_UNSUPPORTED
ERROR_AFFINITY = gffi.dl_geopmd.GEOPM_ERROR_AFFINITY
ERROR_NO_AGENT = gffi.dl_geopmd.GEOPM_ERROR_NO_AGENT

def message(err_number):
    """Return the error message associated with the error code.  Positive
    error codes are interpreted as system error numbers, and
    negative error codes are interpreted as GEOPM error numbers.

    Args:
        err_number (int): Error code to be interpreted.

    Returns:
        str: Error message associated with error code.

    """
    path_max = 4096
    result_cstr = gffi.gffi.new("char[]", path_max)
    gffi.dl_geopmd.geopm_error_message(err_number, result_cstr, path_max)
    return gffi.gffi.string(result_cstr).decode()
