#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import math
from geopmdpy.gffi import gffi
from geopmdpy.gffi import get_dl_geopm
from geopmdpy import error

gffi.cdef("""
int geopm_policystore_connect(const char *data_path);

int geopm_policystore_disconnect();

int geopm_policystore_get_best(const char* agent_name, const char* profile_name,
                               size_t max_policy_vals, double* policy_vals);

int geopm_policystore_set_best(const char* agent_name, const char* profile_name,
                               size_t num_policy_vals, const double* policy_vals);

int geopm_policystore_set_default(const char* agent_name,
                                  size_t num_policy_vals, const double* policy_vals);
""")
_dl = get_dl_geopm()


def connect(database_path):
    """Connect to the database at the given location.  Creates a new
    database if one does not yet exist at the given location.

    Args:
        database_path (str): Path to the database.
    """
    global gffi
    global _dl

    database_path_cstr = gffi.new("char[]", database_path.encode())
    err = _dl.geopm_policystore_connect(database_path_cstr)
    if err < 0:
        raise RuntimeError('geopm_policystore_connect() failed: {}'.format(error.message(err)))


def disconnect():
    """Disconnect the associated database.  No-op if the database has already
    been disconnected.
    """
    global _dl

    err = _dl.geopm_policystore_disconnect()
    if err < 0:
        raise RuntimeError('geopm_policystore_disconnect() failed: {}'.format(error.message(err)))


def get_best(agent_name, profile_name):
    """Get the best known policy for a given agent/profile pair. If no best
    has been recorded, the default for the agent is returned.

    Args:
        agent_name (str): Name of the agent.
        profile_name (str): Name of the profile.

    Returns:
        list[float]: Best known policy for the profile and agent.
    """
    global gffi
    global _dl

    agent_name_cstr = gffi.new("char[]", agent_name.encode())
    profile_name_cstr = gffi.new("char[]", profile_name.encode())
    policy_max = 1024
    policy_array = gffi.new("double[]", policy_max)
    err = _dl.geopm_policystore_get_best(agent_name_cstr, profile_name_cstr,
                                         policy_max, policy_array)
    if err < 0:
        raise RuntimeError('geopm_policystore_get_best() failed: {}'.format(error.message(err)))
    last_non_default_index = next((i for i in reversed(range(len(policy_array))) if not math.isnan(policy_array[i])), -1)
    return list(policy_array[0:last_non_default_index+1])


def set_best(agent_name, profile_name, policy):
    """ Set the record for the best policy for a profile with an agent.

    Args:
        agent_name (str): Name of the agent.
        profile_name (str): Name of the profile.
        policy (list[float]): New policy to use.
    """
    global gffi
    global _dl

    agent_name_cstr = gffi.new("char[]", agent_name.encode())
    profile_name_cstr = gffi.new("char[]", profile_name.encode())
    policy_array = gffi.new("double[]", policy)
    err = _dl.geopm_policystore_set_best(agent_name_cstr, profile_name_cstr,
                                         len(policy), policy_array)
    if err < 0:
        raise RuntimeError('geopm_policystore_set_best() failed: {}'.format(error.message(err)))


def set_default(agent_name, policy):
    """ Set the default policy to use with an agent.

    Args:
        agent_name (str): Name of the agent.
        policy (list[float]): Default policy to use with the agent.
    """
    global gffi
    global _dl

    agent_name_cstr = gffi.new("char[]", agent_name.encode())
    policy_array = gffi.new("double[]", policy)
    err = _dl.geopm_policystore_set_default(agent_name_cstr, len(policy), policy_array)
    if err < 0:
        raise RuntimeError('geopm_policystore_set_default() failed: {}'.format(error.message(err)))
