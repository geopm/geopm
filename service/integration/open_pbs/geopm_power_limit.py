#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import sys

PYTHON_PATHS = [
        "/usr/lib/python3.6/site-packages",
        "/usr/lib64/python3.6/site-packages"]

for p in PYTHON_PATHS:
    if p not in sys.path:
        sys.path.insert(0, p)

import os
import json
import copy
import math

import pbs

from geopmdpy import pio
from geopmdpy import system_files


_SAVED_CONTROLS_PATH = "/run/geopm/pbs-hooks/SAVE_FILES"
_SAVED_CONTROLS_FILE = _SAVED_CONTROLS_PATH + "/power-limit-save-control.json"
_POWER_LIMIT_RESOURCE = "geopm-node-power-limit"
_MAX_POWER_LIMIT_RESOURCE = "geopm-max-node-power-limit"
_MIN_POWER_LIMIT_RESOURCE = "geopm-min-node-power-limit"
_JOB_POWER_LIMIT_RESOURCE = "geopm-job-power-limit"
_DEFAULT_SLOWDOWN_RESOURCE = "geopm-default-slowdown"
_JOB_TYPE_RESOURCE = "geopm-job-type"
_DEFAULT_SLOWDOWN = 0.0

_power_limit_control = {
        "name": "MSR::PLATFORM_POWER_LIMIT:PL1_POWER_LIMIT",
        "domain_type": "board",
        "domain_idx": 0,
        "setting": None
    }
_controls = [
        {"name": "MSR::PLATFORM_POWER_LIMIT:PL1_TIME_WINDOW",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 0.013}, # SDM Vol. 4. Table 2.39 - Recommends 0xD = 13
        {"name": "MSR::PLATFORM_POWER_LIMIT:PL1_CLAMP_ENABLE",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 1},
        _power_limit_control,
        {"name": "MSR::PLATFORM_POWER_LIMIT:PL1_LIMIT_ENABLE",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 1}
    ]


def clip_list(list_to_clip, min_value, max_value):
    """Clip each element in a list.
    """
    return [max(min_value, min(x, max_value)) for x in list_to_clip]


def slowdown_at_power(power, x0, A, B, C):
    return [An * (x0n - power)**2 + Bn * (x0n - power) + Cn
            for x0n, An, Bn, Cn in zip(x0, A, B, C)]


def power_at_slowdown(slowdown, x0, A, B, C):
    return clip_list([(x0n - (-Bn + math.sqrt(Bn**2 - 4 * An * (Cn - slowdown))) / (2 * An))
                      for x0n, An, Bn, Cn in zip(x0, A, B, C)], 0, 1)


def power_deficit_at_slowdown(slowdown, budget, max_node_power, x0, A, B, C):
    """Return the power deficit at a target uniform slowdown across nodes for a
    given job power budget. I.e., the zero-crossing point is where the budget
    is allocated exactly, and where all jobs have the same expected slowdown.
    """
    return max_node_power * sum(power_at_slowdown(slowdown, x0, A, B, C)) - budget


def bisect_power_deficit_by_slowdown(min_slowdown, max_slowdown, max_iters, tolerance, args):
    """Find the amount of slowdown that when all hosts are assigned that
    slowdown, the sum of their power caps equals the budget.

    This implementation assumes slowdown decreases as power increases within
    the [min_slowdown, max_slowdown] range.

    Args:
    min_slowdown Lower bound of slowdown values to search.
    max_slowdown Upper bound of slowdown values to search.
    tolerance How close, in watts, we need to reach the budget.
    args Arguments to forward to power_deficit_at_slowdown
    """
    lhs = min_slowdown
    rhs = max_slowdown

    for iteration in range(max_iters):
        mid = (lhs + rhs) / 2
        deficit = power_deficit_at_slowdown(mid, *args)
        if deficit > tolerance:
            # We are over-using our budget at this level of slowdown. Increase
            # our lower bound of slowdown.
            lhs = mid
        elif deficit < -tolerance:
            # We are under-using the budget at this level of slowdown. Decrease
            # our upper bound of slowdown.
            rhs = mid
        else:
            return mid
    return mid


def allocate_budget_to_nodes(budget, max_node_power, x0, A, B, C):
    """Distribute a given job budget to node power caps, targeting even slowdown
    across nodes, as expected by the node performance models.
    """
    # Even if one node expects 0% slowdown at max power, that may not be true of
    # all nodes executing this job. Limit our search to the achievable range.
    long_pole_slowdown_at_max_power = max(slowdown_at_power(1, x0, A, B, C))
    long_pole_slowdown_at_min_power = max(slowdown_at_power(0, x0, A, B, C))

    max_balanced_power = power_at_slowdown(long_pole_slowdown_at_max_power, x0, A, B, C)

    if budget > sum(max_balanced_power) * max_node_power:
        # Bisection won't do any good in this case, since the equal-slowdown
        # power either doesn't exist or is at a greater power limit than we
        # are able to set on each node.
        slowdown = long_pole_slowdown_at_max_power
    else:
        slowdown = bisect_power_deficit_by_slowdown(
            long_pole_slowdown_at_max_power,
            long_pole_slowdown_at_min_power,
            max_iters=50,
            tolerance=0.1,
            args=(budget, max_node_power, x0, A, B, C))

    power_by_node = [p * max_node_power for p in power_at_slowdown(slowdown, x0, A, B, C)]

    # Evenly distribute slack power budget wherever it can be used.
    unused_budget = budget - sum(power_by_node)
    unused_power_by_node = [max_node_power - p for p in power_by_node]
    total_unused_node_power = sum(unused_power_by_node)
    power_by_node = [p_used + p_unused / total_unused_node_power * unused_budget
                     for p_used, p_unused in zip(power_by_node, unused_power_by_node)]

    power_by_node = clip_list(power_by_node, 0, max_node_power)
    return slowdown, power_by_node


def get_model_from_config(hook_config, job_type, per_host=False):
    if hook_config is None or job_type is None:
        return None

    if 'profiles' not in hook_config:
        pbs.logmsg(pbs.LOG_WARNING, 'Missing profiles section in the GEOPM PBS config')
        return None

    model_max_power = hook_config.get("max_power", None)
    if model_max_power is None:
        pbs.logmsg(pbs.LOG_WARNING, 'Missing max_power in the GEOPM PBS config')
        return None

    if job_type not in hook_config['profiles']:
        pbs.logmsg(pbs.LOG_WARNING, f'Requested job type {job_type} has no performance model in the GEOPM PBS config')
        return None

    profile = hook_config['profiles'][job_type]
    if per_host:
        if 'hosts' in profile:
            models = dict(max_power = model_max_power)
            for host_name, host_data in profile['hosts'].items():
                model = host_data['model']
                model['x0'] = float(model['x0'])
                model['A'] = float(model['A'])
                model['B'] = float(model['B'])
                model['C'] = float(model['C'])
                models[host_name] = model
            return models
    else:
        model_coefficients = profile.get('model', dict())
        try:
            x0 = float(model_coefficients['x0'])
            A = float(model_coefficients['A'])
            B = float(model_coefficients['B'])
            C = float(model_coefficients['C'])
        except:
            pbs.logmsg(pbs.LOG_WARNING, f'Invalid coefficients for profile {job_type} in GEOPM PBS config')
            return None

        return {
            'max_power': model_max_power,
            'x0': x0,
            'A': A,
            'B': B,
            'C': C,
        }


def predict_power_cap_at_performance_factor(job_type, slowdown, min_power_per_node, max_power_per_node):
    """Predict the node power cap needed to achieve a target slowdown for a
    given job type. If job_type is None or is not configured, this function
    assumes a 1:1 linear mapping between power and performance (half power
    results in half performance). Slowdown of 0 means min time, slowdown of
    1 means twice the min time (100% slowdown).
    """
    hook_config = None
    if pbs.hook_config_filename is not None:
        with open(pbs.hook_config_filename) as f:
            hook_config = json.load(f)

    model = get_model_from_config(hook_config, job_type)
    do_use_model = model is not None

    if do_use_model:
        try:
            # Using a quadratic model: slowdown = A * (x0 - percent_of_tdp)^2 + B * (x0 - percent_of_tdp) + C
            # Solve for the positive root (less than 100% of max power) at '-slowdown' offset:
            result = model['max_power'] * (model['x0'] - (-model['B'] + math.sqrt(model['B']**2 - 4 * model['A'] * (model['C'] - slowdown))) / (2 * model['A']))
        except Exception as e:
            pbs.logmsg(pbs.LOG_WARNING, f'Unable to estimate job power. {str(e)}')
            do_use_model = False

    if not do_use_model:
        # Fallback case: Assume 1:1 linear mapping between power and performance
        result = max_power_per_node / (slowdown + 1)

    return min(max(min_power_per_node, result), max_power_per_node)


def read_controls(controls):
    try:
        for c in controls:
            c["setting"] = pio.read_signal(c["name"], c["domain_type"],
                                           c["domain_idx"])
    except RuntimeError as e:
        reject_event(f"Unable to read signal {c['name']}: {e}")


def write_controls(controls):
    try:
        for c in controls:
            pio.write_control(c["name"], c["domain_type"], c["domain_idx"],
                              c["setting"])
    except RuntimeError as e:
        reject_event(f"Unable to write control {c['name']}: {e}")


def resource_to_float(resource_name, resource_str):
    try:
        value = float(resource_str)
        return value
    except ValueError:
        reject_event(f"Invalid value provided for: {resource_name}")


def save_controls_to_file(file_name, controls):
    try:
        with open(file_name, "w") as f:
            f.write(json.dumps(controls))
    except (OSError, ValueError) as e:
        reject_event(f"Unable to write to saved controls file: {e}")


def reject_event(msg):
    e = pbs.event()
    if e.type in [pbs.EXECJOB_PROLOGUE, pbs.EXECJOB_EPILOGUE]:
        e.job.delete()
    e.reject(f"{e.hook_name}: {msg}")


def restore_controls_from_file(file_name):
    controls_json = None
    try:
        with open(file_name) as f:
            controls_json = f.read()
    except (OSError, ValueError) as e:
        reject_event(f"Unable to read saved controls file: {e}")

    try:
        controls = json.loads(controls_json)
        if not controls:
            reject_event("Encountered empty saved controls file")
        write_controls(controls)
    except (json.decoder.JSONDecodeError, KeyError, TypeError) as e:
        reject_event(f"Malformed saved controls file: {e}")
    os.unlink(file_name)


def do_power_limit_prologue():
    e = pbs.event()
    job_id = e.job.id
    server_job = pbs.server().job(job_id)

    node_power_limit_requested = False
    try:
        node_power_limit_str = server_job.Resource_List[_POWER_LIMIT_RESOURCE]
        node_power_limit_requested = bool(node_power_limit_str)
    except KeyError:
        pass

    job_power_limit_requested = False
    try:
        job_power_limit_str = server_job.Resource_List[_JOB_POWER_LIMIT_RESOURCE]
        job_power_limit_requested = bool(job_power_limit_str)
    except KeyError:
        pass

    if not node_power_limit_requested and not job_power_limit_requested:
        if os.path.exists(_SAVED_CONTROLS_FILE):
            restore_controls_from_file(_SAVED_CONTROLS_FILE)
        e.accept()
        return

    if node_power_limit_requested:
        # The user requested a specific node power limit. Do not modify it.
        power_limit = resource_to_float(_POWER_LIMIT_RESOURCE, node_power_limit_str)
    elif job_power_limit_requested:
        # A job power limit has been requested without a specific node power limit.
        # Let's use the node power models to distribute the job power limit.
        job_power_limit = resource_to_float(_JOB_POWER_LIMIT_RESOURCE, job_power_limit_str)
        hook_config = None
        if pbs.hook_config_filename is not None:
            with open(pbs.hook_config_filename) as f:
                hook_config = json.load(f)
        vnode_names = [v.name for v in e.vnode_list.values()]
        use_uniform_limit = True
        if hook_config is not None and 'node_profile_name' in hook_config:
            job_type = hook_config['node_profile_name']
            host_models = get_model_from_config(hook_config, job_type, per_host=True)
            if host_models is not None:
                max_node_power = host_models['max_power']
                try:
                    x0 = [host_models[host]['x0'] for host in vnode_names]
                    A = [host_models[host]['A'] for host in vnode_names]
                    B = [host_models[host]['B'] for host in vnode_names]
                    C = [host_models[host]['C'] for host in vnode_names]
                except ValueError:
                    pbs.logmsg(pbs.LOG_WARNING, 'GEOPM PBS config has an incomplete set of host models. Using uniform power limits.')
                else:
                    use_uniform_limit = False
                    slowdown, power_by_node = allocate_budget_to_nodes(
                        job_power_limit,
                        max_node_power, x0, A, B, C)
                    my_node_idx = vnode_names.index(pbs.get_local_nodename())
                    power_limit = power_by_node[my_node_idx]
        if use_uniform_limit:
            # No hook config is provided, or it does not contain a node profile, so
            # uniformly distribute the job power limit across nodes.
            job_node_count = len(vnode_names)
            power_limit = job_power_limit / job_node_count

    pbs.logmsg(pbs.LOG_DEBUG, f"{e.hook_name}: Requested power limit: {power_limit}")
    current_settings = copy.deepcopy(_controls)
    read_controls(current_settings)
    system_files.secure_make_dirs(_SAVED_CONTROLS_PATH)
    save_controls_to_file(_SAVED_CONTROLS_FILE, current_settings)
    _power_limit_control["setting"] = power_limit
    write_controls(_controls)
    e.accept()


def do_power_limit_epilogue():
    e = pbs.event()
    if os.path.exists(_SAVED_CONTROLS_FILE):
        restore_controls_from_file(_SAVED_CONTROLS_FILE)
    e.accept()


def do_power_limit_queuejob():
    """GEOPM handler for queuejob PBS events. This handler sets a preliminary
    job power resource request on a queued job so that the scheduler knows
    the minimum amount of power needed by the job.
    """
    server = pbs.server()

    requested_resources = pbs.event().job.Resource_List
    submitted_node_limit = requested_resources[_POWER_LIMIT_RESOURCE]
    submitted_job_limit = requested_resources[_JOB_POWER_LIMIT_RESOURCE]
    max_power_in_pbs_server = server.resources_available[_JOB_POWER_LIMIT_RESOURCE]
    if max_power_in_pbs_server is None:
        # No high-level power limit is set. Nothing to do here.
        pbs.event().accept()
        return

    min_power_per_node = server.resources_available[_MIN_POWER_LIMIT_RESOURCE]
    if min_power_per_node is None:
        pbs.event().reject(f'{_MIN_POWER_LIMIT_RESOURCE} must be configured.')

    max_power_per_node = server.resources_available[_MAX_POWER_LIMIT_RESOURCE]
    if max_power_per_node is None:
        pbs.event().reject(f'{_MAX_POWER_LIMIT_RESOURCE} must be configured.')

    min_power_per_node = float(min_power_per_node)
    max_power_per_node = float(max_power_per_node)
    max_power_in_pbs_server = float(max_power_in_pbs_server)

    if (max_power_in_pbs_server is None
            and submitted_node_limit is None
            and submitted_job_limit is None):
        # No limit has been specified by the admin or by the user, so we have
        # nothing to do here.
        return

    # No nodes have been assigned to this job yet since it is still queued. We
    # need to base our power request on how many PBS chunks were requested.
    # Note: the user is allowed to change the request until just before the
    # runjob event.
    # TODO: Also need to do the same thing on modifyjob events?
    node_count = 0
    select = repr(requested_resources['select'])
    for chunk in select.split('+'):
        nchunks = 1
        for c in chunk.split(':'):
            kv = c.split('=')
            if len(kv) == 1:
                nchunks = int(kv[0])
        node_count += nchunks

    pbs.logmsg(pbs.LOG_DEBUG, f'submitted node limit: {submitted_node_limit}, '
                              f'job limit: {submitted_job_limit}, nodes: {node_count}, '
                              f'min power per node: {min_power_per_node}')

    # This hook is meant to influence the scheduler's job-power-driven
    # decisions, so we do not set node limit here (we do that in runjob).
    job_power_limit = max(
        min_power_per_node * node_count,
        (submitted_job_limit or 0),
        (submitted_node_limit or 0) * node_count)
    if submitted_job_limit is None and submitted_node_limit is None:
        # If the user is willing to accept some slowdown and didn't request a
        # specific power limit, then set a power cap that is modeled to cause
        # the allowed slowdown.

        job_type = requested_resources[_JOB_TYPE_RESOURCE]
        slowdown = float(requested_resources[_DEFAULT_SLOWDOWN_RESOURCE]) if requested_resources[_DEFAULT_SLOWDOWN_RESOURCE] is not None else _DEFAULT_SLOWDOWN
        if slowdown < 0:
            pbs.event().reject(f'{_DEFAULT_SLOWDOWN_RESOURCE} must be at least 0. Requested value: {slowdown}')
            return

        job_min_limit = predict_power_cap_at_performance_factor(
                job_type, slowdown, min_power_per_node, max_power_per_node) * node_count
        pbs.logmsg(pbs.LOG_DEBUG, f'job_min_limit = {job_min_limit}')
        job_power_limit = max(job_power_limit, job_min_limit)

    job_power_limit = min(job_power_limit,
                          max_power_per_node * node_count)
    if max_power_in_pbs_server is not None:
        # By default, cap the power low enough that it won't be
        # stuck waiting for more resources than are in the server.
        job_power_limit = min(job_power_limit, max_power_in_pbs_server)

    requested_resources[_JOB_POWER_LIMIT_RESOURCE] = job_power_limit


def hook_main():
    try:
        event_type = pbs.event().type
        if event_type == pbs.EXECJOB_PROLOGUE:
            do_power_limit_prologue()
        elif event_type == pbs.EXECJOB_EPILOGUE:
            do_power_limit_epilogue()
        elif event_type == pbs.QUEUEJOB:
            do_power_limit_queuejob()
        else:
            reject_event("Power limit hook incorrectly configured!")
    except SystemExit:
        pass
    except:
        _, e, _ = sys.exc_info()
        reject_event(f"Unexpected error: {str(e)}")


# Begin hook...
hook_main()
