#!/usr/bin/python

# python 2 compatibility
from __future__ import print_function

import math
import numpy as np
import sys

nid_pos_dict = {int(ll[0]): dict(zip(("column", "row", "chassis", "slot", "node"), tuple(map(int, ll[1:])))) for ll in map(lambda l: l.strip().split(), open('theta_nodelist_broken.txt').readlines())}

# dealing with the deprecated column names in the nekbone traces
keymap = {"seconds": "time",
        "progress-0": "region_progress",
        "runtime-0": "region_runtime",
        "pkg_energy-0": "energy_package",
        "dram_energy-0": "energy_dram"}


def progress(iterable, message=lambda x: x, log=sys.stdout, length=None):
    if length is None:
        length = len(iterable)
    index = 0
    for x in iterable:
        yield(x)
        index += 1
        if log:
            log.write("\r%s = %d/%d" % (message(x), index, length))
            log.flush()
    if log:
        log.write("\n")
        log.flush()


def trace_iterate(trace_file):
    headers = None
    last = {}
    for line in open(trace_file):
        if line[0] == '#':
            continue
        if headers is None:
            raw_headers = line.strip().split("|")
            headers = []
            for h in raw_headers:
                if h in keymap:
                    headers.append(keymap[h])
                else:
                    headers.append(h)
            continue
        ll = line.strip().split("|")
        ldict = {h: {True: lambda: l, False: lambda: float(l)}[l.startswith("0x")]() for h, l in zip(headers, ll)}
        delta = {}
        if ldict['region_id'] == '0x2000000000000000':
            continue
        if ldict['region_id'] != '0x8000000000000000':
            continue
        if ldict['region_id'] not in last:
            last[ldict['region_id']] = ldict.copy()
        if ldict['region_progress'] == 1.0:
            region_id = ldict['region_id']
            out = {}
            for k in ldict:
                out[k+"_i"] = last[region_id][k]
                out[k+"_f"] = ldict[k]
                if type(ldict[k]) == float:
                    out[k + "_delta"] = ldict[k] - last[region_id][k]
            del last[region_id]

            # ignore epochs of 0 time (??)
            if out['time_delta'] > 0:
                yield out


def power_and_temperature_stats(nids, traces):
    global_stats = {'power': 0, 'temperature': 0} ; global_Z = 0
    per_node_stats = {}

    for nid, trace_file in progress(zip(nids, traces), message=lambda (n,_): "Computing Stats [Node %5d]" % n):
        Z = 0
        per_node_stats[nid] = {'power': 0, 'temperature': 0}
        for ldict in trace_iterate(trace_file):
            per_node_stats[nid]['power'] += ldict['energy_package_delta']/ldict['time_delta']
            per_node_stats[nid]['temperature'] += (ldict['temperature_core_i'] + ldict['temperature_core_f'])*0.5
            Z += 1.
        global_stats['power'] += per_node_stats[nid]['power']
        global_stats['temperature'] += per_node_stats[nid]['temperature']
        global_Z += Z

        per_node_stats[nid]['power'] /= Z
        per_node_stats[nid]['temperature'] /= Z
    global_stats['power'] /= global_Z
    global_stats['temperature'] /= global_Z

    return global_stats, per_node_stats


def global_fit(nids, traces, dict_to_signal):
    mu = None ; sigma = None ; N = 0
    for nid, trace_file in progress(zip(nids, traces), message=lambda (n,_): "Global Fit [Node %5d]" % n):
        allsignals = []
        for ldict in trace_iterate(trace_file):
            ldict.update(nid_pos_dict[nid])
            allsignals.append(dict_to_signal(ldict))
        for signal in allsignals[1:-1]:
            if mu is None:
                mu = signal
                sigma = np.matmul(signal.T, signal)
                N = 1
            else:
                mu += signal
                sigma += np.matmul(signal.T, signal)
                N += 1

    mu /= N
    sigma /= N
    sigma -= np.matmul(mu.T, mu)

    return mu, sigma


def compute_likelihood(nids, traces, mu, sigma, dict_to_signal):
    logltable = {}

    siginv = sigma.I

    # TODO compute mean power and temperature
    for nid, trace_file in progress(zip(nids, traces), message=lambda (n,_): "Computing Likelihood [Node %5d]" % n):
        allsignals = []
        for ldict in trace_iterate(trace_file):
            ldict.update(nid_pos_dict[nid])
            allsignals.append(ldict)
        likelihood = 0 ; duration = 0
        for ldict in allsignals[1:-1]:
            signal = dict_to_signal(ldict)
            dist = ((signal - mu) * siginv * (signal - mu).T).item(0)
            likelihood += ldict['time_delta'] * dist
            duration += ldict['time_delta']
        logltable[nid] = likelihood/duration

    return logltable


def compute_outliers(nids, traces, logltable, base_thresh, p_val=0.05):
    outliers = []
    for nid in logltable:
        not_outlier_prob = math.exp(-(logltable[nid] - base_thresh))
        if not_outlier_prob < p_val:
            outliers.append((1-not_outlier_prob, nid))

    return outliers


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Detect some outliers.')
    parser.add_argument('files', metavar='tracefile', type=str, nargs='+',
                        help='trace files')

    args = parser.parse_args()
    traces = args.files
    traces.sort()

    nids = [int(trf[-5:]) for trf in traces]

    dict_to_signal = lambda ldict: np.matrix([
        math.log(ldict['energy_package_delta']/ldict['time_delta']),
        math.log(ldict['cycles_thread_delta']/ldict['cycles_reference_delta']),
        (ldict['temperature_core_i'] + ldict['temperature_core_f'])*0.5,
        ldict['row'],
    ])

    mu, sigma = global_fit(nids, traces, dict_to_signal=dict_to_signal)
    logltable = compute_likelihood(nids, traces, mu, sigma, dict_to_signal=dict_to_signal)
    base_thresh = (lambda L: L[int(len(L)*0.9)])(sorted(logltable.values()))

    outliers = compute_outliers(nids, traces, logltable, base_thresh)

    global_avg, node_stats = power_and_temperature_stats(nids, traces)

    if len(outliers) == 0:
        print("No outliers detected.")
    else:
        print("OUTLIERS IDENTIFIED:")
        outliers.sort()
        for outlier_prob, nid in outliers:
            delta = {k: node_stats[nid][k] - global_avg[k] for k in global_avg.keys()}
            status = {True: "Runt", False: "Pick"}[delta['power'] >= 0]
            print("Node %5d, %5.2f%%, %s, %5.1fW, %5.1fC" % (nid, 100*outlier_prob, status, delta['power'], delta['temperature']))
