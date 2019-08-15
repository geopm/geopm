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

from builtins import str
import cffi
from . import error


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
    """Get the number of domains available on the system of a specific
    domain type.  If the domain is valid, but there are no domains of
    that type on the system, the return value is zero.  Invalid domain
    specificiation will result in a raised exception.

    Args:
        domain (int or str): The domain type from one of topo.DOMAIN_*
            integer values or a domain name string.

    Returns:
        int: The number of domains of the specified type available on
            the system.

    """
    global _dl
    domain = domain_type(domain)
    result = _dl.geopm_topo_num_domain(domain)
    if result < 0:
        raise RuntimeError("geopm_topo_num_domain() failed: {}".format(
            error.message(result)))
    return result

def domain_idx(domain, cpu_idx):
    """Get the index of the domain that is local to a specific Linux
    logical CPU.  The return value will be greater than or equal to
    zero and less than the value returned by num_domain(domain).  An
    exception is raised if either input is out of range or invalid.

    Args:
        domain (int or str): The domain type from one of topo.DOMAIN_*
            integer values or a domain name string.
        cpu_idx (int): The Linux logical CPU that is associated with
            the returned domain type.

    Returns:
        int: Domain index associated with the specified logical CPU.

    """
    global _dl
    domain = domain_type(domain)
    result = _dl.geopm_topo_domain_idx(domain, cpu_idx)
    if result < 0:
        raise RuntimeError("geopm_topo_domain_idx() failed: {}".format(
            error.message(result)))
    return result

def domain_nested(inner_domain, outer_domain, outer_idx):
    """Get a list of all inner domains nested within a specified outer
    domain.  An exception is raised if the inner domain is not within
    the outer domain or if the index is out of range.

    Args:
        inner_domain (int or str): The inner domain type from one of
            topo.DOMAIN_* integer values or a domain name string.
        outer_domain (int or str): The outer domain type from one of
            topo.DOMAIN_* integer values or a domain name string.
        outer_idx (int): Index of the outer domain that is queried.

    Returns:
        list of int: The inner domain indices that are contained
            within the specified outer domain.

    """
    global _ffi
    global _dl
    inner_domain = domain_type(inner_domain)
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
    """Get the domain name corresponding to the domain type specified.  If
    domain is a string, a check is done that the string is a valid
    domain type and then the input string is returned.  This is the
    inverse of the domain_type() function.

    Args:
        domain (int or str): The domain type from one of topo.DOMAIN_*
            integer values or a domain name string.

    Returns:
        str: Domain name string associated with input.

    """
    global _ffi
    global _dl
    domain = domain_type(domain)
    name_max = 1024
    result_cstr = _ffi.new("char[]", name_max)
    err = _dl.geopm_topo_domain_name(domain, name_max, result_cstr)
    if err < 0:
        raise RuntimeError("geopm_topo_domain_name() failed: {}".format(
                           error.message(err)))
    return _ffi.string(result_cstr).decode()

def domain_type(domain):
    """Returns the domain type that is associated with the provided domain
    string.  If domain is of integer type, a check is done that it is
    a valid topo.DOMAIN_* value and then the input value is returned.
    This is the inverse of the domain_name() function.  If domain is a
    string that does not match any of the valid domain names, or if domain
    is an integer that is out of range an exception is raised.

    Args:
        domain (str or int): A domain name string or the domain type
            from one of topo.DOMAIN_* integer values.

    Returns:
        int: A domain type matching one of the topo.DOMAIN_* values
            associated with the input.

    """
    if type(domain) is int:
        if domain >= 0 and domain < NUM_DOMAIN:
            result = domain
        else:
            raise RuntimeError("domain_type is out of range: {}".format(domain))
    else:
        global _dl
        domain_cstr = _ffi.new("char[]", domain.encode())
        result = _dl.geopm_topo_domain_type(domain_cstr)
        if result < 0:
            raise RuntimeError("geopm_topo_domain_type() failed: {}".format(
                error.message(result)))
    return result

def create_cache():
    """Create a cache file for the platform topology if one does not
    exist.  This cache file will be used by any calls to the other
    functions in the topo module as well as any use of the GEOPM
    runtime.  File permissions of the cache file are set to
    "-rw-rw-rw-", i.e. 666. The path for the cache file is
    /tmp/geopm-topo-cache.  If the file exists no operation will be
    performed.  To force the creation of a new cache file call
    os.unlink('/tmp/geopm-topo-cache') prior to calling this function.

    """
    global _dl
    err = _dl.geopm_topo_create_cache()
    if err < 0:
        raise RuntimeError("geopm_topo_create_cache() failed: {}".format(
            error.message(err)))
