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
_MAX_SLOWDOWN_RESOURCE = "geopm-max-slowdown"
_JOB_TYPE_RESOURCE = "geopm-job-type"
_DEFAULT_SLOWDOWN = 0.0
_MODEL_PATH = "/soft/geopm/model.json"

def reject_event(event, msg):
    event.reject(f"{event.hook_name}: {msg}")


# ---------------------------------------------------------------------------
# Piecewise-linear model helpers (pure Python, no numpy dependency)
# ---------------------------------------------------------------------------

def _interp(x, xp, fp):
    """Scalar piecewise-linear interpolation (like numpy.interp).

    *xp* must be sorted ascending.  Clamps to boundary values outside range.
    """
    if x <= xp[0]:
        return fp[0]
    if x >= xp[-1]:
        return fp[-1]
    for i in range(1, len(xp)):
        if x <= xp[i]:
            t = (x - xp[i - 1]) / (xp[i] - xp[i - 1])
            return fp[i - 1] + t * (fp[i] - fp[i - 1])
    return fp[-1]


def _monotonize(values):
    """Return a list where each element is >= all preceding elements."""
    result = list(values)
    for i in range(1, len(result)):
        if result[i] < result[i - 1]:
            result[i] = result[i - 1]
    return result


def _parse_piecewise_model(model_dict):
    """Convert a ``{power_str: fom, …}`` dict to sorted ``(powers, foms)`` lists.

    FOM values are monotonized (non-decreasing with power) to suppress
    measurement noise.
    """
    pairs = sorted((float(k), float(v)) for k, v in model_dict.items())
    powers = [p for p, _ in pairs]
    foms = _monotonize([f for _, f in pairs])
    return powers, foms


def fom_at_power(power, curve):
    """Piecewise-linear interpolation of FOM at a given power level.

    *curve* is ``(power_list, fom_list)`` sorted ascending by power.
    """
    return _interp(power, curve[0], curve[1])


def power_at_fom(target_fom, curve):
    """Inverse piecewise-linear interpolation: minimum power to reach *target_fom*.

    Assumes FOM is monotonically non-decreasing with power.
    """
    powers, foms = curve
    if target_fom <= foms[0]:
        return powers[0]
    if target_fom >= foms[-1]:
        return powers[-1]
    for i in range(1, len(foms)):
        if target_fom <= foms[i]:
            f0, f1 = foms[i - 1], foms[i]
            p0, p1 = powers[i - 1], powers[i]
            if f1 == f0:
                return p0  # saturated segment — minimum power
            t = (target_fom - f0) / (f1 - f0)
            return p0 + t * (p1 - p0)
    return powers[-1]


# ---------------------------------------------------------------------------


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
    model_type = profile.get('model_type', 'original-quadratic')

    if model_type == 'piecewise-linear':
        if per_host:
            if 'hosts' in profile:
                models = dict(max_power=model_max_power, model_type='piecewise-linear')
                for host_name, host_data in profile['hosts'].items():
                    powers, foms = _parse_piecewise_model(host_data['model'])
                    models[host_name] = (powers, foms)
                return models
        else:
            model_data = profile.get('model')
            if model_data is None:
                pbs.logmsg(pbs.LOG_WARNING, f'{event.hook_name}: Missing model data for piecewise-linear profile {job_type}')
                return None
            powers, foms = _parse_piecewise_model(model_data)
            return {
                'max_power': model_max_power,
                'model_type': 'piecewise-linear',
                'curve': (powers, foms),
            }
    else:
        # original-quadratic (default)
        if per_host:
            if 'hosts' in profile:
                models = dict(max_power=model_max_power, model_type='original-quadratic')
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
                'model_type': 'original-quadratic',
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
        model_type = model.get('model_type', 'original-quadratic')
        try:
            if model_type == 'piecewise-linear':
                # Derive target FOM from slowdown: FOM = FOM_max / (1 + slowdown)
                curve = model['curve']
                fom_max = curve[1][-1]  # FOM at highest measured power
                target_fom = fom_max / (1.0 + slowdown)
                result = power_at_fom(target_fom, curve)
            else:
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


def _get_resource(event, resource_name):
    """Look up a resource_available value: queue first, then server.

    This allows power resources to be set at either the queue level (for
    isolated testing) or the server level (for production).  Queue-level
    settings take precedence.
    """
    # Try the job's destination queue first
    job_queue = event.job.queue
    if job_queue:
        try:
            queue_name = job_queue.name if hasattr(job_queue, 'name') else str(job_queue)
            queue = pbs.server().queue(queue_name)
            val = queue.resources_available[resource_name]
            if val is not None:
                return val
        except Exception as e:
            pbs.logmsg(pbs.LOG_DEBUG, f'{event.hook_name}: _get_resource({resource_name}) queue lookup failed: {e}')
    # Fall back to server level
    return pbs.server().resources_available[resource_name]


def do_power_limit_queuejob(event):
    """GEOPM handler for queuejob PBS events. This handler sets a preliminary
    job power resource request on a queued job so that the scheduler knows
    the minimum amount of power needed by the job.
    """
    server = pbs.server()

    requested_resources = event.job.Resource_List
    submitted_node_limit = requested_resources[_POWER_LIMIT_RESOURCE]
    submitted_job_limit = requested_resources[_JOB_POWER_LIMIT_RESOURCE]
    max_power_in_pbs_server = _get_resource(event, _JOB_POWER_LIMIT_RESOURCE)
    if max_power_in_pbs_server is None:
        # No high-level power limit is set. Nothing to do here.
        event.accept()
        return

    min_power_per_node = _get_resource(event, _MIN_POWER_LIMIT_RESOURCE)
    if min_power_per_node is None:
        reject_event(event, f'{_MIN_POWER_LIMIT_RESOURCE} must be configured.')

    max_power_per_node = _get_resource(event, _MAX_POWER_LIMIT_RESOURCE)
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
        slowdown = float(requested_resources[_MAX_SLOWDOWN_RESOURCE]) if requested_resources[_MAX_SLOWDOWN_RESOURCE] is not None else _DEFAULT_SLOWDOWN
        if slowdown < 0:
            reject_event(event, f'{_MAX_SLOWDOWN_RESOURCE} must be at least 0. Requested value: {slowdown}')
            return

        job_min_limit = predict_power_cap_at_performance_factor(
                event, job_type, slowdown, min_power_per_node, max_power_per_node) * node_count
        pbs.logmsg(pbs.LOG_DEBUG, f'{event.hook_name}: job_min_limit = {job_min_limit}')
        job_power_limit = max(job_power_limit, job_min_limit)
    elif requested_resources[_MAX_SLOWDOWN_RESOURCE] is not None and submitted_node_limit is None:
        # The user requested a slowdown tolerance alongside a job power limit
        # (possibly from resources_default).  Use the most restrictive
        # (lowest) of the two resulting power limits.
        job_type = requested_resources[_JOB_TYPE_RESOURCE]
        slowdown = float(requested_resources[_MAX_SLOWDOWN_RESOURCE])
        if slowdown < 0:
            reject_event(event, f'{_MAX_SLOWDOWN_RESOURCE} must be at least 0. Requested value: {slowdown}')
            return

        slowdown_limit = predict_power_cap_at_performance_factor(
                event, job_type, slowdown, min_power_per_node, max_power_per_node) * node_count
        slowdown_limit = max(slowdown_limit, min_power_per_node * node_count)
        pbs.logmsg(pbs.LOG_DEBUG, f'{event.hook_name}: slowdown_limit = {slowdown_limit}, job_power_limit = {job_power_limit}')
        job_power_limit = min(job_power_limit, slowdown_limit)

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
