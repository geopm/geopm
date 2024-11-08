#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Command line interface to write controls through PlatformIO

"""

import sys
from argparse import ArgumentParser
from . import pio
from . import topo
from . import __version_str__


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
    parser_group = parser.add_mutually_exclusive_group()
    parser_group.add_argument('-C', '--config',
                              help='Path to configuration file with one write request per line, use "-" for stdin')
    parser_group.add_argument('-d', '--domain', action='store_true',
                              help='print domains detected')
    parser_group.add_argument('-i', '--info',
                              help='print longer description of a control')
    parser_group.add_argument('-I', '--info-all', action='store_true',
                              help='print longer description of all controls')
    parser_group.add_argument('-c', '--cache', action='store_true',
                              help='Create geopm topo cache if it does not exist')
    parser_group.add_argument('-v', '--version', action='store_true',
                              help='Print version and exit.')
    positional_group = parser.add_argument_group()
    positional_group.add_argument('CONTROL_NAME', nargs='?',
                                  help='Name of control')
    positional_group.add_argument('DOMAIN_TYPE',  nargs='?',
                                  help='Name of the domain for which the control should be written')
    positional_group.add_argument('DOMAIN_INDEX', nargs='?', type=int,
                                  help='Index of the domain, starting from 0')
    positional_group.add_argument('VALUE', nargs='?', type=float,
                                  help='Setting to adjust control to')
    args = parser.parse_args()
    if args.config:
        if args.config == '-':
            batch(sys.stdin)
        else:
            with open(args.config) as input_stream:
                batch(input_stream)
    elif args.domain:
        print_domains()
    elif args.info:
        print_info(args.info)
    elif args.info_all:
        print_info_all()
    elif args.cache:
        topo.create_cache()
    elif args.version:
        print(__version_str__)
    elif args.CONTROL_NAME is not None and args.DOMAIN_TYPE is not None and args.DOMAIN_INDEX is not None and args.VALUE is not None:
        pio.write_control(args.CONTROL_NAME, args.DOMAIN_TYPE, args.DOMAIN_INDEX, args.VALUE)
    elif args.CONTROL_NAME is None and args.DOMAIN_TYPE is None and args.DOMAIN_INDEX is None and args.VALUE is None:
        print_controls()
    else:
        parser.print_help()
        return -1
    return 0

def main():
    err = 0
    try:
        err = run()
    except RuntimeError as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    sys.exit(main())
