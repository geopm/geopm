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

from builtins import next
from builtins import range
import math
import cffi
from . import error
_ffi = cffi.FFI()
_ffi.cdef("""
int geopm_policystore_connect(const char *data_path);

int geopm_policystore_disconnect();

int geopm_policystore_get_best(const char* profile_name, const char* agent_name,
                               size_t max_policy_vals, double* policy_vals);

int geopm_policystore_set_best(const char* profile_name, const char* agent_name,
                               size_t num_policy_vals, const double* policy_vals);

int geopm_policystore_set_default(const char* agent_name,
                                  size_t num_policy_vals, const double* policy_vals);
""")
_dl = _ffi.dlopen('libgeopmpolicy.so')

def connect(database_path):
    """Connect to the database at the given location.  Creates a new
    database if one does not yet exist at the given location.

    Args:
        database_path (str): Path to the database.
    """
    database_path_cstr = _ffi.new("char[]", database_path.encode())
    err = _dl.geopm_policystore_connect(database_path_cstr)
    if err < 0:
        raise RuntimeError('geopm_policystore_connect() failed: {}'.format(error.message(err)))

def disconnect():
    """Disconnect the associated database.  No-op if the database has already
    been disconnected.
    """
    err = _dl.geopm_policystore_disconnect()
    if err < 0:
        raise RuntimeError('geopm_policystore_disconnect() failed: {}'.format(error.message(err)))

def get_best(profile_name, agent_name):
    """Get the best known policy for a given agent/profile pair. If no best
    has been recorded, the default for the agent is returned.

    Args:
        profile_name (str): Name of the profile.
        agent_name (str): Name of the agent.

    Returns:
        list of float: Best known policy for the profile and agent.
    """
    profile_name_cstr = _ffi.new("char[]", profile_name.encode())
    agent_name_cstr = _ffi.new("char[]", agent_name.encode())
    policy_max = 1024
    policy_array = _ffi.new("double[]", policy_max)
    err = _dl.geopm_policystore_get_best(profile_name_cstr, agent_name_cstr,
                                         policy_max, policy_array)
    if err < 0:
        raise RuntimeError('geopm_policystore_get_best() failed: {}'.format(error.message(err)))
    last_non_default_index = next((i for i in reversed(range(len(policy_array))) if not math.isnan(policy_array[i])), -1)
    return list(policy_array[0:last_non_default_index+1])

def set_best(profile_name, agent_name, policy):
    """ Set the record for the best policy for a profile with an agent.

    Args:
        profile_name (str): Name of the profile.
        agent_name (str): Name of the agent.
        policy (list of double): New policy to use.
    """
    profile_name_cstr = _ffi.new("char[]", profile_name.encode())
    agent_name_cstr = _ffi.new("char[]", agent_name.encode())
    policy_array = _ffi.new("double[]", policy)
    err = _dl.geopm_policystore_set_best(profile_name_cstr, agent_name_cstr,
                                         len(policy), policy_array)
    if err < 0:
        raise RuntimeError('geopm_policystore_set_best() failed: {}'.format(error.message(err)))

def set_default(agent_name, policy):
    """ Set the default policy to use with an agent.

    Args:
        agent_name (str): Name of the agent.
        policy (list of double): Default policy to use with the agent.
    """
    agent_name_cstr = _ffi.new("char[]", agent_name.encode())
    policy_array = _ffi.new("double[]", policy)
    err = _dl.geopm_policystore_set_default(agent_name_cstr, len(policy), policy_array)
    if err < 0:
        raise RuntimeError('geopm_policystore_set_default() failed: {}'.format(error.message(err)))
