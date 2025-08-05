#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys
from geopmpy.io import RawReport

def get_region_energy(report, region):
    """
    Calculate energy consumed by a specific region from the report.

    Infer energy based on the average runtime and power consumption
    of all processes in the specified region.

    This function assumes that the report contains data for a single host
    and that the region is present in the report.

    :param report: RawReport object containing the performance data.
    :param region: The name of the region to analyze.
    :return: Total energy consumed in Joules.
    """
    hosts = report.host_names()
    if len(hosts) != 1:
        raise ValueError("Report should contain data from a single host.")

    # Average runtime over all processes for the specified region
    runtime = report.raw_region(hosts[0], region)['runtime (s)']
    # Average measured power when all processes are in the specified region
    power = report.raw_region(hosts[0], region)['power (W)']

    # Calculate energy based on runtime and power
    energy = runtime * power
    return energy

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: derive_energy.py <report> <region>")
        sys.exit(1)
    report = sys.argv[1]
    region = sys.argv[2]
    report = RawReport(report)
    energy = get_region_energy(report, region)
    print(f"Total energy for region {region}: {energy}")
