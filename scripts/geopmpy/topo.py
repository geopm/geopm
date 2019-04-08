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

import cffi
import error


_ffi = cffi.FFI()
_ffi.cdef("""
enum geopm_domain_e {
    GEOPM_DOMAIN_INVALID = -1,
    GEOPM_DOMAIN_BOARD = 0,
    GEOPM_DOMAIN_PACKAGE = 1,
    GEOPM_DOMAIN_CORE = 2,
    GEOPM_DOMAIN_CPU = 3,
    GEOPM_DOMAIN_BOARD_MEMORY = 4,
    GEOPM_DOMAIN_PACKAGE_MEMORY = 5,
    GEOPM_DOMAIN_BOARD_NIC = 6,
    GEOPM_DOMAIN_PACKAGE_NIC = 7,
    GEOPM_DOMAIN_BOARD_ACCELERATOR = 8,
    GEOPM_DOMAIN_PACKAGE_ACCELERATOR = 9,
    GEOPM_NUM_DOMAIN = 10,
};

int geopm_topo_num_domain(int domain_type);

int geopm_topo_domain_idx(int domain_type,
                          int cpu_idx);

int geopm_topo_num_domain_nested(int inner_domain,
                                 int outer_domain);

int geopm_topo_domain_nested(int inner_domain,
                             int outer_domain,
                             int outer_idx,
                             size_t num_domain_nested,
                             int *domain_nested);

int geopm_topo_domain_name(int domain_type,
                           size_t domain_name_max,
                           char *domain_name);

int geopm_topo_domain_type(const char *domain_name);

int geopm_topo_create_cache(void);
""")
_dl = _ffi.dlopen('libgeopmpolicy.so')

DOMAIN_INVALID = _dl.GEOPM_DOMAIN_INVALID
DOMAIN_BOARD = _dl.GEOPM_DOMAIN_BOARD
DOMAIN_PACKAGE = _dl.GEOPM_DOMAIN_PACKAGE
DOMAIN_CORE = _dl.GEOPM_DOMAIN_CORE
DOMAIN_CPU = _dl.GEOPM_DOMAIN_CPU
DOMAIN_BOARD_MEMORY = _dl.GEOPM_DOMAIN_BOARD_MEMORY
DOMAIN_PACKAGE_MEMORY = _dl.GEOPM_DOMAIN_PACKAGE_MEMORY
DOMAIN_BOARD_NIC = _dl.GEOPM_DOMAIN_BOARD_NIC
DOMAIN_PACKAGE_NIC = _dl.GEOPM_DOMAIN_PACKAGE_NIC
DOMAIN_BOARD_ACCELERATOR = _dl.GEOPM_DOMAIN_BOARD_ACCELERATOR
DOMAIN_PACKAGE_ACCELERATOR = _dl.GEOPM_DOMAIN_PACKAGE_ACCELERATOR
NUM_DOMAIN = _dl.GEOPM_NUM_DOMAIN

def num_domain(domain):
    global _dl
    if type(domain) is str:
        domain = domain_type(domain)
    result = _dl.geopm_topo_num_domain(domain)
    if result < 0:
        raise RuntimeError("geopm_topo_num_domain() failed: {}".format(
            error.message(result)))
    return result

def domain_idx(domain, cpu_idx):
    global _dl
    if type(domain) is str:
        domain = domain_type(domain)
    result = _dl.geopm_topo_domain_idx(domain, cpu_idx)
    if result < 0:
        raise RuntimeError("geopm_topo_domain_idx() failed: {}".format(
            error.message(result)))
    return result

def domain_nested(inner_domain, outer_domain, outer_idx):
    global _ffi
    global _dl
    if type(inner_domain) is str:
        inner_domain = domain_type(inner_domain)
    if type(outer_domain) is str:
        outer_domain = domain_type(outer_domain)
    num_domain_nested = _dl.geopm_topo_num_domain_nested(inner_domain, outer_domain)
    if (num_domain_nested < 0):
        raise RuntimeError("geopm_topo_num_domain_nested() failed: {}".format(
            error.message(num_domain_nested)))
    domain_nested_carr = _ffi.new("int[]", num_domain_nested)
    _dl.geopm_topo_domain_nested(inner_domain, outer_domain, outer_idx,
                                 num_domain_nested, domain_nested_carr)
    result = []
    for dom in domain_nested_carr:
        result.append(dom)
    return result

def domain_name(domain):
    global _ffi
    global _dl
    if type(domain) is str:
        domain = domain_type(domain)
    name_max = 1024
    result_cstr = _ffi.new("char[]", name_max)
    err = _dl.geopm_topo_domain_name(domain, name_max, result_cstr)
    if err < 0:
        raise RuntimeError("geopm_topo_domain_name() failed: {}".format(
                           error.message(err)))
    return _ffi.string(result_cstr)

def domain_type(domain_name):
    global _dl
    domain_name_cstr = _ffi.new("char[]", str(domain_name))
    result = _dl.geopm_topo_domain_type(domain_name_cstr)
    if result < 0:
        raise RuntimeError("geopm_topo_domain_type() failed: {}".format(
            error.message(result)))
    return result

def create_cache():
    global _dl
    err = _dl.geopm_topo_create_cache()
    if err < 0:
        raise RuntimeError("geopm_topo_create_cache() failed: {}".format(
            error.message(err)))

if __name__ == "__main__":
    import sys

    sys.stdout.write("Number of cpus: {}\n".format(num_domain("cpu")))
    sys.stdout.write("Number of packages: {}\n".format(num_domain(DOMAIN_PACKAGE)))
    sys.stdout.write("CPU 1 is on package {}\n".format(domain_idx(DOMAIN_PACKAGE, 1)))
    sys.stdout.write("domain_nested('cpu', 'package', 0) = {}\n".format(domain_nested('cpu', 'package', 0)))
    sys.stdout.write("domain_name(3) = {}\n".format(domain_name(3)))
    sys.stdout.write("domain_type('cpu') = {}\n".format(domain_type('cpu')))
    try:
        sys.stdout.write("Number of cpus: {}\n".format(num_domain("foobar")))
        raise Exception("Topo.num_domain('foobar') failed to raise exception")
    except RuntimeError as ex:
        sys.stderr.write('{}\n'.format(ex))
    try:
        sys.stdout.write("Number of cpus: {}\n".format(num_domain(100)))
        raise Exception("Topo.num_domain(100) failed to raise exception")
    except RuntimeError as ex:
        sys.stderr.write('{}\n'.format(ex))
