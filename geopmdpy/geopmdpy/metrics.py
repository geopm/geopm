#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Named metrics, constraints, and scalarization for geopmopt.

This module provides the data model and math that back the generalized
``--metric`` / ``--constraint`` objective CLI. Every optimized quantity -- a
figure of merit scraped from stdout, an energy or power signal, or a derived
expression -- is represented as a :class:`Metric` with a *provider* that knows
how to produce its value from a trial. Constraints fold into the single-scalar
objective through :func:`score`, and best-configuration selection over a set of
scored trials is provided by :func:`select_best`.

The module is intentionally free of any CLI or optimizer dependency so it can be
unit tested in isolation; the optimizer wires it in separately. It never calls
``eval()`` or ``exec()``: ``expr:`` metrics are evaluated with a restricted AST
walker (:func:`safe_eval`) that only permits arithmetic over known metric names.
"""

import ast
import operator
import re
from dataclasses import dataclass, field
from typing import Callable, Dict, List, Optional, Sequence, Tuple

from . import grid
from . import pio

__all__ = [
    'MetricError',
    'MetricSpecError',
    'MetricEvaluationError',
    'AGGREGATIONS',
    'OPERATORS',
    'RESERVED_UNITS',
    'IMMUTABLE_METRICS',
    'BEHAVIOR_CONSTANT',
    'BEHAVIOR_MONOTONE',
    'BEHAVIOR_VARIABLE',
    'BEHAVIOR_LABEL',
    'Metric',
    'MetricProvider',
    'RegexProvider',
    'SignalProvider',
    'ExprProvider',
    'Constraint',
    'Violation',
    'ScoredTrial',
    'signal_behavior',
    'default_aggregation',
    'reduce_series',
    'safe_eval',
    'parse_metric_spec',
    'parse_constraint_spec',
    'characteristic_scale',
    'score',
    'select_best',
]


# --- Exceptions -------------------------------------------------------------

class MetricError(Exception):
    """Base class for metric errors."""


class MetricSpecError(MetricError, ValueError):
    """A metric or constraint specification is malformed or invalid.

    This is a fatal configuration error (raised while parsing the CLI), not a
    per-trial failure.
    """


class MetricEvaluationError(MetricError):
    """A recoverable per-trial failure while evaluating a metric.

    Raised when a metric cannot be produced for a single trial (for example a
    regex miss or an empty signal series). The optimizer converts this into a
    penalized failed trial rather than aborting the run.
    """


# --- Constants --------------------------------------------------------------

#: Supported signal aggregations.
AGGREGATIONS = ('delta', 'mean', 'max', 'min')

#: Sampling domains a ``signal:`` metric may target. These are the coarse
#: domains the optimizer's geopmsession sampler understands; a signal is read at
#: index 0 of the named domain and keyed by signal name in the report.
SIGNAL_DOMAINS = frozenset({'board', 'gpu', 'cpu'})

#: Constraint comparison operators mapped to their boolean implementation.
OPERATORS: Dict[str, Callable[[float, float], bool]] = {
    '<=': operator.le,
    '<': operator.lt,
    '>=': operator.ge,
    '>': operator.gt,
    '==': operator.eq,
}

#: Canonical metric names mapped to their default unit. These names carry a
#: known unit so a constraint spelling is unambiguous; a metric bound to one of
#: these names inherits the unit unless it is dimensionless.
RESERVED_UNITS: Dict[str, str] = {
    'time': 's',
    'energy': 'J',
    'power': 'W',
    'fom': 'arb',
}

#: Metric names that are provided automatically and may not be defined by
#: ``--metric``. ``time`` is always available as the wall-clock/report runtime.
IMMUTABLE_METRICS = frozenset({'time'})

#: Canonical unit token mapped to the grid ``parse_quantity`` category used to
#: validate unit suffixes. ``None`` marks a dimensionless quantity for which a
#: unit suffix is not allowed.
_UNIT_TO_CATEGORY: Dict[str, Optional[str]] = {
    'W': 'power',
    'kW': 'power',
    'J': 'energy',
    's': 'time',
    'Hz': 'frequency',
    'arb': None,
}

# geopm::IOGroup::m_signal_behavior_e enum values (see IOGroup.hpp).
BEHAVIOR_CONSTANT = 0
BEHAVIOR_MONOTONE = 1
BEHAVIOR_VARIABLE = 2
BEHAVIOR_LABEL = 3

#: Matches a valid metric identifier.
_NAME_RE = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')


def _validate_name(name: str) -> str:
    """Return ``name`` if it is a valid metric identifier, else raise."""
    if not _NAME_RE.match(name):
        raise MetricSpecError(
            f"invalid metric name '{name}'; names must match "
            "[A-Za-z_][A-Za-z0-9_]*")
    return name


def _signal_unit(signal_name: str) -> str:
    """Best-effort canonical unit for a signal, inferred from its name.

    GEOPM does not expose a unit string through the Python signal metadata, so
    the common physical quantities are recognized by substring. Anything else
    is treated as dimensionless, which still permits bare (unit-less) constraint
    values.
    """
    upper = signal_name.upper()
    if 'ENERGY' in upper:
        return 'J'
    if 'POWER' in upper:
        return 'W'
    if 'FREQUENCY' in upper:
        return 'Hz'
    if upper == 'TIME' or upper.endswith('::TIME') or upper.endswith('_TIME'):
        return 's'
    return 'arb'


# --- Signal behavior and aggregation ---------------------------------------

def signal_behavior(signal_name: str) -> int:
    """Return the behavior enum for a signal via ``pio.signal_info``.

    Args:
        signal_name: Name of the GEOPM signal to query.

    Returns:
        int: One of :data:`BEHAVIOR_CONSTANT`, :data:`BEHAVIOR_MONOTONE`,
        :data:`BEHAVIOR_VARIABLE`, or :data:`BEHAVIOR_LABEL`.
    """
    return pio.signal_info(signal_name)[2]


def default_aggregation(behavior: int, signal_name: str = '') -> str:
    """Resolve the default aggregation for a signal from its behavior type.

    Monotone signals (counters that only increase) default to ``delta``;
    variable signals default to ``mean``. Constant and label signals carry no
    per-trial quantity and are rejected.

    Args:
        behavior: A signal behavior enum value.
        signal_name: Optional name used only for the error message.

    Returns:
        str: ``'delta'`` or ``'mean'``.

    Raises:
        MetricSpecError: If the behavior is constant or label.
    """
    if behavior == BEHAVIOR_MONOTONE:
        return 'delta'
    if behavior == BEHAVIOR_VARIABLE:
        return 'mean'
    kind = 'constant' if behavior == BEHAVIOR_CONSTANT else 'label'
    where = f" '{signal_name}'" if signal_name else ''
    raise MetricSpecError(
        f"signal{where} has {kind} behavior and is not a valid metric source")


def reduce_series(aggregation: str, series: Sequence[float]) -> float:
    """Reduce a sampled series to a scalar using the named aggregation.

    Args:
        aggregation: One of :data:`AGGREGATIONS`.
        series: The sampled values in time order.

    Returns:
        float: ``last - first`` for ``delta``; the arithmetic mean for
        ``mean``; and the extremum for ``max``/``min``.

    Raises:
        MetricEvaluationError: If the series is empty.
        MetricSpecError: If the aggregation is not recognized.
    """
    if aggregation not in AGGREGATIONS:
        raise MetricSpecError(
            f"unknown aggregation '{aggregation}'; use one of "
            f"{', '.join(AGGREGATIONS)}")
    if not series:
        raise MetricEvaluationError('signal produced no samples')
    if aggregation == 'delta':
        return float(series[-1]) - float(series[0])
    if aggregation == 'mean':
        return sum(float(v) for v in series) / len(series)
    if aggregation == 'max':
        return float(max(series))
    return float(min(series))


# --- Safe expression evaluation --------------------------------------------

_BINARY_OPS = {
    ast.Add: operator.add,
    ast.Sub: operator.sub,
    ast.Mult: operator.mul,
    ast.Div: operator.truediv,
    ast.FloorDiv: operator.floordiv,
    ast.Mod: operator.mod,
    ast.Pow: operator.pow,
}
_UNARY_OPS = {
    ast.UAdd: operator.pos,
    ast.USub: operator.neg,
}


def _expr_names(node: ast.AST) -> List[str]:
    """Validate an expression AST and return the metric names it references.

    Raises:
        MetricSpecError: If the tree contains any node outside the arithmetic
            allow-list (calls, attribute/subscript access, comparisons, etc.).
    """
    names: List[str] = []

    def visit(n: ast.AST) -> None:
        if isinstance(n, ast.Expression):
            visit(n.body)
        elif isinstance(n, ast.BinOp) and type(n.op) in _BINARY_OPS:
            visit(n.left)
            visit(n.right)
        elif isinstance(n, ast.UnaryOp) and type(n.op) in _UNARY_OPS:
            visit(n.operand)
        elif isinstance(n, ast.Name) and isinstance(n.ctx, ast.Load):
            names.append(n.id)
        elif isinstance(n, ast.Constant) and isinstance(n.value, (int, float)) \
                and not isinstance(n.value, bool):
            return
        else:
            raise MetricSpecError(
                'unsupported expression element; only arithmetic over metric '
                'names and numbers is allowed')

    visit(node)
    return names


def safe_eval(expression: str, values: Dict[str, float]) -> float:
    """Evaluate an arithmetic expression over named metric values.

    Only ``+ - * / // % **``, unary ``+``/``-``, parentheses, numeric literals,
    and references to keys of ``values`` are permitted. ``eval()``/``exec()``
    are never used.

    Args:
        expression: The arithmetic expression, e.g. ``'energy / fom'``.
        values: Mapping of metric name to its already-computed value.

    Returns:
        float: The evaluated result.

    Raises:
        MetricSpecError: If the expression cannot be parsed or uses a
            disallowed construct.
        MetricEvaluationError: If a referenced name is missing from ``values``.
    """
    try:
        tree = ast.parse(expression, mode='eval')
    except SyntaxError as ex:
        raise MetricSpecError(f"invalid expression '{expression}': {ex}")
    _expr_names(tree)

    def evaluate(n: ast.AST) -> float:
        if isinstance(n, ast.Expression):
            return evaluate(n.body)
        if isinstance(n, ast.BinOp):
            return _BINARY_OPS[type(n.op)](evaluate(n.left), evaluate(n.right))
        if isinstance(n, ast.UnaryOp):
            return _UNARY_OPS[type(n.op)](evaluate(n.operand))
        if isinstance(n, ast.Name):
            if n.id not in values:
                raise MetricEvaluationError(
                    f"expression references undefined metric '{n.id}'")
            return float(values[n.id])
        # ast.Constant number: guaranteed by _expr_names having passed.
        return float(n.value)

    return float(evaluate(tree))


# --- Providers --------------------------------------------------------------

class MetricProvider:
    """Abstract source of a metric value for a single trial."""

    #: Whether producing this metric requires running the trial under
    #: geopmsession (True for signal sources).
    needs_session = False

    def evaluate(self, context: dict) -> float:
        """Return this metric's value for a trial from ``context``."""
        raise NotImplementedError

    def references(self) -> List[str]:
        """Return metric names this provider depends on (empty by default)."""
        return []


class RegexProvider(MetricProvider):
    """Scrape a float figure of merit from application stdout.

    Uses the first capture group when the pattern has one, otherwise the whole
    match, matching the historical ``--metric-regex`` behavior.
    """

    def __init__(self, pattern: str):
        self.pattern = pattern
        try:
            self.regex = re.compile(pattern)
        except re.error as ex:
            raise MetricSpecError(f"invalid regex '{pattern}': {ex}")

    def evaluate(self, context: dict) -> float:
        stdout = context.get('stdout') or ''
        match = self.regex.search(stdout)
        if not match:
            raise MetricEvaluationError(
                f"could not extract metric using pattern: {self.pattern}")
        text = match.group(1) if match.groups() else match.group(0)
        try:
            return float(text)
        except ValueError:
            raise MetricEvaluationError(
                f"could not convert extracted value to float: {text}")


class SignalProvider(MetricProvider):
    """Sample a GEOPM signal around the trial and reduce it to a scalar.

    The aggregation defaults from the signal's behavior type (monotone ->
    ``delta``, variable -> ``mean``) when not given explicitly.
    """

    needs_session = True

    def __init__(self, signal: str, domain: str, aggregation: Optional[str] = None,
                 behavior: Optional[int] = None):
        self.signal = signal
        self.domain = domain
        if aggregation is not None and aggregation not in AGGREGATIONS:
            raise MetricSpecError(
                f"unknown aggregation '{aggregation}'; use one of "
                f"{', '.join(AGGREGATIONS)}")
        self.aggregation = aggregation
        self.behavior = behavior

    @property
    def resolved_aggregation(self) -> str:
        """The explicit aggregation, or the behavior-derived default."""
        if self.aggregation is not None:
            return self.aggregation
        behavior = self.behavior
        if behavior is None:
            behavior = signal_behavior(self.signal)
        return default_aggregation(behavior, self.signal)

    def evaluate(self, context: dict) -> float:
        signals = context.get('signals') or {}
        key = (self.signal, self.domain)
        stats = signals.get(key)
        if stats is None:
            raise MetricEvaluationError(
                f"no samples for signal {self.signal}@{self.domain}")
        return self._reduce_stats(stats)

    def _reduce_stats(self, stats: dict) -> float:
        """Map the resolved aggregation onto a geopmsession report stats block.

        The report exposes ``count/first/last/min/max/mean/std`` per signal, so
        each supported aggregation is a direct field read: ``delta`` is the
        rollover-corrected ``last - first``, and ``mean``/``max``/``min`` are
        the corresponding statistics. No raw series is retained.
        """
        aggregation = self.resolved_aggregation
        try:
            if aggregation == 'delta':
                return float(stats['last']) - float(stats['first'])
            if aggregation == 'mean':
                return float(stats['mean'])
            if aggregation == 'max':
                return float(stats['max'])
            if aggregation == 'min':
                return float(stats['min'])
        except (KeyError, TypeError):
            raise MetricEvaluationError(
                f"report for signal {self.signal}@{self.domain} is missing the "
                f"statistic needed for aggregation '{aggregation}'")
        raise MetricSpecError(
            f"unknown aggregation '{aggregation}'; use one of "
            f"{', '.join(AGGREGATIONS)}")


class ExprProvider(MetricProvider):
    """A derived metric evaluated from other metrics' values.

    The expression is validated against the arithmetic allow-list at
    construction time (:func:`safe_eval`), so disallowed constructs fail fast.
    """

    def __init__(self, expression: str):
        self.expression = expression
        try:
            tree = ast.parse(expression, mode='eval')
        except SyntaxError as ex:
            raise MetricSpecError(f"invalid expression '{expression}': {ex}")
        self._references = _expr_names(tree)

    @property
    def needs_session(self) -> bool:
        # Session need is determined by the referenced metrics, resolved by the
        # optimizer; the derived metric itself samples nothing.
        return False

    def references(self) -> List[str]:
        return list(self._references)

    def validate(self, known_names) -> None:
        """Raise if any referenced metric is not among ``known_names``."""
        known = set(known_names)
        for name in self._references:
            if name not in known:
                raise MetricSpecError(
                    f"expression '{self.expression}' references undefined "
                    f"metric '{name}'")

    def evaluate(self, context: dict) -> float:
        values = context.get('metrics') or {}
        return safe_eval(self.expression, values)


# --- Metric -----------------------------------------------------------------

@dataclass
class Metric:
    """A named metric: a value source plus its canonical unit."""
    name: str
    provider: MetricProvider
    unit: str = 'arb'

    @property
    def needs_session(self) -> bool:
        return self.provider.needs_session

    def references(self) -> List[str]:
        return self.provider.references()

    def evaluate(self, context: dict) -> float:
        return self.provider.evaluate(context)


def _build_provider(source: str) -> Tuple[MetricProvider, str]:
    """Build a provider from a ``PREFIX:REST`` source, returning it and a unit."""
    if ':' not in source:
        raise MetricSpecError(
            f"invalid metric source '{source}'; expected PREFIX:VALUE "
            "(regex:, signal:, or expr:)")
    prefix, rest = source.split(':', 1)
    prefix = prefix.strip()
    if prefix == 'regex':
        return RegexProvider(rest), 'arb'
    if prefix == 'expr':
        return ExprProvider(rest), 'arb'
    if prefix == 'signal':
        return _build_signal_provider(rest)
    raise MetricSpecError(
        f"unknown metric provider '{prefix}'; use regex:, signal:, or expr:")


def _build_signal_provider(rest: str) -> Tuple[SignalProvider, str]:
    """Parse ``SIGNAL@DOMAIN[:AGG]`` into a :class:`SignalProvider` and unit."""
    if '@' not in rest:
        raise MetricSpecError(
            f"invalid signal source 'signal:{rest}'; expected "
            "signal:SIGNAL@DOMAIN[:AGG]")
    signal, domain_part = rest.split('@', 1)
    signal = signal.strip()
    if not signal:
        raise MetricSpecError('signal source is missing a signal name')
    aggregation = None
    if ':' in domain_part:
        domain, aggregation = domain_part.split(':', 1)
        aggregation = aggregation.strip()
    else:
        domain = domain_part
    domain = domain.strip()
    if not domain:
        raise MetricSpecError(
            f"signal source 'signal:{rest}' is missing a domain")
    return SignalProvider(signal, domain, aggregation or None), _signal_unit(signal)


def parse_metric_spec(spec: str) -> Metric:
    """Parse a ``--metric`` specification ``NAME=SOURCE`` into a :class:`Metric`.

    Args:
        spec: A single ``--metric`` token, e.g. ``power=signal:CPU_POWER@board``
            or ``fom=regex:'GFLOPS: ([0-9.]+)'`` (shell-stripped of quotes).

    Returns:
        Metric: The parsed metric.

    Raises:
        MetricSpecError: If the specification is malformed, names an immutable
            metric, or uses an unknown provider.
    """
    if '=' not in spec:
        raise MetricSpecError(
            f"invalid metric '{spec}'; expected NAME=SOURCE")
    name, source = spec.split('=', 1)
    name = _validate_name(name.strip())
    if name in IMMUTABLE_METRICS:
        raise MetricSpecError(
            f"metric name '{name}' is reserved and provided automatically")
    source = source.strip()
    if not source:
        raise MetricSpecError(f"metric '{name}' is missing a source")
    provider, natural_unit = _build_provider(source)
    if isinstance(provider, SignalProvider) and \
            provider.domain not in SIGNAL_DOMAINS:
        raise MetricSpecError(
            f"metric '{name}' uses unsupported signal domain "
            f"'{provider.domain}'; use one of "
            f"{', '.join(sorted(SIGNAL_DOMAINS))}")
    unit = RESERVED_UNITS.get(name, natural_unit)
    return Metric(name=name, provider=provider, unit=unit)


# --- Constraints ------------------------------------------------------------

# Longest operators first so '<=' is matched before '<'.
_CONSTRAINT_RE = re.compile(
    r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s*(<=|>=|==|<|>)\s*(.+?)\s*$')


@dataclass(frozen=True)
class Constraint:
    """A single feasibility constraint ``name OP value`` in canonical units."""
    name: str
    op: str
    value: float

    def satisfied(self, measured: float) -> bool:
        """Whether ``measured`` satisfies the constraint."""
        return OPERATORS[self.op](measured, self.value)

    def violation(self, measured: float) -> float:
        """Non-negative magnitude by which ``measured`` violates the bound."""
        if self.satisfied(measured):
            return 0.0
        if self.op in ('<=', '<'):
            return measured - self.value
        if self.op in ('>=', '>'):
            return self.value - measured
        return abs(measured - self.value)  # '=='

    @property
    def scale(self) -> float:
        """Characteristic magnitude used to normalize the violation penalty."""
        return characteristic_scale(self.value)


def _parse_constraint_value(text: str, unit: str) -> float:
    """Parse a constraint value, honoring an optional unit suffix.

    A bare number is taken to already be in the metric's canonical unit. A unit
    suffix is validated against the metric's unit category via
    ``grid.parse_quantity``.
    """
    match = grid._QUANTITY_RE.match(text)
    if match is None:
        raise MetricSpecError(f"invalid constraint value '{text}'")
    suffix = match.group(2)
    if not suffix:
        return float(match.group(1))
    category = _UNIT_TO_CATEGORY.get(unit)
    if category is None:
        raise MetricSpecError(
            f"metric is dimensionless; unit suffix '{suffix}' is not allowed")
    try:
        return grid.parse_quantity(text, category)
    except ValueError as ex:
        raise MetricSpecError(str(ex))


def parse_constraint_spec(spec: str, metric_units: Dict[str, str]) -> Constraint:
    """Parse a ``--constraint`` specification ``NAME OP VALUE``.

    Args:
        spec: A single ``--constraint`` token, e.g. ``'power <= 250'`` or
            ``'energy <= 5000J'``.
        metric_units: Mapping of defined metric name to canonical unit, used to
            validate the referenced name and interpret a unit suffix.

    Returns:
        Constraint: The parsed constraint with its value in canonical units.

    Raises:
        MetricSpecError: If the specification is malformed or references an
            undefined metric.
    """
    match = _CONSTRAINT_RE.match(spec)
    if match is None:
        raise MetricSpecError(
            f"invalid constraint '{spec}'; expected 'NAME OP VALUE' with OP "
            "one of <=, >=, <, >, ==")
    name, op, value_text = match.group(1), match.group(2), match.group(3)
    if name not in metric_units:
        raise MetricSpecError(
            f"constraint references undefined metric '{name}'")
    value = _parse_constraint_value(value_text, metric_units[name])
    return Constraint(name=name, op=op, value=value)


# --- Scalarization and selection -------------------------------------------

def characteristic_scale(bound: float) -> float:
    """Return a normalization factor for a constraint bound.

    Using ``1/|bound|`` makes a fully-violated constraint contribute an
    order-one penalty regardless of the metric's units, so heterogeneous
    constraints (Joules vs. Watts vs. seconds) remain commensurable. A zero
    bound falls back to ``1.0``.
    """
    magnitude = abs(bound)
    return 1.0 / magnitude if magnitude else 1.0


@dataclass
class Violation:
    """A constraint violation contribution to the scalarized score."""
    amount: float          # non-negative magnitude of the violation
    scale: float = 1.0     # normalization factor (see characteristic_scale)
    weight: float = 1.0    # per-constraint weight

    def penalty(self) -> float:
        """Return this violation's non-negative penalty contribution."""
        return self.weight * self.scale * max(0.0, self.amount)


def score(objective: float, violations: Sequence[Violation] = (),
          failed: bool = False, failure_penalty: float = 0.0) -> float:
    """Scalarize an objective and its constraint violations.

    The objective is expected in *minimize* sense (callers negate a maximized
    objective). The returned value is::

        objective + sum_i weight_i * scale_i * max(0, violation_i)
                  + (failure_penalty if failed else 0)

    Args:
        objective: The objective value in minimize sense.
        violations: The per-constraint :class:`Violation` contributions.
        failed: Whether the trial failed recoverably.
        failure_penalty: Penalty added when ``failed`` is True.

    Returns:
        float: The scalar score to minimize.
    """
    total = float(objective)
    for violation in violations:
        total += violation.penalty()
    if failed:
        total += failure_penalty
    return total


@dataclass
class ScoredTrial:
    """A trial's objective and per-constraint violations for selection."""
    objective: float                       # minimize sense
    violations: List[float] = field(default_factory=list)  # >= 0 magnitudes
    failed: bool = False
    payload: object = None                 # opaque caller data (e.g. coordinate)

    @property
    def total_violation(self) -> float:
        return sum(v for v in self.violations if v > 0)

    @property
    def feasible(self) -> bool:
        return not self.failed and self.total_violation == 0.0


def select_best(trials: Sequence[ScoredTrial]) -> Optional[ScoredTrial]:
    """Select the best trial: the best feasible objective, else least-infeasible.

    Among fully-feasible, non-failed trials the one with the smallest objective
    (minimize sense) wins. When none are feasible, the non-failed trial with the
    smallest total violation wins (ties broken by objective); if every trial
    failed, the one with the smallest objective is returned.

    Args:
        trials: The scored trials to choose from.

    Returns:
        ScoredTrial or None: The selected trial, or ``None`` if ``trials`` is
        empty.
    """
    if not trials:
        return None
    feasible = [t for t in trials if t.feasible]
    if feasible:
        return min(feasible, key=lambda t: t.objective)
    candidates = [t for t in trials if not t.failed] or list(trials)
    return min(candidates, key=lambda t: (t.total_violation, t.objective))
