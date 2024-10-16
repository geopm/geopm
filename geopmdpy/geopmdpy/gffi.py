#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''The gffi module provides a wrapper around the cffi interface
'''

def get_dl_geopmd():
    '''Get the FFILibrary instance for libgeopmd.so

    Returns:
        FFILibrary: Object used to call functions defined in
                    libgeopmd.so

    '''
    global _dl_geopmd
    if isinstance(_dl_geopmd, Exception):
        raise RuntimeError('Attempted to use libgeopmd.so, which is not loaded. Make sure it is available '
                           'in your system installs or in LD_LIBRARY_PATH') from _dl_geopmd
    return _dl_geopmd

def get_dl_geopm():
    '''Get the FFILibrary instance for libgeopm.so

    Returns:
        FFILibrary: Object used to call functions defined in
                    libgeopm.so

    '''
    global _dl_geopm
    if isinstance(_dl_geopm, Exception):
        raise RuntimeError('Attempted to use libgeopm.so, which is not loaded. Make sure it is available '
                           'in your system installs or in LD_LIBRARY_PATH') from _dl_geopm
    return _dl_geopm

try:
    from _libgeopm_py_cffi import ffi as gffi, lib as _dl_geopm
    libgeopm_ffi = gffi
except Exception as err:
    _dl_geopm = err

try:
    from _libgeopmd_py_cffi import ffi as gffi, lib as _dl_geopmd
    libgeopmd_ffi = gffi
except Exception as err:
    _dl_geopmd = err
