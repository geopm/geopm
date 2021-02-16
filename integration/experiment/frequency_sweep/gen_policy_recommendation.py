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

class FrequencyModel(object):
    "Abstract class for predicting a statistic based on a frequency."
    def __init__(self):
        "Default constructor."
        raise NotImplementedError

    def train(self, df, key):
        """
        Train this model to fit the data of column key contained in the
        dataframe (df), which is indexed by frequency. This modifies
        the instance it is called on, preparing it for calls of the
        methods test and batch_test."""
        raise NotImplementedError

    def evaluate(self, freq):
        """
        Evaluate a trained model on a single frequency (freq). Returns
        the model's prediction for this frequency."""
        raise NotImplementedError

    def batch_evaluate(self, freq_list):
        """
        Evaluate a trained model on a list-like container of frequencies
        (freq_list). Returns a list of values."""
        return [self.evaluate(freq) for freq in freq_list]

    def serialize(self):
        "Convert model to JSON."
        raise NotImplementedError

    def __str__(self):
        return "<%s>" % "PowerLimitModel"

    def repr(self):
        return self.__str__()


class PolynomialFitFrequencyModel(FrequencyModel):
    """
    Implementation of a polynomial model of fixed degree that can be a
    trained to predict a statistic from a frequency."""
    def __init__(self, degree):
        """
        Default constructor; degree (a positive integer) is the degree
        of the polynomial being fit to."""
        self._degree = degree
        self._model = None

    def train(self, df, key):
        new_series, [resid, rank, sv, rcond] = Polynomial.fit(df.index,
                                                              df[key].values,
                                                              deg=self._degree,
                                                              full=True)
        self._model = new_series.convert()
        self._resid = (resid[0] / len(df[key].values))**0.5

    def evaluate(self, freq):
        return sum(self._model.coef * freq ** np.arange(self._degree + 1))

    def residual(self):
        return self._resid

    def __str__(self):
        if self._model is not None:
            terms = ["", " freq"] + \
                    [" freq ** {}".format(i) for i in range(2, self._degree + 1)]
            coeffs = list(self._model.coef)
            expression = ""
            terms.reverse()
            coeffs.reverse()
            firstTerm = True
            for tt, cc in zip(terms, coeffs):
                if not firstTerm:
                    if cc < 0:
                        expression += " - "
                        cc *= -1
                    else:
                        expression += " + "
                expression += str(cc) + tt
                firstTerm = False
            return "<{} {}>".format(self.__class__.__name__, expression)
        else:
            return "<{} (untrained)>".format(self.__class__.__name__)


class CubicFrequencyModel(PolynomialFitFrequencyModel):
    """
    Implementation of a cubic model that can be a trained to predict a
    statistic from a frequency."""
    def __init__(self):
        "Simple constructor."
        super(CubicFrequencyModel, self).__init__(degree=3)


class InverseFrequencyModel(FrequencyModel):
    """
    Implementation of a frequency model that predicts a statistic that is
    linear in inverse frequency."""
    def __init__(self):
        self._model = None

    def train(self, df, key):
        new_series, [resid, rank, sv, rcond] = Polynomial.fit(1./df.index,
                                                              df[key].values,
                                                              deg=1,
                                                              full=True)
        self._model = new_series.convert()
        self._resid = (resid[0] / len(df[key].values))**0.5

    def evaluate(self, freq):
        return sum(self._model.coef * freq ** np.array([0, -1]))

    def residual(self):
        return self._resid

class CrossValidationModel(FrequencyModel):
    """
    Implementation of a Cross Validation model. This creates an ensemble of
    sub-models, gives each one a fraction of the data, and deduces confidence
    values from the distribution of fit values."""
    def __init__(self, base_gen, frac=0.8, size=100):
        """
        Simple constructor; base_gen is a callable that returns a
        FrequencyModel instance (or a derived class), frac is the proportion
        of training data to give to each instance during training (default 0.8),
        and size is the number of sub-models to instantiate for the ensemble
        (default 100)."""

        self._model = [base_gen() for _ in range(size)]
        self._frac = frac

    def train(self, df, key):
        for mm in self._model:
            mm.train(df.sample(frac=self._frac), key)

    def evaluate(self, freq, full=False):
        """
        Evaluate a trained model on a single frequency (freq). Returns
        the model's prediction for this frequency. If the parameter
        full is True, also return the standard deviation and residuals
        (default False)."""

        vals = [mm.evaluate(freq) for mm in self._model]
        mean = sum(vals)/len(vals)
        dev = (sum([val**2 for val in vals])/len(vals) - mean**2)**0.5
        res = (sum([mm.residual() for mm in self._model])/len(self._model))
        if full:
            return mean, [dev, res]
        return mean


def extract_columns(df, region_filter = None):
    """
    Extract the columns of interest from the full report collection
    dataframe. This returns a dataframe indexed by the frequency
    and columns 'runtime' and 'energy'. region_filter (if provided)
    is a container that specifies which regions to include (by default,
    include all of them)."""
    df_filtered = df
    if region_filter and region_filter != 'Epoch':
        df_filtered = df[df['region'].isin(region_filter.split(','))]

    # these are the only columns we need
    df_cols = df_filtered[['FREQ_DEFAULT',
                           'host', 'Profile',
                           'runtime (sec)',
                           'package-energy (joules)']]

    df_cols = df_cols.rename({'FREQ_DEFAULT': 'freq',
                              'runtime (sec)': 'runtime',
                              'package-energy (joules)': 'energy'
                             }, axis = 1)

    df_cols = df_cols.groupby(['freq', 'host', 'Profile']).sum().reset_index()
    df_cols = df_cols[['freq', 'runtime', 'energy']].set_index('freq')

    return df_cols


def dump_stats_summary(df, fname):
    """
    Write mean runtime and energy and the standard deviation of
    runtime and energy for each frequency in CSV format to the
    file fname."""
    means = df.groupby(level=0).mean().rename(lambda x: x+"_mean", axis=1)
    stds = df.groupby(level=0).std().rename(lambda x: x+"_stddev", axis=1)
    pandas.concat([means, stds], axis=1).to_csv(fname)


def policy_min_energy(freqrange, enmodel, rtmodel = None, sticker = None,
                      max_degradation = None):
    """
    Find the frequency over the range freqrange (list-like) that
    has the minimum predicted energy usage (according to the energy model
    enmodel, an instance of PowerLimitModel), subject to the constraint
    that its runtime does not exceed the runtime at sticker frequency
    by more than a factor of (1 + max_degradation), if max_degradation is
    specified. Returns a dictionary with keys power, runtime, and energy,
    and values the optimal frequency and the predicted runtime and energy
    at that limit, respectively."""

    if sticker is None:
        sticker = max(freqrange)

    en_predictions = enmodel.batch_evaluate(freqrange)
    if max_degradation is None:
        # we don't need the runtime model in this case
        best_energy, best_freq = min(zip(en_predictions, freqrange))
        if rtmodel:
            best_runtime = rtmodel.evaluate(best_freq)
        else:
            best_runtime = None
        return {'freq': best_freq,
                'runtime': best_runtime,
                'energy': best_energy}
    else:
        rt_predictions = rtmodel.batch_evaluate(freqrange)
        rt_at_sticker = rtmodel.evaluate(sticker)
        constrained_values = [(energy, runtime, freq)
                              for freq, runtime, energy
                              in zip(freqrange, rt_predictions, en_predictions)
                              if runtime <= (1 + max_degradation) * rt_at_sticker]
        best_energy, best_runtime, best_freq = min(constrained_values)
        return {'freq': best_freq,
                'runtime': best_runtime,
                'energy': best_energy}


def rootssq(ll):
    "Return the root of the sum of the squares of the list of values."
    try:
        return sum([ss**2  for ss in ll])**0.5
    except TypeError:
        pass
    return ll

def normal_comparison(dist1, dist2):
    """Returns the probability that a sample drawn from the normal distribution
    described by dist1 (a tuple of mu and sigma) will be larger than a sample
    drawn from a normal distribution drawn from the normal distribution
    described by dist2."""
    mu1, sigma1 = dist1
    mu2, sigma2 = dist2
    sigma1 = rootssq(sigma1)
    sigma2 = rootssq(sigma2)
    return 0.5 * (1 + math.erf((mu1 - mu2) / (2 * (sigma1 ** 2 + sigma2 ** 2)**0.5)))


def policy_confident_energy(freqrange, enmodel, rtmodel = None, sticker = None,
                            max_degradation = None, confidence = 0.9):
    """
    Find the frequency over the range freqrange (list-like) that
    has the minimum predicted energy usage (according to the energy model
    enmodel, an instance of FrequencyModel), subject to the constraint
    that its runtime does not exceed the runtime at sticker frequency
    by more than a factor of (1 + max_degradation), if max_degradation is
    specified, with at least a confidence (default 0.9) probability. Returns a
    dictionary with keys freq, runtime, and energy, and values the optimal
    frequency, the predicted runtime and energy at that limit,
    respectively, and the standard deviations of these values."""
    if sticker is None:
        sticker = max(freqrange)

    en_predictions = [enmodel.evaluate(freq, True) for freq in freqrange]
    if max_degradation is None:
        # we don't need the runtime model in this case
        best_energy, best_freq = min(zip(en_predictions, freqrange))
        if rtmodel:
            best_runtime = rtmodel.evaluate(best_freq, True)
        else:
            best_runtime = None
        return {'freq': best_freq,
                'runtime': best_runtime[0],
                'runtimedev': rootssq(best_runtime[1]),
                'energy': best_energy[0],
                'energydev': rootssq(best_energy[1])}
    else:
        rt_predictions = [rtmodel.evaluate(freq, True) for freq in freqrange]
        rt_at_sticker, [dev_sticker, res_sticker] = rtmodel.evaluate(sticker, True)
        degraded_rt = rt_at_sticker * (1 + max_degradation),\
                [dev_sticker * (1 + max_degradation), res_sticker * (1 + max_degradation)]
        constrained_values = [(energy, runtime, freq)
                              for freq, runtime, energy
                              in zip(freqrange, rt_predictions, en_predictions)
                              if normal_comparison(degraded_rt, runtime) > confidence]
        if len(constrained_values) == 0:
            # this means that we don't ever have enough confidence to guarantee this
            return None
        best_energy, best_runtime, best_freq = min(constrained_values)
        return {'freq': best_freq,
                'runtime': best_runtime[0],
                'runtimedev': rootssq(best_runtime[1]),
                'energy': best_energy[0],
                'energydev': rootssq(best_energy[1])}


def main(full_df, region_filter, dump_prefix, min_freq, max_freq, sticker, max_degradation, cross_validation=True, freq_step=1e8):
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
    if cross_validation:
        best_policy = policy_confident_energy(freqrange, energy_model, runtime_model, sticker,
                                              max_degradation = max_degradation)
        stickerrt, stickeren = runtime_model.evaluate(sticker, True), energy_model.evaluate(sticker, True)
        sticker_values = {'freq': sticker,
                      'runtime': stickerrt[0],
                      'runtimedev': rootssq(stickerrt[1]),
                      'energy': stickeren[0],
                      'energydev': rootssq(stickeren[1])}
    else:
        best_policy = policy_min_energy(freqrange, energy_model, runtime_model, sticker,
                                        max_degradation = max_degradation)
        stickerrt, stickeren = runtime_model.evaluate(sticker), energy_model.evaluate(sticker)
        sticker_values = {'freq': sticker, 'runtime': stickerrt, 'energy': stickeren}

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

    output = main(df, args.region_filter, args.dump_prefix, min_freq, max_freq, sticker,
                  None if args.min_energy else args.max_degradation,
                  args.confidence)

    if args.confidence:
        sys.stdout.write('AT STICKER = {freq:.0f}Hz, '
                         'RUNTIME = {runtime:.0f} s +/- {runtimedev:.0f}, '
                         'ENERGY = {energy:.0f} J +/- {energydev:.0f}\n'.format(**output['sticker']))
        if output['best']:
            sys.stdout.write('AT STICKER = {freq:.0f}Hz, '
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
        sys.stdout.write('DELTA                  RUNTIME = {:.1f} %,  ENERGY = {:.1f} %\n'
                         .format(relative_delta(output['best']['runtime'], output['sticker']['runtime']),
                                 relative_delta(output['best']['energy'], output['sticker']['energy'])))
