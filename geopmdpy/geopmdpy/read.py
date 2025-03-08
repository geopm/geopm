#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Command line interface to read signals through PlatformIO

"""

import sys
import os
from argparse import ArgumentParser
from . import pio
from . import topo
from . import __version__


def print_domains():
    print(f"""\
board                       {topo.num_domain('board')}
package                     {topo.num_domain('package')}
core                        {topo.num_domain('core')}
cpu                         {topo.num_domain('cpu')}
memory                      {topo.num_domain('memory')}
package_integrated_memory   {topo.num_domain('package_integrated_memory')}
nic                         {topo.num_domain('nic')}
package_integrated_nic      {topo.num_domain('package_integrated_nic')}
gpu                         {topo.num_domain('gpu')}
package_integrated_gpu      {topo.num_domain('package_integrated_gpu')}
gpu_chip                    {topo.num_domain('gpu_chip')}""")

def print_signal_domain(signal_name):
    print(topo.domain_name(pio.signal_domain_type(signal_name)))

def print_info(signal_name):
    print(f'{signal_name}:\n{pio.signal_description(signal_name)}')

def print_info_all():
    for signal_name in pio.signal_names():
        print_info(signal_name)

def print_signals():
    print('\n'.join(pio.signal_names()))

def run():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('-v', '--version', action='version', version=f'%(prog)s {__version__}')
    parser_group = parser.add_mutually_exclusive_group()
    parser_group.add_argument('-d', '--domain', action='store_true',
                              help='print domains detected')
    parser_group.add_argument('-D', '--signal-domain',
                              help='print native domain of specified signal')
    parser_group.add_argument('-i', '--info',
                              help='print longer description of a signal')
    parser_group.add_argument('-I', '--info-all', action='store_true',
                              help='print longer description of all signals')
    parser_group.add_argument('-c', '--cache', action='store_true',
                              help='Create geopm topo cache if it does not exist')
    parser_group.add_argument('REQUEST', nargs='*', default=[],
                              help='When using positional parameters provide three: SIGNAL DOMAIN_TYPE DOMAIN_INDEX')
    args = parser.parse_args()
    if args.domain:
        print_domains()
    elif args.signal_domain:
        print_signal_domain(args.signal_domain)
    elif args.info:
        print_info(args.info)
    elif args.info_all:
        print_info_all()
    elif args.cache:
        topo.create_cache()
    elif len(args.REQUEST) == 0:
        print_signals()
    elif len(args.REQUEST) == 3:
        try:
            args.REQUEST[2] = int(args.REQUEST[2])
        except ValueError as ex:
            raise ValueError(f'invalid domain index: {args.REQUEST[2]}') from ex
        signal = pio.read_signal(*args.REQUEST)
        print(pio.format_signal(signal, pio.signal_info(args.REQUEST[0])[1]))
    else:
        parser.error('When REQUEST is specified, all three parameters must be given: SIGNAL DOMAIN_TYPE DOMAIN_INDEX')
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
