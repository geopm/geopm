#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from . import gffi
from . import error

gffi.gffi.cdef("""

    int geopm_shmem_create_prof(const char *shm_key, size_t size, int pid, int uid, int gid);
    int geopm_shmem_path_prof(const char *shm_key, int pid, int uid, size_t shm_path_max, char *shm_path);

""")
_dl = gffi.get_dl_geopmd()

def create_prof(shm_key, size, pid, uid, gid):
    global _dl
    shm_key_cstr = gffi.gffi.new("char[]", shm_key.encode())
    err = _dl.geopm_shmem_create_prof(shm_key_cstr, size, pid, uid, gid)
    if err < 0:
        raise RuntimeError('geopm_shmem_create() failed: {}'.format(error.message(err)))

def path_prof(shm_key, pid, uid, gid):
    global _dl
    name_max = 1024
    shm_key_cstr = gffi.gffi.new("char[]", shm_key.encode())
    result_cstr = gffi.gffi.new("char[]", name_max)
    err = _dl.geopm_shmem_path_prof(shm_key_cstr, pid, uid, name_max, result_cstr)
    if err < 0:
        raise RuntimeError('geopm_shmem_destroy() failed: {}'.format(error.message(err)))
    return gffi.gffi.string(result_cstr).decode()
