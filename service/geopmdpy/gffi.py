#
#  Copyright (c) 2015 - 2022, Intel Corporation
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
    _dl_geopmpolicy = gffi.dlopen('libgeopmpolicy.so.0',
                                  gffi.RTLD_GLOBAL|gffi.RTLD_LAZY)
except OSError as err:
    _dl_geopmpolicy = err

try:
    _dl_geopmd =  gffi.dlopen('libgeopmd.so.0',
                              gffi.RTLD_GLOBAL|gffi.RTLD_LAZY)
except OSError as err:
    _dl_geopmd = err
