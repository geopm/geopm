#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import argparse
import json
import sys


def merge_field_if_absent_or_equal(field, destination, source):
    if field in source:
        if field in destination and source[field] != destination[field]:
            raise Exception(f'Multiple files define different values for {field}.')
        destination[field] = source[field]


parser = argparse.ArgumentParser('Merge multiple config files for the GEOPM PBS hooks')
parser.add_argument('--schema',
                    help='If a path to the config file schema is given, validate input files against the schema.')
parser.add_argument('config_files', nargs='+',
                    help='Config files to merge')
args = parser.parse_args()

if args.schema is not None:
    try:
        import jsonschema
    except ModuleNotFoundError:
        print('Missing the "jsonschema" python package, which is required for the --schema option.', file=sys.stderr)
        exit(1)

schema = None
if args.schema is not None:
    with open(args.schema) as f:
        schema = json.load(f)

merged_config = dict(profiles={})
exit_with_error = False
for config_file_path in args.config_files:
    try:
        with open(config_file_path) as f:
            config = json.load(f)
    except Exception as e:
        print(f'Unable to load {config_file_path}. Reason: {e}', file=sys.stderr)
        exit_with_error = True
        continue

    if schema is not None:
        try:
            jsonschema.validate(instance=config, schema=schema)
        except jsonschema.exceptions.ValidationError as e:
            print(f'Failed to validate {config_file_path}. Reason: {e.message}', file=sys.stderr)
            exit_with_error = True

    try:
        merge_field_if_absent_or_equal('node_profile_name', merged_config, config)
        merge_field_if_absent_or_equal('max_power', merged_config, config)
        for profile_name in config['profiles']:
            merge_field_if_absent_or_equal(profile_name, merged_config['profiles'], config['profiles'])
    except Exception as e:
        print(f'Failed to add {config_file_path} to the merged config file. Reason: {e}', file=sys.stderr)
        exit_with_error = True

if exit_with_error:
    exit(1)

json.dump(merged_config, sys.stdout, indent=2)
print()
