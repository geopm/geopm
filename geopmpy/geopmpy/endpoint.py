#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
from geopmpy import gffi
from geopmdpy import error
import geopmpy.agent
from typing import Union, Mapping, List, Dict, Tuple

if not hasattr(gffi.dl_geopm, 'geopm_endpoint_create'):
    raise ImportError('geopmpy.endpoint cannot be imported because the installed '
                      'libgeopm does not include the Endpoint feature. '
                      'Rebuild libgeopm with the --enable-beta configuration flag '
                      'then reinstall geopmpy.')

_name_max = 1024
_policy_max = 8192


class Endpoint:
    def __init__(self, name: str):
        """ Create an endpoint object for other API functions.

        Args:
            name (str): Shared memory key substring used to create an endpoint
                        that an agent can attach to.
        """
        name_cstr = gffi.gffi.new("char[]", name.encode())
        p_endpoint = gffi.gffi.new("struct geopm_endpoint_c **")
        self._name = name
        err = gffi.dl_geopm.geopm_endpoint_create(name_cstr, p_endpoint)
        if err != 0:
            self._endpoint = None
            raise RuntimeError("geopm_endpoint_create() failed: {}".format(
                               error.message(err)))
        self._endpoint = p_endpoint[0]

    def __del__(self):
        """ Release resources associated with endpoint.
        """
        err = gffi.dl_geopm.geopm_endpoint_destroy(self._endpoint)
        if err != 0:
            raise RuntimeError("geopm_endpoint_destroy() failed: {}".format(
                               error.message(err)))

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def __repr__(self):
        return f'Endpoint(name={self._name!r})'

    def open(self):
        """ Create shmem regions within the endpoint for policy/sample handling.
        """
        err = gffi.dl_geopm.geopm_endpoint_open(self._endpoint)
        if err != 0:
            raise RuntimeError("geopm_endpoint_open() failed: {}".format(
                               error.message(err)))

    def close(self):
        """Destroy shmem regions within the endpoint.
        """
        err = gffi.dl_geopm.geopm_endpoint_close(self._endpoint)
        if err != 0:
            raise RuntimeError("geopm_endpoint_close() failed: {}".format(
                               error.message(err)))

    def agent(self):
        """Get the name of the attached agent.

        Returns:
            Union[str, None]: The name of the current agent, or None if there
                              is no agent attached.
        """
        agent_name_cstr = gffi.gffi.new("char[]", _name_max)
        err = gffi.dl_geopm.geopm_endpoint_agent(self._endpoint, _name_max, agent_name_cstr)
        if err == 0:
            return gffi.gffi.string(agent_name_cstr).decode()
        elif err == -13:  # GEOPM_ERROR_NO_AGENT
            return None
        else:
            raise RuntimeError("geopm_endpoint_agent() failed: {}".format(
                               error.message(err)))

    def wait_for_agent_attach(self, timeout: float):
        """Block until an agent has attached or the timeout is reached

        Args:
            timeout (float): Timeout in seconds.
        """
        err = gffi.dl_geopm.geopm_endpoint_wait_for_agent_attach(self._endpoint, timeout)
        if err != 0:
            raise RuntimeError("geopm_endpoint_wait_for_agent_attach() failed: {}".format(
                               error.message(err)))

    def stop_wait_loop(self):
        """Stop any wait loops the endpoint is running.
        """
        err = gffi.dl_geopm.geopm_endpoint_wait_for_agent_stop_wait_loop(self._endpoint)
        if err != 0:
            raise RuntimeError("geopm_endpoint_wait_for_agent_stop_wait_loop() failed: {}".format(
                               error.message(err)))

    def reset_wait_loop(self):
        """Reset the endpoint to prepare for wait_for_agent_attach()
        """
        err = gffi.dl_geopm.geopm_endpoint_wait_for_agent_reset_wait_loop(self._endpoint)
        if err != 0:
            raise RuntimeError("geopm_endpoint_wait_for_agent_reset_wait_loop() failed: {}".format(
                               error.message(err)))

    def profile_name(self) -> Union[str, None]:
        """Get the profile name of the attached job.

        Returns:
            Union[str, None]: The name of the current profile, or None if there
                              is no agent attached.
        """
        profile_name_cstr = gffi.gffi.new("char[]", _name_max)
        err = gffi.dl_geopm.geopm_endpoint_profile_name(self._endpoint, _name_max, profile_name_cstr)
        if err == 0:
            return gffi.gffi.string(profile_name_cstr).decode()
        elif err == -13:  # GEOPM_ERROR_NO_AGENT
            return None
        else:
            raise RuntimeError("geopm_endpoint_profile_name() failed: {}".format(
                               error.message(err)))

    def nodes(self) -> List[str]:
        """Get the list of nodes managed by the agent.

        Returns:
            List[str]: List of node names managed by the agent.
        """
        num_node_p = gffi.gffi.new("int *")
        err = gffi.dl_geopm.geopm_endpoint_num_node(self._endpoint, num_node_p)
        if err != 0:
            raise RuntimeError("geopm_endpoint_num_node() failed: {}".format(
                               error.message(err)))

        num_node = num_node_p[0]
        node_list = list()
        for node_idx in range(num_node):
            node_name_cstr = gffi.gffi.new("char[]", _name_max)
            err = gffi.dl_geopm.geopm_endpoint_node_name(
                self._endpoint, node_idx, _name_max, node_name_cstr)
            if err != 0:
                raise RuntimeError("geopm_endpoint_node_name() failed: {}".format(
                    error.message(err)))
            node_list.append(gffi.gffi.string(node_name_cstr).decode())
        return node_list

    def write_policy(self, policy: Mapping[str, float]):
        """Write a policy to the current agent.

        Args:
            policy (Mapping[str, float]): The mapping of list of agent policy names
                                          to policy values to write.
        """
        agent = self.agent()
        if agent is None:
            raise RuntimeError("Cannot write a policy since no agent is running.")
        policy_names = geopmpy.agent.policy_names(agent)
        num_policy = len(policy_names)
        policy_array = gffi.gffi.new("double[]", [policy.get(policy_name, float('nan'))
                                             for policy_name in policy_names])
        err = gffi.dl_geopm.geopm_endpoint_write_policy(self._endpoint, num_policy, policy_array)
        if err != 0:
            raise RuntimeError("geopm_endpoint_write_policy() failed: {}".format(
                error.message(err)))

    def read_sample(self) -> Tuple[float, Dict[str, float]]:
        """Take a sample from the agent.

        Args:
            None

        Returns:
            tuple[float, Dict[str, float]]: The age of the agent's last
                update to the sample in seconds, and a dict of sample names
                mapped to sample values.
        """
        agent = self.agent()
        if agent is None:
            raise RuntimeError("Cannot read samples since no agent is running.")
        sample_names = geopmpy.agent.sample_names(agent)
        num_sample = len(sample_names)
        sample_array = gffi.gffi.new("double[]", num_sample)
        sample_age_p = gffi.gffi.new("double *")
        err = gffi.dl_geopm.geopm_endpoint_read_sample(self._endpoint, num_sample, sample_array, sample_age_p)
        if err != 0:
            raise RuntimeError("geopm_endpoint_write_policy() failed: {}".format(
                error.message(err)))
        return sample_age_p[0], dict(zip(sample_names, sample_array))
