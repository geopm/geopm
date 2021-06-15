#!/usr/bin/env python
#
#  Copyright (c) 2015 - 2021, Intel Corporation
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
Creates a model relating power-limit to both runtime and energy
consumption. Offers power-governor policy recommendations for
minimum energy use subject to a runtime degradation constraint.
'''

import policy_model


def main(full_df, region_filter, dump_prefix, min_pl, max_pl, tdp, max_degradation, cross_validation=True):
    """
    The main function. full_df is a report collection dataframe, region_filter
    is a list of regions to include, dump_prefix a filename prefix for
    debugging output (if specified), min_pl, max_pl, tdp are the minimum,
    maximum, and reference power limits, respectively, max_degradation
    specifies what maximum runtime degradation is accepted. If max_degradation
    is None, then just find the minimum energy configuration, ignoring
    runtime."""
    df = extract_columns(full_df, region_filter)
    if dump_prefix:
        df.to_csv("{}.dat".format(dump_prefix))
        dump_stats_summary(df, "{}.stats".format(dump_prefix))

    if cross_validation:
        runtime_model = policy_model.CrossValidationModel(policy_model.CubicPolicyModel)
        energy_model = policy_model.CrossValidationModel(policy_model.CubicPolicyModel)
    else:
        runtime_model = CubicPolicyModel()
        energy_model = CubicPolicyModel()

    runtime_model.train(df, key='runtime')
    energy_model.train(df, key='energy')

    plrange = [min_pl + i for i in range(int(max_pl - min_pl))] + [max_pl]
    if cross_validation:
        best_policy = policy_model.policy_confident_energy(plrange,
                                                           energy_model,
                                                           runtime_model,
                                                           tdp,
                                                           max_degradation = max_degradation)
        tdprt, tdpen = runtime_model.evaluate(tdp, True), energy_model.evaluate(tdp, True)
        tdp_values = {'power': tdp,
                      'runtime': tdprt['mean'],
                      'runtimedev': rootssq(tdprt['dev']),
                      'energy': tdpen['mean'],
                      'energydev': rootssq(tdpen['dev'])}
    else:
        best_policy = policy_model.policy_min_energy(plrange,
                                                     energy_model,
                                                     runtime_model,
                                                     tdp,
                                                     max_degradation = max_degradation)
        tdprt, tdpen = runtime_model.evaluate(tdp), energy_model.evaluate(tdp)
        tdp_values = {'power': tdp, 'runtime': tdprt, 'energy': tdpen}

    return {'tdp': tdp_values, 'best': best_policy}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_min_power(parser)
    common_args.add_max_power(parser)
    parser.add_argument('--path', required=True,
                        help='path containing reports and machine.json')
    parser.add_argument('--region-filter', default=None, dest='region_filter',
                        help='comma-separated list of regions to include, '
                             'default to include all regions, use \'Epoch\' '
                             'here to analyze just the Epoch data')
    parser.add_argument('--dump-prefix', dest='dump_prefix',
                        help='prefix to dump statistics to, empty to not dump '
                             'stats')
    parser.add_argument('--tdp', default=None, type=float,
                        help='tdp (sticker power limit), default behavior is '
                             'to use max power')
    parser.add_argument('--max-degradation',
                        default=0.1, type=float, dest='max_degradation',
                        help='maximum allowed runtime degradation, default is '
                             '0.1 (i.e., 10%%)')
    parser.add_argument('--min_energy', action='store_true',
                        help='ignore max degradation, just give the minimum '
                             'energy possible')
    parser.add_argument('--disable-confidence', dest='confidence',
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

    min_pl, tdp, max_pl = None, None, None
    if None in [args.min_power, args.tdp, args.max_power] or \
            min(args.min_power, args.tdp, args.max_power) < 0:
        try:
            machine_info = machine.get_machine(args.path)
            min_pl = machine_info.power_package_min()
            tdp = machine_info.power_package_tdp()
            max_pl = machine_info.power_package_tdp()
        except RuntimeError:
            sys.stderr.write('Warning: couldn\'t open machine.json. Falling '
                             'back to default values.\n')
            min_pl = 150
            tdp = 280
            max_pl = 280

    # user-provided arguments override
    if args.min_power and args.min_power > 0:
        min_pl = args.min_power
    if args.tdp and args.tdp > 0:
        tdp = args.tdp
    if args.max_power and args.max_power > 0:
        max_pl = args.max_power

    output = main(df, args.region_filter, args.dump_prefix, min_pl, max_pl, tdp,
                  None if args.min_energy else args.max_degradation,
                  args.confidence)

    if args.confidence:
        sys.stdout.write('AT TDP = {power:.0f}W, '
                         'RUNTIME = {runtime:.0f} s +/- {runtimedev:.0f}, '
                         'ENERGY = {energy:.0f} J +/- {energydev:.0f}\n'.format(**output['tdp']))
        if output['best']:
            sys.stdout.write('AT PL  = {power:.0f}W, '
                             'RUNTIME = {runtime:.0f} s +/- {runtimedev:.0f}, '
                             'ENERGY = {energy:.0f} J +/- {energydev:.0f}\n'.format(**output['best']))
        else:
            sys.stdout.write("NO SUITABLE POLICY WAS FOUND.\n")
    else:
        sys.stdout.write('AT TDP = {power:.0f}W, '
                         'RUNTIME = {runtime:.0f} s, '
                         'ENERGY = {energy:.0f} J\n'.format(**output['tdp']))
        sys.stdout.write('AT PL  = {power:.0f}W, '
                         'RUNTIME = {runtime:.0f} s, '
                         'ENERGY = {energy:.0f} J\n'.format(**output['best']))

    relative_delta = lambda new, old: 100 * (new - old) / old

    if output['best']:
        sys.stdout.write('DELTA          RUNTIME = {:.1f} %,  ENERGY = {:.1f} %\n'
                         .format(relative_delta(output['best']['runtime'], output['tdp']['runtime']),
                                 relative_delta(output['best']['energy'], output['tdp']['energy'])))
