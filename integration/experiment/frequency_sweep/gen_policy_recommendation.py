#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

'''
Creates a model relating frequency to both runtime and energy
consumption. Offers frequency policy recommendations for
minimum energy use subject to a runtime degradation constraint.
'''

import argparse
import math
import sys

import pandas
import numpy as np
from numpy.polynomial.polynomial import Polynomial

import geopmpy.io

from experiment import common_args
from experiment import machine



# TODO 
# change everything to use 'index' rather than 'freq'
# as the keys
# also to expect a pair of indices in the case when...

def main(full_df, region_filter, dump_prefix, min_freq, max_freq, min_uncore, max_uncore, sticker, max_degradation, cross_validation=True, freq_step=1e8, uncore_step=1e8):
    """
    The main function. full_df is a report collection dataframe, region_filter
    is a list of regions to include, dump_prefix a filename prefix for
    debugging output (if specified), min_freq, max_freq, sticker are the minimum,
    maximum, and sticker frequencies, respectively, max_degradation
    specifies what maximum runtime degradation is accepted. If max_degradation
    is None, then just find the minimum energy configuration, ignoring
    runtime."""
    df = extract_columns(full_df, region_filter)
    if dump_prefix:
        df.to_csv("{}.dat".format(dump_prefix))
        dump_stats_summary(df, "{}.stats".format(dump_prefix))

    if cross_validation:
        runtime_model = CrossValidationModel(InverseFrequencyModel)
        energy_model = CrossValidationModel(CubicFrequencyModel)
    else:
        runtime_model = InverseFrequencyModel()
        energy_model = CubicFrequencyModel()

    runtime_model.train(df, key='runtime')
    energy_model.train(df, key='energy')

    freqrange = [min_freq + i * freq_step for i in range(int((max_freq - min_freq)/freq_step))] + [max_freq]
    uncorerange = [min_uncore + i * uncore_step for i in range(int((max_uncore - min_uncore)/uncore_step))] + [max_uncore]
    if cross_validation:
        best_policy = policy_confident_energy(freqrange, uncorerange, energy_model, runtime_model, sticker,
                                              max_degradation = max_degradation)
        stickerrt, stickeren = runtime_model.evaluate(sticker, True), energy_model.evaluate(sticker, True)
        sticker_values = {'freq': sticker[0],
                      'uncore': sticker[1],
                      'runtime': stickerrt[0],
                      'runtimedev': rootssq(stickerrt[1]),
                      'energy': stickeren[0],
                      'energydev': rootssq(stickeren[1])}
    else:
        best_policy = policy_min_energy(freqrange, uncorerange, energy_model, runtime_model, sticker,
                                        max_degradation = max_degradation)
        stickerrt, stickeren = runtime_model.evaluate(sticker), energy_model.evaluate(sticker)
        sticker_values = {'freq': sticker[0], 'uncore': sticker[1], 'runtime': stickerrt, 'energy': stickeren}

    return {'sticker': sticker_values, 'best': best_policy}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_min_frequency(parser)
    common_args.add_max_frequency(parser)
    parser.add_argument('--path', required=True,
                        help='path containing reports and machine.json')
    parser.add_argument('--region-filter', default=None, dest='region_filter',
                        help='comma-separated list of regions to include, '
                             'default to include all regions, use \'Epoch\' '
                             'here to analyze just the Epoch data')
    parser.add_argument('--dump-prefix', dest='dump_prefix',
                        help='prefix to dump statistics to, empty to not dump '
                             'stats')
    parser.add_argument('--sticker', default=None, type=float,
                        help='sticker frequency, default behavior is '
                             'to use max frequency')
    parser.add_argument('--max-degradation',
                        default=0.1, type=float, dest='max_degradation',
                        help='maximum allowed runtime degradation, default is '
                             '0.1 (i.e., 10%%)')
    parser.add_argument('--min_energy', action='store_true',
                        help='ignore max degradation, just give the minimum '
                             'energy possible')
    parser.add_argument('--no-confidence', dest='confidence',
                        action='store_false',
                        help='Ignore uncertainty when giving a recommendation.')

    args = parser.parse_args()

    try:
        if args.region_filter == 'Epoch':
            df = geopmpy.io.RawReportCollection('*report', dir_name=args.path).get_epoch_df()
        else:
            df = geopmpy.io.RawReportCollection('*report', dir_name=args.path).get_df()
    except RuntimeError:
        sys.stderr.write('<geopm> Error: No report data found in ' + path + \
                         '; run a power sweep before using this analysis.\n')
        sys.exit(1)

    min_freq, sticker, max_freq = None, None, None
    if None in [args.min_frequency, args.sticker, args.max_frequency] or \
            min(args.min_frequency, args.sticker, args.max_frequency) < 0:
        try:
            machine_info = machine.get_machine(args.path)
            min_freq = machine_info.frequency_min()
            sticker = machine_info.frequency_sticker()
            max_freq = machine_info.frequency_max()
        except RuntimeError:
            sys.stderr.write('Warning: couldn\'t open machine.json. Falling '
                             'back to default values.\n')
            min_freq = 1.0e9
            sticker = 2.4e9
            max_freq = 3.7e9

    # user-provided arguments override
    if args.min_frequency and args.min_frequency > 0:
        min_freq = args.min_frequency
    if args.sticker and args.sticker > 0:
        sticker = args.sticker
    if args.max_frequency and args.max_frequency > 0:
        max_freq = args.max_frequency

    min_uncore = 1.2e9
    max_uncore = 2.7e9

    output = main(df, args.region_filter, args.dump_prefix, min_freq, max_freq,
                  min_uncore, max_uncore, (sticker, max_uncore), 
                  None if args.min_energy else args.max_degradation,
                  args.confidence)

    if args.confidence:
        sys.stdout.write('AT STICKER = {freq:.0f}Hz, {uncore:.0f}Hz, '
                         'RUNTIME = {runtime:.0f} s +/- {runtimedev:.0f}, '
                         'ENERGY = {energy:.0f} J +/- {energydev:.0f}\n'.format(**output['sticker']))
        if output['best']:
            sys.stdout.write('AT         = {freq:.0f}Hz, {uncore:.0f}Hz, '
                             'RUNTIME = {runtime:.0f} s +/- {runtimedev:.0f}, '
                             'ENERGY = {energy:.0f} J +/- {energydev:.0f}\n'.format(**output['best']))
        else:
            sys.stdout.write("NO SUITABLE POLICY WAS FOUND.\n")
    else:
        sys.stdout.write('AT STICKER = {freq:.0f}Hz, '
                         'RUNTIME    = {runtime:.0f} s, '
                         'ENERGY     = {energy:.0f} J\n'.format(**output['sticker']))
        sys.stdout.write('AT freq    = {freq:.0f}Hz, '
                         'RUNTIME    = {runtime:.0f} s, '
                         'ENERGY     = {energy:.0f} J\n'.format(**output['best']))

    relative_delta = lambda new, old: 100 * (new - old) / old

    if output['best']:
        sys.stdout.write('DELTA                                    RUNTIME = {:.1f} %,  ENERGY = {:.1f} %\n'
                         .format(relative_delta(output['best']['runtime'], output['sticker']['runtime']),
                                 relative_delta(output['best']['energy'], output['sticker']['energy'])))
