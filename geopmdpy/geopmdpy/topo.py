#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


from . import gffi
from . import error

DOMAIN_INVALID = gffi.dl_geopmd.GEOPM_DOMAIN_INVALID
DOMAIN_BOARD = gffi.dl_geopmd.GEOPM_DOMAIN_BOARD
DOMAIN_PACKAGE = gffi.dl_geopmd.GEOPM_DOMAIN_PACKAGE
DOMAIN_CORE = gffi.dl_geopmd.GEOPM_DOMAIN_CORE
DOMAIN_CPU = gffi.dl_geopmd.GEOPM_DOMAIN_CPU
DOMAIN_MEMORY = gffi.dl_geopmd.GEOPM_DOMAIN_MEMORY
DOMAIN_PACKAGE_INTEGRATED_MEMORY = gffi.dl_geopmd.GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY
DOMAIN_NIC = gffi.dl_geopmd.GEOPM_DOMAIN_NIC
DOMAIN_PACKAGE_INTEGRATED_NIC = gffi.dl_geopmd.GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC
DOMAIN_GPU = gffi.dl_geopmd.GEOPM_DOMAIN_GPU
DOMAIN_PACKAGE_INTEGRATED_GPU = gffi.dl_geopmd.GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU
DOMAIN_GPU_CHIP = gffi.dl_geopmd.GEOPM_DOMAIN_GPU_CHIP
NUM_DOMAIN = gffi.dl_geopmd.GEOPM_NUM_DOMAIN

def num_domain(domain):
    """Get the number of domains available on the system of a specific
    domain type.  If the domain is valid, but there are no domains of
    that type on the system, the return value is zero.  Invalid domain
    specification will result in a raised exception.

    Args:
        domain (int or str): The domain type from one of topo.DOMAIN_*
            integer values or a domain name string.

    Returns:
        int: The number of domains of the specified type available on
            the system.

    """
    domain = domain_type(domain)
    result = gffi.dl_geopmd.geopm_topo_num_domain(domain)
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
    domain = domain_type(domain)
    result = gffi.dl_geopmd.geopm_topo_domain_idx(domain, cpu_idx)
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
        list[int]: The inner domain indices that are contained
            within the specified outer domain.

    """
    inner_domain = domain_type(inner_domain)
    outer_domain = domain_type(outer_domain)
    num_domain_nested = gffi.dl_geopmd.geopm_topo_num_domain_nested(inner_domain, outer_domain)
    if (num_domain_nested < 0):
        raise RuntimeError("geopm_topo_num_domain_nested() failed: {}".format(
            error.message(num_domain_nested)))
    domain_nested_carr = gffi.gffi.new("int[]", num_domain_nested)
    gffi.dl_geopmd.geopm_topo_domain_nested(inner_domain, outer_domain, outer_idx,
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
    domain = domain_type(domain)
    name_max = 1024
    result_cstr = gffi.gffi.new("char[]", name_max)
    err = gffi.dl_geopmd.geopm_topo_domain_name(domain, name_max, result_cstr)
    if err < 0:
        raise RuntimeError("geopm_topo_domain_name() failed: {}".format(
                           error.message(err)))
    return gffi.gffi.string(result_cstr).decode()

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
        domain_cstr = gffi.gffi.new("char[]", domain.encode())
        result = gffi.dl_geopmd.geopm_topo_domain_type(domain_cstr)
        if result < 0:
            raise RuntimeError("geopm_topo_domain_type() failed: {}".format(
                error.message(result)))
    return result

def create_cache():
    """Create a cache file for the platform topology if one does not
    exist.  This cache file will be used by any calls to the other
    functions in the topo module as well as any use of the GEOPM
    runtime.  File permissions of the cache file are set to
    "-rw-r--r--", i.e. 644. The path for the cache file is
    /run/geopm/geopm-topo-cache.  If this file is older than
    the last boot it will be regenerated.  If the file exists but has
    improper permissions is will be regenerated.  If the file has been
    created since the last boot with the correct permissions, no
    operation will be performed.  To force the creation of a new cache
    file call os.unlink('/run/geopm/geopm-topo-cache') prior
    to calling this function.

    """
    err = gffi.dl_geopmd.geopm_topo_create_cache()
    if err < 0:
        raise RuntimeError("geopm_topo_create_cache() failed: {}".format(
            error.message(err)))
