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
enum geopm_region_hash_e {
    GEOPM_REGION_HASH_INVALID  = 0x0,
    GEOPM_REGION_HASH_UNMARKED = 0x725e8066, /* Note the value is the geopm_crc32_str() of the stringified enum */
};

enum geopm_region_hint_e {
    GEOPM_REGION_HINT_UNKNOWN =   0x100000000, // Region with unknown or varying characteristics
    GEOPM_REGION_HINT_COMPUTE =   0x200000000, // Region dominated by compute
    GEOPM_REGION_HINT_MEMORY =    0x400000000, // Region dominated by memory access
    GEOPM_REGION_HINT_NETWORK =   0x800000000, // Region dominated by network traffic
    GEOPM_REGION_HINT_IO =        0x1000000000, // Region dominated by disk access
    GEOPM_REGION_HINT_SERIAL =    0x2000000000, // Single threaded region
    GEOPM_REGION_HINT_PARALLEL =  0x4000000000, // Region is threaded
    GEOPM_REGION_HINT_IGNORE =    0x8000000000, // Do not add region time to epoch
    GEOPM_MASK_REGION_HINT =      0xFF00000000,
};

int geopm_prof_region(const char *region_name,
                      uint64_t hint,
                      uint64_t *region_id);

int geopm_prof_enter(uint64_t region_id);

int geopm_prof_exit(uint64_t region_id);

int geopm_prof_progress(uint64_t region_id,
                        double fraction);

int geopm_prof_epoch(void);

int geopm_prof_shutdown(void);
""")
_dl = _ffi.dlopen('libgeopm.so')

REGION_HASH_INVALID = _dl.GEOPM_REGION_HASH_INVALID
REGION_HASH_UNMARKED = _dl.GEOPM_REGION_HASH_UNMARKED
REGION_HINT_UNKNOWN = _dl.GEOPM_REGION_HINT_UNKNOWN
REGION_HINT_COMPUTE = _dl.GEOPM_REGION_HINT_COMPUTE
REGION_HINT_MEMORY = _dl.GEOPM_REGION_HINT_MEMORY
REGION_HINT_NETWORK = _dl.GEOPM_REGION_HINT_NETWORK
REGION_HINT_IO = _dl.GEOPM_REGION_HINT_IO
REGION_HINT_SERIAL = _dl.GEOPM_REGION_HINT_SERIAL
REGION_HINT_PARALLEL = _dl.GEOPM_REGION_HINT_PARALLEL
REGION_HINT_IGNORE = _dl.GEOPM_REGION_HINT_IGNORE
MASK_REGION_HINT = _dl.GEOPM_MASK_REGION_HINT

def region(region_name, hint):
    region_name_cstr = _ffi.new("char[]", str(region_name))
    region_id_cptr = _ffi.new("uint64_t*")
    err = _dl.geopm_prof_region(region_name_cstr, hint, region_id_cptr)
    if err:
        raise RuntimeError("geopm_prof_region() failed: {}".format(
            error.message(err)))
    return region_id_cptr[0]

def region_enter(region_id):
    err = _dl.geopm_prof_enter(region_id)
    if err:
        raise RuntimeError("geopm_prof_enter() failed: {}".format(
            error.message(err)))

def region_exit(region_id):
    err = _dl.geopm_prof_exit(region_id)
    if err:
        raise RuntimeError("geopm_prof_exit() failed: {}".format(
            error.message(err)))

def progress(region_id, fraction):
    err = _dl.geopm_prof_progress(region_id, fraction)
    if err:
        raise RuntimeError("geopm_prof_progress() failed: {}".format(
            error.message(err)))

def epoch():
    err = _dl.geopm_prof_epoch()
    if err:
        raise RuntimeError("geopm_prof_epoch() failed: {}".format(
            error.message(err)))

def shutdown():
    err = _dl.geopm_prof_shutdown()
    if err:
        raise RuntimeError("geopm_prof_shutdown() failed: {}".format(
            error.message(err)))

