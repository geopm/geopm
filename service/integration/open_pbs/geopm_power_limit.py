import sys

PYTHON_PATHS = [
        "/usr/lib/python3.6/site-packages",
        "/usr/lib64/python3.6/site-packages"]

for p in PYTHON_PATHS:
    if p not in sys.path:
        sys.path.insert(0, p)

import os
import json

import pbs

from geopmdpy import pio


_SAVED_CONTROLS_FILE = "run/geopm-service/SAVE_FILES/saved_controls.json"
_POWER_LIMIT_RESOURCE = "geopm-node-power-limit"

_power_limit_control = {
        "name": "MSR::PLATFORM_POWER_LIMIT::PLATFORM_POWER_LIMIT",
        "domain_type": "board",
        "domain_idx": 0,
        "setting": None
    }
_controls = [
        {"name": "MSR::PLATFORM_POWER_LIMIT::PL1_TIME_WINDOW",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 0.013},
        {"name": "MSR::PLATFORM_POWER_LIMIT::PL1_CLAMP_ENABLE",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 1},
        _power_limit_control,
        {"name": "MSR::PLATFORM_POWER_LIMIT::PL1_LIMIT_ENABLE",
         "domain_type": "board",
         "domain_idx": 0,
         "setting": 1}
    ]


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
    # pbs.logmsg(pbs.LOG_DEBUG, "Executing geopm_power_limit prologue")

    e = pbs.event()
    job_id = e.job.id
    server_job = pbs.server().job(job_id)

    power_limit_requested = False
    try:
        power_limit_str = server_job.Resource_List[_POWER_LIMIT_RESOURCE]
        power_limit_requested = bool(power_limit_str)
    except KeyError:
        # power_limit_requested = False
        pass

    if not power_limit_requested:
        if os.path.exists(_SAVED_CONTROLS_FILE):
            restore_controls_from_file(_SAVED_CONTROLS_FILE)
        e.accept()

    power_limit = resource_to_float(_POWER_LIMIT_RESOURCE, power_limit_str)
    pbs.logmsg(pbs.LOG_DEBUG, f"Requested power limit: {power_limit}")
    current_settings = _controls.copy()
    read_controls(current_settings)
    save_controls_to_file(_SAVED_CONTROLS_FILE, current_settings)
    _power_limit_control["setting"] = power_limit
    write_controls(_controls)
    e.accept()


def do_power_limit_epilogue():
    # pbs.logmsg(pbs.LOG_DEBUG, "Executing geopm_power_limit epilogue")

    e = pbs.event()
    if os.path.exists(_SAVED_CONTROLS_FILE):
        restore_controls_from_file(_SAVED_CONTROLS_FILE)
    e.accept()


# Begin hook...
try:
    event_type = pbs.event().type
    if event_type == pbs.EXECJOB_PROLOGUE:
        do_power_limit_prologue()
    elif event_type == pbs.EXECJOB_EPILOGUE:
        do_power_limit_epilogue()
    else:
        reject_event("Power limit hook incorrectly configured!")
except SystemExit:
    pass
except:
    _, e, _ = sys.exc_info()
    reject_event(f"Unexpected error: {str(e)}")
