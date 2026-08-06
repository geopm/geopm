#!/usr/bin/env python3
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import os
import ast
from unittest import TestCase, main

from geopmdpy import metrics
from geopmdpy.metrics import (
    Constraint,
    ExprProvider,
    Metric,
    MetricEvaluationError,
    MetricProvider,
    MetricSpecError,
    RegexProvider,
    ScoredTrial,
    SignalProvider,
    Violation,
    BEHAVIOR_CONSTANT,
    BEHAVIOR_LABEL,
    BEHAVIOR_MONOTONE,
    BEHAVIOR_VARIABLE,
    characteristic_scale,
    default_aggregation,
    parse_constraint_spec,
    parse_metric_spec,
    reduce_series,
    safe_eval,
    score,
    select_best,
)


class TestReservedMetrics(TestCase):
    """A1: reserved registry, name validation, immutable names."""

    def test_reserved_units(self):
        self.assertEqual('s', metrics.RESERVED_UNITS['time'])
        self.assertEqual('J', metrics.RESERVED_UNITS['energy'])
        self.assertEqual('W', metrics.RESERVED_UNITS['power'])
        self.assertEqual('arb', metrics.RESERVED_UNITS['fom'])

    def test_valid_names_accepted(self):
        for name in ('fom', 'power', '_x', 'CamelCase', 'a1_b2'):
            metric = parse_metric_spec(f'{name}=regex:x')
            self.assertEqual(name, metric.name)

    def test_invalid_names_rejected(self):
        for name in ('1abc', 'has-dash', 'has space', 'a.b', ''):
            with self.assertRaises(MetricSpecError):
                parse_metric_spec(f'{name}=regex:x')

    def test_immutable_name_rejected(self):
        with self.assertRaises(MetricSpecError):
            parse_metric_spec('time=regex:x')

    def test_canonical_names_are_definable(self):
        # energy/power/fom are canonical but user-definable (see plan §2).
        metric = parse_metric_spec('power=signal:CPU_POWER@board:mean')
        self.assertEqual('power', metric.name)
        self.assertEqual('W', metric.unit)


class TestMetricDataclass(TestCase):
    """A2: Metric dataclass and provider base."""

    def test_metric_defaults(self):
        provider = RegexProvider('x')
        metric = Metric(name='m', provider=provider)
        self.assertEqual('m', metric.name)
        self.assertEqual('arb', metric.unit)
        self.assertFalse(metric.needs_session)
        self.assertEqual([], metric.references())

    def test_base_provider_evaluate_not_implemented(self):
        with self.assertRaises(NotImplementedError):
            MetricProvider().evaluate({})

    def test_base_provider_references_empty(self):
        self.assertEqual([], MetricProvider().references())


class TestRegexProvider(TestCase):
    """A3: stdout figure-of-merit scraping."""

    def test_capture_group(self):
        provider = RegexProvider(r'GFLOPS: ([0-9.]+)')
        self.assertEqual(42.5, provider.evaluate({'stdout': 'GFLOPS: 42.5\n'}))

    def test_whole_match_when_no_group(self):
        provider = RegexProvider(r'[0-9]+\.[0-9]+')
        self.assertEqual(3.14, provider.evaluate({'stdout': 'pi=3.14'}))

    def test_multiline_stdout(self):
        provider = RegexProvider(r'RESULT=([0-9.]+)')
        stdout = 'warming up\nRESULT=7.0\ndone\n'
        self.assertEqual(7.0, provider.evaluate({'stdout': stdout}))

    def test_no_match_raises_recoverable(self):
        provider = RegexProvider(r'RESULT=([0-9.]+)')
        with self.assertRaises(MetricEvaluationError):
            provider.evaluate({'stdout': 'no match here'})

    def test_empty_stdout_raises_recoverable(self):
        provider = RegexProvider(r'RESULT=([0-9.]+)')
        with self.assertRaises(MetricEvaluationError):
            provider.evaluate({'stdout': ''})

    def test_non_float_capture_raises_recoverable(self):
        provider = RegexProvider(r'RESULT=(\w+)')
        with self.assertRaises(MetricEvaluationError):
            provider.evaluate({'stdout': 'RESULT=fast'})

    def test_invalid_pattern_raises_spec_error(self):
        with self.assertRaises(MetricSpecError):
            RegexProvider(r'(unbalanced')


class TestSignalProvider(TestCase):
    """A4: signal sampling with behavior-derived default aggregation."""

    def test_default_aggregation_monotone(self):
        provider = SignalProvider('CPU_ENERGY', 'board',
                                  behavior=BEHAVIOR_MONOTONE)
        self.assertEqual('delta', provider.resolved_aggregation)

    def test_default_aggregation_variable(self):
        provider = SignalProvider('CPU_POWER', 'board',
                                  behavior=BEHAVIOR_VARIABLE)
        self.assertEqual('mean', provider.resolved_aggregation)

    def test_explicit_aggregation_overrides_behavior(self):
        provider = SignalProvider('CPU_ENERGY', 'board', aggregation='max',
                                  behavior=BEHAVIOR_MONOTONE)
        self.assertEqual('max', provider.resolved_aggregation)

    def test_constant_behavior_rejected(self):
        provider = SignalProvider('CONST', 'board', behavior=BEHAVIOR_CONSTANT)
        with self.assertRaises(MetricSpecError):
            _ = provider.resolved_aggregation

    def test_label_behavior_rejected(self):
        provider = SignalProvider('LABEL', 'board', behavior=BEHAVIOR_LABEL)
        with self.assertRaises(MetricSpecError):
            _ = provider.resolved_aggregation

    def test_unknown_aggregation_rejected(self):
        with self.assertRaises(MetricSpecError):
            SignalProvider('CPU_POWER', 'board', aggregation='p95')

    def test_reductions(self):
        series = [10.0, 12.0, 8.0, 20.0]
        self.assertEqual(10.0, reduce_series('delta', series))
        self.assertEqual(12.5, reduce_series('mean', series))
        self.assertEqual(20.0, reduce_series('max', series))
        self.assertEqual(8.0, reduce_series('min', series))

    def test_reduce_empty_series_recoverable(self):
        with self.assertRaises(MetricEvaluationError):
            reduce_series('mean', [])

    def test_reduce_unknown_aggregation(self):
        with self.assertRaises(MetricSpecError):
            reduce_series('median', [1.0, 2.0])

    def test_evaluate_from_context(self):
        provider = SignalProvider('CPU_ENERGY', 'board',
                                  behavior=BEHAVIOR_MONOTONE)
        context = {'signals': {('CPU_ENERGY', 'board'):
                               {'first': 100.0, 'last': 250.0}}}
        self.assertEqual(150.0, provider.evaluate(context))

    def test_evaluate_reduces_report_stats_block(self):
        """evaluate() maps each aggregation onto a geopmsession report stats
        block (count/first/last/min/max/mean/std), the shape produced by
        ``geopmsession --report``.  This is the Phase 1 signal-sampling path:
        delta is the rollover-corrected ``last - first``; mean/max/min read the
        matching statistic directly.
        """
        stats = {'count': 4, 'first': 100.0, 'last': 260.0,
                 'min': 90.0, 'max': 275.0, 'mean': 180.0, 'std': 12.0}
        signals = {('CPU_ENERGY', 'board'): stats}
        cases = {'delta': 160.0, 'mean': 180.0, 'max': 275.0, 'min': 90.0}
        for aggregation, expected in cases.items():
            provider = SignalProvider('CPU_ENERGY', 'board',
                                      aggregation=aggregation,
                                      behavior=BEHAVIOR_MONOTONE)
            self.assertEqual(
                expected, provider.evaluate({'signals': signals}),
                msg=f'aggregation {aggregation!r} reduced the stats block '
                    f'incorrectly')

    def test_evaluate_missing_statistic_recoverable(self):
        """A stats block missing the field an aggregation needs raises a
        recoverable evaluation error rather than a hard failure."""
        provider = SignalProvider('CPU_ENERGY', 'board', aggregation='mean',
                                  behavior=BEHAVIOR_MONOTONE)
        signals = {('CPU_ENERGY', 'board'): {'first': 1.0, 'last': 2.0}}
        with self.assertRaises(MetricEvaluationError):
            provider.evaluate({'signals': signals})

    def test_evaluate_missing_series_recoverable(self):
        provider = SignalProvider('CPU_ENERGY', 'board',
                                  behavior=BEHAVIOR_MONOTONE)
        with self.assertRaises(MetricEvaluationError):
            provider.evaluate({'signals': {}})

    def test_default_aggregation_helper(self):
        self.assertEqual('delta', default_aggregation(BEHAVIOR_MONOTONE))
        self.assertEqual('mean', default_aggregation(BEHAVIOR_VARIABLE))
        with self.assertRaises(MetricSpecError):
            default_aggregation(BEHAVIOR_CONSTANT)


class TestSafeEval(TestCase):
    """A5: safe AST expression evaluation."""

    def test_simple_division(self):
        self.assertEqual(2.0, safe_eval('energy / fom',
                                        {'energy': 10.0, 'fom': 5.0}))

    def test_nested_expression(self):
        value = safe_eval('(energy + 2) / fom - 1',
                          {'energy': 8.0, 'fom': 5.0})
        self.assertEqual(1.0, value)

    def test_unary_and_power(self):
        self.assertEqual(-8.0, safe_eval('-x ** 3', {'x': 2.0}))

    def test_missing_name_recoverable(self):
        with self.assertRaises(MetricEvaluationError):
            safe_eval('energy / fom', {'energy': 1.0})

    def test_division_by_zero_is_recoverable(self):
        """A runtime divide-by-zero becomes a recoverable evaluation error."""
        with self.assertRaises(MetricEvaluationError) as context:
            safe_eval('energy / fom', {'energy': 1.0, 'fom': 0.0})
        self.assertIn('energy / fom', str(context.exception))

    def test_numeric_overflow_is_recoverable(self):
        """A runtime numeric overflow becomes a recoverable evaluation error."""
        with self.assertRaises(MetricEvaluationError):
            safe_eval('x ** y', {'x': 10.0, 'y': 1000000.0})

    def test_rejects_call(self):
        for expr in ('foo()', '__import__("os")', 'abs(-1)'):
            with self.assertRaises(MetricSpecError):
                safe_eval(expr, {})

    def test_rejects_attribute(self):
        with self.assertRaises(MetricSpecError):
            safe_eval('os.system', {})

    def test_rejects_subscript(self):
        with self.assertRaises(MetricSpecError):
            safe_eval('a[0]', {'a': 1.0})

    def test_rejects_comparison(self):
        with self.assertRaises(MetricSpecError):
            safe_eval('energy < fom', {'energy': 1.0, 'fom': 2.0})

    def test_syntax_error_is_spec_error(self):
        with self.assertRaises(MetricSpecError):
            safe_eval('energy /', {'energy': 1.0})

    def test_expr_provider_validate_unknown_name(self):
        provider = ExprProvider('energy / fom')
        provider.validate({'energy', 'fom'})  # no raise
        with self.assertRaises(MetricSpecError):
            provider.validate({'energy'})

    def test_expr_provider_rejects_call_at_construction(self):
        with self.assertRaises(MetricSpecError):
            ExprProvider('os.system("rm -rf /")')

    def test_expr_provider_references(self):
        provider = ExprProvider('energy / fom + power')
        self.assertEqual({'energy', 'fom', 'power'},
                         set(provider.references()))

    def test_module_has_no_eval_or_exec(self):
        # The module must never call the eval()/exec() builtins. Parse the
        # source and assert no such call exists (docstrings mentioning them by
        # name are fine).
        source_path = os.path.join(os.path.dirname(metrics.__file__),
                                   'metrics.py')
        with open(source_path) as fid:
            tree = ast.parse(fid.read())
        for node in ast.walk(tree):
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
                self.assertNotIn(node.func.id, ('eval', 'exec'))


class TestParseMetricSpec(TestCase):
    """A6: --metric parsing and provider dispatch."""

    def test_regex_provider(self):
        metric = parse_metric_spec('fom=regex:GFLOPS: ([0-9.]+)')
        self.assertIsInstance(metric.provider, RegexProvider)
        self.assertEqual('arb', metric.unit)

    def test_signal_provider_with_aggregation(self):
        metric = parse_metric_spec('power=signal:CPU_POWER@board:mean')
        self.assertIsInstance(metric.provider, SignalProvider)
        self.assertEqual('CPU_POWER', metric.provider.signal)
        self.assertEqual('board', metric.provider.domain)
        self.assertEqual('mean', metric.provider.aggregation)
        self.assertEqual('W', metric.unit)
        self.assertTrue(metric.needs_session)

    def test_signal_provider_without_aggregation(self):
        metric = parse_metric_spec('energy=signal:CPU_ENERGY@board')
        self.assertIsNone(metric.provider.aggregation)
        self.assertEqual('J', metric.unit)

    def test_expr_provider(self):
        metric = parse_metric_spec('eff=expr:fom / power')
        self.assertIsInstance(metric.provider, ExprProvider)

    def test_missing_equals(self):
        with self.assertRaises(MetricSpecError):
            parse_metric_spec('fom regex:x')

    def test_empty_name(self):
        with self.assertRaises(MetricSpecError):
            parse_metric_spec('=regex:x')

    def test_empty_source(self):
        with self.assertRaises(MetricSpecError):
            parse_metric_spec('fom=')

    def test_unknown_provider(self):
        with self.assertRaises(MetricSpecError):
            parse_metric_spec('m=bogus:value')

    def test_signal_missing_domain(self):
        with self.assertRaises(MetricSpecError):
            parse_metric_spec('power=signal:CPU_POWER')


class TestParseConstraintSpec(TestCase):
    """A6: --constraint parsing, operators, and unit suffixes."""

    UNITS = {'power': 'W', 'energy': 'J', 'time': 's', 'fom': 'arb'}

    def test_each_operator(self):
        for op in ('<=', '>=', '<', '>', '=='):
            spec = f'power {op} 250'
            constraint = parse_constraint_spec(spec, self.UNITS)
            self.assertEqual('power', constraint.name)
            self.assertEqual(op, constraint.op)
            self.assertEqual(250.0, constraint.value)

    def test_bare_value_is_canonical(self):
        constraint = parse_constraint_spec('power <= 250', self.UNITS)
        self.assertEqual(250.0, constraint.value)

    def test_power_unit_suffix(self):
        constraint = parse_constraint_spec('power <= 0.25kW', self.UNITS)
        self.assertEqual(250.0, constraint.value)

    def test_energy_unit_suffix(self):
        constraint = parse_constraint_spec('energy <= 5000J', self.UNITS)
        self.assertEqual(5000.0, constraint.value)

    def test_dimensionless_rejects_suffix(self):
        with self.assertRaises(MetricSpecError):
            parse_constraint_spec('fom >= 3W', self.UNITS)

    def test_undefined_metric(self):
        with self.assertRaises(MetricSpecError):
            parse_constraint_spec('ghost <= 1', self.UNITS)

    def test_bad_operator(self):
        with self.assertRaises(MetricSpecError):
            parse_constraint_spec('power ~ 250', self.UNITS)

    def test_missing_value(self):
        with self.assertRaises(MetricSpecError):
            parse_constraint_spec('power <=', self.UNITS)

    def test_constraint_satisfaction_and_violation(self):
        constraint = parse_constraint_spec('power <= 250', self.UNITS)
        self.assertTrue(constraint.satisfied(200.0))
        self.assertFalse(constraint.satisfied(300.0))
        self.assertEqual(0.0, constraint.violation(200.0))
        self.assertEqual(50.0, constraint.violation(300.0))

    def test_lower_bound_violation(self):
        constraint = parse_constraint_spec('fom >= 100', self.UNITS)
        self.assertEqual(0.0, constraint.violation(120.0))
        self.assertEqual(40.0, constraint.violation(60.0))


class TestScalarizer(TestCase):
    """A7: single scalarizer."""

    def test_objective_only(self):
        self.assertEqual(3.0, score(3.0))

    def test_single_violation(self):
        violations = [Violation(amount=50.0, scale=1.0, weight=1.0)]
        self.assertEqual(52.0, score(2.0, violations))

    def test_multiple_heterogeneous_violations(self):
        # Power over by 100 W (bound 250), energy over by 500 J (bound 5000).
        violations = [
            Violation(amount=100.0, scale=characteristic_scale(250.0)),
            Violation(amount=500.0, scale=characteristic_scale(5000.0)),
        ]
        # 0.0 + 100/250 + 500/5000 = 0.4 + 0.1 = 0.5
        self.assertAlmostEqual(0.5, score(0.0, violations))

    def test_weight_scales_penalty(self):
        base = score(0.0, [Violation(amount=10.0, scale=1.0, weight=1.0)])
        heavy = score(0.0, [Violation(amount=10.0, scale=1.0, weight=3.0)])
        self.assertEqual(3 * base, heavy)

    def test_satisfied_violation_contributes_nothing(self):
        # Negative/zero amount must not reduce the score.
        self.assertEqual(5.0, score(5.0, [Violation(amount=-100.0)]))

    def test_failure_penalty(self):
        self.assertEqual(1005.0,
                         score(5.0, failed=True, failure_penalty=1000.0))

    def test_monotonic_in_violation(self):
        small = score(0.0, [Violation(amount=1.0)])
        large = score(0.0, [Violation(amount=2.0)])
        self.assertLess(small, large)

    def test_characteristic_scale(self):
        self.assertEqual(0.5, characteristic_scale(2.0))
        self.assertEqual(1.0, characteristic_scale(0.0))
        self.assertEqual(0.5, characteristic_scale(-2.0))


class TestSelectBest(TestCase):
    """A8: feasibility bookkeeping and best-config selection."""

    def test_empty_returns_none(self):
        self.assertIsNone(select_best([]))

    def test_best_feasible_objective(self):
        trials = [
            ScoredTrial(objective=5.0, violations=[], payload='a'),
            ScoredTrial(objective=3.0, violations=[], payload='b'),
            ScoredTrial(objective=4.0, violations=[], payload='c'),
        ]
        self.assertEqual('b', select_best(trials).payload)

    def test_feasible_preferred_over_better_infeasible(self):
        trials = [
            ScoredTrial(objective=1.0, violations=[10.0], payload='infeasible'),
            ScoredTrial(objective=9.0, violations=[], payload='feasible'),
        ]
        self.assertEqual('feasible', select_best(trials).payload)

    def test_least_infeasible_fallback(self):
        trials = [
            ScoredTrial(objective=1.0, violations=[50.0], payload='far'),
            ScoredTrial(objective=2.0, violations=[5.0], payload='near'),
        ]
        self.assertEqual('near', select_best(trials).payload)

    def test_failed_trials_excluded_when_others_exist(self):
        trials = [
            ScoredTrial(objective=0.0, failed=True, payload='failed'),
            ScoredTrial(objective=7.0, violations=[3.0], payload='infeasible'),
        ]
        self.assertEqual('infeasible', select_best(trials).payload)

    def test_all_failed_returns_min_objective(self):
        trials = [
            ScoredTrial(objective=8.0, failed=True, payload='hi'),
            ScoredTrial(objective=2.0, failed=True, payload='lo'),
        ]
        self.assertEqual('lo', select_best(trials).payload)

    def test_feasible_property(self):
        self.assertTrue(ScoredTrial(objective=1.0).feasible)
        self.assertFalse(ScoredTrial(objective=1.0, violations=[0.1]).feasible)
        self.assertFalse(ScoredTrial(objective=1.0, failed=True).feasible)


if __name__ == '__main__':
    main()
