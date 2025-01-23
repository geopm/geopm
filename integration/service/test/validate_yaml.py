#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from argparse import ArgumentParser
import os
import sys
import yaml

def main():
    parser = ArgumentParser()
    parser.add_argument('yaml_file',
                        help='Yaml file to validate')
    args = parser.parse_args()

    try:
        with open(args.yaml_file) as fid:
            yaml_info = yaml.safe_load(fid)
        os.unlink(args.yaml_file)
    except:
        sys.stderr.write(f"Warning: Temporary file {args.yaml_file} not deleted.\n")

if __name__ == '__main__':
    main()
