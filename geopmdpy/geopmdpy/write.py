#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Command line interface to write controls through PlatformIO

"""

import sys
import os
from argparse import ArgumentParser
from . import pio
from . import topo
from . import __version__
from . import read

def print_control_domain(control_name):
    print(topo.domain_name(pio.control_domain_type(control_name)))

def print_info(control_name):
    print(f'{control_name}:\n{pio.control_description(control_name)}')

def print_info_all():
    for control_name in pio.control_names():
        print_info(control_name)

def print_controls():
    print('\n'.join(pio.control_names()))

def batch(input_stream):
    requests = [line.split() for line in input_stream.readlines()]
    ctl_idx = []
    for rr in requests:
        try:
            ctl_idx.append(pio.push_control(rr[0], rr[1], int(rr[2])))
        except Exception as ex:
            raise RuntimeError(f'Unable to parse request configuration line: {" ".join(rr)}') from ex
    for ii, rr in enumerate(requests):
        pio.adjust(ctl_idx[ii], float(rr[3]))
    pio.write_batch()

def run():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('-v', '--version', action='version', version=f'%(prog)s {__version__}')
    parser_group = parser.add_mutually_exclusive_group()
    parser_group.add_argument('-d', '--domain', action='store_true',
                              help='print domains detected')
    parser_group.add_argument('-D', '--control-domain',
                              help='print native domain of specified control')
    parser_group.add_argument('-i', '--info',
                              help='print longer description of a control')
    parser_group.add_argument('-I', '--info-all', action='store_true',
                              help='print longer description of all controls')
    parser_group.add_argument('-c', '--cache', action='store_true',
                              help='Create geopm topo cache if it does not exist')
    parser_group.add_argument('-f', '--config',
                              help='Path to configuration file with one write request per line, use "-" for stdin')
    parser_group.add_argument('-e', '--enable-fixed', action='store_true',
                              help='enable msr fixed counters')
    parser_group.add_argument('REQUEST', nargs='*', default=[],
                              help='When using positional parameters provide four: CONTROL DOMAIN_TYPE DOMAIN_INDEX VALUE')
    args = parser.parse_args()
    if args.config:
        if args.config == '-':
            batch(sys.stdin)
        else:
            with open(args.config) as input_stream:
                batch(input_stream)
    elif args.domain:
        read.print_domains()
    elif args.control_domain:
        print_control_domain(args.control_domain)
    elif args.info:
        print_info(args.info)
    elif args.info_all:
        print_info_all()
    elif args.cache:
        topo.create_cache()
    elif args.enable_fixed:
        pio.enable_fixed_counters()
    elif len(args.REQUEST) == 0:
        print_controls()
    elif len(args.REQUEST) == 4:
        args.REQUEST[2] = int(args.REQUEST[2])
        args.REQUEST[3] = float(args.REQUEST[3])
        pio.write_control(*args.REQUEST)
    else:
        parser.error('When REQUEST is specified, all four parameters must be given: CONTROL DOMAIN_TYPE DOMAIN_INDEX VALUE')
    return 0

def main():
    err = 0
    try:
        err = run()
    except Exception as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    sys.exit(main())
