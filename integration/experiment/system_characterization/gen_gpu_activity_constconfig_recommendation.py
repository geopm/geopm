#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Finds the energy efficient frequency for a provided frequency sweep
and provides that as part of a ConstConfigIO configuration file for the GPU Activity Agent
'''

import argparse
import json
import os
import sys
from pathlib import Path

import geopmpy.io

from integration.experiment import util
from integration.experiment import machine

def extract_columns(df):
    """
    Extract the columns of interest from the full report collection
    dataframe.

    Parameters:
        df: The input dataframe containing report data.

    Returns:
        A filtered dataframe with relevant columns.
    """
    # Explicitly a copy to deal with setting on copy errors
    df_filtered = df.copy()

    # Use requested frequency from the agent
    df_filtered['requested gpu-frequency (Hz)'] = df['FREQ_GPU_DEFAULT']

    # these are the only columns we need
    try:
        df_filtered = df_filtered[['runtime (s)',
                                   'package-energy (J)',
                                   'dram-energy (J)',
                                   'frequency (Hz)',
                                   'gpu-frequency (Hz)',
                                   'gpu-energy (J)',
                                   'requested gpu-frequency (Hz)']]

    except KeyError:
        df_filtered = df_filtered[['runtime (s)',
                                   'package-energy (J)',
                                   'dram-energy (J)',
                                   'frequency (Hz)',
                                   'gpu-energy (J)',
                                   'requested gpu-frequency (Hz)']]

        df_filtered['gpu-frequency (Hz)'] = df_filtered['GPU_CORE_FREQUENCY_STATUS']

    return df_filtered

def get_config_from_frequency_sweep(full_df, mach, energy_margin, use_freq_req, hostname):
    """
    The main function. full_df is a report collection dataframe.

    Parameters:
        full_df: The input dataframe containing report data.
        mach: The machine object containing machine-specific information.
        energy_margin: The acceptable energy margin for frequency selection.
        use_freq_req: Whether to use requested frequency instead of achieved frequency.
        hostname: The hostname of the node being characterized.

    Returns:
        A dictionary containing the configuration.
    """
    df = extract_columns(full_df)

    if df['gpu-frequency (Hz)'].isna().any():
        raise RuntimeError('NaNs detected in the input report files')

    # Round entries to nearest step size
    frequency_step = mach.gpu_frequency_step()
    df.loc[:,'gpu-frequency (Hz)'] = (df['gpu-frequency (Hz)'] /
                                      frequency_step).round(decimals=0) * frequency_step

    energy_col = 'gpu-energy (J)'
    if use_freq_req:
        freq_col = 'requested gpu-frequency (Hz)'
    else:
        freq_col = 'gpu-frequency (Hz)'

    gpu_freq_efficient = util.energy_efficient_frequency(df, freq_col, energy_col, energy_margin)

    if not hostname:
        host_str = ''
    else:
        host_str = '@' + hostname

    json_dict = {
                    ("GPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY" + host_str) : {
                        "domain" : "board",
                        "description" : "Defines the efficient compute frequency to use for GPUs.  " +
                                        "This value is based on a workload that scales strongly with the frequency domain.",
                        "units" : "hertz",
                        "aggregation" : "average",
                        "values" : [gpu_freq_efficient],
                    },
                }

    return json_dict

def validate_positive_float(value):
    """
    Validate that the provided value is a positive float.

    Parameters:
        value: The input value to validate.

    Returns:
        The validated float value.

    Raises:
        argparse.ArgumentTypeError: If the value is not a positive float.
    """
    try:
        fvalue = float(value)
        if fvalue < 0:
            raise argparse.ArgumentTypeError(f"Invalid value: {value}. Must be non-negative.")
        return fvalue
    except ValueError:
        raise argparse.ArgumentTypeError(f"Invalid value: {value}. Must be a float.")


def main():
    """
    Main function to generate GPU activity constant configuration recommendation.

    Returns:
        Exit code indicating success or failure.
    """
    try:
        parser = argparse.ArgumentParser(description="Generate GPU activity constant configuration recommendation.")
        parser.add_argument('--const-config-path', required=False, default=None,
                            help='Path containing existing ConstConfigIO configuration file.')
        parser.add_argument('--gpu-energy-margin', default=0, type=validate_positive_float, dest='gpu_energy_margin',
                            help='Percentage of additional energy it is acceptable to consume if it results '
                                 'in a lower frequency selection for Fe (energy efficient frequency).  This is useful for analyzing '
                                 'noisy systems that have many GPU frequencies near the Fe energy consumption value.')
        parser.add_argument('--input-path', required=True, dest='input_path',
                            help='Path containing reports and machine.json.')
        parser.add_argument('--output-path', required=False, default='-', dest='output_path',
                            help='Output directory to dump node characterization info to.')
        parser.add_argument('--hostname', required=False, default='', dest='hostname',
                            help='Hostname of the node being characterized.')
        parser.add_argument('--use-requested-frequency', action='store_true', default=False,
                            dest='use_freq_req',
                            help='Use the frequency that was requested during the frequency sweep instead '
                                 'of the achieved frequency for a given run.  This is useful in cases where '
                                 'multiple frequency domains or settings are impacted (i.e. core frequency causes '
                                 'an uncore frequency change) and the achieved frequency does not reflect this '
                                 'behavior.')
        args = parser.parse_args()

        input_path = Path(args.input_path)
        if not input_path.exists():
            raise RuntimeError(f"Input path '{input_path}' does not exist.")

        with geopmpy.io.RawReportCollection('*gpufreqsweep*' + args.hostname + '*report', dir_name=str(input_path)) as report_collection:
            df = report_collection.get_df()

        mach = machine.get_machine(str(input_path))
        output = get_config_from_frequency_sweep(df, mach, args.gpu_energy_margin,
                                                args.use_freq_req, args.hostname)
        output = util.merge_const_config(output, args.const_config_path)

        output_str = json.dumps(output, indent=4)
        if args.output_path != '-':
            output_path = Path(args.output_path)
            if not output_path.exists():
                raise RuntimeError(f"Output path '{output_path}' does not exist.")
            if args.hostname:
                prefix = args.hostname
            else:
                prefix = 'constconfig'
            constconfig_outfile = f'{output_path}/{prefix}.json'
            with open(constconfig_outfile, 'w') as file:
                file.writelines(output_str)
            print(f"Configuration successfully written to {constconfig_outfile}.")
        else:
            sys.stdout.write(output_str + "\n")
            print("Configuration successfully written to stdout.")
    except Exception as ex:
        if 'GEOPM_DEBUG' in os.environ:
            raise
        sys.stderr.write('Error: <geopm> gen_gpu_activity_constconfig_recommendation.py: {ex}\n\n')
        return -1
    return 0

if __name__ == "__main__":
    sys.exit(main())
