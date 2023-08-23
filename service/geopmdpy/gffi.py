#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''The gffi module provides a wrapper around the cffi interface

This module enables a single cffi.FFI() object to be used throughout
all of the GEOPM python modules and also enables us to enforce that
the libgeopm.so dynamic library is opened prior to libgeopmd.so.
This is required because libgeopmd.so allocates static objects that
depend on static objects defined in libgeopm.so (in particular
the geopm::ApplicationSampler).

'''

import cffi
import os

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

def get_dl_geopm():
    '''Get the FFILibrary instance for libgeopm.so

    Returns:
        FFILibrary: Object used to call functions defined in
                    libgeopm.so

    '''
    global _dl_geopm
    if type(_dl_geopm) is OSError:
        raise _dl_geopm
    return _dl_geopm

# Enforce load order of libgeopm.so and libgeopmd.so
try:
    old_env = os.environ.get('GEOPM_PROGRAM_FILTER')
    os.environ['GEOPM_PROGRAM_FILTER'] = ''
    _dl_geopm = gffi.dlopen('libgeopm.so.1',
                            gffi.RTLD_GLOBAL|gffi.RTLD_LAZY)
    if old_env is not None:
        os.environ['GEOPM_PROGRAM_FILTER'] = old_env
    else:
        os.environ.pop('GEOPM_PROGRAM_FILTER')
except OSError as err:
    _dl_geopm = err

try:
    _dl_geopmd =  gffi.dlopen('libgeopmd.so.1',
                              gffi.RTLD_GLOBAL|gffi.RTLD_LAZY)
except OSError as err:
    _dl_geopmd = err
