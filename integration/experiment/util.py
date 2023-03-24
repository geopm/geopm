#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Helper functions useful for both tests and experiments

import json
import jsonschema
import sys
import os
import socket
import subprocess
import io
import yaml
import shlex

import geopmpy.launcher


def detect_launcher():
    """
    Heuristic to determine the resource manager used on the system.
    Returns name of resource manager or launcher, otherwise a
    LookupError is raised.
    """
    # Try the environment
    result = os.environ.get('GEOPM_LAUNCHER', None)
    if not result:
        # Check for known host names
        slurm_hosts = ['mr-fusion', 'mcfly']
        alps_hosts = ['theta']
        hostname = socket.gethostname()
        if any(hostname.startswith(word) for word in slurm_hosts):
            result = 'srun'
        elif any(hostname.startswith(word) for word in alps_hosts):
            result = 'aprun'
    if not result:
        try:
            exec_str = 'srun --version'
            subprocess.check_call(exec_str, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, shell=True)
            result = 'srun'
        except subprocess.CalledProcessError:
            pass
    if not result:
        try:
            exec_str = 'aprun --version'
            subprocess.check_call(exec_str, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, shell=True)
            result = 'aprun'
        except subprocess.CalledProcessError:
            pass
    if not result:
        raise LookupError('Unable to determine resource manager')
    return result


def allocation_node_test(test_exec, stdout, stderr):
    argv = shlex.split(test_exec)
    launcher = detect_launcher()
    argv.insert(1, launcher)
    if launcher == 'aprun':
        argv.insert(2, '-q')  # Use quiet flag with aprun to suppress end of job info string
    argv.insert(2, '--geopm-ctl-disable')
    launcher = geopmpy.launcher.Factory().create(argv, num_rank=1, num_node=1,
                                                 job_name="geopm_allocation_test", quiet=True)
    launcher.run(stdout, stderr)


def geopmwrite(write_str):
    test_exec = "dummy -- geopmwrite " + write_str
    stdout = io.StringIO()
    stderr = io.StringIO()
    try:
        allocation_node_test(test_exec, stdout, stderr)
    except subprocess.CalledProcessError as err:
        sys.stderr.write(stderr.getvalue())
        raise err
    output = stdout.getvalue().splitlines()
    last_line = None
    if len(output) > 0:
        last_line = output[-1]
    return last_line


def geopmread(read_str):
    test_exec = "dummy -- geopmread " + read_str
    stdout = io.StringIO()
    stderr = io.StringIO()
    try:
        allocation_node_test(test_exec, stdout, stderr)
    except subprocess.CalledProcessError as err:
        raise subprocess.CalledProcessError(err.returncode, stderr.getvalue()) from err
    output = stdout.getvalue()
    last_line = output.splitlines()[-1]

    if last_line.startswith('0x'):
        result = int(last_line)
    else:
        try:
            result = float(last_line)
        except ValueError:
            result = list(yaml.safe_load(output).values())[0]
    return result


def geopmread_domain():
    test_exec = "dummy -- geopmread -d"
    stdout = io.StringIO()
    stderr = io.StringIO()
    try:
        allocation_node_test(test_exec, stdout, stderr)
    except subprocess.CalledProcessError as err:
        sys.stderr.write(stderr.getvalue())
        raise err
    output = stdout.getvalue()
    result = {}
    for line in output.strip().split('\n'):
        domain, count = tuple(line.split())
        result[domain] = int(count)
    return result


def get_node_memory_info():
    test_exec = 'dummy -- cat /proc/meminfo'
    stdout = io.StringIO()
    stderr = io.StringIO()
    try:
        allocation_node_test(test_exec, stdout, stderr)
    except subprocess.CalledProcessError as err:
        sys.stderr.write(stderr.getvalue())
        raise err
    output = stdout.getvalue()
    result = {}
    for line in output.strip().split('\n'):
        if 'MemTotal' in line:
            key, total, units = tuple(line.split())
            if units != 'kB':
                raise RuntimeError("Unknown how to convert units: {}".format(units))
            # Convert to bytes.  Note: in spite of 'kB' label, meminfo
            # displays kibibytes
            result['MemTotal'] = float(total) * 1024
            break
    return result

def merge_const_config(new_config, old_config_path=None):
    root_dir = os.getenv('GEOPM_SOURCE')
    schema_file = root_dir + "/service/json_schemas/const_config_io.schema.json"
    with open(schema_file, "r") as f:
        schema = json.load(f)

    old_config = {};
    if old_config_path:
        with open(old_config_path, "r") as f:
            old_config = json.load(f)
        # Check the old config
        jsonschema.validate(old_config, schema=schema)

    # Check the new config
    jsonschema.validate(new_config, schema=schema)

    # Combine & check
    combined_config = {**old_config, **new_config}
    jsonschema.validate(combined_config, schema=schema)

    return combined_config

def energy_efficient_frequency(df, freq_col_name, energy_col_name, energy_margin):
    """
    Find the frequency that provides the minimum package energy consumption
    within the dataframe provided.
    """
    df_mean = df.groupby(freq_col_name)[energy_col_name].mean()
    df_std = df.groupby(freq_col_name)[energy_col_name].std()

    energy_efficient_frequency = df_mean.idxmin()
    energy_reading = df_mean[energy_efficient_frequency];

    energy_reading_cov = df_std[energy_efficient_frequency] / df_mean[energy_efficient_frequency];

    if len(df_mean) > 1:
        if energy_margin != 0.0:
            sys.stderr.write('Warning: <geopm> util.py: Found Fe = {} with energy = {}.  Coefficient of variation: {:.2%}. '
                             'Searching for alternate based on an energy margin of {}\n'.format(energy_efficient_frequency,
                                                                                                energy_reading,
                                                                                                energy_reading_cov, energy_margin))

            # Grab all energy readings associated with frequencies that a 1Hz below Fe
            # TODO: Consider iloc instead and just grab all idx prior to Fe
            df_mean = df_mean.loc[:energy_efficient_frequency - 1]

            # Find any energy reading that is within a given margin of Fe's energy while having a lower frequency
            energy_max_acceptable = energy_reading + energy_reading * energy_margin
            df_in_margin = df_mean.loc[df_mean <= energy_max_acceptable]

            # If any values are within margin
            if len(df_in_margin) > 1:
                # Since we've been given a margin, we're going to find the minimum frequency within
                # that margin
                energy_efficient_frequency = df_in_margin.index.min()
                # Associated energy reading
                energy_reading = df_in_margin.loc[energy_efficient_frequency];

            else:
                sys.stderr.write('Warning: <geopm> util.py: No alternate found with energy-margin {}.  Using Fe={}.\n'.format(energy_margin,
                                                                                                                              energy_efficient_frequency))

        else:
            df_mean = df_mean.loc[:energy_efficient_frequency - 1]
            nearby_energy_count = len([e for e in df_mean if (e - energy_reading) / e < 0.05]);
            sys.stderr.write('Warning: <geopm> util.py: Found {} possible alternate Fe value(s) within 5% '
                             'energy consumption of Fe for \'{}\'.  Coefficient of variation: {:.2%}. '
                             'Consider using the energy-margin options.\n'.format(nearby_energy_count, freq_col_name, energy_reading_cov))

    return energy_efficient_frequency
