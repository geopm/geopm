#!/usr/bin/env python
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

NAN = float("nan")

def trace_file_iterator(nid):
    for power in powers:
        for it in iterations:
            try:
                ff = open(trace_template % (power, it, nid))
            except IOError:
                pass
            else:
                yield(ff, power, it)


def trace_iterator(file_pointer):
    fp = file_pointer
    if type(file_pointer) == str:
        fp = open(file_pointer)

    headers = None
    last = {}
    for line in fp:
        if line[0] == '#':
            continue

        if headers is None:
            headers = line.strip().split("|")
            continue

        ll = dict(zip(headers, line.strip().split("|")))

        rval = {}
        rval.update(ll)
        for k in ll:
            try:
                rval[k + "_f"] = float(ll[k])
            except ValueError:
                pass

        for k in last:
            rval[k + "_prev"] = last[k]
            try:
                rval[k + "_prev_f"] = float(last[k])
                rval[k + "_delta"] = float(rval[k]) - float(last[k])
            except ValueError:
                pass

        yield(rval)
        last = ll

def report_file_iterator():
    for power in powers:
        for it in iterations:
            try:
                ff = open(report_template % (power, it))
            except IOError:
                pass
            else:
                yield(ff, power, it)

def parse_report(file_pointer):
    fp = file_pointer
    if type(file_pointer) == str:
        fp = open(file_pointer)

    report = "".join(fp.readlines())
    host_blocks = report.split("\n\n")[1:]

    data = {}

    for host_block in host_blocks:
        hbs = host_block.split("\n")
        if len(hbs) == 0:
            continue
        hostline, hbs = hbs[0], hbs[1:]
        if not hostline.startswith("Host:"):
            continue
        host_id = int(hostline.split(": nid")[1])
        data[host_id] = {}
        regions = []
        for line in hbs:
            if line.startswith("   "):
                regions[-1].append(line)
            else:
                regions.append([line])

        for region in regions:
            if region[0].startswith("Region"):
                region_name = region[0].split()[1]
            else:
                region_name = "Application Total"
            data[host_id][region_name] = {}
            for line in region[1:]:
                k,v = line.strip().split(": ")
                try:
                    v = float(v)
                except ValueError:
                    pass
                data[host_id][region_name][k] = v
    return data

if __name__ == '__main__':
    print(parse_report('../310274-dgemm-4node-1rank-62thread-2019-02-06_1420/dgemm_power_balancer_205_1_report'))
