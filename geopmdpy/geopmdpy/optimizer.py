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
import math
import shutil
import tempfile
import logging
from argparse import ArgumentParser, ArgumentTypeError, REMAINDER
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
from .grid import ControlGrid, add_grid_cli_arguments

try:
    import yaml
except ImportError:
    yaml = None

# Default geopmsession sampling period in seconds. It is small enough that the
# RAPL energy counter is sampled several times before it can wrap, which is
# what makes the report energy delta rollover-safe (issue #4043).
_DEFAULT_SAMPLE_PERIOD = 0.01

# Default weight applied to the bound-violation penalty of the ENERGY_BOUNDED
# objective. The penalty is scaled by the observed energy magnitude so that a
# fully violated bound is at least as costly as a representative trial's energy;
# the weight lets that penalty be tuned relative to energy differences.
_DEFAULT_BOUND_PENALTY_WEIGHT = 1.0

# Objective value assigned to a recoverable failure that occurs before any
# trial has succeeded, when the automatic penalty cannot yet be derived from
# observed successes. It is finite so it never corrupts the surrogate model.
_BOOTSTRAP_PENALTY = 1e9

# Module-level logger
logger = logging.getLogger(__name__)


class OptimizationError(Exception):
    """Exception raised when optimization fails fatally and must abort the run."""
    pass


class RecoverableEvaluationError(Exception):
    """A trial-level failure that can be penalized instead of aborting the run.

    Raised internally by the evaluation strategies for the recoverable failure
    set (timeout, non-zero exit, figure-of-merit scrape miss, non-positive
    power/runtime). :meth:`ApplicationEvaluator.evaluate` converts it into a
    failed :class:`TrialResult` rather than propagating it.
    """
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

        except RecoverableEvaluationError as e:
            logger.warning(f"Trial failed (recoverable): {e}")
            return TrialResult(failed=True, failure_reason=str(e))
        except subprocess.TimeoutExpired:
            reason = f"Application evaluation timed out after {self.timeout} seconds"
            logger.warning(f"Trial failed (recoverable): {reason}")
            return TrialResult(failed=True, failure_reason=reason)
        except OptimizationError:
            raise
        except FileNotFoundError as e:
            raise OptimizationError(f"Command not found: {e}")
        except PermissionError as e:
            raise OptimizationError(f"Permission denied: {e}")
        except Exception as e:
            raise OptimizationError(f"Evaluation failed: {e}")

    def _check_returncode(self, result) -> None:
        """Flag a non-zero application exit as a recoverable trial failure."""
        if result.returncode != 0:
            if self.print_stdout:
                logger.info(f"Application stdout: \n{result.stdout}\n")
            logger.debug(f"Application stderr: \n{result.stderr}\n")
            raise RecoverableEvaluationError(
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
            raise RecoverableEvaluationError("Application produced no output")
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
            raise RecoverableEvaluationError("Session reported non-positive runtime")
        average_power = energy / runtime
        if average_power <= 0:
            raise RecoverableEvaluationError("Session reported non-positive power")
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
                    raise RecoverableEvaluationError("Application produced no output")
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
            RecoverableEvaluationError: If the metric cannot be scraped or
                parsed; the trial is penalized rather than aborting the run.
        """
        match = self.regex.search(stdout)
        if not match:
            logger.debug(f"Failed to match regex '{self.metric_regex}' in stdout (first 200 chars): {stdout[:200]}")
            raise RecoverableEvaluationError(
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
            raise RecoverableEvaluationError(
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

        # ENERGY_BOUNDED state, (re)initialized at the start of optimize().
        self._metric_bound = None
        self._penalty_weight = _DEFAULT_BOUND_PENALTY_WEIGHT
        self._energy_scale = None

        # Recoverable-failure penalty policy, set at the start of optimize().
        self._penalty = 'auto'

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

    def _bound_violation(self, fom: float) -> float:
        """Return the normalized amount by which a FoM misses the bound.

        The violation is expressed as a fraction of the bound magnitude so it
        is dimensionless and comparable across problems. It is zero for a
        feasible trial and positive for an infeasible one. When maximizing the
        figure of merit the bound is a floor (feasible when ``fom >= bound``);
        when minimizing it is a ceiling (feasible when ``fom <= bound``).

        Args:
            fom: Figure of merit scraped from the trial.

        Returns:
            float: Non-negative normalized bound violation.
        """
        bound = self._metric_bound
        scale = abs(bound) if bound != 0 else 1.0
        if self.evaluator.maximize:
            return max(0.0, bound - fom) / scale
        return max(0.0, fom - bound) / scale

    def _score_result(self, trial: TrialResult,
                      objective_mode: ObjectiveMode) -> float:
        """Reduce a TrialResult to the scalar objective value to minimize.

        Runtime and energy objectives are minimized directly. For the figure
        of merit objectives the maximize/minimize sign convention is applied
        and, for the efficiency objective, average power is folded in. The
        energy-bounded objective minimizes energy with a soft penalty applied
        when the figure-of-merit bound is violated.

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
            return self._score_energy_bounded(trial)

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

    def _score_energy_bounded(self, trial: TrialResult) -> float:
        """Score an energy-bounded trial as energy plus a bound-violation penalty.

        Feasible trials score their plain energy. Infeasible trials score
        strictly higher: ``energy + penalty_weight * energy_scale * violation``,
        where ``energy_scale`` tracks the largest energy magnitude observed so
        far so the penalty stays commensurate with energy differences.

        Args:
            trial: Raw measurements, with ``fom`` and ``energy`` populated.

        Returns:
            float: The penalized energy objective to minimize.
        """
        energy = trial.energy
        magnitude = abs(energy)
        if self._energy_scale is None:
            self._energy_scale = magnitude
        else:
            self._energy_scale = max(self._energy_scale, magnitude)

        violation = self._bound_violation(trial.fom)
        if violation <= 0.0:
            return energy
        return energy + self._penalty_weight * self._energy_scale * violation

    def _penalty_value(self) -> float:
        """Return the objective value assigned to a recoverable failure.

        For the ``auto`` policy the penalty is derived to be strictly worse
        (larger, since the objective is minimized) than every successful trial
        observed so far, so the surrogate model learns to avoid the failing
        region without an infinite objective. The value is always finite. When
        a failure precedes any success the automatic value cannot be derived,
        so a finite bootstrap penalty is used and a warning is emitted.

        Returns:
            float: The finite penalty objective for a failed trial.
        """
        if isinstance(self._penalty, (int, float)):
            return float(self._penalty)

        successes = [h['metric'] for h in self.evaluation_history
                     if not h.get('failed')]
        if not successes:
            logger.warning(
                "A trial failed before any trial succeeded; using a bootstrap "
                f"penalty of {_BOOTSTRAP_PENALTY}")
            return float(_BOOTSTRAP_PENALTY)

        worst = max(successes)
        spread = worst - min(successes)
        # Keep the penalty strictly worse than the worst success even when all
        # successes tie (spread == 0).
        margin = spread if spread > 0 else (abs(worst) if worst != 0 else 1.0)
        penalty = worst + margin
        if not math.isfinite(penalty):
            penalty = sys.float_info.max
        return penalty

    def _handle_failed_trial(self, trial: TrialResult) -> float:
        """Turn a failed trial into an objective value per the penalty policy.

        Args:
            trial: The failed :class:`TrialResult`.

        Returns:
            float: The penalty objective to report to the optimizer.

        Raises:
            OptimizationError: When the penalty policy is ``none``, restoring
                the legacy behavior of aborting the run on any failure.
        """
        if self._penalty == 'none':
            raise OptimizationError(
                f"Trial failed and --penalty is 'none': {trial.failure_reason}")
        penalty = self._penalty_value()
        logger.warning(
            f"Penalizing failed trial ({trial.failure_reason}) "
            f"with objective {penalty}")
        return penalty

    def _record_history(self, coordinate: List[int], metric: float,
                        trial: 'TrialResult' = None,
                        objective_mode: ObjectiveMode = None) -> None:
        """Append a completed trial to the evaluation history and log it.

        Args:
            coordinate: Grid coordinate that was evaluated.
            metric: Scalar objective value produced by :meth:`_score_result`.
            trial: Raw measurements, used to record energy-bounded feasibility.
            objective_mode: The active objective, used to decide what to record.
        """
        entry = {
            'coordinate': coordinate.copy(),
            'metric': metric,
            'config_commands': self.control_grid.get_config_str(coordinate)
        }
        if trial is not None and trial.failed:
            entry['failed'] = True
            entry['failure_reason'] = trial.failure_reason
            # A failed trial can never satisfy a figure-of-merit bound.
            if objective_mode == ObjectiveMode.ENERGY_BOUNDED:
                entry['feasible'] = False
        elif objective_mode == ObjectiveMode.ENERGY_BOUNDED and trial is not None:
            entry['fom'] = trial.fom
            entry['energy'] = trial.energy
            entry['feasible'] = self._bound_violation(trial.fom) <= 0.0
        self.evaluation_history.append(entry)

        logger.info(f"Evaluation {len(self.evaluation_history)}: "
                   f"coordinate={coordinate}, metric={metric}")

    def optimize(self, trials: int = 50, n_initial_points: int = 10,
                 random_state: int = 42,
                 objective_mode: ObjectiveMode = ObjectiveMode.RAW_METRIC,
                 metric_bound: float = None,
                 penalty_weight: float = _DEFAULT_BOUND_PENALTY_WEIGHT,
                 penalty='auto') -> dict:
        """Run Bayesian optimization.

        Args:
            trials: Number of optimization iterations
            n_initial_points: Number of random initial evaluations
            random_state: Random seed for reproducibility
            objective_mode: The resolved optimization objective.
            metric_bound: Figure-of-merit bound for the ENERGY_BOUNDED
                objective. Trials whose FoM violates the bound are penalized.
            penalty_weight: Weight of the bound-violation penalty for the
                ENERGY_BOUNDED objective.
            penalty: Recoverable-failure policy: ``'auto'`` derives a finite
                penalty worse than every success, ``'none'`` re-raises on any
                failure (legacy behavior), or a float sets a fixed penalty.

        Returns:
            dict: Optimization results including best configuration and value
        """
        # (Re)initialize the energy-bounded penalty state for this run.
        self._metric_bound = metric_bound
        self._penalty_weight = penalty_weight
        self._energy_scale = None
        self._penalty = penalty

        @use_named_args(self.space)
        def objective(**params):
            """Objective function for optimization."""
            # Convert parameter indices to actual coordinate values
            coordinate = [int(params[dim.name]) for dim in self.space]  # Convert numpy types to int
            trial = self._evaluate_coordinate(coordinate)
            if trial.failed:
                metric = self._handle_failed_trial(trial)
            else:
                metric = self._score_result(trial, objective_mode)
            self._record_history(coordinate, metric, trial, objective_mode)
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

        if objective_mode == ObjectiveMode.ENERGY_BOUNDED:
            return self._build_bounded_result(result)

        # Select the best successful trial, excluding penalized failures.
        best_coordinate, best_metric = self._select_best(result)
        best_config = self.control_grid.get_config_str(best_coordinate)

        # Convert metric back if maximizing. Runtime and energy objectives are
        # already reported in their natural (minimized) units.
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

    def _select_best(self, result):
        """Return the best successful coordinate and its minimization metric.

        Failed trials are excluded from selection. When the evaluation history
        is available it is the source of truth; a run in which every trial
        failed raises a clear error. When the history is empty (for example
        under a mocked optimizer) the scikit-optimize result is used directly.

        Args:
            result: The scikit-optimize result from ``gp_minimize``.

        Returns:
            tuple: ``(best_coordinate, best_metric)`` in minimization space.

        Raises:
            OptimizationError: If every recorded trial failed.
        """
        successes = [h for h in self.evaluation_history if not h.get('failed')]
        if self.evaluation_history and not successes:
            raise OptimizationError(
                "All trials failed; no successful configuration was found")
        if successes:
            best = min(successes, key=lambda h: h['metric'])
            return list(best['coordinate']), best['metric']
        # Fallback for callers that do not populate history (mocked runs).
        return [int(x) for x in result.x], result.fun

    def _build_bounded_result(self, result) -> dict:
        """Assemble the result for the ENERGY_BOUNDED objective.

        Prefers the feasible trial with the lowest energy. When no trial
        satisfied the bound, falls back to the successful trial that minimized
        the penalized objective (the least-infeasible trial) and marks the
        result as not satisfying the bound. Failed trials are never selected,
        and a run in which every trial failed raises a clear error.

        Args:
            result: The scikit-optimize result from ``gp_minimize``.

        Returns:
            dict: Result including ``bound_satisfied`` and ``best_fom``.

        Raises:
            OptimizationError: If every recorded trial failed.
        """
        non_failed = [h for h in self.evaluation_history if not h.get('failed')]
        if self.evaluation_history and not non_failed:
            raise OptimizationError(
                "All trials failed; no successful configuration was found")

        feasible = [h for h in non_failed if h.get('feasible')]
        if feasible:
            best = min(feasible, key=lambda h: h['energy'])
            bound_satisfied = True
        else:
            best = min(non_failed, key=lambda h: h['metric'])
            bound_satisfied = False

        best_coordinate = list(best['coordinate'])
        best_config = self.control_grid.get_config_str(best_coordinate)
        return {
            'best_coordinate': best_coordinate,
            'best_metric': best['energy'],
            'best_config': best_config,
            'best_fom': best['fom'],
            'bound_satisfied': bound_satisfied,
            'n_evaluations': len(result.func_vals),
            'optimization_result': result
        }

    def summarize(self, result: dict, objective_mode: ObjectiveMode) -> str:
        """Build a human-readable summary of an optimization result.

        Args:
            result: The dictionary returned by :meth:`optimize`.
            objective_mode: The objective the run used.

        Returns:
            str: A multi-line summary suitable for printing to the user.
        """
        lines = [f"Best configuration:\n{result['best_config']}"]
        if objective_mode == ObjectiveMode.ENERGY_BOUNDED:
            status = ('satisfied' if result.get('bound_satisfied')
                      else 'NOT satisfied')
            bound = self._metric_bound
            lines.append(
                f"Figure-of-merit bound {bound} was {status} "
                f"(best figure of merit: {result.get('best_fom')})")
            lines.append(f"Energy at best configuration: {result['best_metric']}")
        return '\n'.join(lines)


def _penalty_arg(value):
    """Parse the ``--penalty`` argument into ``'auto'``, ``'none'``, or a float.

    Args:
        value: The raw command-line string.

    Returns:
        The literal ``'auto'`` / ``'none'`` or the parsed float value.

    Raises:
        ArgumentTypeError: If the value is neither keyword nor a valid float.
    """
    if value in ('auto', 'none'):
        return value
    try:
        return float(value)
    except ValueError:
        raise ArgumentTypeError(
            f"--penalty must be 'auto', 'none', or a number, not {value!r}")


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
        '--metric-bound',
        type=float,
        default=None,
        dest='metric_bound',
        help='Minimize energy over the --efficiency domain subject to keeping '
             'the --metric-regex figure of merit at or above this bound (at or '
             'below when --minimize is set); requires --metric-regex and '
             '--efficiency',
    )
    parser.add_argument(
        '--sample-period',
        type=float,
        default=_DEFAULT_SAMPLE_PERIOD,
        dest='sample_period',
        help='geopmsession sampling period in seconds for energy-based '
             'objectives; shorter periods add overhead (default: %(default)s)',
    )
    parser.add_argument(
        '--penalty',
        type=_penalty_arg,
        default='auto',
        help="How to handle a recoverable trial failure (timeout, non-zero "
             "exit, figure-of-merit scrape miss, non-positive power/runtime): "
             "'auto' penalizes the trial with a value worse than every success, "
             "'none' aborts the run on any failure, or a number sets a fixed "
             "penalty (default: %(default)s)",
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

    if args.list_controls:
        print(ControlGrid(['--list-controls']).list_controls_str())
        return 0

    if not args.defer_write:
        pio.save_control()
    try:
        # Remove leading '--' from launch command if present
        launch_command = args.launch
        if launch_command and launch_command[0] == '--':
            launch_command = launch_command[1:]

        if not launch_command:
            raise ValueError("Error: No launch command specified")

        # Create control grid from the --sweep specifications
        if not args.sweep:
            raise ValueError("Error: No control parameters specified (use --sweep)")

        grid_args = []
        for spec in args.sweep:
            grid_args.extend(['--sweep', spec])
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

        objective_mode = resolve_objective_mode(
            args.metric_regex, args.efficiency_domain, args.metric_bound,
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
            objective_mode=objective_mode,
            metric_bound=args.metric_bound,
            penalty=args.penalty
        )

        # Print results
        logger.info("Optimization completed!")
        logger.info(f"Best metric: {result['best_metric']}")
        logger.info(f"Best coordinate: {result['best_coordinate']}")
        logger.info(f"Number of evaluations: {result['n_evaluations']}")

        # Always print the best configuration to stdout for user visibility
        print(optimizer.summarize(result, objective_mode))

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
