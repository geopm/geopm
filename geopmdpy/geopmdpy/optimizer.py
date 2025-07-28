#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
Bayesian optimization tool for finding optimal control configurations.

This module uses Bayesian optimization to find the best control parameter
settings for a given application. It integrates with the ControlGrid
functionality to define the parameter space and uses subprocess execution
to evaluate different configurations.
"""

import sys
import os
import subprocess
import re
import logging
from argparse import ArgumentParser, REMAINDER
from typing import List

try:
    from skopt import gp_minimize
    from skopt.space import Integer
    from skopt.utils import use_named_args
except ImportError:
    raise ImportError(
        "scikit-optimize is required for Bayesian optimization. "
        "Install with: pip install scikit-optimize"
    )

from . import pio
from .grid import ControlGrid, _CLI_FLAG_TO_CONTROL

# Module-level logger
logger = logging.getLogger(__name__)


class OptimizationError(Exception):
    """Exception raised when optimization fails."""
    pass


class ApplicationEvaluator:
    """
    Evaluates application performance by launching a subprocess and parsing its output.

    This class is designed to assess the performance of an application by executing it
    with specific control configurations and extracting a performance metric from its
    output. It uses a regular expression to parse the application's stdout for a figure
    of merit, which can be maximized or minimized based on the user's preference.

    Attributes:
        launch_command (List[str]): Command and arguments to launch the application.
        metric_regex (str): Python-style regex to extract the performance metric.
        maximize (bool): Whether to maximize or minimize the extracted metric.
        timeout (int): Timeout in seconds for the application's execution.
        print_stdout (bool): Whether to log the application's stdout to the info level.
    """
    def __init__(self, launch_command: List[str], metric_regex: str, maximize: bool = True,
                 timeout: int = 300, print_stdout: bool = False):
        """Initialize the application evaluator.

        Args:
            launch_command: Command and arguments to launch the application
            metric_regex: Python-style regex to extract figure of merit from stdout
            maximize: Whether to maximize (True) or minimize (False) the metric
            timeout: Timeout in seconds for application execution
            print_stdout: Whether to log application stdout to info level
        """
        self.launch_command = launch_command
        self.metric_regex = metric_regex
        self.maximize = maximize
        self.timeout = timeout
        self.regex = re.compile(self.metric_regex)
        self.print_stdout = print_stdout

    def evaluate(self, control_grid: ControlGrid, coordinate: List[int], config_file: str = None) -> float:
        """Evaluate application with given control configuration.

        Args:
            control_grid: An instance of ControlGrid used to write the configuration.
            coordinate: A list of integers representing the coordinate in the control grid.
            config_file: A file path to populate with the geopmwrite configuration file.

        Returns:
            float: Figure of merit extracted from application output.

        Raises:
            OptimizationError: If evaluation fails.
        """
        try:
            if config_file is None:
                control_grid.write_config(coordinate)
            else:
                with open(config_file, 'w') as fid:
                    fid.write(control_grid.get_config_str(coordinate))

            # Launch application
            logger.debug(f"Launching application: {' '.join(self.launch_command)}")
            result = subprocess.run(
                self.launch_command,
                capture_output=True,
                text=True,
                timeout=self.timeout
            )

            if result.returncode != 0:
                if self.print_stdout:
                    logger.info(f"Application stdout: \n{result.stdout}\n")
                logger.debug(f"Application stderr: \n{result.stderr}\n")
                raise OptimizationError(
                    f"Application failed with return code {result.returncode}: {result.stderr}"
                )

            # Extract metric from stdout
            logger.debug(f"Application stdout length: {len(result.stdout)} characters")
            if self.print_stdout:
                logger.info(f"Application stdout: \n{result.stdout}\n")
            if not result.stdout:
                raise OptimizationError("Application produced no output")
            metric = self._extract_metric(result.stdout)
            logger.debug(f"Extracted metric: {metric}")

            # Convert to minimization problem if needed
            if self.maximize:
                return -metric  # Negate for maximization
            else:
                return metric

        except subprocess.TimeoutExpired:
            raise OptimizationError(f"Application evaluation timed out after {self.timeout} seconds")
        except FileNotFoundError as e:
            raise OptimizationError(f"Command not found: {e}")
        except PermissionError as e:
            raise OptimizationError(f"Permission denied: {e}")
        except Exception as e:
            raise OptimizationError(f"Evaluation failed: {e}")

    def _extract_metric(self, stdout: str) -> float:
        """Extract figure of merit from application stdout.

        Args:
            stdout: Standard output from application

        Returns:
            float: Extracted metric value

        Raises:
            OptimizationError: If metric cannot be extracted
        """
        match = self.regex.search(stdout)
        if not match:
            logger.debug(f"Failed to match regex '{self.metric_regex}' in stdout (first 200 chars): {stdout[:200]}")
            raise OptimizationError(
                f"Could not extract metric from output using pattern: {self.metric_regex}"
            )

        # Try to convert the matched group to float
        try:
            if match.groups():
                # Use first capture group if available
                metric_str = match.group(1)
            else:
                # Use entire match
                metric_str = match.group(0)

            logger.debug(f"Extracted metric string: '{metric_str}'")
            return float(metric_str)
        except ValueError:
            raise OptimizationError(
                f"Could not convert extracted value to float: {metric_str}"
            )


class BayesianOptimizer:
    """Bayesian optimizer for control parameter tuning."""

    def __init__(self, control_grid: ControlGrid, evaluator: ApplicationEvaluator, config_file: str = None):
        """Initialize the Bayesian optimizer.

        Args:
            control_grid: ControlGrid defining the parameter space
            evaluator: ApplicationEvaluator for performance assessment
        """
        self.control_grid = control_grid
        self.evaluator = evaluator
        self.space = self._create_search_space()
        self.best_result = None
        self.evaluation_history = []
        self.config_file = config_file

        logger.debug(f"Initialized optimizer with {len(self.space)} dimensions")

    def _create_search_space(self) -> List[Integer]:
        """Create search space from control grid dimensions.

        Returns:
            List of Integer dimensions for optimization
        """
        space = []
        grid_data = self.control_grid.get_grid_data()

        # Validate that grid data matches control structure
        expected_dims = len(self.control_grid.control_name)
        if len(grid_data) != expected_dims:
            raise ValueError(
                f"Grid data dimensions ({len(grid_data)}) do not match "
                f"control dimensions ({expected_dims}). "
                f"This indicates a problem with the ControlGrid configuration."
            )

        for idx, dim in enumerate(grid_data):
            settings = dim["settings"]
            # Validate each dimension
            if len(settings) == 0:
                raise ValueError(f"No settings available for {dim['control']}.")
            if len(settings) == 1:
                raise ValueError(f"Only one setting available for {dim['control']}.")

            logger.debug(f"Dimension {idx}: {dim['control']}@{dim['domain']}-{dim['domain_idx']} "
                        f"with {len(settings)} settings")

            # Create integer space representing indices into the settings array
            space.append(Integer(
                0, len(settings) - 1,
                name=f"{dim['control']}@{dim['domain']}-{dim['domain_idx']}"
            ))
        return space

    def optimize(self, trials: int = 50, n_initial_points: int = 10,
                 random_state: int = 42, use_efficiency: int = 0) -> dict:
        """Run Bayesian optimization.

        Args:
            trials: Number of optimization iterations
            n_initial_points: Number of random initial evaluations
            random_state: Random seed for reproducibility
            use_efficiency: Whether to optimize for efficiency
                0: use metric directly
                1: maximizing, divide by average power
                -1: minimizing, multiply by average power

        Returns:
            dict: Optimization results including best configuration and value
        """
        def get_energy():
            all_signals = pio.signal_names()
            if "BOARD_ENERGY" in all_signals:
                return pio.read_signal("BOARD_ENERGY", 0, 0)
            result = 0
            if "GPU_ENERGY" in all_signals:
                result = pio.read_signal("GPU_ENERGY", 0, 0)
            if "CPU_ENERGY" in all_signals:
                result += pio.read_signal("CPU_ENERGY", 0, 0)
            if "DRAM_ENERGY" in all_signals:
                result += pio.read_signal("DRAM_ENERGY", 0, 0)
            if result == 0:
                raise OptimizationError("No energy signals available to compute efficiency")
            return result

        @use_named_args(self.space)
        def objective(**params):
            """Objective function for optimization."""
            # Convert parameter indices to actual coordinate values
            # The coordinate should match the number of dimensions in the grid
            coordinate = [int(params[dim.name]) for dim in self.space]  # Convert numpy types to int

            # Validate coordinate length matches expected dimensions
            expected_dims = len(self.control_grid.control_name)
            if len(coordinate) != expected_dims:
                raise ValueError(
                           f"Coordinate length mismatch: got {len(coordinate)}, "
                           f"expected {expected_dims}. "
                           f"Space: {len(self.space)}, "
                           f"Grid data: {len(self.control_grid.get_grid_data())}")

            # Validate coordinate values are within bounds
            grid_data = self.control_grid.get_grid_data()
            for idx, coord_val in enumerate(coordinate):
                if idx >= len(grid_data):
                    raise ValueError(f"Coordinate index {idx} out of range for grid data (size {len(grid_data)})")

                settings_count = len(grid_data[idx]["settings"])
                if coord_val < 0 or coord_val >= settings_count:
                    raise ValueError(f"Coordinate value {coord_val} at index {idx} out of range [0, {settings_count-1}]")

            # Evaluate application
            if use_efficiency:
                start_time = pio.read_signal("TIME", 0, 0)
                start_energy = get_energy()
            metric = self.evaluator.evaluate(self.control_grid, coordinate, self.config_file)
            if use_efficiency:
                end_energy = get_energy()
                end_time = pio.read_signal("TIME", 0, 0)
                average_power = (end_energy - start_energy) / (end_time - start_time)
                logger.info(f"Average power consumed: {average_power} W")
                if average_power <= 0:
                    raise OptimizationError("Average power consumed is non-positive")
                if use_efficiency == 1:
                    metric /= average_power
                elif use_efficiency == -1:
                    metric *= average_power


            # Store evaluation history
            self.evaluation_history.append({
                'coordinate': coordinate.copy(),
                'metric': metric,
                'config_commands': self.control_grid.get_config_str(coordinate)
            })

            logger.info(f"Evaluation {len(self.evaluation_history)}: "
                       f"coordinate={coordinate}, metric={metric}")

            return metric


        # Run optimization
        logger.info(f"Starting Bayesian optimization with {trials} evaluations...")

        result = gp_minimize(
            func=objective,
            dimensions=self.space,
            n_calls=trials,
            n_initial_points=n_initial_points,
            random_state=random_state,
            acq_func='EI'  # Expected Improvement
        )

        # Store best result
        self.best_result = result

        # Convert best coordinate to actual configuration
        best_coordinate = [int(x) for x in result.x]
        best_config = self.control_grid.get_config_str(best_coordinate)

        # Convert metric back if maximizing
        best_metric = result.fun
        if self.evaluator.maximize:
            best_metric = -best_metric

        return {
            'best_coordinate': best_coordinate,
            'best_metric': best_metric,
            'best_config': best_config,
            'n_evaluations': len(result.func_vals),
            'optimization_result': result
        }


def get_parser():
    """Create argument parser for the optimizer."""
    parser = ArgumentParser(description="Bayesian optimization for control parameter tuning")

    # Control grid options (similar to grid.py)
    for flag, control in _CLI_FLAG_TO_CONTROL.items():
        if flag == 'gpu_power_intel':
            flag = 'gpu_power'
        elif flag == 'gpu_power_nvml':
            continue
        flag_dash = flag.replace('_', '-')
        parser.add_argument(
            f"--{flag_dash}",
            default=None,
            dest=f'{flag}_domain',
            help=f"Provide a grid over {control[0]} for the given domain.",
        )

    # Optimization options
    parser.add_argument(
        '--trials',
        type=int,
        default=50,
        help='Number of optimization iterations (default: %(default)s)'
    )

    parser.add_argument(
        '--n-initial-points',
        type=int,
        default=10,
        help='Number of random initial evaluations (default: %(default)s)'
    )

    parser.add_argument(
        '--metric-regex',
        required=True,
        help='Python-style regex to extract figure of merit from application stdout'
    )

    parser.add_argument(
        '--minimize',
        action='store_true',
        help='Minimize the metric (default is to maximize)'
    )

    parser.add_argument(
        '--random-seed',
        type=int,
        default=42,
        help='Random seed for reproducibility (default: %(default)s)'
    )

    parser.add_argument(
        '--application-timeout',
        type=int,
        default=300,
        help='Timeout in seconds for application execution (default: %(default)s)'
    )

    parser.add_argument(
        '--output-file',
        default='-',
        help='Output the best configuration to a geompwrite configuration file (default: stdout)'
    )

    parser.add_argument(
        '--verbosity',
        type=int,
        default=1,
        choices=[0, 1, 2, 3],
        help='Verbosity level: 0=ERROR, 1=WARNING, 2=INFO, 3=DEBUG (default: %(default)s)'
    )

    parser.add_argument(
        '--print-stdout',
        action='store_true',
        help='Print application stdout to info level log (default: False)'
    )

    parser.add_argument(
        '--defer-write',
        action='store_true',
        help='Defer writing the configuration, requires --output-file',
    )
    parser.add_argument(
        '--efficiency',
        action='store_true',
        help='Optimize for efficiency by dividing the metric by the average power consumed',
    )

    # Application launch command
    parser.add_argument(
        'launch',
        nargs=REMAINDER,
        help='Command to launch application for evaluation'
    )

    return parser


def main():
    """Main function for the optimization tool."""
    err = 0
    parser = get_parser()
    args = parser.parse_args()

    if args.defer_write and (args.output_file is None or args.output_file == '-'):
        raise ValueError('Must specify a valid --output-file (not stdout) when using --defer-write')

    # Set up logging based on verbosity level
    log_levels = [logging.ERROR, logging.WARNING, logging.INFO, logging.DEBUG]
    logging.basicConfig(
        level=log_levels[args.verbosity],
        format='%(levelname)s: %(message)s'
    )

    pio.save_control()
    try:
        # Remove leading '--' from launch command if present
        launch_command = args.launch
        if launch_command and launch_command[0] == '--':
            launch_command = launch_command[1:]

        if not launch_command:
            raise ValueError("Error: No launch command specified")

        # Create control grid from arguments
        grid_args = []
        for flag in _CLI_FLAG_TO_CONTROL.keys():
            if flag in ['gpu_power_nvml']:
                continue
            if flag == 'gpu_power_intel':
                flag = 'gpu_power'

            domain = getattr(args, f'{flag}_domain', None)
            if domain is not None:
                grid_args.extend([f'--{flag.replace("_", "-")}', domain])

        if not grid_args:
            raise ValueError("Error: No control parameters specified")

        control_grid = ControlGrid(grid_args)

        if len(control_grid.control_name) == 0:
            raise ValueError("Error: No valid control dimensions configured")

        # Validate control grid before creating optimizer
        grid_data = control_grid.get_grid_data()
        for dim in grid_data:
            settings = dim["settings"]
            if len(settings) == 0:
                raise RuntimeError(f"Error: Control '{dim['control']}' on domain '{dim['domain']}' "
                                   f"index {dim['domain_idx']} has no available settings.")
        # Create application evaluator
        evaluator = ApplicationEvaluator(
            launch_command=launch_command,
            metric_regex=args.metric_regex,
            maximize=not args.minimize,
            timeout=args.application_timeout,
            print_stdout=args.print_stdout
        )
        # Create and run optimizer
        config_file = None
        if args.defer_write:
            config_file = args.output_file
        optimizer = BayesianOptimizer(control_grid, evaluator, config_file)
        if not args.efficiency:
            efficiency = 0
        elif args.minimize:
            efficiency = -1
        else:
            efficiency = 1
        result = optimizer.optimize(
            trials=args.trials,
            n_initial_points=args.n_initial_points,
            random_state=args.random_seed,
            use_efficiency=efficiency
        )

        # Print results
        logger.info("Optimization completed!")
        logger.info(f"Best metric: {result['best_metric']}")
        logger.info(f"Best coordinate: {result['best_coordinate']}")
        logger.info(f"Number of evaluations: {result['n_evaluations']}")

        # Always print the best configuration to stdout for user visibility
        print(f"Best configuration:\n{result['best_config']}")

        if args.output_file != '-':
            with open(args.output_file, 'w') as fid:
                fid.write(result['best_config'])
            logger.info(f"Best configuration written to {args.output_file}")

    except Exception as e:
        if "GEOPM_DEBUG" in os.environ:
            raise
        logger.error(f"Error: {e}")
        err = 1
    finally:
        pio.restore_control()
    return err

if __name__ == "__main__":
    sys.exit(main())
