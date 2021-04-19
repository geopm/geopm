#!/usr/bin/env python
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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

import cffi
from . import error
import json


_ffi = cffi.FFI()
_ffi.cdef("""

int geopm_agent_supported(const char *agent_name);

int geopm_agent_num_policy(const char *agent_name,
                           int *num_policy);

int geopm_agent_policy_name(const char *agent_name,
                            int policy_idx,
                            size_t policy_name_max,
                            char *policy_name);

int geopm_agent_policy_json(const char *agent_name,
                            const double *policy_array,
                            size_t json_string_max,
                            char *json_string);

int geopm_agent_num_sample(const char *agent_name,
                           int *num_sample);

int geopm_agent_sample_name(const char *agent_name,
                            int sample_idx,
                            size_t sample_name_max,
                            char *sample_name);

int geopm_agent_num_avail(int *num_agent);

int geopm_agent_name(int agent_idx,
                     size_t agent_name_max,
                     char *agent_name);

int geopm_agent_enforce_policy(void);
""")
_dl = _ffi.dlopen('libgeopmpolicy.so', _ffi.RTLD_GLOBAL|_ffi.RTLD_LAZY)
_name_max = 1024
_policy_max = 8192

def policy_names(agent_name):
    """Get the names of the policies for a given agent.

    Args:
        agent_name (str): Name of agent type.

    Returns:
        list of str: Policy names required for the agent configuration.

    """
    agent_name_cstr = _ffi.new("char[]", agent_name.encode())
    num_policy = _ffi.new("int *")
    err = _dl.geopm_agent_num_policy(agent_name_cstr, num_policy)
    if err < 0:
        raise RuntimeError("geopm_agent_num_policy() failed: {}".format(
            error.message(err)))
    result = []
    for policy_idx in range(num_policy[0]):
        buff = _ffi.new("char[]", _name_max)
        err = _dl.geopm_agent_policy_name(agent_name_cstr, policy_idx, _name_max, buff)
        if err < 0:
            raise RuntimeError("geopm_agent_policy_name() failed: {}".format(
                error.message(err)))
        result.append(_ffi.string(buff).decode())
    return result


def policy_json(agent_name, policy_values):
    """Create a JSON policy for the given agent.

    This can be written to a file to control the agent statically.

    Args:
        agent_name (str): Name of agent type.
        policy_values (list of float): Values to use for each
        respective policy field.

    Returns:
        str: JSON str containing a valid policy using the given values.

    """
    agent_name_cstr = _ffi.new("char[]", agent_name.encode())
    policy_array = _ffi.new("double[]", policy_values)

    json_string = _ffi.new("char[]", _policy_max)
    err = _dl.geopm_agent_policy_json(agent_name_cstr, policy_array,
                                      _policy_max, json_string)

    if err < 0:
        raise RuntimeError("geopm_agent_policy_json() failed: {}".format(
            error.message(err)))
    return _ffi.string(json_string).decode()


def sample_names(agent_name):
    """Get all samples produced by the given agent.

    Args:
        agent_name (str): Name of agent type.

    Returns:
        list of str: List of sample names.
    """
    agent_name_cstr = _ffi.new("char[]", agent_name.encode())
    num_sample = _ffi.new("int *")
    err = _dl.geopm_agent_num_sample(agent_name_cstr, num_sample)
    if err < 0:
        raise RuntimeError("geopm_agent_num_sample() failed: {}".format(
            error.message(err)))
    result = []
    for sample_idx in range(num_sample[0]):
        buff = _ffi.new("char[]", _name_max)
        err = _dl.geopm_agent_sample_name(agent_name_cstr, sample_idx, _name_max, buff)
        if err < 0:
            raise RuntimeError("geopm_agent_sample_name() failed: {}".format(
                error.message(err)))
        result.append(_ffi.string(buff).decode())
    return result


def names():
    """Get the list of all available agents.

    Returns:
        list of str: List of all agent names.
    """
    num_agent = _ffi.new("int *")
    err = _dl.geopm_agent_num_avail(num_agent)
    if err < 0:
        raise RuntimeError("geopm_agent_num_avail() failed: {}".format(
            error.message(err)))
    buff = _ffi.new("char[]", _name_max)
    result = []
    for agent_idx in range(num_agent[0]):
        err = _dl.geopm_agent_name(agent_idx, _name_max, buff)
        if err < 0:
            raise RuntimeError("geopm_agent_name() failed: {}".format(
                error.message(err)))
        result.append(_ffi.string(buff).decode())
    return result


def enforce_policy():
    """Enforce a static implementation of the agent's policy.  The agent
       and the policy are chosen based on the GEOPM environment
       variables and configuration files.
    """
    err = _dl.geopm_agent_enforce_policy()
    if err < 0:
        raise RuntimeError("geopm_agent_enforce_policy() failed: {}".format(
            error.message(err)))


class AgentConf(object):
    """The GEOPM agent configuration parameters.

    This class contains all the parameters necessary to run the GEOPM
    agent with a workload.

    Attributes:
        path: The output path for this configuration file.
        options: A dict of the options for this agent.

    """
    def __init__(self, path, agent='monitor', options=dict()):
        supported_agents = {'monitor', 'power_governor', 'power_balancer', 'energy_efficient',
                            'frequency_map'}
        self._path = path
        if agent not in supported_agents:
            raise SyntaxError('<geopm> geopmpy.io: AgentConf does not support agent type: ' + agent + '!')
        self._agent = agent
        self._options = options

    def __repr__(self):
        return json.dumps(self._options)

    def __str__(self):
        return self.__repr__()

    def get_path(self):
        return self._path

    def get_agent(self):
        return self._agent

    def write(self):
        """Write the current config to a file."""
        policy_items = policy_names(self._agent)
        name_offsets = { name: offset for offset, name in enumerate(policy_items)}
        policy_values = [float('nan')] * len(name_offsets)

        # Earlier versions of this function had special handling per agent instead
        # of using the agent's policy names. Translate the old-style inputs to
        # use the new style for backward compatibility.
        old_names = []
        if self._agent in ['power_governor', 'power_balancer']:
            old_names = ['power_budget']
        elif self._agent in ['frequency_map', 'energy_efficient']:
            old_names = ['frequency_min', 'frequency_max']
        policy_dict = self._options.copy()
        for offset, name in enumerate(old_names):
            if name in policy_dict:
                policy_dict[policy_items[offset]] = policy_dict.pop(name)

        for (policy_name, policy_value) in policy_dict.items():
            if policy_name not in name_offsets:
                raise KeyError('<geopm> geopmpy.io: Policy "{}" does not exist in agent "{}"'.format(policy_name, self._agent))
            policy_offset = name_offsets[policy_name]
            policy_values[policy_offset] = policy_value

        with open(self._path, "w") as outfile:
            outfile.write(policy_json(self._agent, policy_values))


