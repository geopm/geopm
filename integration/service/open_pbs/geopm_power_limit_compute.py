#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# This file contains the prologue and epilogue hooks for GEOPM power limiting
# functionality in PBS environments.  It should be installed on the PBS server host.
# The server/queuejob functionality is in a separate file (geopm_power_limit_server.py)
# which should be installed on the PBS server.

import sys
import glob
import os
import json
import copy
import math
import signal
import pbs

path_config = {"aux_lib_paths": ["/usr/lib/python3.6/site-packages",
                                 "/usr/lib64/python3.6/site-packages"]}

if pbs.hook_config_filename is not None:
    with open(pbs.hook_config_filename) as f:
        path_config.update(json.loads(f.read()))

for p in path_config['aux_lib_paths']:
    if p not in sys.path:
        sys.path.insert(0, p)

from geopmdpy import pio
from geopmdpy import system_files

os.environ["ZES_ENABLE_SYSMAN"] = "1"
os.environ["ZE_FLAT_DEVICE_HIERARCHY"] = "COMPOSITE"

_RESOURCE_FILE_SEARCH_PATH = "/var/tmp"
_SAVED_CONTROLS_PATH = "/run/geopm/pbs-hooks/SAVE_FILES"
_SAVED_CONTROLS_FILE = _SAVED_CONTROLS_PATH + "/power-limit-save-control.json"
_POWER_LIMIT_RESOURCE = "geopm-node-power-limit"
_JOB_POWER_LIMIT_RESOURCE = "geopm-job-power-limit"

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
    return clip_list([(x0n - (-Bn + math.sqrt(abs(Bn**2 - 4 * An * (Cn - slowdown)))) / (2 * An))
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


def read_controls(event, controls):
    # Unblock SIGCHLD temporarily (hook env may have it blocked); restore after all reads.
    old_mask = signal.pthread_sigmask(signal.SIG_BLOCK, [])  # Query current mask (no change)
    did_unblock = False
    if signal.SIGCHLD in old_mask:
        signal.pthread_sigmask(signal.SIG_UNBLOCK, [signal.SIGCHLD])
        did_unblock = True
    try:
        for c in controls:
            c["setting"] = pio.read_signal(c["name"], c["domain_type"],
                                           c["domain_idx"])
    except RuntimeError as e:
        reject_event(event, f"Unable to read signal {c['name']}: {e}")
    finally:
        if did_unblock:
            signal.pthread_sigmask(signal.SIG_SETMASK, old_mask)

def write_controls(event, controls):
    try:
        for c in controls:
            pio.write_control(c["name"], c["domain_type"], c["domain_idx"],
                              c["setting"])
    except RuntimeError as e:
        reject_event(event, f"Unable to write control {c['name']}: {e}")


def resource_to_float(event, resource_name, resource_str):
    try:
        value = float(resource_str)
        return value
    except ValueError:
        reject_event(event, f"Invalid value provided for: {resource_name}")


def save_controls_to_file(event, file_name, controls):
    try:
        with open(file_name, "w") as f:
            f.write(json.dumps(controls))
    except (OSError, ValueError) as e:
        reject_event(event, f"Unable to write to saved controls file: {e}")


def reject_event(event, msg):
    if event.type in [pbs.EXECJOB_PROLOGUE, pbs.EXECJOB_EPILOGUE]:
        event.job.delete()
    event.reject(f"{event.hook_name}: {msg}")


def restore_controls_from_file(event, file_name):
    controls_json = None
    try:
        with open(file_name) as f:
            controls_json = f.read()
    except (OSError, ValueError) as e:
        reject_event(event, f"Unable to read saved controls file: {e}")

    try:
        controls = json.loads(controls_json)
        if not controls:
            reject_event(event, "Encountered empty saved controls file")
        write_controls(event, controls)
    except (json.decoder.JSONDecodeError, KeyError, TypeError) as e:
        reject_event(event, f"Malformed saved controls file: {e}")
    os.unlink(file_name)


def parse_resource_file(event, path):
    result = {}
    if not path:
        return result
    try:
        with open(path) as f:
            data = f.read()
        for pair in data.split(';'):
            pair = pair.strip()
            if not pair:
                continue
            if '=' in pair:
                k, v = pair.split('=', 1)
                result[k.strip()] = v.strip()
    except FileNotFoundError as e:
        reject_event(event, f"logue resources file not found: {e}")
    return result


def load_resources(event, job_id):
    prefix = str(job_id)
    pattern = os.path.join(_RESOURCE_FILE_SEARCH_PATH, f"{prefix}*.resources")
    matches = glob.glob(pattern)
    name =  matches[0] if matches else None

    return parse_resource_file(event, name)


def do_power_limit_prologue(event):
    job_id = event.job.id

    if os.path.exists(_SAVED_CONTROLS_FILE):
        restore_controls_from_file(event, _SAVED_CONTROLS_FILE)

    resource_dict = load_resources(event, job_id)

    node_power_limit_str = resource_dict.get(_POWER_LIMIT_RESOURCE)
    if node_power_limit_str is not None:
        node_power_limit = resource_to_float(event, _POWER_LIMIT_RESOURCE, node_power_limit_str)
        node_power_limit_requested = (node_power_limit > 0)
    else:
        node_power_limit_requested = False

    job_power_limit_str = resource_dict.get(_JOB_POWER_LIMIT_RESOURCE)
    if job_power_limit_str is not None:
        job_power_limit = resource_to_float(event, _JOB_POWER_LIMIT_RESOURCE, job_power_limit_str)
        job_power_limit_requested = (job_power_limit > 0)
    else:
        job_power_limit_requested = False

    if not node_power_limit_requested and not job_power_limit_requested:
        event.accept()
        return

    if node_power_limit_requested:
        # The user requested a specific node power limit. Do not modify it.
        power_limit = node_power_limit
    elif job_power_limit_requested:
        # A job power limit has been requested without a specific node power limit.
        # Let's use the node power models to distribute the job power limit.
        hook_config = None
        if pbs.hook_config_filename is not None:
            with open(pbs.hook_config_filename) as f:
                hook_config = json.load(f)
        vnode_names = [v.name for v in event.vnode_list.values()]
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
                except (ValueError, KeyError):
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

    pbs.logmsg(pbs.LOG_DEBUG, f"{event.hook_name}: Requested power limit: {power_limit}")
    current_settings = copy.deepcopy(_controls)
    read_controls(event, current_settings)
    system_files.secure_make_dirs(_SAVED_CONTROLS_PATH)
    save_controls_to_file(event, _SAVED_CONTROLS_FILE, current_settings)
    _power_limit_control["setting"] = power_limit
    write_controls(event, _controls)
    event.accept()


def do_power_limit_epilogue(event):
    if os.path.exists(_SAVED_CONTROLS_FILE):
        restore_controls_from_file(event, _SAVED_CONTROLS_FILE)
    event.accept()


def hook_main():
    try:
        event = pbs.event()
        event_type = event.type
        if event_type == pbs.EXECJOB_PROLOGUE:
            do_power_limit_prologue(event)
        elif event_type == pbs.EXECJOB_EPILOGUE:
            do_power_limit_epilogue(event)
        else:
            reject_event(event, "Power limit compute hook incorrectly configured!")
    except SystemExit:
        pass
    except:
        _, e, _ = sys.exc_info()
        try:
            event = event if 'event' in locals() else pbs.event()
            reject_event(event, f"Unexpected error: {str(e)}")
        except:
            pass

# Begin hook...
hook_main()
