#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# This file contains the server hook for GEOPM power limiting functionality
# in PBS environments. It should be installed on the PBS server host.
# The prologue and epilogue hooks are in a separate file (geopm_power_limit_compute.py)
# which should be installed on compute nodes.

import sys

PYTHON_PATHS = [
        "/usr/lib/python3.6/site-packages",
        "/usr/lib64/python3.6/site-packages"]

for p in PYTHON_PATHS:
    if p not in sys.path:
        sys.path.insert(0, p)

import json
import math

import pbs


_POWER_LIMIT_RESOURCE = "geopm-node-power-limit"
_MAX_POWER_LIMIT_RESOURCE = "geopm-max-node-power-limit"
_MIN_POWER_LIMIT_RESOURCE = "geopm-min-node-power-limit"
_JOB_POWER_LIMIT_RESOURCE = "geopm-job-power-limit"
_DEFAULT_SLOWDOWN_RESOURCE = "geopm-default-slowdown"
_JOB_TYPE_RESOURCE = "geopm-job-type"
_DEFAULT_SLOWDOWN = 0.0
_MODEL_PATH = "/soft/geopm/model.json"

def reject_event(event, msg):
    event.reject(f"{event.hook_name}: {msg}")


def get_model_from_config(event, hook_config, job_type, per_host=False):
    if hook_config is None or job_type is None:
        return None

    if 'profiles' not in hook_config:
        pbs.logmsg(pbs.LOG_WARNING, f'{event.hook_name}: Missing profiles section in the GEOPM PBS config')
        return None

    model_max_power = hook_config.get("max_power", None)
    if model_max_power is None:
        pbs.logmsg(pbs.LOG_WARNING, f'{event.hook_name}: Missing max_power in the GEOPM PBS config')
        return None

    if job_type not in hook_config['profiles']:
        pbs.logmsg(pbs.LOG_WARNING, f'{event.hook_name}: Requested job type {job_type} has no performance model in the GEOPM PBS config')
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
            pbs.logmsg(pbs.LOG_WARNING, f'{event.hook_name}: Invalid coefficients for profile {job_type} in GEOPM PBS config')
            return None

        return {
            'max_power': model_max_power,
            'x0': x0,
            'A': A,
            'B': B,
            'C': C,
        }


def load_hook_config(event, model_path=_MODEL_PATH):
    """Load GEOPM model config JSON.

    Preference order:
      1) model_path (default: /etc/geopm/model.json)
      2) pbs.hook_config_filename (if model_path is missing)
    """
    hook_config = None
    try:
        with open(model_path) as f:
            hook_config = json.load(f)
    except FileNotFoundError:
        if pbs.hook_config_filename is not None:
            try:
                with open(pbs.hook_config_filename) as f:
                    hook_config = json.load(f)
            except (OSError, ValueError, json.decoder.JSONDecodeError) as e:
                pbs.logmsg(pbs.LOG_WARNING,
                           f'{event.hook_name}: Unable to read model config at {pbs.hook_config_filename}: {e}')
    except (OSError, ValueError, json.decoder.JSONDecodeError) as e:
        pbs.logmsg(pbs.LOG_WARNING, f'{event.hook_name}: Unable to read model config at {model_path}: {e}')
    return hook_config


def predict_power_cap_at_performance_factor(event, job_type, slowdown, min_power_per_node, max_power_per_node):
    """Predict the node power cap needed to achieve a target slowdown for a
    given job type. If job_type is None or is not configured, this function
    assumes a 1:1 linear mapping between power and performance (half power
    results in half performance). Slowdown of 0 means min time, slowdown of
    1 means twice the min time (100% slowdown).
    """
    hook_config = load_hook_config(event)

    model = get_model_from_config(event, hook_config, job_type)
    do_use_model = model is not None

    if do_use_model:
        try:
            # Using a quadratic model: slowdown = A * (x0 - percent_of_tdp)^2 + B * (x0 - percent_of_tdp) + C
            # Solve for the positive root (less than 100% of max power) at '-slowdown' offset:
            result = model['max_power'] * (model['x0'] - (-model['B'] + math.sqrt(abs(model['B']**2 - 4 * model['A'] * (model['C'] - slowdown)))) / (2 * model['A']))
        except Exception as e:
            pbs.logmsg(pbs.LOG_WARNING, f'{event.hook_name}: Unable to estimate job power. {str(e)}')
            do_use_model = False

    if not do_use_model:
        # Fallback case: Assume 1:1 linear mapping between power and performance
        result = max_power_per_node / (slowdown + 1)

    return min(max(min_power_per_node, result), max_power_per_node)


def do_power_limit_queuejob(event):
    """GEOPM handler for queuejob PBS events. This handler sets a preliminary
    job power resource request on a queued job so that the scheduler knows
    the minimum amount of power needed by the job.
    """
    server = pbs.server()

    requested_resources = event.job.Resource_List
    submitted_node_limit = requested_resources[_POWER_LIMIT_RESOURCE]
    submitted_job_limit = requested_resources[_JOB_POWER_LIMIT_RESOURCE]
    max_power_in_pbs_server = server.resources_available[_JOB_POWER_LIMIT_RESOURCE]
    if max_power_in_pbs_server is None:
        # No high-level power limit is set. Nothing to do here.
        event.accept()
        return

    min_power_per_node = server.resources_available[_MIN_POWER_LIMIT_RESOURCE]
    if min_power_per_node is None:
        reject_event(event, f'{_MIN_POWER_LIMIT_RESOURCE} must be configured.')

    max_power_per_node = server.resources_available[_MAX_POWER_LIMIT_RESOURCE]
    if max_power_per_node is None:
        reject_event(event, f'{_MAX_POWER_LIMIT_RESOURCE} must be configured.')

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
    node_count = 0
    select = repr(requested_resources['select'])
    for chunk in select.split('+'):
        nchunks = 1
        for c in chunk.split(':'):
            kv = c.split('=')
            if len(kv) == 1:
                nchunks = int(kv[0])
        node_count += nchunks

    pbs.logmsg(pbs.LOG_DEBUG, f'{event.hook_name}: submitted node limit: {submitted_node_limit}, '
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
            reject_event(event, f'{_DEFAULT_SLOWDOWN_RESOURCE} must be at least 0. Requested value: {slowdown}')
            return

        job_min_limit = predict_power_cap_at_performance_factor(
                event, job_type, slowdown, min_power_per_node, max_power_per_node) * node_count
        pbs.logmsg(pbs.LOG_DEBUG, f'{event.hook_name}: job_min_limit = {job_min_limit}')
        job_power_limit = max(job_power_limit, job_min_limit)

    job_power_limit = min(job_power_limit,
                          max_power_per_node * node_count)
    if max_power_in_pbs_server is not None:
        # By default, cap the power low enough that it won't be
        # stuck waiting for more resources than are in the server.
        job_power_limit = min(job_power_limit, max_power_in_pbs_server)

    requested_resources[_JOB_POWER_LIMIT_RESOURCE] = job_power_limit

def do_power_limit_modifyjob(event):
    """GEOPM handler for modifyjob PBS events. This handler simply calls the
    queue job handler to reevaluate the server-specified vs. user-specified
    resources.
    """
    do_power_limit_queuejob(event)


def hook_main():
    try:
        event = pbs.event()
        if event.type == pbs.HOOK_EVENT_QUEUEJOB:
            do_power_limit_queuejob(event)
        elif event.type == pbs.HOOK_EVENT_MODIFYJOB:
            do_power_limit_modifyjob(event)
        else:
            reject_event(event, "Power limit server hook incorrectly configured!")
    except SystemExit:
        pass
    except:
        _, e, _ = sys.exc_info()
        reject_event(event if 'event' in locals() else pbs.event(), f"Unexpected error: {str(e)}")


# Begin hook...
hook_main()
