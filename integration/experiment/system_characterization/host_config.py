#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import json
import jsonschema
import glob
import os
from argparse import ArgumentParser
import geopmdpy.schemas


def combine(directory, append_file=None):
    """Combine a set of host specific parameter configuration files

    Args:

        directory (str): Directory containing host specific files,
                         where each file is named after the hostname
                         and suffixed with ".json".

        append_file (str): Path to a configuration file that is not
                           host specific which should also be combined.

    """
    json_schema = json.loads(geopmdpy.schemas.GEOPM_CONST_CONFIG_IO_SCHEMA)
    if append_file:
        output = _load_json(append_file, json_schema)
    else:
        output = dict()
    host_files = glob.glob(f'{directory}/*.json')
    for host_file in host_files:
        host_name, _ = os.path.splitext(os.path.basename(host_file))
        host_data = _load_json(host_file, json_schema)
        for key, value in host_data.items():
            output[f'{key}@{host_name}'] = value
    return json.dumps(output, sort_keys=True, indent=4)

def _load_json(path, schema):
    try:
        with open(path) as fid:
            result = json.load(fid)
        jsonschema.validate(result, schema=schema)
    except json.decoder.JSONDecodeError as ex:
        raise RuntimeError(f'File {path} is not a JSON file') from ex
    except jsonschema.exceptions.ValidationError as ex:
        raise RuntimeError(f'File {path} is not a ConstConfigIO JSON file') from ex
    return result

def _run(directory, append_file):
    """Write the results of combine() to standard output

    """
    sys.stdout.write(combine(directory, append_file))

def main():
    """Convert ConstConfig input files into host specific parameters.

    Reads from a directory containing per host configuration files.
    These files must be named after the host name, and contain the
    ".json" extension.  This may be combined with a file that contains
    parameters that are not host specific.  Files in the directory
    that do not end in ".json" are ignored.

    """
    err = 0
    prog = sys.argv[0]
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('directory', type=str,
                        help='Directory containing a configuration file per host')
    parser.add_argument('-a', '--append', dest='file', type=str,
                        help='File with data that is not host specific to be appended without modification')
    opts = parser.parse_args(sys.argv[1:])
    try:
        _run(opts.directory, opts.file)
    except Exception as ex:
        sys.stderr.write(f'Error: {ex}\n')
        err = -1
    return err

if __name__ == '__main__':
    exit(main())
