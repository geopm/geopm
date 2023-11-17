#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import json
import glob
import os
from argparse import ArgumentParser


def combine(directory, append_file=None):
    """Combine a set of host specific parameter configuration files

    Args:

        directory (str): Directory containing host specific files,
                         where each file is named after the hostname.
                         Optionally files may contain the ".json"
                         suffix.

        append_file (str): Path to a configuration file that is not
                           host specific which should also be combined.

    """
    if append_file:
        with open(append_file) as fid:
            output = json.load(fid)
        if type(output) is not dict:
            raise TypeError('Append file is not JSON object')
    else:
        output = dict()
    host_files = glob.glob(f'{directory}/*')
    for host_file in host_files:
        host_name = os.path.basename(host_file).split('.json')[0]
        try:
            with open(host_file) as fid:
                host_data = json.load(fid)
        except json.decoder.JSONDecodeError:
            sys.stderr.write(f'Warning: File {host_file} is not a JSON file, it will be ignored\n')
            host_data = dict()
        if type(host_data) is not dict:
            sys.stderr.write(f'Warning: File {host_file} is not a JSON object, it will be ignored\n')
            host_data = dict()
        for key, value in host_data.items():
            output[f'{key}@{host_name}'] = value
    return json.dumps(output, sort_keys=True, indent=4)

def run(directory, append_file):
    """Write the results of combine() to standard output

    Args:

        directory (str): Directory containing host specific files,
                         where each file is named after the hostname.
                         Optionally files may contain the ".json"
                         suffix.

        append_file (str): Path to a configuration file that is not
                           host specific which should also be combined.

    """
    output = combine(directory, append_file)
    sys.stdout.write(output)

def main():
    """Convert ConstConfig input files into host specific parameters.

    Reads from a directory containing per host configuration files.
    These files must be named after the host name, and may contain the
    ".json" extension.  This may be combined with a file that contains
    parameters that are not host specific.

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
        run(opts.directory, opts.file)
    except Exception as ex:
        sys.stderr.write(f'Error: {ex}\n')
        err = -1
    return err

if __name__ == '__main__':
    exit(main())
