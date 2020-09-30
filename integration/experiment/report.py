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
Helper functions for report data.
'''

import sys
import pandas


def extract_trial(name):
    ''' Remove the iteration number after the last underscore. '''
    pieces = name.split('_')
    return '_'.join(pieces[:-1]), int(pieces[-1])


def prepare_columns(df):
    # remove iteration from end of profile name
    df['trial'] = df['Profile']
    df[['Profile', 'trial']] = df['Profile'].apply(extract_trial).tolist()

    # rename some columns
    df['runtime'] = df['runtime (sec)']
    df['network_time'] = df['network-time (sec)']
    df['energy'] = df['package-energy (joules)']
    df['power'] = df['power (watts)']
    # for epoch vs totals - see #1288
    try:
        df['frequency'] = df['frequency (Hz)']
    except KeyError:
        pass


def prepare_metrics(df, perf_metric):
    energy_metric = 'energy_perf'
    if perf_metric == 'FOM':
        # if FOM is not present, use inverse runtime
        if 'FOM' not in df.columns or df["FOM"].isnull().any():
            df['FOM'] = 1.0 / df['runtime']
            sys.stdout.write('Using inverse runtime for performance.\n')
        # use perf per watt
        df[energy_metric] = df[perf_metric] / df['power']
        sys.stdout.write('Using performance per watt instead of energy.\n')
    elif perf_metric == 'runtime':
        df[energy_metric] = df['energy']
    else:
        raise RuntimeError("Invalid performance metric: {}".format(perf_metric))


def perf_metric_label(perf_metric):
    if perf_metric == 'FOM':
        perf_metric_label = 'Performance'
        energy_metric_label = 'Perf. per watt'
    elif perf_metric == 'runtime':
        perf_metric_label = 'Runtime'
        energy_metric_label = 'Energy'
    else:
        raise RuntimeError("Invalid performance metric: {}".format(perf_metric))
    return perf_metric_label, energy_metric_label


def energy_perf_summary(df, loop_key, loop_vals, baseline, perf_metric, use_stdev):
    energy_metric = 'energy_perf'
    if baseline is not None:
        if baseline not in df[loop_key].unique():
            raise RuntimeError('Baseline value for {} "{}" not present in data.'.format(loop_key, baseline))
        base_data = df.loc[df[loop_key] == baseline]
        base_data.set_index('trial')
        base_data = base_data.groupby('trial').mean()
        base_energy = base_data[energy_metric].mean()
        base_perf = base_data[perf_metric].mean()
    else:
        # if no baseline, do not normalize
        base_energy = 1.0
        base_perf = 1.0

    data = []
    for rr in loop_vals:
        row_df = df.loc[df[loop_key] == rr]
        # error calculated across all nodes within one trial
        row_df.set_index('trial')
        row_df = row_df.groupby('trial').mean()

        mean_energy = row_df[energy_metric].mean() / base_energy
        mean_perf = row_df[perf_metric].mean() / base_perf
        if use_stdev:
            energy_std = row_df[energy_metric].std() / base_energy
            min_delta_energy = energy_std
            max_delta_energy = energy_std
            perf_std = row_df[perf_metric].std() / base_perf
            min_delta_perf = perf_std
            max_delta_perf = perf_std
        else:
            min_energy = row_df[energy_metric].min() / base_energy
            max_energy = row_df[energy_metric].max() / base_energy
            min_delta_energy = mean_energy - min_energy
            max_delta_energy = max_energy - mean_energy
            min_perf = row_df[perf_metric].min() / base_perf
            max_perf = row_df[perf_metric].max() / base_perf
            min_delta_perf = mean_perf - min_perf
            max_delta_perf = max_perf - mean_perf

        data.append([mean_energy, mean_perf,
                     min_delta_energy, max_delta_energy,
                     min_delta_perf, max_delta_perf])
    return pandas.DataFrame(data, index=loop_vals,
                            columns=[energy_metric, 'performance',
                                     'min_delta_energy', 'max_delta_energy',
                                     'min_delta_perf', 'max_delta_perf'])


if __name__ == '__main__':
    # TODO: temp tests
    profiles = ['bal_0', 'bal_0', 'bal_1', 'bal_1',
                'gov_0', 'gov_0', 'gov_1', 'gov_1', ]
    hosts = ['mcfly1', 'mcfly2', 'mcfly1', 'mcfly2',
             'mcfly1', 'mcfly2', 'mcfly1', 'mcfly2', ]
    runtimes = [6, 7, 8, 9,
                5.4, 5.5, 3.3, 2.3]
    network_times = [1, 2, 3, 4,
                     1, 2, 3, 4]
    energies = [88, 99, 44, 55,
                444, 555, 666, 777]
    foms = [45, 56, 67, 78,
            33.4, 55.5, 66.6, 77.5]
    bad_foms = [1, 1, float('nan'), 1, 1, 1, 1, 1]
    df_no_fom = pandas.DataFrame({"Profile": profiles,
                                  "runtime (sec)": runtimes,
                                  "network-time (sec)": network_times,
                                  "package-energy (joules)": energies,
                                  'host': hosts,
                                  })
    df_bad_fom = pandas.DataFrame({"Profile": profiles,
                                   "runtime (sec)": runtimes,
                                   "network-time (sec)": network_times,
                                   "package-energy (joules)": energies,
                                   "FOM": bad_foms,
                                   'host': hosts,
                                   })
    df_good_fom = pandas.DataFrame({"Profile": profiles,
                                    "runtime (sec)": runtimes,
                                    "network-time (sec)": network_times,
                                    "package-energy (joules)": energies,
                                    "FOM": foms,
                                    'host': hosts,
                                    })

    for ddf in [df_no_fom, df_bad_fom, df_good_fom]:
        for metric in ['runtime', 'FOM']:
            df = ddf.copy()
            prepare_columns(df, metric)
            sys.stdout.write('{}:\n{}\n'.format(metric, df))
            result = energy_perf_summary(df, 'Profile', ['gov', 'bal'], 'gov', metric, False)
            sys.stdout.write('{}\n\n'.format(result))
            result = energy_perf_summary(df, 'Profile', ['gov', 'bal'], None, metric, True)
            sys.stdout.write('{}\n\n'.format(result))
