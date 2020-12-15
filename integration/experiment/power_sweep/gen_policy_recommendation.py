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
Creates a model relating power-limit to both runtime and energy
consumption. Offers power-governor policy recommendations for
minimum energy use subject to a runtime degradation constraint.
'''

import os
import sys
import argparse
import json

import numpy as np
from sklearn.preprocessing import PolynomialFeatures
from sklearn.linear_model import LinearRegression

import geopmpy.io

from experiment import common_args
from experiment import machine

# abstract model class
# TODO add types and help text! modern python!
class PowerLimitModel:
    def __init__(self):
        # TODO raise not implemented
        pass

    def train(self, PL_list, value_list):
        # TODO raise not implemented
        pass

    def test(self, PL):
        # TODO raise not implemented
        pass

    def batch_test(self, PL_list):
        return [self.test(PL) for PL in PL_list]

    def serialize(self):
        # TODO raise not implemented
        # actually make this convert to/from json nicely
        pass

    def __str__(self):
        # TODO classname?
        return "<%s>" % "PowerLimitModel"

    def repr(self):
        return self.__str__()


class PolynomialFitPowerModel(PowerLimitModel):
    def __init__(self, degree):
        self._degree = degree
        self._transform = PolynomialFeatures(
                degree=self._degree,
                include_bias=False)
        self._model = None

    def train(self, PL_list, value_list):
        # TODO check if PL_list and value_list are the same length
        x1 = np.array(PL_list).reshape((-1, 1))
        yy = np.array(value_list)
        xn = self._transform.fit_transform(x1)
        self._model = LinearRegression().fit(xn, yy)

    def test(self, PL):
        # TODO shouldn't have to look inside the coeffs and 
        # intercept to do this
        return self._model.intercept_ + \
               sum([
                   c * PL**(i+1) for i, c in enumerate(self._model.coef_)
                   ])

    def __str__(self):
        if self._model:
            terms = ["", " PL"] + \
                    [" PL ** %d" % (i+1)
                            for i in range(1, len(self._model.coef_))]
            coeffs = [self._model.intercept] + self._model.coef_
            expression = ""
            terms.reverse() ; coeffs.reverse() ; firstTerm = True
            for tt, cc in zip(terms, coeffs):
                if not firstTerm:
                    if cc < 0:
                        expression += " - "
                        cc *= -1
                    else:
                        expression += " + "
                expression += str(cc) + tt
            # TODO classname
            return "<%s %s>" % ("Polyfitmodel", expression)
        else:
            return "<%s (untrained)>" % "Polyfitmodel"  #TODO classname


class CubicPowerModel(PolynomialFitPowerModel):
    def __init__(self):
        super().__init__(degree=3)


def extract_from_reports(df, region_filter):
    simplified_data = []
    
    for profile in df['Profile'].unique():
        # profile will be something like "unifiedmodel_power_governor_250_1"
        power_limit = int(profile.split('_')[-2])  # TODO - access the policy
        df_profile = df[df['Profile'] == profile]
        for host in df_profile['host'].unique():
            df_profile_host = df_profile[df_profile['host'] == host]
            total_rt = 0
            total_en = 0
            for region_name in df_profile_host['region'].unique():
                if region_filter and region_name not in region_filter:
                    continue
                region = df_profile_host[df_profile_host['region'] == region_name]
                runtime = sum(region['runtime (sec)'])
                energy = sum(region['package-energy (joules)'])
    
                total_rt += runtime
                total_en += energy
    
            simplified_data.append((power_limit, total_rt, total_en))
    return simplified_data


def dump_simplified_data(simplified_data, fname):
    with open(fname, "w") as raw_out:
        for pl, rt, en in sorted(simplified_data):
            raw_out.write("{} {} {}\n".format(pl, rt, en))


def dump_stats_summary(simplified_data, fname):
    count = {}
    for pl, rt, en in sorted(simplified_data):
        if pl in count:
            count[pl] += 1
        else:
            count[pl] = 1

    rtx = {} ; rtxx = {}
    enx = {} ; enxx = {}
    for pl in count:
        rtx[pl] = 0 ; rtxx[pl] = 0
        enx[pl] = 0 ; enxx[pl] = 0


    for pl, rt, en in sorted(simplified_data):
        rtx[pl] += rt
        rtxx[pl] += rt**2
        enx[pl] += en
        enxx[pl] += en**2

    for pl in count:
        rtx[pl] /= count[pl]
        rtxx[pl] /= count[pl]
        enx[pl] /= count[pl]
        enxx[pl] /= count[pl]

    with open(fname, "w") as stats_out:
        for pl in sorted(count.keys()):
            stats_out.write(
                    "{} {} {} {} {}\n".format((
                        pl,
                        rtx[pl],
                        (rtxx[pl] - rtx[pl]**2)**.5,
                        enx[pl],
                        (enxx[pl] - enx[pl]**2)**.5)))


def analyze_extracted_data(simplified_data, rtmodel, enmodel):
    rtmodel.train([pl for pl, _, __ in simplified_data],
            [rt for _, rt, __ in simplified_data])
    enmodel.train([pl for pl, _, __ in simplified_data],
            [en for _, __, en in simplified_data])


def policy_min_energy(plrange, enmodel, rtmodel=None, pltdp=None, max_degradation=None):
    if pltdp is None:
        pltdp = max(plrange)

    en_predictions = enmodel.batch_test(plrange)
    if max_degradation is None:
        # we don't need the runtime model in this case
        besten, bestpl = min(zip(en_predictions, plrange))
        if rtmodel:
            bestrt = rtmodel.test(bestpl)
        else:
            bestrt = None
        return bestpl, bestrt, besten
    else:
        rt_predictions = rtmodel.batch_test(plrange)
        rt_at_tdp = rtmodel.test(pltdp)
        constrained_values = [
                (en, pl, rt)
                for pl, rt, en in zip(plrange, rt_predictions, en_predictions)
                if rt <= (1 + max_degradation) * rt_at_tdp]
        besten, bestpl, bestrt = min(constrained_values)
        return bestpl, bestrt, besten


def main(df,
        region_filter,
        dump_prefix,
        min_pl,
        max_pl,
        tdp,
        min_energy,
        max_degradation):
    simplified_data = extract_from_reports(df, args.region_filter)
    if dump_prefix:
        dump_simplified_data(simplified_data, "{}.dat".format(dump_prefix))
        dump_stats_summary(simplified_data, "{}.stats".format(dump_prefix))

    rtmodel = CubicPowerModel()
    enmodel = CubicPowerModel()

    analyze_extracted_data(simplified_data, rtmodel, enmodel)

    plrange = [min_pl + i for i in range(int(max_pl - min_pl))] + [max_pl]
    bestpl, bestrt, besten = policy_min_energy(plrange, enmodel, rtmodel, tdp, max_degradation=(None if args.min_energy else args.max_degradation))
    tdprt, tdpen = rtmodel.test(tdp), enmodel.test(tdp)

    return {'tdp': {'power': tdp, 'runtime': tdprt, 'energy': tdpen}, 
            'best': {'power': bestpl, 'runtime': bestrt, 'energy': besten}}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    common_args.add_min_power(parser)
    common_args.add_max_power(parser)
    parser.add_argument('--path', required=True,
                        help='path containing reports and machine.json')
    parser.add_argument('--region_filter', default=None,
                        help='comma-separated list of regions to include, '
                             'default to include all regions')
    parser.add_argument('--dump_prefix',
                        help='prefix to dump statistics to, empty to not dump '
                             'stats')
    parser.add_argument('--tdp', default=-1, type=float,
                        help='tdp (sticker power limit), default behavior is '
                             'to use max power')
    parser.add_argument('--max_degradation', default=0.1, type=float,
                        help='maximum allowed runtime degradation, default is '
                             '0.1 (i.e., 10%%)')
    parser.add_argument('--min_energy', action='store_true',
                        help='ignore max degradation, just give the minimum '
                             'energy possible')

    args = parser.parse_args()

    try:
        df = geopmpy.io.RawReportCollection(
                "*report", dir_name=args.path).get_df()
    except RuntimeError:
        sys.stderr.write("<geopm> Error: No report data found in " + path + \
                "; run a power sweep before using this analysis.\n")
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
            sys.stderr.write("Warning: couldn't open machine.json. Falling "
                             "back to default values.\n")
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
                  args.min_energy, args.max_degradation)

    sys.stdout.write(
            "AT TDP = {power:.0f}W, "
            "RUNTIME = {runtime:.0f} s, "
            "ENERGY = {energy:.0f} J\n"\
                    .format(**output['tdp']))
    sys.stdout.write(
            "AT PL  = {power:.0f}W, "
            "RUNTIME = {runtime:.0f} s, "
            "ENERGY = {energy:.0f} J\n"\
                    .format(**output['best']))

    relative_delta = lambda new, old: (new - old) / old

    sys.stdout.write(
            "DELTA          RUNTIME = {:.1f} %,  ENERGY = {:.1f} %\n"\
                    .format(
                        100 * relative_delta(
                            output['best']['runtime'],
                            output['tdp']['runtime']),
                        100 * relative_delta(
                            output['best']['energy'],
                            output['tdp']['energy'])))
