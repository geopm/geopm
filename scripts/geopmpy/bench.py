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

import cffi
import error


_ffi = cffi.FFI()
_ffi.cdef("""
    struct geopm_model_region_c;
    int geopm_model_region_factory(const char *name, double big_o, int verbosity, struct geopm_model_region_c **result);
    int geopm_model_region_run(struct geopm_model_region_c *model_region_c);
""")
_dl = _ffi.dlopen('libgeopm.so')

def model_region_factory(region_name, big_o, verbosity):
    region_name_cstr = _ffi.new("char[]", str(region_name))
    result = _ffi.new("struct geopm_model_region_c**")
    err = _dl.geopm_model_region_factory(region_name_cstr, big_o, verbosity, result)
    if err:
        raise RuntimeError("geopm_model_region_factory() failed: {}".format(
            error.message(err)))
    return result[0]

def model_region_run(model_region):
    err = _dl.geopm_model_region_run(model_region)
    if err:
        raise RuntimeError("geopm_model_region_run() failed: {}".format(
            error.message(err)))
