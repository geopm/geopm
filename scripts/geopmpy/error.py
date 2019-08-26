#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

from __future__ import absolute_import

import cffi

_ffi = cffi.FFI()
_ffi.cdef("""
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
_dl = _ffi.dlopen('libgeopmpolicy.so')

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
    global _ffi
    global _dl

    name_max = 1024
    result_cstr = _ffi.new("char[]", name_max)
    _dl.geopm_error_message(err_number, result_cstr, name_max)
    return _ffi.string(result_cstr).decode()
