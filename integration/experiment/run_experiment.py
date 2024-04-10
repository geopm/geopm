#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Run any GEOPM appconf with an experiment script. Works with appconfs that
support ``create_appconf(Machine, argv)``, and with experiments that support
``launch(app_conf, args, experiment_cli_args)``. Both appconf and experiment
must also support ``setup_run_args(parser)``.
'''

import apps
import experiment
import argparse
import importlib
import inspect
import os
import pkgutil
from integration.experiment import machine


def list_compatible_apps(args, parent_parser):
    """List the apps that can be launched with this experiment script.
    """
    compatible_module_names = list()
    all_app_packages = list(
        m.name
        for m in pkgutil.iter_modules([os.path.dirname(apps.__file__)])
        if m.ispkg)
    for package_name in all_app_packages:
        try:
            module = importlib.import_module(f'apps.{package_name}.{package_name}')
        except ModuleNotFoundError:
            # Do not list apps that we cannot use
            continue
        global_functions = set(
            name for name, function in inspect.getmembers(module, inspect.isfunction))
        if global_functions.issuperset({'create_appconf', 'setup_run_args'}):
            compatible_module_names.append(package_name)
    print('\n'.join(compatible_module_names))


def list_compatible_experiments(args, parent_parser):
    """List the experiments that can be launched with this experiment script.
    """
    compatible_module_names = list()
    all_experiment_packages = list(
        m.name
        for m in pkgutil.iter_modules([os.path.dirname(experiment.__file__)])
        if m.ispkg)
    for package_name in all_experiment_packages:
        try:
            module = importlib.import_module(f'experiment.{package_name}.{package_name}')
        except ModuleNotFoundError:
            # Do not list experiments that we cannot use
            continue
        global_functions = set(
            name for name, function in inspect.getmembers(module, inspect.isfunction))
        if global_functions.issuperset({'launch', 'setup_run_args'}):
            compatible_module_names.append(package_name)
    print('\n'.join(compatible_module_names))


def run_app(args, parent_parser):
    """Launch an app with the selected experiment type.
    """
    app_name = args.application
    experiment_name = args.experiment
    app_experiment_args = args.app_experiment_args
    app_module = importlib.import_module(f'apps.{app_name}.{app_name}')
    experiment_module = importlib.import_module(f'experiment.{experiment_name}.{experiment_name}')

    parser = argparse.ArgumentParser()
    experiment_module.setup_run_args(parser)
    app_module.setup_run_args(parser)
    known_args, extra_args = parser.parse_known_args(app_experiment_args)
    mach = machine.init_output_dir(known_args.output_dir)

    app_conf = app_module.create_appconf(mach, known_args)
    experiment_module.launch(app_conf=app_conf, args=known_args,
                             experiment_cli_args=extra_args)


def show_help(args, parent_parser):
    """Show the help message for a given app and/or experiment, or show the
    default help message if no app or experiment is provided.
    """
    try:
        app_name = args.application
        experiment_name = args.experiment
    except AttributeError:
        # Catch the case where the user wants help with the wrapper interface itself
        app_name = None
        experiment_name = None

    parser = argparse.ArgumentParser()
    if app_name is not None:
        try:
            app_module = importlib.import_module(f'apps.{app_name}.{app_name}')
        except ModuleNotFoundError:
            parent_parser.error(f'Cannot find module: {app_name}')
        app_module.setup_run_args(parser)
    if experiment_name is not None:
        try:
            experiment_module = importlib.import_module(f'experiment.{experiment_name}.{experiment_name}')
        except ModuleNotFoundError:
            parent_parser.error(f'Cannot find module: {experiment_name}')
        experiment_module.setup_run_args(parser)

    if experiment_name is not None or app_name is not None:
        # Either app, experiment, or both were provided. Show the help message
        # from that (or those) module(s).
        parser.print_help()
    else:
        # No app or experiment was provided. Show the help message for this CLI
        parent_parser.print_help()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.set_defaults(func=show_help)
    subparsers = parser.add_subparsers()

    parser_list_apps = subparsers.add_parser(
        'apps', help='List the applications can run with this CLI')
    parser_list_apps.set_defaults(func=list_compatible_apps)

    parser_list_experiments = subparsers.add_parser(
        'experiments', help='List the experiments can run with this CLI')
    parser_list_experiments.set_defaults(func=list_compatible_experiments)

    parser_run = subparsers.add_parser(
        'run', help='Run an application with the selected experiment type')
    parser_run.add_argument('experiment', help='Name of an experiment to run')
    parser_run.add_argument('application', help='Name of an application to run')
    parser_run.add_argument('app_experiment_args',
                            nargs=argparse.REMAINDER,
                            help='Arguments for the experiment and app.')
    parser_run.set_defaults(func=run_app)

    parser_help = subparsers.add_parser(
        'help', help='Show help for an application with the experiment')
    parser_help.add_argument('--experiment', help='Name of an experiment')
    parser_help.add_argument('--application', help='Name of an application')
    parser_help.set_defaults(func=show_help)

    args = parser.parse_args()
    if args.func is not None:
        args.func(args, parser)
