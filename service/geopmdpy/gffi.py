#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''The gffi module provides a wrapper around the cffi interface

This module enables a single cffi.FFI() object to be used throughout
all of the GEOPM python modules and also enables us to enforce that
the libgeopmpolicy.so dynamic library is opened prior to libgeopmd.so.
This is required because libgeopmd.so allocates static objects that
depend on static objects defined in libgeopmpolicy.so (in particular
the geopm::ApplicationSampler).

'''

import cffi

'''gffi is the global FFI object used by all geopm python modules

'''
gffi = cffi.FFI()

def get_dl_geopmd():
    '''Get the FFILibrary instance for libgeopmd.so

    Returns:
        FFILibrary: Object used to call functions defined in
                    libgeopmd.so

    '''
    global _dl_geopmd
    if type(_dl_geopmd) is OSError:
        raise _dl_geopmd
    return _dl_geopmd

def get_dl_geopmpolicy():
    '''Get the FFILibrary instance for libgeopmpolicy.so

    Returns:
        FFILibrary: Object used to call functions defined in
                    libgeopmpolicy.so

    '''
    global _dl_geopmpolicy
    if type(_dl_geopmpolicy) is OSError:
        raise _dl_geopmpolicy
    return _dl_geopmpolicy

# Enforce load order of libgeopmpolicy.so and libgeopmd.so
try:
    _dl_geopmpolicy = gffi.dlopen('libgeopmpolicy.so.1',
                                  gffi.RTLD_GLOBAL|gffi.RTLD_LAZY)
except OSError as err:
    _dl_geopmpolicy = err

try:
    _dl_geopmd =  gffi.dlopen('libgeopmd.so.1',
                              gffi.RTLD_GLOBAL|gffi.RTLD_LAZY)
except OSError as err:
    _dl_geopmd = err
