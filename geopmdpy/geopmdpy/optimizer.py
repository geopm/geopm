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
from dataclasses import dataclass, field
from typing import Dict, List, Optional

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
from . import metrics
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


@dataclass
class ObjectiveSpec:
    """Canonical description of an optimization objective.

    Produced by :func:`expand_objective` from the command-line inputs and
    consumed by the evaluator and optimizer. The objective is expressed as an
    arithmetic expression over the base per-trial metric names ``fom``,
    ``energy``, ``runtime``/``time``, and ``power``; it is evaluated in *raw*
    (natural, unsigned) units and the optimization direction is applied
    separately via :attr:`minimize`.

    Attributes:
        objective_expr: Arithmetic expression over base metric names giving the
            quantity to optimize (e.g. ``'fom'``, ``'fom / power'``,
            ``'energy'``, ``'runtime'``).
        minimize: Whether the objective is minimized (True) or maximized.
        label: Human-readable name of the objective for summaries.
        needs_session: Whether trials must run under geopmsession to sample
            energy/power signals.
        requires_fom: Whether a figure of merit is scraped from stdout.
        efficiency_domain: Energy domain (``board``/``cpu``/``gpu``) for
            session objectives, or None.
        constraints: Feasibility constraints applied to base metric names.
        penalty_weight: Weight of the constraint-violation penalty folded into
            the scalarized objective.
        metric_map: Named metrics defined by the general ``--metric`` flags,
            keyed by name. Empty for the legacy flag path.
    """
    objective_expr: str
    minimize: bool
    label: str
    needs_session: bool = False
    requires_fom: bool = False
    efficiency_domain: Optional[str] = None
    constraints: List[metrics.Constraint] = field(default_factory=list)
    penalty_weight: float = _DEFAULT_BOUND_PENALTY_WEIGHT
    metric_map: Dict[str, metrics.Metric] = field(default_factory=dict)


def expand_objective(metric_regex, efficiency_domain, metric_bound, minimize,
                     penalty_weight=_DEFAULT_BOUND_PENALTY_WEIGHT):
    """Expand the command-line inputs into a canonical :class:`ObjectiveSpec`.

    This is the single place that interprets the ``--metric-regex`` /
    ``--efficiency`` / ``--metric-bound`` / ``--minimize`` combination; the rest
    of the optimizer is driven purely by the resulting spec and the shared
    ``metrics`` scalarizer.

    Args:
        metric_regex: The ``--metric-regex`` value, or None when omitted.
        efficiency_domain: The ``--efficiency`` domain, or None when omitted.
        metric_bound: The ``--metric-bound`` value, or None when omitted.
        minimize: Whether ``--minimize`` was requested.
        penalty_weight: Weight for the constraint-violation penalty.

    Returns:
        ObjectiveSpec: The canonical objective.

    Raises:
        ValueError: If the combination of inputs is invalid.
    """
    maximize = not minimize
    if metric_bound is not None:
        if metric_regex is None:
            raise ValueError('--metric-bound requires --metric-regex')
        if efficiency_domain is None:
            raise ValueError('--metric-bound requires --efficiency')
        # Maximizing the figure of merit makes the bound a floor; minimizing
        # makes it a ceiling.
        op = '>=' if maximize else '<='
        constraint = metrics.Constraint(name='fom', op=op,
                                        value=float(metric_bound))
        return ObjectiveSpec(
            objective_expr='energy', minimize=True, label='energy',
            needs_session=True, requires_fom=True,
            efficiency_domain=efficiency_domain, constraints=[constraint],
            penalty_weight=penalty_weight)
    if metric_regex is not None:
        if efficiency_domain is not None:
            return ObjectiveSpec(
                objective_expr='fom / power', minimize=minimize,
                label='efficiency', needs_session=True, requires_fom=True,
                efficiency_domain=efficiency_domain)
        return ObjectiveSpec(
            objective_expr='fom', minimize=minimize, label='figure of merit',
            requires_fom=True)
    if efficiency_domain is not None:
        return ObjectiveSpec(
            objective_expr='energy', minimize=True, label='energy',
            needs_session=True, efficiency_domain=efficiency_domain)
    return ObjectiveSpec(objective_expr='runtime', minimize=True,
                         label='runtime')


def build_objective(metric_specs, maximize, minimize,
                    constraint_specs=None,
                    penalty_weight=_DEFAULT_BOUND_PENALTY_WEIGHT):
    """Build a canonical :class:`ObjectiveSpec` from the general metric flags.

    This is the counterpart to :func:`expand_objective` for the general
    ``--metric`` / ``--maximize`` / ``--minimize NAME`` / ``--constraint``
    interface. Metric definitions are parsed and validated with the shared
    ``metrics`` module, a single objective and direction are resolved, and
    constraint values are unit-normalized against the referenced metric.

    Args:
        metric_specs: List of ``NAME=SOURCE`` metric definitions, or None.
        maximize: Name of the metric to maximize, or None.
        minimize: Name of the metric to minimize, or None.
        constraint_specs: List of ``'NAME OP VALUE'`` constraints, or None.
        penalty_weight: Weight for the constraint-violation penalty.

    Returns:
        ObjectiveSpec: The canonical objective, with :attr:`metric_map`
        populated by the parsed metrics.

    Raises:
        ValueError: If no objective, more than one objective, a duplicate
            metric, or a reference to an undefined metric is detected.
        metrics.MetricSpecError: If a metric or constraint spelling is invalid.
    """
    metric_specs = metric_specs or []
    constraint_specs = constraint_specs or []

    # Parse metric definitions into a name -> Metric map.
    metric_map = {}
    for spec_text in metric_specs:
        metric = metrics.parse_metric_spec(spec_text)
        if metric.name in metric_map:
            raise ValueError(
                f"metric '{metric.name}' is defined more than once")
        metric_map[metric.name] = metric

    # Names that may be referenced: user metrics plus reserved/immutable names.
    known_names = (set(metric_map) | set(metrics.RESERVED_UNITS)
                   | set(metrics.IMMUTABLE_METRICS))

    # Validate derived-metric references now that all names are known.
    for metric in metric_map.values():
        for ref in metric.references():
            if ref not in known_names:
                raise ValueError(
                    f"metric '{metric.name}' references undefined metric "
                    f"'{ref}'")

    # Resolve exactly one objective and its direction.
    if maximize is not None and minimize is not None:
        raise ValueError(
            '--maximize and --minimize NAME are mutually exclusive')
    if maximize is not None:
        objective_name, minimize_flag = maximize, False
    elif minimize is not None:
        objective_name, minimize_flag = minimize, True
    else:
        # No objective given: default to minimizing wall-clock runtime.
        objective_name, minimize_flag = 'time', True
    if objective_name not in known_names:
        raise ValueError(
            f"objective metric '{objective_name}' is not defined")

    # Unit table for constraint parsing: reserved canonical units overlaid with
    # the units of any user-defined metrics of the same name.
    metric_units = dict(metrics.RESERVED_UNITS)
    for name, metric in metric_map.items():
        metric_units[name] = metric.unit

    constraints = [metrics.parse_constraint_spec(text, metric_units)
                   for text in constraint_specs]

    # The scalarized objective expression: a derived metric expands to its
    # expression, any other metric evaluates to its own name.
    objective_metric = metric_map.get(objective_name)
    if isinstance(getattr(objective_metric, 'provider', None),
                  metrics.ExprProvider):
        objective_expr = objective_metric.provider.expression
    else:
        objective_expr = objective_name

    needs_session = any(m.needs_session for m in metric_map.values())
    requires_fom = any(isinstance(m.provider, metrics.RegexProvider)
                       for m in metric_map.values())

    return ObjectiveSpec(
        objective_expr=objective_expr,
        minimize=minimize_flag,
        label=objective_name,
        needs_session=needs_session,
        requires_fom=requires_fom,
        constraints=constraints,
        penalty_weight=penalty_weight,
        metric_map=metric_map,
    )


def _objective_fom_regex(spec):
    """Return the regex pattern that supplies the scraped figure of merit.

    The current evaluator scrapes a single figure of merit from stdout. When
    the general ``--metric`` flags define a regex-backed metric, its pattern
    drives that scrape.

    Args:
        spec: The :class:`ObjectiveSpec` produced by :func:`build_objective`.

    Returns:
        The regex pattern string, or None when no regex metric is defined.
    """
    for metric in spec.metric_map.values():
        if isinstance(metric.provider, metrics.RegexProvider):
            return metric.provider.pattern
    return None


#: Human-readable name for each signal behavior enum, used by --list-metrics.
_BEHAVIOR_NAMES = {
    metrics.BEHAVIOR_CONSTANT: 'constant',
    metrics.BEHAVIOR_MONOTONE: 'monotone',
    metrics.BEHAVIOR_VARIABLE: 'variable',
    metrics.BEHAVIOR_LABEL: 'label',
}


def _signal_behavior_aggregation(provider):
    """Resolve the behavior name and default aggregation for a signal metric.

    Args:
        provider: A :class:`metrics.SignalProvider`.

    Returns:
        tuple: ``(behavior_name, aggregation)``. Both are ``'n/a'`` when the
        signal is unavailable on the platform (for example a GPU signal on a
        GPU-less node); the aggregation is ``'n/a'`` when the behavior carries
        no per-trial quantity.
    """
    try:
        behavior = metrics.signal_behavior(provider.signal)
    except Exception:
        return 'n/a', 'n/a'
    behavior_name = _BEHAVIOR_NAMES.get(behavior, 'n/a')
    if provider.aggregation is not None:
        return behavior_name, provider.aggregation
    try:
        aggregation = metrics.default_aggregation(behavior, provider.signal)
    except metrics.MetricSpecError:
        aggregation = 'n/a'
    return behavior_name, aggregation


def list_metrics_str(metric_specs=None):
    """Render the reserved metrics and referenced signals for ``--list-metrics``.

    The listing is the objective-side analogue of ``--list-controls``: it names
    the reserved canonical metrics (with units) that any objective or constraint
    may reference, followed by each ``signal:`` metric defined via ``--metric``
    with its native behavior and default aggregation. Signals that cannot be
    resolved on the platform render as ``n/a`` so a single missing signal does
    not abort the listing.

    Args:
        metric_specs: The list of ``--metric`` ``NAME=SOURCE`` strings, or None.

    Returns:
        str: A multi-line, fixed-width listing suitable for printing.
    """
    lines = [f"{'METRIC':<12}{'UNIT':<6}"]
    for name, unit in metrics.RESERVED_UNITS.items():
        lines.append(f"{name:<12}{unit:<6}")

    signal_metrics = []
    for spec_text in (metric_specs or []):
        try:
            metric = metrics.parse_metric_spec(spec_text)
        except metrics.MetricSpecError:
            continue
        if isinstance(metric.provider, metrics.SignalProvider):
            signal_metrics.append(metric)

    if signal_metrics:
        lines.append("")
        lines.append(f"{'SIGNAL':<24}{'DOMAIN':<10}{'BEHAVIOR':<12}"
                     f"{'AGGREGATION':<12}")
        for metric in signal_metrics:
            provider = metric.provider
            behavior_name, aggregation = _signal_behavior_aggregation(provider)
            lines.append(f"{provider.signal:<24}{provider.domain:<10}"
                         f"{behavior_name:<12}{aggregation:<12}")

    return "\n".join(lines)


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
        metric_values: Values of the general ``--metric`` definitions evaluated
            for this trial, keyed by metric name (empty for the legacy path).
        failed: Whether the trial failed in a recoverable way.
        failure_reason: Human-readable description of a recoverable failure.
    """
    fom: Optional[float] = None
    energy: Optional[float] = None
    runtime: Optional[float] = None
    average_power: Optional[float] = None
    metric_values: Optional[dict] = None
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
        timeout (int): Timeout in seconds for the application's execution.
        print_stdout (bool): Whether to log the application's stdout to the info level.
        efficiency_domain (str): Domain used to measure energy, or None.
        sample_period (float): geopmsession sampling period for session objectives.
        needs_session (bool): Whether trials run under geopmsession for energy.
    """
    def __init__(self, launch_command: List[str], metric_regex: str = None,
                 timeout: int = 300, print_stdout: bool = False,
                 needs_session: bool = False,
                 efficiency_domain: str = None,
                 sample_period: float = _DEFAULT_SAMPLE_PERIOD,
                 metric_map: dict = None):
        """Initialize the application evaluator.

        Args:
            launch_command: Command and arguments to launch the application
            metric_regex: Python-style regex to extract figure of merit from
                stdout. When None, no figure of merit is scraped and only the
                application's wall-clock runtime is measured.
            timeout: Timeout in seconds for application execution
            print_stdout: Whether to log application stdout to info level
            needs_session: Whether trials run under geopmsession so that energy
                and time are sampled with rollover accounting. Set by the
                objective spec from the selected metric kinds.
            efficiency_domain: Domain used to measure energy for session
                objectives (``board``/``cpu``/``gpu``).
            sample_period: geopmsession sampling period in seconds.
            metric_map: General ``--metric`` definitions keyed by name. Any
                ``signal:`` metrics are sampled under geopmsession and their
                values are returned per trial. Empty for the legacy path.
        """
        self.launch_command = launch_command
        self.metric_regex = metric_regex
        self.timeout = timeout
        self.regex = re.compile(self.metric_regex) if self.metric_regex is not None else None
        self.print_stdout = print_stdout
        self.efficiency_domain = efficiency_domain
        self.sample_period = sample_period
        self.needs_session = needs_session
        self.metric_map = metric_map or {}
        # The distinct (signal, domain) pairs any signal: metric requests.
        self._signal_specs = []
        seen = set()
        for metric in self.metric_map.values():
            provider = metric.provider
            if isinstance(provider, metrics.SignalProvider):
                key = (provider.signal, provider.domain)
                if key not in seen:
                    seen.add(key)
                    self._signal_specs.append(key)
        self._energy_signals = []
        if self.needs_session:
            if yaml is None:
                raise OptimizationError(
                    "The pyyaml module is required for energy objectives; "
                    "install with: python3 -m pip install pyyaml")
            if shutil.which('geopmsession') is None:
                raise OptimizationError(
                    "geopmsession was not found on PATH; it is required for "
                    "energy-based objectives (--efficiency)")
            # An efficiency objective samples domain energy; a session that is
            # needed only for user signal: metrics has no efficiency domain.
            if self.efficiency_domain is not None:
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
        """Build the geopmsession signal-config for the trial.

        Always samples ``TIME``; adds the efficiency-domain energy signals when
        an efficiency objective is active, and one line per distinct user
        ``signal:`` metric so each appears under ``report['metrics']``.
        """
        lines = ['TIME board 0']
        seen = {('TIME', 'board')}
        for name in self._energy_signals:
            key = (name, 'board')
            if key not in seen:
                seen.add(key)
                lines.append(f'{name} board 0')
        for signal, domain in self._signal_specs:
            key = (signal, domain)
            if key not in seen:
                seen.add(key)
                lines.append(f'{signal} {domain} 0')
        return '\n'.join(lines) + '\n'

    def _report_runtime(self, report: dict) -> float:
        """Return the run's wall-clock runtime from the session report TIME."""
        report_metrics = report.get('metrics')
        if not report_metrics:
            raise OptimizationError("Session report is missing metrics")
        if 'TIME' not in report_metrics:
            raise OptimizationError("Session report is missing the TIME metric")
        runtime = report_metrics['TIME']['last'] - report_metrics['TIME']['first']
        if runtime <= 0:
            raise RecoverableEvaluationError("Session reported non-positive runtime")
        return runtime

    def _evaluate_metric_map(self, stdout: str, signals: dict) -> dict:
        """Evaluate the general ``--metric`` definitions for one trial.

        Providers are evaluated in dependency order so a derived ``expr:``
        metric sees the metrics it references. Regex and signal providers have
        no metric-map references and are evaluated first; derived metrics follow
        once their referents are available. A missing metric surfaces as a
        recoverable trial failure rather than aborting the run.

        Args:
            stdout: Application standard output (for regex providers).
            signals: Mapping of ``(signal, domain)`` to the report stats block.

        Returns:
            dict: Metric name to evaluated value.
        """
        if not self.metric_map:
            return {}
        context = {'stdout': stdout, 'signals': signals, 'metrics': {}}
        values = context['metrics']
        pending = list(self.metric_map.items())
        made_progress = True
        while pending and made_progress:
            made_progress = False
            still_pending = []
            for name, metric in pending:
                unmet = [r for r in metric.references()
                         if r in self.metric_map and r not in values]
                if unmet:
                    still_pending.append((name, metric))
                    continue
                try:
                    values[name] = metric.evaluate(context)
                except metrics.MetricEvaluationError as ex:
                    raise RecoverableEvaluationError(str(ex))
                made_progress = True
            pending = still_pending
        # Any metric still pending references an unavailable name; evaluate it to
        # surface the precise error as a recoverable failure.
        for name, metric in pending:
            try:
                values[name] = metric.evaluate(context)
            except metrics.MetricEvaluationError as ex:
                raise RecoverableEvaluationError(str(ex))
        return values

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
            if self._energy_signals:
                energy, runtime, average_power = self._compute_energy_metrics(report)
            else:
                runtime = self._report_runtime(report)
                energy = None
                average_power = None

            # Sample any user signal: metrics from the report stats blocks.
            signals_context = {}
            report_metrics = report.get('metrics') or {}
            for signal, domain in self._signal_specs:
                stats = report_metrics.get(signal)
                if stats is None:
                    raise RecoverableEvaluationError(
                        f"session report is missing signal {signal}@{domain}")
                signals_context[(signal, domain)] = stats

            metric_values = self._evaluate_metric_map(result.stdout, signals_context)

            fom = None
            if self.regex is not None:
                if not result.stdout:
                    raise RecoverableEvaluationError("Application produced no output")
                fom = self._extract_metric(result.stdout)

            return TrialResult(fom=fom, energy=energy, runtime=runtime,
                               average_power=average_power,
                               metric_values=metric_values)
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

        # Objective spec and scoring state, (re)initialized in optimize().
        self._spec = None
        self._objective_scale = None

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

    def _trial_values(self, trial: TrialResult) -> dict:
        """Map a trial's raw measurements to canonical base metric names.

        The returned mapping is the evaluation context for the objective
        expression and constraints: ``fom``, ``energy``, ``runtime``/``time``,
        and ``power`` are populated when the corresponding measurement is
        available.

        Args:
            trial: Raw measurements from :meth:`_evaluate_coordinate`.

        Returns:
            dict: Base metric name to measured value.
        """
        values = {}
        if trial.fom is not None:
            values['fom'] = trial.fom
        if trial.energy is not None:
            values['energy'] = trial.energy
        if trial.runtime is not None:
            values['runtime'] = trial.runtime
            values['time'] = trial.runtime
        if trial.average_power is not None:
            values['power'] = trial.average_power
        # General --metric definitions (regex/signal/expr) evaluate to their own
        # names; overlay them so the objective and constraints can reference
        # them. A user metric may intentionally shadow a reserved name.
        if trial.metric_values:
            values.update(trial.metric_values)
        return values

    def _score_trial(self, trial: TrialResult):
        """Scalarize a successful trial through the shared metrics scalarizer.

        The objective expression is evaluated in raw units, the optimization
        direction is applied (negating a maximized objective), and each
        constraint contributes a violation penalty normalized by the running
        objective magnitude so the penalty stays commensurate with objective
        differences.

        Args:
            trial: Raw measurements, with the fields the spec needs populated.

        Returns:
            tuple: ``(score, objective_raw, feasible, total_violation)`` where
            ``score`` is the value to minimize, ``objective_raw`` is the
            objective in natural units, ``feasible`` is True when no constraint
            is violated, and ``total_violation`` is the summed violation
            magnitude.
        """
        spec = self._spec
        values = self._trial_values(trial)
        objective_raw = metrics.safe_eval(spec.objective_expr, values)

        magnitude = abs(objective_raw)
        if self._objective_scale is None:
            self._objective_scale = magnitude
        else:
            self._objective_scale = max(self._objective_scale, magnitude)

        objective = objective_raw if spec.minimize else -objective_raw

        violations = []
        total_violation = 0.0
        for constraint in spec.constraints:
            measured = values[constraint.name]
            amount = constraint.violation(measured)
            if amount > 0.0:
                total_violation += amount
            # Scale the penalty by the observed objective magnitude so it is
            # commensurate with objective differences, and by the constraint's
            # characteristic scale so a fully violated bound is order-one.
            violations.append(metrics.Violation(
                amount=amount,
                scale=self._objective_scale * constraint.scale,
                weight=spec.penalty_weight))

        score = metrics.score(objective, violations)
        return score, objective_raw, total_violation == 0.0, total_violation

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

        successes = [h['score'] for h in self.evaluation_history
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

    def _record_history(self, coordinate: List[int], score: float,
                        objective_raw: float = None,
                        trial: 'TrialResult' = None,
                        feasible: bool = True) -> None:
        """Append a completed trial to the evaluation history and log it.

        Args:
            coordinate: Grid coordinate that was evaluated.
            score: Scalar objective value produced by :meth:`_score_trial`
                (or the penalty for a failed trial).
            objective_raw: The objective in natural units, for reporting.
            trial: Raw measurements, used to record feasibility and the FoM.
            feasible: Whether every constraint was satisfied.
        """
        entry = {
            'coordinate': coordinate.copy(),
            'score': score,
            'config_commands': self.control_grid.get_config_str(coordinate)
        }
        if trial is not None and trial.failed:
            entry['failed'] = True
            entry['failure_reason'] = trial.failure_reason
            # A failed trial can never satisfy a constraint.
            entry['feasible'] = False
        else:
            entry['objective_raw'] = objective_raw
            entry['feasible'] = feasible
            if trial is not None:
                entry['fom'] = trial.fom
        self.evaluation_history.append(entry)

        logger.info(f"Evaluation {len(self.evaluation_history)}: "
                   f"coordinate={coordinate}, score={score}")

    def optimize(self, spec: ObjectiveSpec, trials: int = 50,
                 n_initial_points: int = 10, random_state: int = 42,
                 penalty='auto') -> dict:
        """Run Bayesian optimization for the given objective spec.

        Args:
            spec: The canonical objective produced by :func:`expand_objective`.
            trials: Number of optimization iterations.
            n_initial_points: Number of random initial evaluations.
            random_state: Random seed for reproducibility.
            penalty: Recoverable-failure policy: ``'auto'`` derives a finite
                penalty worse than every success, ``'none'`` re-raises on any
                failure, or a float sets a fixed penalty.

        Returns:
            dict: Optimization results including the best configuration and its
                objective value.
        """
        # (Re)initialize scoring state for this run.
        self._spec = spec
        self._objective_scale = None
        self._penalty = penalty

        @use_named_args(self.space)
        def objective(**params):
            """Objective function for optimization."""
            coordinate = [int(params[dim.name]) for dim in self.space]
            trial = self._evaluate_coordinate(coordinate)
            if trial.failed:
                score = self._handle_failed_trial(trial)
                self._record_history(coordinate, score, trial=trial)
                return score
            score, objective_raw, feasible, _ = self._score_trial(trial)
            self._record_history(coordinate, score, objective_raw=objective_raw,
                                 trial=trial, feasible=feasible)
            return score

        logger.info(f"Starting Bayesian optimization with {trials} evaluations...")

        result = gp_minimize(
            func=objective,
            dimensions=self.space,
            n_calls=trials,
            n_initial_points=n_initial_points,
            random_state=random_state,
            acq_func='EI'  # Expected Improvement
        )

        self.best_result = result

        best = self._select_best_entry(result)
        best_coordinate = list(best['coordinate'])
        best_config = self.control_grid.get_config_str(best_coordinate)

        objective_raw = best.get('objective_raw')
        if objective_raw is None:
            # Empty-history fallback (mocked runs): recover natural units from
            # the signed score.
            objective_raw = (best['score'] if spec.minimize
                             else -best['score'])

        result_dict = {
            'best_coordinate': best_coordinate,
            'best_metric': objective_raw,
            'best_config': best_config,
            'n_evaluations': len(result.func_vals),
            'optimization_result': result,
            'label': spec.label,
        }
        if spec.constraints:
            result_dict['best_fom'] = best.get('fom')
            result_dict['constraints_satisfied'] = best.get('feasible', False)
        return result_dict

    def _select_best_entry(self, result) -> dict:
        """Return the best trial's history entry.

        A fully-feasible successful trial with the lowest score wins; when no
        feasible trial exists the least-penalized successful trial is used.
        Failed trials are never selected, and a run in which every trial failed
        raises a clear error. When the history is empty (mocked runs) an entry
        is synthesized from the scikit-optimize result.

        Args:
            result: The scikit-optimize result from ``gp_minimize``.

        Returns:
            dict: The selected history entry.

        Raises:
            OptimizationError: If every recorded trial failed.
        """
        non_failed = [h for h in self.evaluation_history if not h.get('failed')]
        if self.evaluation_history and not non_failed:
            raise OptimizationError(
                "All trials failed; no successful configuration was found")
        if non_failed:
            feasible = [h for h in non_failed if h.get('feasible')]
            pool = feasible if feasible else non_failed
            return min(pool, key=lambda h: h['score'])
        # Fallback for callers that do not populate history (mocked runs).
        return {
            'coordinate': [int(x) for x in result.x],
            'score': result.fun,
            'objective_raw': None,
            'feasible': True,
            'fom': None,
        }

    def summarize(self, result: dict, spec: ObjectiveSpec) -> str:
        """Build a human-readable summary of an optimization result.

        Args:
            result: The dictionary returned by :meth:`optimize`.
            spec: The objective the run used.

        Returns:
            str: A multi-line summary suitable for printing to the user.
        """
        lines = [f"Best configuration:\n{result['best_config']}"]
        if spec.constraints:
            status = ('satisfied' if result.get('constraints_satisfied')
                      else 'NOT satisfied')
            lines.append(
                f"Constraints were {status} "
                f"(best figure of merit: {result.get('best_fom')})")
            lines.append(
                f"{spec.label} at best configuration: {result['best_metric']}")
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
        nargs='?',
        const=True,
        default=None,
        metavar='NAME',
        help='Without an argument, minimize the legacy --metric-regex figure '
             'of merit (default is to maximize). With a metric NAME, select '
             'that --metric as the objective to minimize (mutually exclusive '
             'with --maximize).'
    )

    parser.add_argument(
        '--metric',
        action='append',
        default=None,
        dest='metric',
        metavar='NAME=SOURCE',
        help="Define a named metric NAME=SOURCE, where SOURCE is one of "
             "regex:'PATTERN', signal:SIGNAL@DOMAIN[:AGG], or "
             "expr:'EXPRESSION'. Repeatable; use with --maximize/--minimize "
             "NAME and --constraint."
    )

    parser.add_argument(
        '--maximize',
        default=None,
        metavar='NAME',
        help='Name of the --metric to maximize as the objective (mutually '
             'exclusive with --minimize NAME).'
    )

    parser.add_argument(
        '--constraint',
        action='append',
        default=None,
        dest='constraint',
        metavar='NAME OP VALUE',
        help="Add a feasibility constraint 'NAME OP VALUE', with OP one of "
             "<=, >=, <, >, ==; VALUE may carry a unit suffix (e.g. 250W, "
             "5000J). Repeatable."
    )

    parser.add_argument(
        '--list-metrics',
        action='store_true',
        dest='list_metrics',
        help='List the reserved canonical metrics and the signals referenced '
             'by --metric (with each signal behavior and default aggregation), '
             'then exit.'
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

    if args.list_metrics:
        print(list_metrics_str(args.metric))
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

        # The general --metric interface and the legacy --metric-regex
        # interface are mutually exclusive on a single invocation.
        minimize_name = args.minimize if isinstance(args.minimize, str) else None
        legacy_minimize = args.minimize is True
        uses_general = bool(args.metric or args.maximize is not None
                            or args.constraint or minimize_name is not None)
        uses_legacy = bool(args.metric_regex is not None
                           or args.efficiency_domain is not None
                           or args.metric_bound is not None
                           or legacy_minimize)
        if uses_general and uses_legacy:
            raise ValueError(
                'the general --metric/--maximize/--minimize NAME/--constraint '
                'flags cannot be combined with the legacy --metric-regex/'
                '--efficiency/--metric-bound/--minimize flags')

        if uses_general:
            spec = build_objective(
                args.metric, args.maximize, minimize_name, args.constraint)
            metric_regex = _objective_fom_regex(spec)
        else:
            spec = expand_objective(
                args.metric_regex, args.efficiency_domain, args.metric_bound,
                legacy_minimize)
            metric_regex = args.metric_regex

        # Create application evaluator
        evaluator = ApplicationEvaluator(
            launch_command=launch_command,
            metric_regex=metric_regex,
            timeout=args.application_timeout,
            print_stdout=args.print_stdout,
            needs_session=spec.needs_session,
            efficiency_domain=spec.efficiency_domain,
            sample_period=args.sample_period,
            metric_map=spec.metric_map
        )
        # Create and run optimizer
        config_file = None
        if args.defer_write:
            config_file = args.output_file
        optimizer = BayesianOptimizer(control_grid, evaluator, config_file)

        result = optimizer.optimize(
            spec=spec,
            trials=args.trials,
            n_initial_points=args.n_initial_points,
            random_state=args.random_seed,
            penalty=args.penalty
        )

        # Print results
        logger.info("Optimization completed!")
        logger.info(f"Best metric: {result['best_metric']}")
        logger.info(f"Best coordinate: {result['best_coordinate']}")
        logger.info(f"Number of evaluations: {result['n_evaluations']}")

        # Always print the best configuration to stdout for user visibility
        print(optimizer.summarize(result, spec))

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
