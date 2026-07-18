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
import time
import shutil
import tempfile
import logging
from argparse import ArgumentParser, REMAINDER
from dataclasses import dataclass
from enum import Enum
from typing import List, Optional

try:
    from skopt import gp_minimize
    from skopt.space import Integer
    from skopt.utils import use_named_args
except ImportError:
    raise ImportError(
        "scikit-optimize is required for Bayesian optimization. "
        "Install with: python3 -m pip install scikit-optimize"
    )

from . import pio
from .grid import ControlGrid, _CLI_FLAG_TO_CONTROL, add_grid_cli_arguments

try:
    import yaml
except ImportError:
    yaml = None

# Default geopmsession sampling period in seconds. It is small enough that the
# RAPL energy counter is sampled several times before it can wrap, which is
# what makes the report energy delta rollover-safe (issue #4043).
_DEFAULT_SAMPLE_PERIOD = 0.01

# Module-level logger
logger = logging.getLogger(__name__)


class OptimizationError(Exception):
    """Exception raised when optimization fails."""
    pass


class ObjectiveMode(Enum):
    """The optimization objective resolved from the command-line inputs.

    The mode is a function of whether ``--metric-regex``, ``--efficiency``,
    and ``--metric-bound`` are supplied; see :func:`resolve_objective_mode`
    and the objective matrix in the geopmopt documentation.
    """
    #: Optimize the figure of merit scraped from stdout (``--minimize`` toggles).
    RAW_METRIC = 'raw_metric'
    #: Optimize figure of merit per average power over the efficiency domain.
    EFFICIENCY = 'efficiency'
    #: Minimize total wall-clock runtime (no figure of merit required).
    RUNTIME = 'runtime'
    #: Minimize total energy over the efficiency domain (no FoM required).
    ENERGY = 'energy'
    #: Minimize energy subject to a bound on the figure of merit.
    ENERGY_BOUNDED = 'energy_bounded'


#: Objective modes that require sampling energy around each trial.
_ENERGY_MODES = frozenset(
    {ObjectiveMode.EFFICIENCY, ObjectiveMode.ENERGY, ObjectiveMode.ENERGY_BOUNDED})


def resolve_objective_mode(metric_regex, efficiency_domain, metric_bound,
                           minimize):
    """Resolve the optimization objective from the command-line inputs.

    Args:
        metric_regex: The ``--metric-regex`` value, or None when omitted.
        efficiency_domain: The ``--efficiency`` domain, or None when omitted.
        metric_bound: The ``--metric-bound`` value, or None when omitted.
        minimize: Whether ``--minimize`` was requested. The optimization
            direction does not change which objective is selected; it is
            accepted here so this function is the single place that consumes
            all objective-determining inputs.

    Returns:
        ObjectiveMode: The resolved objective.

    Raises:
        ValueError: If the combination of inputs is invalid.
    """
    if metric_bound is not None:
        if metric_regex is None:
            raise ValueError('--metric-bound requires --metric-regex')
        if efficiency_domain is None:
            raise ValueError('--metric-bound requires --efficiency')
        return ObjectiveMode.ENERGY_BOUNDED
    if metric_regex is not None:
        if efficiency_domain is not None:
            return ObjectiveMode.EFFICIENCY
        return ObjectiveMode.RAW_METRIC
    if efficiency_domain is not None:
        return ObjectiveMode.ENERGY
    return ObjectiveMode.RUNTIME


@dataclass
class TrialResult:
    """Raw measurements produced by evaluating a single control configuration.

    Fields are populated incrementally by the evaluation strategies. ``fom`` is
    the figure of merit scraped from application stdout; ``energy``,
    ``runtime``, and ``average_power`` are filled in when an efficiency
    objective is active. ``failed`` / ``failure_reason`` are reserved for later
    recoverable-failure handling and are unused today.

    Attributes:
        fom: Figure of merit extracted from application output.
        energy: Energy consumed during the run (Joules).
        runtime: Wall-clock or reported runtime of the run (seconds).
        average_power: Mean power over the run (Watts).
        failed: Whether the trial failed in a recoverable way.
        failure_reason: Human-readable description of a recoverable failure.
    """
    fom: Optional[float] = None
    energy: Optional[float] = None
    runtime: Optional[float] = None
    average_power: Optional[float] = None
    failed: bool = False
    failure_reason: Optional[str] = None


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
        objective_mode (ObjectiveMode): The resolved optimization objective.
        efficiency_domain (str): Domain used to measure energy, or None.
        sample_period (float): geopmsession sampling period for energy objectives.
        needs_session (bool): Whether trials run under geopmsession for energy.
    """
    def __init__(self, launch_command: List[str], metric_regex: str = None, maximize: bool = True,
                 timeout: int = 300, print_stdout: bool = False,
                 objective_mode: ObjectiveMode = ObjectiveMode.RAW_METRIC,
                 efficiency_domain: str = None,
                 sample_period: float = _DEFAULT_SAMPLE_PERIOD):
        """Initialize the application evaluator.

        Args:
            launch_command: Command and arguments to launch the application
            metric_regex: Python-style regex to extract figure of merit from
                stdout. When None, no figure of merit is scraped and only the
                application's wall-clock runtime is measured.
            maximize: Whether to maximize (True) or minimize (False) the metric
            timeout: Timeout in seconds for application execution
            print_stdout: Whether to log application stdout to info level
            objective_mode: The resolved optimization objective. Energy
                objectives launch the application under geopmsession so that
                energy and time are sampled with rollover accounting.
            efficiency_domain: Domain used to measure energy for energy
                objectives (``board``/``cpu``/``gpu``).
            sample_period: geopmsession sampling period in seconds.
        """
        self.launch_command = launch_command
        self.metric_regex = metric_regex
        self.maximize = maximize
        self.timeout = timeout
        self.regex = re.compile(self.metric_regex) if self.metric_regex is not None else None
        self.print_stdout = print_stdout
        self.objective_mode = objective_mode
        self.efficiency_domain = efficiency_domain
        self.sample_period = sample_period
        self.needs_session = objective_mode in _ENERGY_MODES
        self._energy_signals = None
        if self.needs_session:
            if yaml is None:
                raise OptimizationError(
                    "The pyyaml module is required for energy objectives; "
                    "install with: python3 -m pip install pyyaml")
            if shutil.which('geopmsession') is None:
                raise OptimizationError(
                    "geopmsession was not found on PATH; it is required for "
                    "energy-based objectives (--efficiency)")
            self._energy_signals = energy_signal_names(self.efficiency_domain)

    def evaluate(self, control_grid: ControlGrid, coordinate: List[int], config_file: str = None) -> 'TrialResult':
        """Evaluate application with given control configuration.

        Args:
            control_grid: An instance of ControlGrid used to write the configuration.
            coordinate: A list of integers representing the coordinate in the control grid.
            config_file: A file path to populate with the geopmwrite configuration file.

        Returns:
            TrialResult: Container holding the trial's measurements. Runtime
                objectives populate ``runtime``; figure-of-merit objectives
                populate ``fom``; energy objectives populate ``energy``,
                ``runtime``, and ``average_power`` from the session report.

        Raises:
            OptimizationError: If evaluation fails.
        """
        try:
            if config_file is None:
                control_grid.write_config(coordinate)
            else:
                with open(config_file, 'w') as fid:
                    fid.write(f'{control_grid.get_config_str(coordinate)}\n')

            if self.needs_session:
                return self._evaluate_session()
            return self._evaluate_direct()

        except OptimizationError:
            raise
        except subprocess.TimeoutExpired:
            raise OptimizationError(f"Application evaluation timed out after {self.timeout} seconds")
        except FileNotFoundError as e:
            raise OptimizationError(f"Command not found: {e}")
        except PermissionError as e:
            raise OptimizationError(f"Permission denied: {e}")
        except Exception as e:
            raise OptimizationError(f"Evaluation failed: {e}")

    def _check_returncode(self, result) -> None:
        """Raise OptimizationError when the launched application exits non-zero."""
        if result.returncode != 0:
            if self.print_stdout:
                logger.info(f"Application stdout: \n{result.stdout}\n")
            logger.debug(f"Application stderr: \n{result.stderr}\n")
            raise OptimizationError(
                f"Application failed with return code {result.returncode}: {result.stderr}"
            )

    def _evaluate_direct(self) -> 'TrialResult':
        """Run the application directly and measure wall-clock runtime.

        Used for the RAW_METRIC and RUNTIME objectives, which do not need
        energy measurement.
        """
        logger.debug(f"Launching application: {' '.join(self.launch_command)}")
        start = time.monotonic()
        result = subprocess.run(
            self.launch_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            timeout=self.timeout
        )
        runtime = time.monotonic() - start

        self._check_returncode(result)

        logger.debug(f"Application stdout length: {len(result.stdout)} characters")
        if self.print_stdout:
            logger.info(f"Application stdout: \n{result.stdout}\n")

        # Without a metric regex the objective is runtime-based; only the
        # measured wall-clock time is required.
        if self.regex is None:
            return TrialResult(runtime=runtime)

        if not result.stdout:
            raise OptimizationError("Application produced no output")
        metric = self._extract_metric(result.stdout)
        logger.debug(f"Extracted metric: {metric}")
        return TrialResult(fom=metric, runtime=runtime)

    def _signal_config_str(self) -> str:
        """Build the geopmsession signal-config for the efficiency domain."""
        lines = ['TIME board 0']
        lines.extend(f'{name} board 0' for name in self._energy_signals)
        return '\n'.join(lines) + '\n'

    def _session_argv(self, signal_path: str, report_path: str) -> List[str]:
        """Build the geopmsession command line that wraps the application."""
        return [
            'geopmsession',
            '--period', repr(self.sample_period),
            '--report-out', report_path,
            '--trace-out', '/dev/null',
            '--signal-config', signal_path,
            '--', *self.launch_command,
        ]

    def _load_report(self, report_path: str) -> dict:
        """Load and minimally validate the geopmsession YAML report."""
        try:
            with open(report_path) as fid:
                report = yaml.safe_load(fid)
        except FileNotFoundError:
            raise OptimizationError("geopmsession did not produce a report")
        if not isinstance(report, dict):
            raise OptimizationError("geopmsession produced a malformed report")
        return report

    def _compute_energy_metrics(self, report: dict):
        """Derive (energy, runtime, average_power) from a session report.

        The energy and runtime are computed from the ``last - first`` deltas of
        the report metrics, which are rollover-corrected by geopmsession, so a
        RAPL wrap during the run does not corrupt the result.
        """
        metrics = report.get('metrics')
        if not metrics:
            raise OptimizationError("Session report is missing metrics")
        if 'TIME' not in metrics:
            raise OptimizationError("Session report is missing the TIME metric")
        runtime = metrics['TIME']['last'] - metrics['TIME']['first']
        energy = 0.0
        for name in self._energy_signals:
            if name not in metrics:
                raise OptimizationError(f"Session report is missing the {name} metric")
            energy += metrics[name]['last'] - metrics[name]['first']
        if runtime <= 0:
            raise OptimizationError("Session reported non-positive runtime")
        average_power = energy / runtime
        return energy, runtime, average_power

    def _evaluate_session(self) -> 'TrialResult':
        """Run the application under geopmsession and read energy from the report.

        Used for the energy objectives (EFFICIENCY / ENERGY / ENERGY_BOUNDED).
        """
        signal_fd, signal_path = tempfile.mkstemp(prefix='geopmopt_signal_', suffix='.conf')
        report_fd, report_path = tempfile.mkstemp(prefix='geopmopt_report_', suffix='.yaml')
        try:
            os.close(signal_fd)
            os.close(report_fd)
            with open(signal_path, 'w') as fid:
                fid.write(self._signal_config_str())

            argv = self._session_argv(signal_path, report_path)
            logger.debug(f"Launching application under geopmsession: {' '.join(argv)}")
            result = subprocess.run(
                argv,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
                timeout=self.timeout
            )

            self._check_returncode(result)

            if self.print_stdout:
                logger.info(f"Application stdout: \n{result.stdout}\n")

            report = self._load_report(report_path)
            energy, runtime, average_power = self._compute_energy_metrics(report)

            fom = None
            if self.regex is not None:
                if not result.stdout:
                    raise OptimizationError("Application produced no output")
                fom = self._extract_metric(result.stdout)

            return TrialResult(fom=fom, energy=energy, runtime=runtime,
                               average_power=average_power)
        finally:
            for path in (signal_path, report_path):
                try:
                    os.unlink(path)
                except FileNotFoundError:
                    pass


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

def energy_signal_names(domain: str) -> List[str]:
    """Return the energy signal names used to measure the given domain.

    This is the single source of truth for the domain-to-signal mapping. It is
    used both to read energy in-process (:func:`get_energy`) and to build the
    geopmsession signal-config for the energy objectives.

    Args:
        domain: One of ``board``, ``cpu``, or ``gpu``.

    Returns:
        List[str]: Available energy signal names whose deltas sum to the
            domain energy. ``board`` prefers a single ``BOARD_ENERGY`` signal
            and otherwise sums the available component signals.

    Raises:
        ValueError: If the domain is not supported.
        OptimizationError: If no energy signals are available for the domain.
    """
    all_signals = pio.signal_names()
    names: List[str] = []
    if domain == 'board':
        if "BOARD_ENERGY" in all_signals:
            return ["BOARD_ENERGY"]
        # If we don't have board energy sum all components
        if "GPU_ENERGY" in all_signals:
            names.append("GPU_ENERGY")
        # Prefer powercap for energy measurements over default
        if "POWERCAP::CPU_ENERGY_CONSUMED" in all_signals:
            names.append("POWERCAP::CPU_ENERGY_CONSUMED")
        elif "CPU_ENERGY" in all_signals:
            names.append("CPU_ENERGY")
        if "DRAM_ENERGY" in all_signals:
            names.append("DRAM_ENERGY")
    elif domain == 'cpu':
        # Prefer powercap for energy measurements over default
        if "POWERCAP::CPU_ENERGY_CONSUMED" in all_signals:
            names.append("POWERCAP::CPU_ENERGY_CONSUMED")
        elif "CPU_ENERGY" in all_signals:
            names.append("CPU_ENERGY")
    elif domain == 'gpu':
        if "GPU_ENERGY" in all_signals:
            names.append("GPU_ENERGY")
    else:
        raise ValueError(f'Unsupported domain {domain}, must be one of "board", "gpu", or "cpu"')
    if not names:
        raise OptimizationError("No energy signals available to compute efficiency")
    return names


def get_energy(domain: str):
    result = sum(pio.read_signal(name, 0, 0)
                 for name in energy_signal_names(domain))
    if result == 0:
        raise OptimizationError("No energy signals available to compute efficiency")
    return result


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

    def _evaluate_coordinate(self, coordinate: List[int]) -> TrialResult:
        """Execute a single trial and return its raw measurements.

        Validates the coordinate and launches the application through the
        evaluator. The evaluator selects the direct or geopmsession execution
        strategy and populates the energy fields for energy objectives.

        Args:
            coordinate: Grid coordinate to evaluate.

        Returns:
            TrialResult: Figure of merit and/or wall-clock runtime, plus
                energy/runtime/average power when an energy objective is active.
        """
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

        return self.evaluator.evaluate(self.control_grid, coordinate, self.config_file)

    def _score_result(self, trial: TrialResult,
                      objective_mode: ObjectiveMode) -> float:
        """Reduce a TrialResult to the scalar objective value to minimize.

        Runtime and energy objectives are minimized directly. For the figure
        of merit objectives the maximize/minimize sign convention is applied
        and, for the efficiency objective, average power is folded in.

        Args:
            trial: Raw measurements from :meth:`_evaluate_coordinate`.
            objective_mode: The resolved optimization objective.

        Returns:
            float: Scalar objective value in minimization space.
        """
        if objective_mode == ObjectiveMode.RUNTIME:
            return trial.runtime
        if objective_mode == ObjectiveMode.ENERGY:
            return trial.energy
        if objective_mode == ObjectiveMode.ENERGY_BOUNDED:
            raise NotImplementedError(
                "The ENERGY_BOUNDED objective is implemented in a later phase")

        # RAW_METRIC and EFFICIENCY optimize the figure of merit.
        if self.evaluator.maximize:
            metric = -trial.fom  # Negate for maximization
        else:
            metric = trial.fom
        if objective_mode == ObjectiveMode.EFFICIENCY:
            average_power = trial.average_power
            logger.info(f"Average power consumed: {average_power} W")
            if average_power <= 0:
                raise OptimizationError("Average power consumed is non-positive")
            if self.evaluator.maximize:
                metric /= average_power
            else:
                metric *= average_power
        return metric

    def _record_history(self, coordinate: List[int], metric: float) -> None:
        """Append a completed trial to the evaluation history and log it.

        Args:
            coordinate: Grid coordinate that was evaluated.
            metric: Scalar objective value produced by :meth:`_score_result`.
        """
        self.evaluation_history.append({
            'coordinate': coordinate.copy(),
            'metric': metric,
            'config_commands': self.control_grid.get_config_str(coordinate)
        })

        logger.info(f"Evaluation {len(self.evaluation_history)}: "
                   f"coordinate={coordinate}, metric={metric}")

    def optimize(self, trials: int = 50, n_initial_points: int = 10,
                 random_state: int = 42,
                 objective_mode: ObjectiveMode = ObjectiveMode.RAW_METRIC) -> dict:
        """Run Bayesian optimization.

        Args:
            trials: Number of optimization iterations
            n_initial_points: Number of random initial evaluations
            random_state: Random seed for reproducibility
            objective_mode: The resolved optimization objective.

        Returns:
            dict: Optimization results including best configuration and value
        """

        @use_named_args(self.space)
        def objective(**params):
            """Objective function for optimization."""
            # Convert parameter indices to actual coordinate values
            coordinate = [int(params[dim.name]) for dim in self.space]  # Convert numpy types to int
            trial = self._evaluate_coordinate(coordinate)
            metric = self._score_result(trial, objective_mode)
            self._record_history(coordinate, metric)
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

        # Convert metric back if maximizing. Runtime and energy objectives are
        # already reported in their natural (minimized) units.
        best_metric = result.fun
        if (objective_mode in (ObjectiveMode.RAW_METRIC, ObjectiveMode.EFFICIENCY)
                and self.evaluator.maximize):
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
    add_grid_cli_arguments(parser)

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
        default=None,
        help='Python-style regex to extract the figure of merit from '
             'application stdout. When omitted, the objective defaults to '
             'total wall-clock runtime, or total energy when --efficiency is '
             'also provided.'
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
        default=None,
        dest='efficiency_domain',
        help='Optimize for efficiency by dividing the metric by the average power consumed over the specified domain',
    )
    parser.add_argument(
        '--sample-period',
        type=float,
        default=_DEFAULT_SAMPLE_PERIOD,
        dest='sample_period',
        help='geopmsession sampling period in seconds for energy-based '
             'objectives; shorter periods add overhead (default: %(default)s)',
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
    print_exception = args.verbosity >= 3 or "GEOPM_DEBUG" in os.environ

    if not args.defer_write:
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
        dimension_specified = False
        for control_flag in _CLI_FLAG_TO_CONTROL.keys():
            flag_dash = control_flag.replace('_', '-')
            domain = getattr(args, f'{control_flag}_domain', None)
            if domain is not None:
                dimension_specified = True
                grid_args.extend([f'--{flag_dash}', domain])
            for suffix in ('min', 'max', 'step'):
                value = getattr(args, f'{control_flag}_{suffix}', None)
                if value is not None:
                    grid_args.extend([f'--{flag_dash}-{suffix}', str(value)])

        if not dimension_specified:
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

        if (args.efficiency_domain is not None and
            args.efficiency_domain not in ('board', 'gpu', 'cpu')):
            raise ValueError(f'Unsupported domain {args.efficiency_domain}, must be one of "board", "gpu", or "cpu"')

        # The --metric-bound argument is introduced in a later phase; until
        # then it is always absent.
        metric_bound = getattr(args, 'metric_bound', None)
        objective_mode = resolve_objective_mode(
            args.metric_regex, args.efficiency_domain, metric_bound,
            args.minimize)

        # Create application evaluator
        evaluator = ApplicationEvaluator(
            launch_command=launch_command,
            metric_regex=args.metric_regex,
            maximize=not args.minimize,
            timeout=args.application_timeout,
            print_stdout=args.print_stdout,
            objective_mode=objective_mode,
            efficiency_domain=args.efficiency_domain,
            sample_period=args.sample_period
        )
        # Create and run optimizer
        config_file = None
        if args.defer_write:
            config_file = args.output_file
        optimizer = BayesianOptimizer(control_grid, evaluator, config_file)

        result = optimizer.optimize(
            trials=args.trials,
            n_initial_points=args.n_initial_points,
            random_state=args.random_seed,
            objective_mode=objective_mode
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
                fid.write(f"{result['best_config']}\n")
            logger.info(f"Best configuration written to {args.output_file}")

    except Exception as e:
        if print_exception:
            raise
        logger.error(f"Error: {e}")
        err = 1
    finally:
        if not args.defer_write:
            pio.restore_control()
    return err

if __name__ == "__main__":
    sys.exit(main())
