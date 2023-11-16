#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
import json
from argparse import ArgumentParser


def run(host_name, append_file):
    if host_name is None:
        raise RuntimeError('The command line option -n/--host-name is required')
    host_data = json.load(sys.stdin)
    if append_file:
        with open(append_file) as fid:
            append_data = json.load(fid)
    else:
        append_data = dict()
    if type(host_data) is not dict:
        raise TypeError('Host file is not json object')
    if type(append_data) is not dict:
        raise TypeError('Append file is not json object')
    for key, value in host_data.items():
        append_data[f'{key}@{host_name}'] = value
    sys.stdout.write(json.dumps(append_data, sort_keys=True, indent=4))

def main():
    """Convert ConstConfig input files into host specific parameters.

    Reads from standard in, writes converted file to standard out.

    """
    err = 0
    prog = sys.argv[0]
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('host_name', type=str)
    parser.add_argument('-a', '--append', dest='file', type=str, help='File with data that is not host specific to be appended without modification')
    opts = parser.parse_args(sys.argv[1:])
    try:
        run(opts.host_name, opts.file)
    except Exception as ex:
        sys.stderr.write(f'Error: {ex}\n')
        err = -1
    return err

if __name__ == '__main__':
    exit(main())
