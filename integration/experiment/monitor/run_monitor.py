#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run any GEOPM appconf with the GEOPM monitor agent. Works with appconfs that
support ``create_appconf(Machine, argv)``.
'''

import apps
import argparse
import importlib
import inspect
import os
import pkgutil
import sys

from experiment import machine
from experiment.monitor import monitor

def list_compatible_apps(args, parent_parser):
    """List the apps that can be launched with this experiment script.
    """
    compatible_module_names = list()
    all_app_packages = list(
        m.name
        for m in pkgutil.iter_modules([os.path.dirname(apps.__file__)])
        if m.ispkg)
    for package_name in all_app_packages:
        module = importlib.import_module(f'apps.{package_name}.{package_name}')
        global_functions = set(
            name for name, function in inspect.getmembers(module, inspect.isfunction))
        if global_functions.issuperset({'create_appconf', 'setup_run_args'}):
            compatible_module_names.append(package_name)
    print('\n'.join(compatible_module_names))


def run_app(args, parent_parser):
    """Launch an app with the monitor experiment type.
    """
    app_name = args.application
    app_experiment_args = args.app_experiment_args
    module = importlib.import_module(f'apps.{app_name}.{app_name}')

    parser = argparse.ArgumentParser()
    monitor.setup_run_args(parser)
    module.setup_run_args(parser)
    known_args, extra_args = parser.parse_known_args(app_experiment_args)
    mach = machine.init_output_dir(known_args.output_dir)

    app_conf = module.create_appconf(mach, known_args)
    monitor.launch(app_conf=app_conf, args=known_args,
                   experiment_cli_args=extra_args)


def show_help(args, parent_parser):
    """Show the help message for a given app, or show the default help message
    if no apps provided.
    """
    app_name = vars(args).get('application', None)

    if app_name is None:
        parent_parser.print_help()
    else:
        try:
            module = importlib.import_module(f'apps.{app_name}.{app_name}')
        except ModuleNotFoundError:
            parent_parser.error(f'Cannot find app: {app_name}')
        parser = argparse.ArgumentParser()
        monitor.setup_run_args(parser)
        module.setup_run_args(parser)
        parser.print_help()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.set_defaults(func=show_help)
    subparsers = parser.add_subparsers()

    parser_list = subparsers.add_parser(
        'list', help='List the applications can run with this experiment')
    parser_list.set_defaults(func=list_compatible_apps)

    parser_run = subparsers.add_parser(
        'run', help='Run an application with the monitor experiment')
    parser_run.add_argument('application', help='Name of an application to run')
    parser_run.add_argument('app_experiment_args', nargs=argparse.REMAINDER, help='Arguments for the experiment and app.')
    parser_run.set_defaults(func=run_app)

    parser_help = subparsers.add_parser(
        'help', help='Show help for an application with the monitor experiment')
    parser_help.add_argument('application', nargs='?', help='Name of an application')
    parser_help.set_defaults(func=show_help)

    args = parser.parse_args()
    if args.func is not None:
        args.func(args, parser)
