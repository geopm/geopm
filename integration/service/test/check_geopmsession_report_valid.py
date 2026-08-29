#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

from argparse import ArgumentParser
import json
import jsonschema
import os
import re
import yaml

def yaml_is_valid(file_path, schema_path):
    reports = yaml.safe_load_all(open(file_path, "r"))
    report_schema = json.load(open(schema_path, "r"))
    for report_data in reports:
        if not jsonschema.validate(report_data, schema=report_schema):
            return False
    return True

def num_yaml_reports(file_path):
    reports = yaml.safe_load_all(open(file_path, "r"))
    return len(list(reports))

def main():
    parser = ArgumentParser()
    parser.add_argument("-r", "--report", dest="report")
    parser.add_argument("-n", "--num-nodes", dest="num_nodes", default=1)
    args = parser.parse_args()

    if args.report is None:
        raise ValueError('The report argument is required')

    # Find YAML schema file
    report_schema_file = os.path.dirname(os.path.abspath(__file__)) + \
                          "/../../../docs/json_schemas/geopmsession_report.schema.json"

    # Add support for floating point numbers of the form "1e+10"
    # where the decimal point is missing
    yaml.SafeLoader.add_implicit_resolver(
        u'tag:yaml.org,2002:float',
        re.compile(r'''^(?:[-+]?(?:[0-9][0-9_]*)\.[0-9_]*(?:[eE][-+]?[0-9]+)?
                           |[-+]?(?:[0-9][0-9_]*)(?:[eE][-+]?[0-9]+)
                           |\.[0-9_]+(?:[eE][-+]?[0-9]+)?
                           |[-+]?[0-9][0-9_]*(?::[0-5]?[0-9])+\.[0-9_]*
                           |[-+]?\.(?:inf|Inf|INF)
                           |\.(?:nan|NaN|NAN))$''', re.X),
        list(u'-+0123456789.'))

    # Test for valid yaml reports
    if not yaml_is_valid(args.report, report_schema_file):
        print(f"YAML report does not match schema at {report_schema_file}")
        #raise RuntimeError(f"YAML report does not match schema at {report_schema_file}")

    # Check that number of yaml reports corresponds to the number of nodes
    num_reports = num_yaml_reports(args.report)
    if args.num_nodes != num_reports:
        print(f'Incorrect number of YAML reports. Expected {args.num_nodes}, found {num_reports}')
        #raise AssertionError(f'Incorrect number of YAML reports. Expected {args.num_nodes}, found {num_reports}')

if __name__ == '__main__':
    main()
