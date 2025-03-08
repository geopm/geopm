#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from . import gffi
from . import error

def create_prof(shm_key, size, pid, uid, gid):
    shm_key_cstr = gffi.gffi.new("char[]", shm_key.encode())
    err = gffi.dl_geopmd.geopm_shmem_create_prof(shm_key_cstr, size, pid, uid, gid)
    if err < 0:
        raise RuntimeError('geopm_shmem_create_prof() failed: {}'.format(error.message(err)))

def path_prof(shm_key, pid, uid, gid):
    name_max = 1024
    shm_key_cstr = gffi.gffi.new("char[]", shm_key.encode())
    result_cstr = gffi.gffi.new("char[]", name_max)
    err = gffi.dl_geopmd.geopm_shmem_path_prof(shm_key_cstr, pid, uid, name_max, result_cstr)
    if err < 0:
        raise RuntimeError('geopm_shmem_path_prof() failed: {}'.format(error.message(err)))
    return gffi.gffi.string(result_cstr).decode()
