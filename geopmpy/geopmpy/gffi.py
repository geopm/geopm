#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''The gffi module provides a wrapper around the cffi interface
'''

def get_dl_geopm():
    '''Get the FFILibrary instance for libgeopm.so

    Returns:
        FFILibrary: Object used to call functions defined in
                    libgeopm.so

    '''
    if isinstance(_dl_geopm, Exception):
        raise RuntimeError('Attempted to use libgeopm.so, which is not loaded. Make sure it is available '
                           'in your system installs or in LD_LIBRARY_PATH') from _dl_geopm
    return _dl_geopm

try:
    from _libgeopm_py_cffi import ffi as gffi, lib as _dl_geopm
except Exception as err:
    _dl_geopm = err
