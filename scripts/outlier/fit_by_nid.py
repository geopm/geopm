#!/usr/bin/env python2
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

from config import *
from utils import *
import stats

import os
import sys

import numpy as np
import numpy.matlib as npml

# TODO is there a better way to do memoization in python
avg_package_energy = None
def compute_package_energy():
    global avg_package_energy
    if avg_package_energy is not None:
        return avg_package_energy
    package_energy = {nid: {power: [] for power in powers} for nid in nid_list}
    for ff, power, _ in report_file_iterator():
        report = parse_report(ff)
        for nid in report:
            package_energy[nid][power].append(report[nid]['dgemm']['package-energy (joules)'])
        ff.close()
    avg_package_energy = {nid: {power: [] for power in powers} for nid in nid_list}
    for nid in nid_list:
        for power in powers:
            avg_package_energy[nid][power] = sum(package_energy[nid][power])/len(package_energy[nid][power])
    return avg_package_energy

# power/frequency slope at sticker
def stat1():
    fits = {}
    slopes = {}
    for nid in nid_list:
        x1 = [] ; y1 = []
        x2 = [] ; y2 = []
        for ff, _, __ in trace_file_iterator(nid):
            for tline in trace_iterator(ff):
                if tline["region_id"] == DGEMM_REGION:
                    if tline["time_delta"] < 1e-10:
                        continue
                    if tline["frequency_f"] < Pn/P1:
                        x1.append(tline["frequency_f"])
                        y1.append(tline["energy_package_delta"]/tline["time_delta"])
                    else:
                        x2.append(tline["frequency_f"])
                        y2.append(tline["energy_package_delta"]/tline["time_delta"])
            ff.close()
        best_fit = np.polyfit(x1+x2, y1+y2, deg=3)
        if len(x1) == 0:
            best_fit1 = [NAN, NAN]
        else:
            best_fit1 = np.polyfit(x1, y1, deg=1)
        best_fit2 = np.polyfit(x2, y2, deg=3)
        fits[nid] = [list(best_fit),list(best_fit1),list(best_fit2)]
        slopes[nid] = sum([c*a for c,a in zip([3, 2, 1, 0], best_fit2)])/1.3
    return slopes

# energy consumption at TDP
def stat2():
    avg_package_energy = compute_package_energy()
    rval = {}
    for nid in nid_list:
        for power in powers:
            rval[nid] = avg_package_energy[nid][215]
    return rval

# minimum energy consumption across power caps
def stat3():
    avg_package_energy = compute_package_energy()
    rval = {}
    for nid in nid_list:
        for power in powers:
            rval[nid] = min(avg_package_energy[nid].values())
    return rval

# power cap at which energy consumption is minimized
def stat4():
    avg_package_energy = compute_package_energy()
    rval = {}
    for nid in nid_list:
        for power in powers:
            rval[nid] = min([(v, k) for k, v in avg_package_energy[nid].items()])[1]
    return rval

################################################################################

# TODO optional, use trace_iterator for this?
def stat5():
    # must be at least 2
    window = 1000

    rval = {}

    for nid in nid_list:
        XTXacc = npml.zeros((3,3))
        XTyacc = npml.zeros((3,1))
        for ff, pw, it in trace_file_iterator(nid):
            subt = []
            subdt = []
            subdE = []
            subdTs = []
            subITs = []
            subaTa = []

            datafile = []
            for tline in trace_iterator(ff):
                datafile.append([tline['time_f'], tline['energy_package_f'], tline['temperature_core_f']])

            iidx = 0
            fidx = window - 1

            tempsum = 0.5*(datafile[iidx+1][0]-datafile[iidx][0])*datafile[iidx][2] \
                    + 0.5*(datafile[fidx][0]-datafile[fidx-1][0])*datafile[fidx][2]
            for subidx in range(iidx+1,fidx):
                tempsum += 0.5*(datafile[subidx+1][0]-datafile[subidx-1][0])*datafile[subidx][2]

            for idx in range(1,len(datafile)-window):
                tempsum += 0.5*(-datafile[iidx+1][0]+datafile[iidx][0])*datafile[iidx][2] \
                        + 0.5*( datafile[iidx][0]-datafile[iidx+1][0])*datafile[iidx+1][2]\
                        + 0.5*(-datafile[fidx][0]  +datafile[fidx+1][0])*datafile[fidx][2]\
                        + 0.5*( datafile[fidx+1][0]-datafile[fidx][0])*datafile[fidx+1][2]

                iidx = idx
                fidx = iidx + window - 1

                t = 0.5*(datafile[iidx][0] + datafile[fidx][0])
                dt = datafile[fidx][0] - datafile[iidx][0]
                dE = datafile[fidx][1] - datafile[iidx][1]
                dTs = datafile[fidx][2] - datafile[iidx][2]
                ITs = tempsum

                X = np.matrix([[ \
                        dE,
                        ITs,
                        dt,
                    ]])

                y = dTs # delta temperature

                XTXacc += X.T * X
                XTyacc += X.T * y

        k1, k2, k3 = (XTXacc.I * XTyacc).T.tolist()[0]
        rval[nid] = (k1, k2, k3)
    return rval

stat1dict = stat1()
stat2dict = stat2()
stat3dict = stat3()
stat4dict = stat4()
stat5dict = stat5()
allstats = {}
for nid in stat1dict:
    allstats[nid] = (stat1dict[nid], stat2dict[nid], stat3dict[nid], stat4dict[nid]) + stat5dict[nid]

for statidx in range(7):
    print "Outliers on statistic %d" % statidx, stats.outliers(allstats, lambda x: x[statidx])
