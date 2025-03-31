#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause

from argparse import ArgumentParser
import json
import jsonschema
import os
import yaml

def main():
    parser = ArgumentParser()
    parser.add_argument("report")
    args = parser.parse_args()
    if args.report is None:
        raise ValueError('The report argument is required')
    report_schema_file = os.path.dirname(os.path.abspath(__file__)) + \
                          "/../../../docs/json_schemas/geopmsession_report.schema.json"
    with open(report_schema_file) as fid:
        report_schema = json.load(fid)
    with open(args.report, "r") as fid:
        reports = yaml.safe_load_all(fid)

        for idx, report_data in enumerate(reports):
            jsonschema.validate(report_data, schema=report_schema)

if __name__ == '__main__':
    main()
