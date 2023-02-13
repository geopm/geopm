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
_SIGNAL = "GPU_CORE_FREQUENCY_MIN_CONTROL"
_CONTROL = "GPU_CORE_FREQUENCY_MIN_CONTROL"
_DOMAIN = "gpu"
_DOMAIN_IDX = 0


def resource_to_float(resource_name, resource_str):
    try:
        value = float(resource_str)
        return value
    except ValueError:
        reject_event(f"Invalid value provided for: {resource_name}")


def save_control(file_name, name, domain_type, domain_idx, setting):
    controls = [{"name": name,
                 "domain_type": domain_type,
                 "domain_idx": domain_idx,
                 "setting": setting}]

    try:
        with open(file_name, "w") as f:
            f.write(json.dumps(controls))
    except (OSError, ValueError) as e:
        reject_event(f"Unable to write to saved controls file: {e}")


def read_signal(signal, domain, domain_idx):
    try:
        value = pio.read_signal(signal, domain, domain_idx)
        return value
    except RuntimeError as e:
        reject_event(f"Unable to read signal {signal}: {e}")


def write_control(control, domain, domain_idx, setting):
    try:
        pio.write_control(control, domain, domain_idx, setting)
    except RuntimeError as e:
        reject_event(f"Unable to write control {control}: {e}")


def reject_event(msg):
    e = pbs.event()
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
        for c in controls:
            name = c["name"]
            domain = c["domain_type"]
            domain_idx = c["domain_idx"]
            setting = c["setting"]
            pbs.logmsg(pbs.LOG_DEBUG, f"Restoring {name} to {setting}")
            write_control(name, domain, domain_idx, setting)
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
    original_setting = read_signal(_SIGNAL, _DOMAIN, _DOMAIN_IDX)
    pbs.logmsg(pbs.LOG_DEBUG,
               f"Current power limit setting: {original_setting}")
    save_control(_SAVED_CONTROLS_FILE, _CONTROL, _DOMAIN, _DOMAIN_IDX,
                 original_setting)
    write_control(_CONTROL, _DOMAIN, _DOMAIN_IDX, power_limit)
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
