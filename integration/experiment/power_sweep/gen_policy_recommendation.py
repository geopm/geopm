#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Creates a model relating power-limit to both runtime and energy
consumption. Offers power-governor policy recommendations for
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

class PowerLimitModel(object):
    "Abstract class for predicting a statistic based on a power limit."
    def __init__(self):
        "Default constructor."
        raise NotImplementedError

    def train(self, df, key):
        """
        Train this model to fit the data of column key contained in the
        dataframe (df), which is indexed by power limit. This modifies
        the instance it is called on, preparing it for calls of the
        methods test and batch_test."""
        raise NotImplementedError

    def evaluate(self, PL):
        """
        Evaluate a trained model on a single power limit (PL). Returns
        the model's prediction for this power limit."""
        raise NotImplementedError

    def batch_evaluate(self, PL_list):
        """
        Evaluate a trained model on a list-like container of power limits
        (PL_list). Returns a list of values."""
        return [self.evaluate(PL) for PL in PL_list]

    def serialize(self):
        "Convert model to JSON."
        raise NotImplementedError

    def __str__(self):
        return "<%s>" % "PowerLimitModel"

    def repr(self):
        return self.__str__()


class PolynomialFitPowerModel(PowerLimitModel):
    """
    Implementation of a polynomial model of fixed degree that can be a
    trained to predict a statistic from a power limit."""
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

    def evaluate(self, PL):
        return sum(self._model.coef * PL ** np.arange(self._degree + 1))

    def residual(self):
        return self._resid

    def __str__(self):
        if self._model is not None:
            terms = ["", " PL"] + \
                    [" PL ** {}".format(i) for i in range(2, self._degree + 1)]
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


class CubicPowerModel(PolynomialFitPowerModel):
    """
    Implementation of a cubic model that can be a trained to predict a
    statistic from a power limit."""
    def __init__(self):
        "Simple constructor."
        super(CubicPowerModel, self).__init__(degree=3)


class CrossValidationModel(PowerLimitModel):
    """
    Implementation of a Cross Validation model. This creates an ensemble of
    sub-models, gives each one a fraction of the data, and deduces confidence
    values from the distribution of fit values."""
    def __init__(self, base_gen, frac=0.8, size=100):
        """
        Simple constructor; base_gen is a callable that returns a
        PowerLimitModel instance (or a derived class), frac is the proportion
        of training data to give to each instance during training (default 0.8),
        and size is the number of sub-models to instantiate for the ensemble
        (default 100)."""

        self._model = [base_gen() for _ in range(size)]
        self._frac = frac

    def train(self, df, key):
        for mm in self._model:
            mm.train(df.sample(frac=self._frac), key)

    def evaluate(self, PL, full=False):
        """
        Evaluate a trained model on a single power limit (PL). Returns
        the model's prediction for this power limit. If the parameter
        full is True, also return the standard deviation and residuals
        (default False)."""

        vals = [mm.evaluate(PL) for mm in self._model]
        mean = sum(vals)/len(vals)
        dev = (sum([val**2 for val in vals])/len(vals) - mean**2)**0.5
        res = (sum([mm.residual() for mm in self._model])/len(self._model))
        if full:
            return {'mean': mean, 'dev': [dev, res]}
        return mean


def extract_columns(df, region_filter = None):
    """
    Extract the columns of interest from the full report collection
    dataframe. This returns a dataframe indexed by the power limit
    and columns 'runtime' and 'energy'. region_filter (if provided)
    is a container that specifies which regions to include (by default,
    include all of them)."""
    df_filtered = df
    if region_filter and region_filter != 'Epoch':
        df_filtered = df[df['region'].isin(region_filter.split(','))]

    # these are the only columns we need
    df_cols = df_filtered[['POWER_PACKAGE_LIMIT_TOTAL',
                           'host', 'Profile',
                           'runtime (s)',
                           'package-energy (J)']]

    df_cols = df_cols.rename({'POWER_PACKAGE_LIMIT_TOTAL': 'power_limit',
                              'runtime (s)': 'runtime',
                              'package-energy (J)': 'energy'
                             }, axis = 1)

    df_cols = df_cols.groupby(['power_limit', 'host', 'Profile']).sum().reset_index()
    df_cols = df_cols[['power_limit', 'runtime', 'energy']].set_index('power_limit')

    return df_cols


def dump_stats_summary(df, fname):
    """
    Write mean runtime and energy and the standard deviation of
    runtime and energy for each power limit in CSV format to the
    file fname."""
    means = df.groupby(level=0).mean().rename(lambda x: x+"_mean", axis=1)
    stds = df.groupby(level=0).std().rename(lambda x: x+"_stddev", axis=1)
    pandas.concat([means, stds], axis=1).to_csv(fname)


def policy_min_energy(plrange, enmodel, rtmodel = None, pltdp = None,
                      max_degradation = None):
    """
    Find the power limit over the range plrange (list-like) that
    has the minimum predicted energy usage (according to the energy model
    enmodel, an instance of PowerLimitModel), subject to the constraint
    that its runtime does not exceed the runtime at power limit pltdp
    by more than a factor of (1 + max_degradation), if max_degradation is
    specified. Returns a dictionary with keys power, runtime, and energy,
    and values the optimal power limit and the predicted runtime and energy
    at that limit, respectively."""

    if pltdp is None:
        pltdp = max(plrange)

    en_predictions = enmodel.batch_evaluate(plrange)
    if max_degradation is None:
        # we don't need the runtime model in this case
        best_energy, best_pl = min(zip(en_predictions, plrange))
        if rtmodel:
            best_runtime = rtmodel.evaluate(best_pl)
        else:
            best_runtime = None
        return {'power': best_pl,
                'runtime': best_runtime,
                'energy': best_energy}
    else:
        rt_predictions = rtmodel.batch_evaluate(plrange)
        rt_at_tdp = rtmodel.evaluate(pltdp)
        constrained_values = [(energy, runtime, pl)
                              for pl, runtime, energy
                              in zip(plrange, rt_predictions, en_predictions)
                              if runtime <= (1 + max_degradation) * rt_at_tdp]
        best_energy, best_runtime, best_pl = min(constrained_values)
        return {'power': best_pl,
                'runtime': best_runtime,
                'energy': best_energy}


def rootssq(ll):
    """Return the root of the sum of the squares of the list of values. If a
    non-iterable is passed in, just return it."""
    try:
        return sum([ss**2  for ss in ll])**0.5
    except TypeError:
        pass  # ll is not iterable
    return ll

def normal_comparison(dist1, dist2):
    """Returns the probability that a sample drawn from the normal distribution
    described by dist1 (a dictionary with keys 'mean' and 'dev' and values the
    mean and standard deviation of the distribution, respectively; the standard
    deviation may be a simple float or may be a list of error magnitudes in the
    same dimensions as the mean which will be combined by rootssq) will be
    larger than a sample drawn from a normal distribution drawn from the normal
    distribution described by dist2."""
    mu1, sigma1 = dist1['mean'], dist1['dev']
    mu2, sigma2 = dist2['mean'], dist2['dev']
    sigma1 = rootssq(sigma1)
    sigma2 = rootssq(sigma2)
    return 0.5 * (1 + math.erf((mu1 - mu2) / (2 * (sigma1 ** 2 + sigma2 ** 2)**0.5)))


def policy_confident_energy(plrange, enmodel, rtmodel = None, pltdp = None,
                            max_degradation = None, confidence = 0.9):
    """
    Find the power limit over the range plrange (list-like) that
    has the minimum predicted energy usage (according to the energy model
    enmodel, an instance of PowerLimitModel), subject to the constraint
    that its runtime does not exceed the runtime at power limit pltdp
    by more than a factor of (1 + max_degradation), if max_degradation is
    specified, with at least a confidence (default 0.9) probability. Returns a
    dictionary with keys power, runtime, and energy, and values the optimal
    power limit, the predicted runtime and energy at that limit,
    respectively, and the standard deviations of these values."""
    if pltdp is None:
        pltdp = max(plrange)

    en_predictions = [enmodel.evaluate(pl, True) for pl in plrange]
    if max_degradation is None:
        # we don't need the runtime model in this case
        best_energy, best_pl = min(zip(en_predictions, plrange))
        if rtmodel:
            best_runtime = rtmodel.evaluate(best_pl, True)
        else:
            best_runtime = None
        return {'power': best_pl,
                'runtime': best_runtime['mean'],
                'runtimedev': rootssq(best_runtime['dev']),
                'energy': best_energy['mean'],
                'energydev': rootssq(best_energy['dev'])}
    else:
        rt_predictions = [rtmodel.evaluate(pl, True) for pl in plrange]
        rt_tdp_dist = rtmodel.evaluate(pltdp, True)
        rt_at_tdp, [dev_tdp, res_tdp] = rt_tdp_dist['mean'], rt_tdp_dist['dev']
        degraded_rt = {'mean': rt_at_tdp * (1 + max_degradation),\
                'dev': [dev_tdp * (1 + max_degradation), res_tdp * (1 + max_degradation)]}
        constrained_values = [(energy['mean'], energy, runtime, pl)
                              for pl, runtime, energy
                              in zip(plrange, rt_predictions, en_predictions)
                              if normal_comparison(degraded_rt, runtime) > confidence]
        if len(constrained_values) == 0:
            # this means that we don't ever have enough confidence to guarantee this
            return None
        _, best_energy, best_runtime, best_pl = min(constrained_values)
        return {'power': best_pl,
                'runtime': best_runtime['mean'],
                'runtimedev': rootssq(best_runtime['dev']),
                'energy': best_energy['mean'],
                'energydev': rootssq(best_energy['dev'])}


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
        runtime_model = CrossValidationModel(CubicPowerModel)
        energy_model = CrossValidationModel(CubicPowerModel)
    else:
        runtime_model = CubicPowerModel()
        energy_model = CubicPowerModel()

    runtime_model.train(df, key='runtime')
    energy_model.train(df, key='energy')

    plrange = [min_pl + i for i in range(int(max_pl - min_pl))] + [max_pl]
    if cross_validation:
        best_policy = policy_confident_energy(plrange, energy_model, runtime_model, tdp,
                                              max_degradation = max_degradation)
        tdprt, tdpen = runtime_model.evaluate(tdp, True), energy_model.evaluate(tdp, True)
        tdp_values = {'power': tdp,
                      'runtime': tdprt['mean'],
                      'runtimedev': rootssq(tdprt['dev']),
                      'energy': tdpen['mean'],
                      'energydev': rootssq(tdpen['dev'])}
    else:
        best_policy = policy_min_energy(plrange, energy_model, runtime_model, tdp,
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
