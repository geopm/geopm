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
    if isinstance(_dl_geopmd, Exception):
        raise RuntimeError('Attempted to use libgeopmd.so, which is not loaded. Make sure it is available '
                           'in your system installs or in LD_LIBRARY_PATH') from _dl_geopmd
    return _dl_geopmd

try:
    from _libgeopmd_py_cffi import ffi as gffi, lib as _dl_geopmd
except Exception as err:
    _dl_geopmd = err
