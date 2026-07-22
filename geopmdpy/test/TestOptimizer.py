#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
"""
Unit tests for the Bayesian optimizer.
"""

import unittest
from unittest.mock import patch, MagicMock, mock_open
import subprocess
import tempfile
import os
import sys
import re
import math
from argparse import ArgumentTypeError

# Skip test if skopt not available
skip_test = False
skip_msg = 'The skopt module is required to test optimizer'
try:
    import skopt
    from geopmdpy import optimizer
    from geopmdpy import metrics
except ImportError:
    skip_test = True

@unittest.skipIf(skip_test, skip_msg)
class TestApplicationEvaluator(unittest.TestCase):
    def setUp(self):
        self.launch_command = ["echo", "Performance: 123.45"]
        self.metric_regex = "Performance: ([0-9.]+)"
        self.evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, self.metric_regex, timeout=300
        )

    def test_init(self):
        """Test ApplicationEvaluator initialization."""
        self.assertEqual(self.evaluator.launch_command, self.launch_command)
        self.assertEqual(self.evaluator.metric_regex, self.metric_regex)
        self.assertEqual(self.evaluator.timeout, 300)
        self.assertFalse(self.evaluator.print_stdout)

    def test_init_with_print_stdout(self):
        """Test ApplicationEvaluator initialization with print_stdout enabled."""
        evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, self.metric_regex,
            timeout=600, print_stdout=True
        )
        self.assertEqual(evaluator.timeout, 600)
        self.assertTrue(evaluator.print_stdout)

    def test_compile_regex(self):
        """Test regex compilation."""
        # Test direct regex
        evaluator = optimizer.ApplicationEvaluator(
            ["echo", "test"], "Performance: ([0-9.]+)"
        )
        self.assertIsNotNone(evaluator.regex)

        # Test regex with special characters
        evaluator = optimizer.ApplicationEvaluator(
            ["echo", "test"], r"Score:\s+([0-9]*\.?[0-9]+)"
        )
        self.assertIsNotNone(evaluator.regex)

    @patch('subprocess.run')
    def test_evaluate_success(self, mock_run):
        """Test successful evaluation."""
        # Mock subprocess result
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Performance: 123.45 GFLOPS"
        mock_run.return_value = mock_result

        # Mock ControlGrid
        mock_grid = MagicMock()
        mock_grid.write_config = MagicMock()

        coordinate = [2]
        result = self.evaluator.evaluate(mock_grid, coordinate)

        # Raw figure of merit is returned; sign convention is applied at scoring
        self.assertEqual(result.fom, 123.45)
        mock_grid.write_config.assert_called_once_with(coordinate)
        mock_run.assert_called_once_with(
            self.launch_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            timeout=300
        )

    @patch('subprocess.run')
    def test_evaluate_minimization(self, mock_run):
        """Test evaluation for minimization problem."""
        evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, self.metric_regex
        )

        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Performance: 456.78 GFLOPS"
        mock_run.return_value = mock_result

        mock_grid = MagicMock()
        coordinate = [1]
        result = evaluator.evaluate(mock_grid, coordinate)

        # Raw figure of merit is returned regardless of maximize/minimize
        self.assertEqual(result.fom, 456.78)

    @patch('subprocess.run')
    def test_evaluate_with_config_file(self, mock_run):
        """Test evaluation with config file output."""
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Performance: 789.01 GFLOPS"
        mock_run.return_value = mock_result

        mock_grid = MagicMock()
        mock_grid.get_config_str.return_value = "CPU_FREQUENCY_MAX_CONTROL package 0 2000000"

        with patch('builtins.open', mock_open()) as mock_file:
            coordinate = [1]
            config_file = "/tmp/test_config.conf"
            result = self.evaluator.evaluate(mock_grid, coordinate, config_file)

            self.assertEqual(result.fom, 789.01)
            mock_file.assert_called_once_with(config_file, 'w')
            mock_grid.get_config_str.assert_called_once_with(coordinate)

    @patch('subprocess.run')
    def test_evaluate_failed_process(self, mock_run):
        """A non-zero exit is a recoverable failure surfaced as trial data."""
        mock_result = MagicMock()
        mock_result.returncode = 1
        mock_result.stderr = "Error occurred"
        mock_result.stdout = "Some output"
        mock_run.return_value = mock_result

        mock_grid = MagicMock()
        coordinate = [0]

        result = self.evaluator.evaluate(mock_grid, coordinate)

        self.assertTrue(result.failed)
        self.assertIn("Application failed", result.failure_reason)

    @patch('subprocess.run')
    def test_evaluate_timeout(self, mock_run):
        """A timeout is a recoverable failure surfaced as trial data."""
        mock_run.side_effect = subprocess.TimeoutExpired("cmd", 300)

        mock_grid = MagicMock()
        coordinate = [0]

        result = self.evaluator.evaluate(mock_grid, coordinate)

        self.assertTrue(result.failed)
        self.assertIn("timed out after 300 seconds", result.failure_reason)

    @patch('subprocess.run')
    def test_evaluate_custom_timeout(self, mock_run):
        """A custom timeout is reported in the recoverable failure reason."""
        evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, self.metric_regex, timeout=600
        )

        mock_run.side_effect = subprocess.TimeoutExpired("cmd", 600)
        mock_grid = MagicMock()

        result = evaluator.evaluate(mock_grid, [0])

        self.assertTrue(result.failed)
        self.assertIn("timed out after 600 seconds", result.failure_reason)

    @patch('subprocess.run')
    def test_evaluate_no_output(self, mock_run):
        """Missing output when a metric is expected is a recoverable failure."""
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = ""
        mock_run.return_value = mock_result

        mock_grid = MagicMock()

        result = self.evaluator.evaluate(mock_grid, [0])

        self.assertTrue(result.failed)
        self.assertIn("produced no output", result.failure_reason)

    @patch('subprocess.run')
    def test_evaluate_no_regex_returns_runtime(self, mock_run):
        """Without a metric regex, evaluate measures runtime and skips the FoM."""
        evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, metric_regex=None
        )
        self.assertIsNone(evaluator.regex)

        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = ""  # No output is fine when no metric is scraped
        mock_run.return_value = mock_result

        mock_grid = MagicMock()
        result = evaluator.evaluate(mock_grid, [0])

        self.assertIsNone(result.fom)
        self.assertIsNotNone(result.runtime)
        self.assertGreaterEqual(result.runtime, 0.0)

    @patch('subprocess.run')
    def test_evaluate_no_regex_failed_process(self, mock_run):
        """A non-zero exit is recoverable even without a metric regex."""
        evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, metric_regex=None
        )
        mock_result = MagicMock()
        mock_result.returncode = 1
        mock_result.stderr = "boom"
        mock_result.stdout = ""
        mock_run.return_value = mock_result

        mock_grid = MagicMock()
        result = evaluator.evaluate(mock_grid, [0])
        self.assertTrue(result.failed)
        self.assertIn("Application failed", result.failure_reason)

    @patch('subprocess.run')
    def test_evaluate_command_not_found_is_fatal(self, mock_run):
        """A missing command is a fatal error, not a recoverable failure."""
        mock_run.side_effect = FileNotFoundError("no such file")
        mock_grid = MagicMock()
        with self.assertRaises(optimizer.OptimizationError) as context:
            self.evaluator.evaluate(mock_grid, [0])
        self.assertIn("Command not found", str(context.exception))

    @patch('subprocess.run')
    def test_evaluate_permission_denied_is_fatal(self, mock_run):
        """A permission error is a fatal error, not a recoverable failure."""
        mock_run.side_effect = PermissionError("denied")
        mock_grid = MagicMock()
        with self.assertRaises(optimizer.OptimizationError) as context:
            self.evaluator.evaluate(mock_grid, [0])
        self.assertIn("Permission denied", str(context.exception))

    def test_extract_metric_success(self):
        """Test successful metric extraction."""
        stdout = "Application output\nPerformance: 456.78 GFLOPS\nMore output"
        metric = self.evaluator._extract_metric(stdout)
        self.assertEqual(metric, 456.78)

    def test_extract_metric_no_capture_group(self):
        """Test metric extraction without capture group."""
        evaluator = optimizer.ApplicationEvaluator(
            ["echo", "test"], r"[0-9]+\.[0-9]+"
        )
        stdout = "Score is 123.45 points"
        metric = evaluator._extract_metric(stdout)
        self.assertEqual(metric, 123.45)

    def test_extract_metric_no_match(self):
        """A regex miss is a recoverable failure."""
        stdout = "Application output without metric"

        with self.assertRaises(optimizer.RecoverableEvaluationError) as context:
            self.evaluator._extract_metric(stdout)

        self.assertIn("Could not extract metric", str(context.exception))

    def test_extract_metric_invalid_value(self):
        """An unparsable matched value is a recoverable failure."""
        evaluator = optimizer.ApplicationEvaluator(
            ["echo", "test"], "Performance: ([a-z]+)"
        )
        stdout = "Performance: invalid"

        with self.assertRaises(optimizer.RecoverableEvaluationError) as context:
            evaluator._extract_metric(stdout)

        self.assertIn("Could not convert", str(context.exception))


@unittest.skipIf(skip_test, skip_msg)
class TestGetEnergy(unittest.TestCase):
    @patch('geopmdpy.optimizer.pio')
    def test_get_energy_board_with_board_signal(self, mock_pio):
        """Test get_energy for board domain with BOARD_ENERGY signal."""
        mock_pio.signal_names.return_value = ["BOARD_ENERGY", "CPU_ENERGY"]
        mock_pio.read_signal.return_value = 1000.0

        result = optimizer.get_energy('board')
        self.assertEqual(result, 1000.0)
        mock_pio.read_signal.assert_called_once_with("BOARD_ENERGY", 0, 0)

    @patch('geopmdpy.optimizer.pio')
    def test_get_energy_board_without_board_signal(self, mock_pio):
        """Test get_energy for board domain without BOARD_ENERGY signal."""
        mock_pio.signal_names.return_value = ["CPU_ENERGY", "GPU_ENERGY", "DRAM_ENERGY"]
        mock_pio.read_signal.side_effect = lambda signal, d1, d2: {
            "CPU_ENERGY": 500.0,
            "GPU_ENERGY": 300.0,
            "DRAM_ENERGY": 200.0
        }[signal]

        result = optimizer.get_energy('board')
        self.assertEqual(result, 1000.0)  # 500 + 300 + 200

    @patch('geopmdpy.optimizer.pio')
    def test_get_energy_cpu(self, mock_pio):
        """Test get_energy for cpu domain."""
        mock_pio.signal_names.return_value = ["CPU_ENERGY", "GPU_ENERGY"]
        mock_pio.read_signal.return_value = 750.0

        result = optimizer.get_energy('cpu')
        self.assertEqual(result, 750.0)
        mock_pio.read_signal.assert_called_once_with("CPU_ENERGY", 0, 0)

    @patch('geopmdpy.optimizer.pio')
    def test_get_energy_gpu(self, mock_pio):
        """Test get_energy for gpu domain."""
        mock_pio.signal_names.return_value = ["CPU_ENERGY", "GPU_ENERGY"]
        mock_pio.read_signal.return_value = 400.0

        result = optimizer.get_energy('gpu')
        self.assertEqual(result, 400.0)
        mock_pio.read_signal.assert_called_once_with("GPU_ENERGY", 0, 0)

    def test_get_energy_invalid_domain(self):
        """Test get_energy with invalid domain."""
        with self.assertRaises(ValueError) as context:
            optimizer.get_energy('invalid')
        self.assertIn('Unsupported domain invalid', str(context.exception))

    @patch('geopmdpy.optimizer.pio')
    def test_get_energy_no_signals(self, mock_pio):
        """Test get_energy when no energy signals are available."""
        mock_pio.signal_names.return_value = ["OTHER_SIGNAL"]

        with self.assertRaises(optimizer.OptimizationError) as context:
            optimizer.get_energy('cpu')
        self.assertIn("No energy signals available", str(context.exception))


@unittest.skipIf(skip_test, skip_msg)
class TestTrialResult(unittest.TestCase):
    def test_defaults(self):
        """A TrialResult defaults to empty measurements and not-failed."""
        trial = optimizer.TrialResult()
        self.assertIsNone(trial.fom)
        self.assertIsNone(trial.energy)
        self.assertIsNone(trial.runtime)
        self.assertIsNone(trial.average_power)
        self.assertFalse(trial.failed)
        self.assertIsNone(trial.failure_reason)

    def test_fields(self):
        """Fields are populated positionally/by keyword as expected."""
        trial = optimizer.TrialResult(fom=1.0, energy=2.0, runtime=4.0,
                                      average_power=0.5)
        self.assertEqual(trial.fom, 1.0)
        self.assertEqual(trial.energy, 2.0)
        self.assertEqual(trial.runtime, 4.0)
        self.assertEqual(trial.average_power, 0.5)


@unittest.skipIf(skip_test, skip_msg)
class TestBayesianOptimizer(unittest.TestCase):
    def setUp(self):
        # Mock the control grid without using spec
        self.mock_grid = MagicMock()
        self.mock_grid.control_name = ['cpu_frequency']
        self.mock_grid.get_grid_data.return_value = [
            {
                "control": "CPU_FREQUENCY_MAX_CONTROL",
                "domain": "package",
                "domain_idx": 0,
                "settings": [1000000, 1500000, 2000000, 2500000, 3000000]
            }
        ]
        self.mock_grid.get_config_str.return_value = "CPU_FREQUENCY_MAX_CONTROL package 0 2000000"

        # Mock the evaluator
        self.mock_evaluator = MagicMock()

    def test_init_with_skopt(self):
        """Test initialization when scikit-optimize is available."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        self.assertEqual(opt.control_grid, self.mock_grid)
        self.assertEqual(opt.evaluator, self.mock_evaluator)
        self.assertEqual(len(opt.space), 1)
        self.assertEqual(len(opt.evaluation_history), 0)
        self.assertIsNone(opt.config_file)

    def test_init_with_config_file(self):
        """Test initialization with config file."""
        config_file = "/tmp/test.conf"
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator, config_file)
        self.assertEqual(opt.config_file, config_file)

    def test_create_search_space(self):
        """Test search space creation."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        space = opt._create_search_space()

        self.assertEqual(len(space), 1)
        # Check that space was created correctly
        self.mock_grid.get_grid_data.assert_called()

    def test_create_search_space_validation_errors(self):
        """Test search space creation with validation errors."""
        # Test empty settings
        self.mock_grid.get_grid_data.return_value = [
            {
                "control": "CPU_FREQUENCY_MAX_CONTROL",
                "domain": "package",
                "domain_idx": 0,
                "settings": []
            }
        ]

        with self.assertRaises(ValueError) as context:
            optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)

        self.assertIn("No settings available", str(context.exception))

        # Test single setting
        self.mock_grid.get_grid_data.return_value = [
            {
                "control": "CPU_FREQUENCY_MAX_CONTROL",
                "domain": "package",
                "domain_idx": 0,
                "settings": [2000000]
            }
        ]

        with self.assertRaises(ValueError) as context:
            optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)

        self.assertIn("Only one setting available", str(context.exception))

    def test_create_search_space_dimension_mismatch(self):
        """Test search space creation with dimension mismatch."""
        self.mock_grid.control_name = ['cpu_frequency', 'cpu_power']  # 2 controls
        self.mock_grid.get_grid_data.return_value = [  # But only 1 grid data entry
            {
                "control": "CPU_FREQUENCY_MAX_CONTROL",
                "domain": "package",
                "domain_idx": 0,
                "settings": [1000000, 2000000]
            }
        ]

        with self.assertRaises(ValueError) as context:
            optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)

        self.assertIn("Grid data dimensions", str(context.exception))

    def _opt_with_spec(self, spec):
        """Return a BayesianOptimizer primed with a scoring spec."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._spec = spec
        opt._objective_scale = None
        return opt

    def test_score_trial_raw_metric_maximize(self):
        """Maximizing the figure of merit minimizes its negation."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', None, None, False)
        opt = self._opt_with_spec(spec)
        trial = optimizer.TrialResult(fom=123.45)
        score, raw, feasible, violation = opt._score_trial(trial)
        self.assertEqual(score, -123.45)
        self.assertEqual(raw, 123.45)
        self.assertTrue(feasible)
        self.assertEqual(violation, 0.0)

    def test_score_trial_raw_metric_minimize(self):
        """Minimizing the figure of merit keeps its sign."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', None, None, True)
        opt = self._opt_with_spec(spec)
        trial = optimizer.TrialResult(fom=123.45)
        score, raw, _, _ = opt._score_trial(trial)
        self.assertEqual(score, 123.45)
        self.assertEqual(raw, 123.45)

    def test_score_trial_efficiency_maximize(self):
        """Maximizing efficiency minimizes the negated FoM per watt."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', None, False)
        opt = self._opt_with_spec(spec)
        trial = optimizer.TrialResult(fom=1000.0, average_power=100.0)
        score, raw, _, _ = opt._score_trial(trial)
        self.assertEqual(score, -10.0)
        self.assertEqual(raw, 10.0)

    def test_score_trial_efficiency_minimize(self):
        """Minimizing efficiency keeps the FoM-per-watt sign."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', None, True)
        opt = self._opt_with_spec(spec)
        trial = optimizer.TrialResult(fom=50.0, average_power=5.0)
        score, raw, _, _ = opt._score_trial(trial)
        self.assertEqual(score, 10.0)
        self.assertEqual(raw, 10.0)

    def test_score_trial_runtime(self):
        """The runtime objective minimizes wall-clock time directly."""
        spec = optimizer.expand_objective(None, None, None, False)
        opt = self._opt_with_spec(spec)
        trial = optimizer.TrialResult(runtime=12.5)
        score, raw, _, _ = opt._score_trial(trial)
        self.assertEqual(score, 12.5)
        self.assertEqual(raw, 12.5)

    def test_score_trial_energy(self):
        """The energy objective minimizes measured energy directly."""
        spec = optimizer.expand_objective(None, 'cpu', None, False)
        opt = self._opt_with_spec(spec)
        trial = optimizer.TrialResult(energy=987.0, runtime=1.0,
                                      average_power=987.0)
        score, raw, _, _ = opt._score_trial(trial)
        self.assertEqual(score, 987.0)
        self.assertEqual(raw, 987.0)

    def test_score_trial_energy_bounded_feasible(self):
        """A feasible energy-bounded trial scores its plain energy."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', 100.0, False)
        opt = self._opt_with_spec(spec)
        # maximize form: feasible when fom >= bound
        trial = optimizer.TrialResult(fom=120.0, energy=500.0, runtime=1.0,
                                      average_power=500.0)
        score, raw, feasible, violation = opt._score_trial(trial)
        self.assertEqual(score, 500.0)
        self.assertEqual(raw, 500.0)
        self.assertTrue(feasible)
        self.assertEqual(violation, 0.0)

    def test_score_trial_energy_bounded_infeasible(self):
        """An infeasible energy-bounded trial scores strictly above its energy."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', 100.0, False)
        opt = self._opt_with_spec(spec)
        # deficit = 100 - 50 = 50; scale = |energy|(500) * (1/100) = 5;
        # penalty = weight(1) * 5 * 50 = 250; score = 500 + 250 = 750
        trial = optimizer.TrialResult(fom=50.0, energy=500.0, runtime=1.0,
                                      average_power=500.0)
        score, _, feasible, violation = opt._score_trial(trial)
        self.assertGreater(score, 500.0)
        self.assertEqual(score, 750.0)
        self.assertFalse(feasible)
        self.assertEqual(violation, 50.0)

    def test_score_trial_energy_bounded_minimize(self):
        """When minimizing the FoM the bound is a ceiling."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', 100.0, True)
        opt = self._opt_with_spec(spec)
        # feasible when fom <= bound
        feasible = optimizer.TrialResult(fom=80.0, energy=500.0, runtime=1.0,
                                         average_power=500.0)
        score, _, is_feasible, _ = opt._score_trial(feasible)
        self.assertEqual(score, 500.0)
        self.assertTrue(is_feasible)
        # infeasible when fom > bound: excess = 150 - 100 = 50
        opt2 = self._opt_with_spec(spec)
        infeasible = optimizer.TrialResult(fom=150.0, energy=500.0, runtime=1.0,
                                           average_power=500.0)
        score2, _, is_feasible2, _ = opt2._score_trial(infeasible)
        self.assertEqual(score2, 750.0)
        self.assertFalse(is_feasible2)

    def test_score_trial_missing_value_raises(self):
        """A metric that the trial did not measure raises an evaluation error."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', None, False)
        opt = self._opt_with_spec(spec)
        # No average_power measured, so 'power' is unavailable.
        trial = optimizer.TrialResult(fom=1.0)
        with self.assertRaises(metrics.MetricEvaluationError):
            opt._score_trial(trial)

    def test_select_best_entry_prefers_feasible(self):
        """The best feasible point wins even if an infeasible score is lower."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt.evaluation_history = [
            {'coordinate': [0], 'score': 100.0, 'objective_raw': 100.0,
             'fom': 50.0, 'feasible': False},
            {'coordinate': [1], 'score': 300.0, 'objective_raw': 300.0,
             'fom': 120.0, 'feasible': True},
            {'coordinate': [2], 'score': 250.0, 'objective_raw': 250.0,
             'fom': 110.0, 'feasible': True},
        ]
        mock_result = MagicMock()
        best = opt._select_best_entry(mock_result)
        self.assertEqual(best['coordinate'], [2])
        self.assertEqual(best['objective_raw'], 250.0)
        self.assertEqual(best['fom'], 110.0)

    def test_select_best_entry_all_infeasible(self):
        """With no feasible point the least-penalized trial is reported."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt.evaluation_history = [
            {'coordinate': [0], 'score': 900.0, 'objective_raw': 400.0,
             'fom': 50.0, 'feasible': False},
            {'coordinate': [1], 'score': 600.0, 'objective_raw': 500.0,
             'fom': 80.0, 'feasible': False},
        ]
        mock_result = MagicMock()
        best = opt._select_best_entry(mock_result)
        self.assertEqual(best['coordinate'], [1])
        self.assertEqual(best['fom'], 80.0)

    def test_summarize_constraint_satisfied(self):
        """The summary reports constraint feasibility and the best figure of merit."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', 100.0, False)
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        result = {
            'best_config': 'CONFIG', 'best_metric': 250.0,
            'best_fom': 110.0, 'constraints_satisfied': True,
        }
        summary = opt.summarize(result, spec)
        self.assertIn('satisfied', summary)
        self.assertIn('110.0', summary)
        self.assertNotIn('NOT satisfied', summary)

    def test_summarize_constraint_violated(self):
        """The summary flags a violated constraint."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', 100.0, False)
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        result = {
            'best_config': 'CONFIG', 'best_metric': 250.0,
            'best_fom': 80.0, 'constraints_satisfied': False,
        }
        summary = opt.summarize(result, spec)
        self.assertIn('NOT satisfied', summary)

    def test_summarize_unconstrained(self):
        """Unconstrained objectives summarize just the best configuration."""
        spec = optimizer.expand_objective(None, None, None, False)
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        result = {'best_config': 'CONFIG', 'best_metric': 1.0}
        summary = opt.summarize(result, spec)
        self.assertEqual(summary, 'Best configuration:\nCONFIG')

    def test_penalty_value_auto_worse_than_successes(self):
        """The auto penalty is strictly worse than every observed success."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._penalty = 'auto'
        opt.evaluation_history = [
            {'coordinate': [0], 'score': 10.0},
            {'coordinate': [1], 'score': 30.0},
            {'coordinate': [2], 'score': 20.0},
        ]
        penalty = opt._penalty_value()
        # worst = 30, spread = 20, penalty = 50
        self.assertEqual(penalty, 50.0)
        self.assertGreater(penalty, 30.0)
        self.assertTrue(math.isfinite(penalty))

    def test_penalty_value_auto_tie(self):
        """When all successes tie, the auto penalty still exceeds them."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._penalty = 'auto'
        opt.evaluation_history = [
            {'coordinate': [0], 'score': 10.0},
            {'coordinate': [1], 'score': 10.0},
        ]
        penalty = opt._penalty_value()
        self.assertGreater(penalty, 10.0)
        self.assertTrue(math.isfinite(penalty))

    def test_penalty_value_auto_excludes_failed(self):
        """Prior failed trials do not inflate the auto penalty."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._penalty = 'auto'
        opt.evaluation_history = [
            {'coordinate': [0], 'score': 10.0},
            {'coordinate': [1], 'score': 999.0, 'failed': True},
        ]
        # Only the success (10.0) is considered: worst=10, spread=0 -> +10 -> 20
        self.assertEqual(opt._penalty_value(), 20.0)

    def test_penalty_value_auto_bootstrap_warns(self):
        """A failure before any success uses a finite bootstrap penalty."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._penalty = 'auto'
        opt.evaluation_history = []
        with self.assertLogs('geopmdpy.optimizer', level='WARNING'):
            penalty = opt._penalty_value()
        self.assertEqual(penalty, float(optimizer._BOOTSTRAP_PENALTY))
        self.assertTrue(math.isfinite(penalty))

    def test_penalty_value_fixed(self):
        """A fixed numeric penalty policy returns that value verbatim."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._penalty = 123.5
        opt.evaluation_history = [{'coordinate': [0], 'score': 10.0}]
        self.assertEqual(opt._penalty_value(), 123.5)

    def test_handle_failed_trial_none_raises(self):
        """The 'none' policy re-raises, aborting the run on any failure."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._penalty = 'none'
        trial = optimizer.TrialResult(failed=True, failure_reason='boom')
        with self.assertRaises(optimizer.OptimizationError) as context:
            opt._handle_failed_trial(trial)
        self.assertIn('boom', str(context.exception))

    def test_handle_failed_trial_auto_warns(self):
        """The 'auto' policy penalizes and logs the failure at WARNING."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._penalty = 'auto'
        opt.evaluation_history = [{'coordinate': [0], 'score': 10.0}]
        trial = optimizer.TrialResult(failed=True, failure_reason='boom')
        with self.assertLogs('geopmdpy.optimizer', level='WARNING'):
            penalty = opt._handle_failed_trial(trial)
        self.assertEqual(penalty, 20.0)

    def test_handle_failed_trial_fixed(self):
        """A fixed penalty policy returns the configured value."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._penalty = 5.0
        trial = optimizer.TrialResult(failed=True, failure_reason='boom')
        with self.assertLogs('geopmdpy.optimizer', level='WARNING'):
            self.assertEqual(opt._handle_failed_trial(trial), 5.0)

    def test_record_history_failed_trial(self):
        """A failed trial is marked failed and infeasible."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        trial = optimizer.TrialResult(failed=True, failure_reason='boom')
        opt._record_history([0], 999.0, trial=trial)
        entry = opt.evaluation_history[0]
        self.assertTrue(entry['failed'])
        self.assertEqual(entry['failure_reason'], 'boom')
        self.assertFalse(entry['feasible'])

    def test_select_best_entry_excludes_failed(self):
        """Best selection ignores penalized failed trials."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt.evaluation_history = [
            {'coordinate': [0], 'score': 10.0, 'feasible': True},
            {'coordinate': [1], 'score': 5.0, 'failed': True},
            {'coordinate': [2], 'score': 20.0, 'feasible': True},
        ]
        best = opt._select_best_entry(MagicMock())
        self.assertEqual(best['coordinate'], [0])
        self.assertEqual(best['score'], 10.0)

    def test_select_best_entry_all_failed_raises(self):
        """A run where every trial failed raises a clear summary error."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt.evaluation_history = [
            {'coordinate': [0], 'score': 999.0, 'failed': True},
            {'coordinate': [1], 'score': 999.0, 'failed': True},
        ]
        with self.assertRaises(optimizer.OptimizationError) as context:
            opt._select_best_entry(MagicMock())
        self.assertIn('All trials failed', str(context.exception))

    def test_record_history(self):
        """History records the coordinate, score, and config commands."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._record_history([1], 42.0)
        self.assertEqual(len(opt.evaluation_history), 1)
        self.assertEqual(opt.evaluation_history[0]['coordinate'], [1])
        self.assertEqual(opt.evaluation_history[0]['score'], 42.0)

    def test_record_history_success_feasibility(self):
        """Successful history entries record feasibility, fom, and objective."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        feasible = optimizer.TrialResult(fom=120.0, energy=500.0)
        opt._record_history([0], 500.0, objective_raw=500.0, trial=feasible,
                            feasible=True)
        infeasible = optimizer.TrialResult(fom=50.0, energy=400.0)
        opt._record_history([1], 700.0, objective_raw=400.0, trial=infeasible,
                            feasible=False)
        self.assertTrue(opt.evaluation_history[0]['feasible'])
        self.assertEqual(opt.evaluation_history[0]['fom'], 120.0)
        self.assertEqual(opt.evaluation_history[0]['objective_raw'], 500.0)
        self.assertFalse(opt.evaluation_history[1]['feasible'])

    def test_evaluate_coordinate_delegates(self):
        """_evaluate_coordinate validates and returns the evaluator's result."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        self.mock_evaluator.evaluate.return_value = optimizer.TrialResult(fom=7.0)
        trial = opt._evaluate_coordinate([1])
        self.assertEqual(trial.fom, 7.0)
        self.mock_evaluator.evaluate.assert_called_once()

    @patch('geopmdpy.optimizer.gp_minimize')
    def test_optimize_basic(self, mock_gp_minimize):
        """Test basic optimization run."""
        # Mock optimization result
        mock_result = MagicMock()
        mock_result.x = [2]
        mock_result.fun = -123.45
        mock_result.func_vals = [-100, -110, -123.45, -120]
        mock_gp_minimize.return_value = mock_result

        # Mock evaluator to return some values
        self.mock_evaluator.evaluate.side_effect = [-100, -110, -123.45, -120]

        spec = optimizer.expand_objective('FOM: ([0-9.]+)', None, None, False)
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        result = opt.optimize(spec=spec, trials=4, n_initial_points=2,
                              random_state=42)

        # Check results
        self.assertEqual(result['best_coordinate'], [2])
        self.assertEqual(result['best_metric'], 123.45)  # Positive for maximization
        self.assertEqual(result['n_evaluations'], 4)
        self.assertIn('best_config', result)

        # Verify gp_minimize was called correctly
        mock_gp_minimize.assert_called_once()
        call_args = mock_gp_minimize.call_args
        self.assertEqual(call_args[1]['n_calls'], 4)
        self.assertEqual(call_args[1]['n_initial_points'], 2)
        self.assertEqual(call_args[1]['random_state'], 42)

    @patch('geopmdpy.optimizer.gp_minimize')
    def test_optimize_with_efficiency(self, mock_gp_minimize):
        """Test optimization in the efficiency objective mode."""
        # Mock optimization result
        mock_result = MagicMock()
        mock_result.x = [1]
        mock_result.fun = -50.0  # Efficiency metric
        mock_result.func_vals = [-50.0]
        mock_gp_minimize.return_value = mock_result

        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', None, False)
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)

        result = opt.optimize(spec=spec, trials=1, n_initial_points=1)

        # Just verify the optimize method was called without errors
        self.assertIsNotNone(result)
        self.assertIn('best_coordinate', result)


@unittest.skipIf(skip_test, skip_msg)
class TestSessionStrategy(unittest.TestCase):
    """Tests for the geopmsession-wrapped energy evaluation strategy."""

    def _make_energy_evaluator(self, domain='cpu', signals=('CPU_ENERGY',),
                               sample_period=0.01, metric_regex=None):
        """Construct a session-wrapped evaluator with preflight mocked out."""
        with patch('geopmdpy.optimizer.shutil.which',
                   return_value='/usr/bin/geopmsession'), \
             patch('geopmdpy.optimizer.pio') as mock_pio:
            mock_pio.signal_names.return_value = list(signals)
            evaluator = optimizer.ApplicationEvaluator(
                ["app"], metric_regex=metric_regex, needs_session=True,
                efficiency_domain=domain, sample_period=sample_period)
        return evaluator

    def test_needs_session_flag_controls_preflight(self):
        """The needs_session flag drives session wrapping; default is direct."""
        with patch('geopmdpy.optimizer.shutil.which',
                   return_value='/usr/bin/geopmsession'), \
             patch('geopmdpy.optimizer.pio') as mock_pio:
            mock_pio.signal_names.return_value = ['CPU_ENERGY']
            evaluator = optimizer.ApplicationEvaluator(
                ["app"], needs_session=True, efficiency_domain='cpu')
        self.assertTrue(evaluator.needs_session)
        evaluator = optimizer.ApplicationEvaluator(["app"])
        self.assertFalse(evaluator.needs_session)

    def test_init_missing_geopmsession_raises(self):
        """A missing geopmsession is caught at construction time."""
        with patch('geopmdpy.optimizer.shutil.which', return_value=None), \
             patch('geopmdpy.optimizer.pio'):
            with self.assertRaises(optimizer.OptimizationError) as ctx:
                optimizer.ApplicationEvaluator(
                    ["app"], needs_session=True, efficiency_domain='cpu')
            self.assertIn("geopmsession", str(ctx.exception))

    def test_init_missing_yaml_raises(self):
        """A missing pyyaml module is caught at construction time."""
        with patch('geopmdpy.optimizer.yaml', None), \
             patch('geopmdpy.optimizer.shutil.which',
                   return_value='/usr/bin/geopmsession'), \
             patch('geopmdpy.optimizer.pio'):
            with self.assertRaises(optimizer.OptimizationError) as ctx:
                optimizer.ApplicationEvaluator(
                    ["app"], needs_session=True, efficiency_domain='cpu')
            self.assertIn("pyyaml", str(ctx.exception))

    def test_init_no_session_skips_preflight(self):
        """A direct evaluator does not require geopmsession or pyyaml."""
        with patch('geopmdpy.optimizer.shutil.which', return_value=None):
            evaluator = optimizer.ApplicationEvaluator(
                ["app"], needs_session=False)
        self.assertFalse(evaluator.needs_session)
        self.assertEqual(evaluator._energy_signals, [])

    def test_signal_config_cpu(self):
        """CPU domain emits TIME plus the CPU energy signal."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        self.assertEqual(evaluator._signal_config_str(),
                         'TIME board 0\nCPU_ENERGY board 0\n')

    def test_signal_config_gpu(self):
        """GPU domain emits TIME plus the GPU energy signal."""
        evaluator = self._make_energy_evaluator(domain='gpu',
                                                signals=('GPU_ENERGY',))
        self.assertEqual(evaluator._signal_config_str(),
                         'TIME board 0\nGPU_ENERGY board 0\n')

    def test_signal_config_board_component_sum(self):
        """Without BOARD_ENERGY the board domain sums component signals."""
        evaluator = self._make_energy_evaluator(
            domain='board',
            signals=('GPU_ENERGY', 'POWERCAP::CPU_ENERGY_CONSUMED',
                     'DRAM_ENERGY'))
        self.assertEqual(
            evaluator._signal_config_str(),
            'TIME board 0\nGPU_ENERGY board 0\n'
            'POWERCAP::CPU_ENERGY_CONSUMED board 0\nDRAM_ENERGY board 0\n')

    def test_session_argv(self):
        """The geopmsession command line wraps the launch command exactly."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',),
                                                sample_period=0.05)
        evaluator.launch_command = ["mybin", "--flag", "1"]
        argv = evaluator._session_argv('/tmp/sig.conf', '/tmp/rep.yaml')
        self.assertEqual(argv, [
            'geopmsession',
            '--period', repr(0.05),
            '--report-out', '/tmp/rep.yaml',
            '--trace-out', '/dev/null',
            '--signal-config', '/tmp/sig.conf',
            '--', 'mybin', '--flag', '1',
        ])

    def test_compute_energy_metrics(self):
        """Energy and runtime come from the last-first metric deltas."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        report = {'metrics': {
            'TIME': {'first': 1000.0, 'last': 1010.0},
            'CPU_ENERGY': {'first': 500.0, 'last': 600.0},
        }}
        energy, runtime, average_power = evaluator._compute_energy_metrics(report)
        self.assertEqual(energy, 100.0)
        self.assertEqual(runtime, 10.0)
        self.assertEqual(average_power, 10.0)

    def test_compute_energy_metrics_rollover(self):
        """A RAPL rollover corrected by the report yields positive energy.

        A naive two-point read of the raw counter would go negative when the
        counter wraps, but the report's ``last`` value is rollover-corrected so
        ``last - first`` stays positive.
        """
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        report = {'metrics': {
            'TIME': {'first': 0.0, 'last': 5.0},
            # Raw counter wrapped from 90 back to 10, but report corrects to 130
            'CPU_ENERGY': {'first': 90.0, 'last': 130.0},
        }}
        energy, runtime, average_power = evaluator._compute_energy_metrics(report)
        self.assertEqual(energy, 40.0)
        self.assertEqual(runtime, 5.0)
        self.assertEqual(average_power, 8.0)

    def test_compute_energy_metrics_missing_time(self):
        """A report without TIME is rejected."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        report = {'metrics': {'CPU_ENERGY': {'first': 0.0, 'last': 1.0}}}
        with self.assertRaises(optimizer.OptimizationError):
            evaluator._compute_energy_metrics(report)

    def test_compute_energy_metrics_missing_signal(self):
        """A report missing an energy signal is rejected."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        report = {'metrics': {'TIME': {'first': 0.0, 'last': 1.0}}}
        with self.assertRaises(optimizer.OptimizationError):
            evaluator._compute_energy_metrics(report)

    def test_compute_energy_metrics_non_positive_runtime(self):
        """A non-positive runtime is a recoverable failure."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        report = {'metrics': {
            'TIME': {'first': 5.0, 'last': 5.0},
            'CPU_ENERGY': {'first': 0.0, 'last': 1.0},
        }}
        with self.assertRaises(optimizer.RecoverableEvaluationError):
            evaluator._compute_energy_metrics(report)

    def test_load_report_missing_file(self):
        """A missing report file raises a clear error."""
        evaluator = self._make_energy_evaluator()
        with self.assertRaises(optimizer.OptimizationError):
            evaluator._load_report('/nonexistent/geopmopt_report.yaml')

    def test_load_report_malformed(self):
        """A non-mapping report is rejected."""
        evaluator = self._make_energy_evaluator()
        with patch('geopmdpy.optimizer.yaml') as mock_yaml, \
             patch('builtins.open', mock_open(read_data='- a\n- b\n')):
            mock_yaml.safe_load.return_value = ['a', 'b']
            with self.assertRaises(optimizer.OptimizationError):
                evaluator._load_report('/tmp/report.yaml')

    def test_evaluate_session_success_and_cleanup(self):
        """A successful session run returns metrics and cleans up temp files."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        report = {'metrics': {
            'TIME': {'first': 0.0, 'last': 10.0},
            'CPU_ENERGY': {'first': 0.0, 'last': 200.0},
        }}
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = ""
        created = []
        real_mkstemp = tempfile.mkstemp

        def tracking_mkstemp(*args, **kwargs):
            fd, path = real_mkstemp(*args, **kwargs)
            created.append(path)
            return fd, path

        with patch('geopmdpy.optimizer.subprocess.run',
                   return_value=mock_result) as mock_run, \
             patch('geopmdpy.optimizer.tempfile.mkstemp',
                   side_effect=tracking_mkstemp), \
             patch.object(evaluator, '_load_report', return_value=report):
            trial = evaluator._evaluate_session()

        self.assertEqual(trial.energy, 200.0)
        self.assertEqual(trial.runtime, 10.0)
        self.assertEqual(trial.average_power, 20.0)
        self.assertIsNone(trial.fom)
        mock_run.assert_called_once()
        self.assertEqual(len(created), 2)
        for path in created:
            self.assertFalse(os.path.exists(path))

    def test_evaluate_session_cleanup_on_error(self):
        """Temp files are removed even when the session fails."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        mock_result = MagicMock()
        mock_result.returncode = 1
        mock_result.stdout = ""
        mock_result.stderr = "boom"
        created = []
        real_mkstemp = tempfile.mkstemp

        def tracking_mkstemp(*args, **kwargs):
            fd, path = real_mkstemp(*args, **kwargs)
            created.append(path)
            return fd, path

        with patch('geopmdpy.optimizer.subprocess.run',
                   return_value=mock_result), \
             patch('geopmdpy.optimizer.tempfile.mkstemp',
                   side_effect=tracking_mkstemp):
            with self.assertRaises(optimizer.RecoverableEvaluationError):
                evaluator._evaluate_session()

        self.assertEqual(len(created), 2)
        for path in created:
            self.assertFalse(os.path.exists(path))

    def test_evaluate_session_extracts_fom(self):
        """When a regex is provided the session run also scrapes the FOM."""
        evaluator = self._make_energy_evaluator(
            domain='cpu', signals=('CPU_ENERGY',),
            metric_regex=r'FOM: ([0-9.]+)')
        report = {'metrics': {
            'TIME': {'first': 0.0, 'last': 2.0},
            'CPU_ENERGY': {'first': 0.0, 'last': 10.0},
        }}
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "FOM: 42.0\n"
        with patch('geopmdpy.optimizer.subprocess.run',
                   return_value=mock_result), \
             patch('geopmdpy.optimizer.tempfile.mkstemp',
                   side_effect=tempfile.mkstemp), \
             patch.object(evaluator, '_load_report', return_value=report):
            trial = evaluator._evaluate_session()
        self.assertEqual(trial.fom, 42.0)
        self.assertEqual(trial.energy, 10.0)

    def test_evaluate_wraps_recoverable_session_failure(self):
        """evaluate() turns a recoverable session failure into trial data."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        mock_result = MagicMock()
        mock_result.returncode = 1
        mock_result.stdout = ""
        mock_result.stderr = "boom"
        mock_grid = MagicMock()
        with patch('geopmdpy.optimizer.subprocess.run',
                   return_value=mock_result), \
             patch('geopmdpy.optimizer.tempfile.mkstemp',
                   side_effect=tempfile.mkstemp):
            trial = evaluator.evaluate(mock_grid, [0])
        self.assertTrue(trial.failed)
        self.assertIn("Application failed", trial.failure_reason)


@unittest.skipIf(skip_test, skip_msg)
class TestOptimizerMain(unittest.TestCase):
    @patch('sys.argv', ['optimizer.py', '--sweep', 'cpu-freq@package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--trials', '10',
                       'echo', 'Performance: 123.45'])
    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    def test_main_basic_success(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """Test basic successful execution of main function."""
        # Mock pio
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()

        # Mock ControlGrid
        mock_grid = MagicMock()
        mock_grid.control_name = ['cpu_frequency']
        mock_grid.get_grid_data.return_value = [
            {"control": "CPU_FREQUENCY_MAX_CONTROL", "domain": "package",
             "domain_idx": 0, "settings": [1000000, 2000000]}
        ]
        mock_grid_class.return_value = mock_grid

        # Mock the optimization process
        mock_optimizer = MagicMock()
        mock_optimizer.optimize.return_value = {
            'best_metric': 123.45,
            'best_coordinate': [1],
            'best_config': 'CPU_FREQUENCY_MAX_CONTROL package 0 2000000',
            'n_evaluations': 10
        }
        mock_optimizer.summarize.return_value = (
            "Best configuration:\nCPU_FREQUENCY_MAX_CONTROL package 0 2000000")
        mock_optimizer_class.return_value = mock_optimizer

        with patch('builtins.print') as mock_print:
            result = optimizer.main()

        self.assertEqual(result, 0)
        mock_pio.save_control.assert_called_once()
        mock_pio.restore_control.assert_called_once()
        mock_print.assert_called_once_with("Best configuration:\nCPU_FREQUENCY_MAX_CONTROL package 0 2000000")

    @patch('sys.argv', ['optimizer.py', '--metric-regex', 'test'])
    @patch('geopmdpy.optimizer.pio')
    def test_main_no_launch_command(self, mock_pio):
        """Test main function with no launch command."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()

        result = optimizer.main()
        self.assertEqual(result, 1)
        mock_pio.restore_control.assert_called_once()

    @patch('sys.argv', ['optimizer.py', '--metric-regex', 'test', 'echo', 'hello'])
    @patch('geopmdpy.optimizer.pio')
    def test_main_no_control_parameters(self, mock_pio):
        """Test main function with no control parameters."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()

        result = optimizer.main()
        self.assertEqual(result, 1)

    @patch('sys.argv', ['optimizer.py', '--sweep', 'cpu-freq@package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--application-timeout', '600',
                       '--print-stdout',
                       '--verbosity', '3',
                       'echo', 'Performance: 123.45'])
    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    def test_main_with_custom_options(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """Test main function with custom timeout and print-stdout options."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()

        # Mock ControlGrid
        mock_grid = MagicMock()
        mock_grid.control_name = ['cpu_frequency']
        mock_grid.get_grid_data.return_value = [
            {"control": "CPU_FREQUENCY_MAX_CONTROL", "domain": "package",
             "domain_idx": 0, "settings": [1000000, 2000000]}
        ]
        mock_grid_class.return_value = mock_grid

        # Mock the optimization process
        mock_optimizer = MagicMock()
        mock_optimizer.optimize.return_value = {
            'best_metric': 123.45,
            'best_coordinate': [1],
            'best_config': 'CPU_FREQUENCY_MAX_CONTROL package 0 2000000',
            'n_evaluations': 10
        }
        mock_optimizer_class.return_value = mock_optimizer

        with patch('builtins.print'):
            result = optimizer.main()
        self.assertEqual(result, 0)

    @patch('sys.argv', ['optimizer.py', '--sweep', 'cpu-freq@package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--defer-write',
                       '--output-file', 'valid_file.conf',
                       'echo', 'Performance: 123.45'])  # Add missing launch command
    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    def test_main_defer_write_with_valid_output_file(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """Test main with defer-write and valid output file - should succeed."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()

        # Mock ControlGrid
        mock_grid = MagicMock()
        mock_grid.control_name = ['cpu_frequency']
        mock_grid.get_grid_data.return_value = [
            {"control": "CPU_FREQUENCY_MAX_CONTROL", "domain": "package",
             "domain_idx": 0, "settings": [1000000, 2000000]}
        ]
        mock_grid_class.return_value = mock_grid

        # Mock optimizer
        mock_optimizer = MagicMock()
        mock_optimizer.optimize.return_value = {
            'best_metric': 123.45,
            'best_coordinate': [1],
            'best_config': 'CPU_FREQUENCY_MAX_CONTROL package 0 2000000',
            'n_evaluations': 10
        }
        mock_optimizer_class.return_value = mock_optimizer

        with patch('builtins.print'), \
             patch('builtins.open', mock_open()):
            result = optimizer.main()

        # Should succeed with valid output file
        self.assertEqual(result, 0)
        mock_pio.save_control.assert_not_called()
        mock_pio.restore_control.assert_not_called()
        

    @patch('sys.argv', ['optimizer.py', '--sweep', 'cpu-freq@package',
                       '--metric-regex', '[invalid', 'echo', 'Performance: 123.45'])
    @patch('geopmdpy.optimizer.pio')
    def test_main_invalid_metric_regex(self, mock_pio):
        """Test main with invalid metric regex pattern."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()
        result = optimizer.main()
        self.assertEqual(result, 1)
        mock_pio.restore_control.assert_called_once()

    @patch('sys.argv', ['optimizer.py', '--sweep', 'cpu-freq@package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--minimize',
                       '--', 'echo', 'Performance: 123.45'])
    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    def test_main_minimize_option(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """Test main function with minimize option."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()
        mock_grid = MagicMock()
        mock_grid.control_name = ['cpu_frequency']
        mock_grid.get_grid_data.return_value = [
            {"control": "CPU_FREQUENCY_MAX_CONTROL", "domain": "package",
             "domain_idx": 0, "settings": [1000000, 2000000]}
        ]
        mock_grid_class.return_value = mock_grid
        mock_optimizer = MagicMock()
        mock_optimizer.optimize.return_value = {
            'best_metric': 123.45,
            'best_coordinate': [1],
            'best_config': 'CPU_FREQUENCY_MAX_CONTROL package 0 2000000',
            'n_evaluations': 10
        }
        mock_optimizer_class.return_value = mock_optimizer

        with patch('builtins.print'):
            result = optimizer.main()
        self.assertEqual(result, 0)
        # The spec passed to optimize carries the minimize direction.
        _, optimize_kwargs = mock_optimizer.optimize.call_args
        self.assertTrue(optimize_kwargs['spec'].minimize)

    @patch('sys.argv', ['optimizer.py', '--sweep', 'cpu-freq@package',
                       'echo', 'hello'])
    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    def test_main_runtime_default(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """With no metric regex and no efficiency, main uses the runtime objective."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()
        mock_grid = MagicMock()
        mock_grid.control_name = ['cpu_frequency']
        mock_grid.get_grid_data.return_value = [
            {"control": "CPU_FREQUENCY_MAX_CONTROL", "domain": "package",
             "domain_idx": 0, "settings": [1000000, 2000000]}
        ]
        mock_grid_class.return_value = mock_grid
        mock_optimizer = MagicMock()
        mock_optimizer.optimize.return_value = {
            'best_metric': 1.5,
            'best_coordinate': [1],
            'best_config': 'CPU_FREQUENCY_MAX_CONTROL package 0 2000000',
            'n_evaluations': 10
        }
        mock_optimizer_class.return_value = mock_optimizer

        with patch('builtins.print'):
            result = optimizer.main()
        self.assertEqual(result, 0)
        # The evaluator has no metric regex and optimize gets the runtime spec.
        args, kwargs = mock_optimizer_class.call_args
        evaluator = args[1]
        self.assertIsNone(evaluator.metric_regex)
        _, optimize_kwargs = mock_optimizer.optimize.call_args
        self.assertEqual(optimize_kwargs['spec'].objective_expr, 'runtime')
        self.assertEqual(optimize_kwargs['spec'].label, 'runtime')


@unittest.skipIf(skip_test, skip_msg)
class TestGetParser(unittest.TestCase):
    def test_get_parser(self):
        """Test argument parser creation."""
        parser = optimizer.get_parser()
        self.assertIsNotNone(parser)

        # Test that basic arguments are present
        args = parser.parse_args(['--sweep', 'cpu-freq@package',
                                 '--metric-regex', 'test',
                                 'echo', 'hello'])
        self.assertEqual(args.sweep, ['cpu-freq@package'])
        self.assertEqual(args.metric_regex, 'test')
        self.assertEqual(args.launch, ['echo', 'hello'])

    def test_parser_with_all_options(self):
        """Test parser with all available options."""
        parser = optimizer.get_parser()

        args = parser.parse_args([
            '--sweep', 'cpu-freq@package',
            '--sweep', 'cpu-power@board',
            '--metric-regex', 'Performance: ([0-9.]+)',
            '--minimize',
            '--trials', '100',
            '--n-initial-points', '20',
            '--random-seed', '123',
            '--application-timeout', '600',
            '--output-file', 'output.conf',
            '--verbosity', '2',
            '--print-stdout',
            '--defer-write',
            '--efficiency', 'cpu',
            '--metric-bound', '100.0',
            '--sample-period', '0.25',
            '--penalty', '250.0',
            'echo', 'test'
        ])

        self.assertEqual(args.sweep, ['cpu-freq@package', 'cpu-power@board'])
        self.assertEqual(args.metric_regex, 'Performance: ([0-9.]+)')
        self.assertTrue(args.minimize)
        self.assertEqual(args.trials, 100)
        self.assertEqual(args.n_initial_points, 20)
        self.assertEqual(args.random_seed, 123)
        self.assertEqual(args.application_timeout, 600)
        self.assertEqual(args.output_file, 'output.conf')
        self.assertEqual(args.verbosity, 2)
        self.assertTrue(args.print_stdout)
        self.assertTrue(args.defer_write)
        self.assertEqual(args.efficiency_domain, 'cpu')
        self.assertEqual(args.metric_bound, 100.0)
        self.assertEqual(args.sample_period, 0.25)
        self.assertEqual(args.penalty, 250.0)
        self.assertEqual(args.launch, ['echo', 'test'])

    def test_parser_defaults(self):
        """Test parser default values."""
        parser = optimizer.get_parser()

        args = parser.parse_args([
            '--sweep', 'cpu-freq@package',
            '--metric-regex', 'test',
            'echo', 'hello'
        ])

        self.assertEqual(args.trials, 50)
        self.assertEqual(args.n_initial_points, 10)
        self.assertEqual(args.random_seed, 42)
        self.assertEqual(args.application_timeout, 300)
        self.assertEqual(args.output_file, '-')
        self.assertEqual(args.verbosity, 1)
        self.assertFalse(args.minimize)
        self.assertFalse(args.print_stdout)
        self.assertFalse(args.defer_write)
        self.assertIsNone(args.efficiency_domain)
        self.assertIsNone(args.metric_bound)
        self.assertEqual(args.sample_period, optimizer._DEFAULT_SAMPLE_PERIOD)
        self.assertEqual(args.penalty, 'auto')

    def test_parser_penalty_none(self):
        """--penalty accepts the 'none' policy."""
        parser = optimizer.get_parser()
        args = parser.parse_args([
            '--sweep', 'cpu-freq@package', '--metric-regex', 'test',
            '--penalty', 'none', 'echo', 'hello'
        ])
        self.assertEqual(args.penalty, 'none')

    def test_parser_penalty_numeric(self):
        """--penalty accepts a numeric policy parsed as a float."""
        parser = optimizer.get_parser()
        args = parser.parse_args([
            '--sweep', 'cpu-freq@package', '--metric-regex', 'test',
            '--penalty', '42.5', 'echo', 'hello'
        ])
        self.assertEqual(args.penalty, 42.5)

    def test_parser_penalty_invalid(self):
        """--penalty rejects a value that is neither a policy nor a number."""
        parser = optimizer.get_parser()
        with self.assertRaises(SystemExit):
            parser.parse_args([
                '--sweep', 'cpu-freq@package', '--metric-regex', 'test',
                '--penalty', 'bogus', 'echo', 'hello'
            ])

    def test_penalty_arg_helper(self):
        """_penalty_arg normalizes policies and numbers, rejecting garbage."""
        self.assertEqual(optimizer._penalty_arg('auto'), 'auto')
        self.assertEqual(optimizer._penalty_arg('none'), 'none')
        self.assertEqual(optimizer._penalty_arg('3.5'), 3.5)
        with self.assertRaises(ArgumentTypeError):
            optimizer._penalty_arg('bogus')

    def test_parser_metric_regex_optional(self):
        """--metric-regex is optional and defaults to None."""
        parser = optimizer.get_parser()

        # Omitting --metric-regex is allowed; it defaults to None.
        args = parser.parse_args(['--sweep', 'cpu-freq@package', 'echo', 'test'])
        self.assertIsNone(args.metric_regex)
        self.assertEqual(args.launch, ['echo', 'test'])


@unittest.skipIf(skip_test, skip_msg)
class TestExpandObjective(unittest.TestCase):
    def test_raw_metric(self):
        """Regex without efficiency selects the figure-of-merit objective."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', None, None, False)
        self.assertEqual(spec.objective_expr, 'fom')
        self.assertEqual(spec.label, 'figure of merit')
        self.assertFalse(spec.minimize)
        self.assertFalse(spec.needs_session)
        self.assertTrue(spec.requires_fom)
        self.assertEqual(spec.constraints, [])

    def test_efficiency(self):
        """Regex with efficiency selects the FoM-per-watt objective."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', None, False)
        self.assertEqual(spec.objective_expr, 'fom / power')
        self.assertEqual(spec.label, 'efficiency')
        self.assertTrue(spec.needs_session)
        self.assertTrue(spec.requires_fom)
        self.assertEqual(spec.efficiency_domain, 'cpu')

    def test_runtime(self):
        """No regex and no efficiency selects the runtime objective."""
        spec = optimizer.expand_objective(None, None, None, False)
        self.assertEqual(spec.objective_expr, 'runtime')
        self.assertEqual(spec.label, 'runtime')
        self.assertTrue(spec.minimize)
        self.assertFalse(spec.needs_session)
        self.assertFalse(spec.requires_fom)

    def test_energy(self):
        """Efficiency without a regex selects the energy objective."""
        spec = optimizer.expand_objective(None, 'cpu', None, False)
        self.assertEqual(spec.objective_expr, 'energy')
        self.assertEqual(spec.label, 'energy')
        self.assertTrue(spec.minimize)
        self.assertTrue(spec.needs_session)
        self.assertEqual(spec.efficiency_domain, 'cpu')

    def test_energy_bounded_maximize(self):
        """A metric bound with maximize adds a lower-bound FoM constraint."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', 100.0, False)
        self.assertEqual(spec.objective_expr, 'energy')
        self.assertEqual(spec.label, 'energy')
        self.assertTrue(spec.minimize)
        self.assertTrue(spec.needs_session)
        self.assertTrue(spec.requires_fom)
        self.assertEqual(len(spec.constraints), 1)
        constraint = spec.constraints[0]
        self.assertEqual(constraint.name, 'fom')
        self.assertEqual(constraint.op, '>=')
        self.assertEqual(constraint.value, 100.0)

    def test_energy_bounded_minimize_flips_operator(self):
        """A metric bound with minimize makes the FoM constraint an upper bound."""
        spec = optimizer.expand_objective('FOM: ([0-9.]+)', 'cpu', 100.0, True)
        constraint = spec.constraints[0]
        self.assertEqual(constraint.op, '<=')
        self.assertEqual(constraint.value, 100.0)

    def test_direction_does_not_change_expression(self):
        """The minimize flag does not change the objective expression."""
        for minimize in (True, False):
            self.assertEqual(
                optimizer.expand_objective(
                    'FOM: ([0-9.]+)', None, None, minimize).objective_expr,
                'fom')
            self.assertEqual(
                optimizer.expand_objective(
                    None, None, None, minimize).objective_expr,
                'runtime')

    def test_metric_bound_without_regex_raises(self):
        """A metric bound requires a metric regex."""
        with self.assertRaises(ValueError) as context:
            optimizer.expand_objective(None, 'cpu', 100.0, False)
        self.assertIn('--metric-regex', str(context.exception))

    def test_metric_bound_without_efficiency_raises(self):
        """A metric bound requires an efficiency domain."""
        with self.assertRaises(ValueError) as context:
            optimizer.expand_objective('FOM: ([0-9.]+)', None, 100.0, False)
        self.assertIn('--efficiency', str(context.exception))


@unittest.skipIf(skip_test, skip_msg)
class TestOptimizationError(unittest.TestCase):
    def test_optimization_error(self):
        """Test OptimizationError exception."""
        error = optimizer.OptimizationError("Test error message")
        self.assertEqual(str(error), "Test error message")
        self.assertIsInstance(error, Exception)


# --- Section 4 illustrative command (shared by parser and builder tests) -----
_SECTION4_METRICS = [
    "fom=regex:GFLOPS: ([0-9.]+)",
    "power=signal:CPU_POWER@board:mean",
    "energy=signal:CPU_ENERGY@board:delta",
]
_SECTION4_CONSTRAINTS = ['power <= 250', 'energy <= 5000']


@unittest.skipIf(skip_test, skip_msg)
class TestPhaseCParser(unittest.TestCase):
    """C1: the general --metric/--maximize/--minimize/--constraint flags."""

    def test_general_flags_present(self):
        """The §4 argument vector parses into the general-interface dests."""
        parser = optimizer.get_parser()
        argv = ['--sweep', 'cpu-freq@board', '--sweep', 'cpu-power@board']
        for metric in _SECTION4_METRICS:
            argv += ['--metric', metric]
        argv += ['--maximize', 'fom']
        for constraint in _SECTION4_CONSTRAINTS:
            argv += ['--constraint', constraint]
        argv += ['--', './app']

        args = parser.parse_args(argv)
        self.assertEqual(args.metric, _SECTION4_METRICS)
        self.assertEqual(args.maximize, 'fom')
        self.assertEqual(args.constraint, _SECTION4_CONSTRAINTS)
        self.assertIsNone(args.minimize)
        self.assertEqual(args.launch, ['--', './app'])

    def test_general_flag_defaults_are_none(self):
        """The general flags default to None when omitted."""
        parser = optimizer.get_parser()
        args = parser.parse_args(['--sweep', 'cpu-freq@board', 'echo', 'hi'])
        self.assertIsNone(args.metric)
        self.assertIsNone(args.maximize)
        self.assertIsNone(args.constraint)
        self.assertIsNone(args.minimize)

    def test_minimize_name_is_captured(self):
        """--minimize NAME captures the metric name as the objective."""
        parser = optimizer.get_parser()
        args = parser.parse_args([
            '--sweep', 'cpu-freq@board', '--metric', 'energy=signal:CPU_ENERGY@board',
            '--minimize', 'energy', '--', './app'])
        self.assertEqual(args.minimize, 'energy')

    def test_minimize_bare_is_true(self):
        """A bare --minimize retains the legacy global-minimize sentinel."""
        parser = optimizer.get_parser()
        args = parser.parse_args([
            '--sweep', 'cpu-freq@board', '--metric-regex', 'FOM: ([0-9.]+)',
            '--minimize', '--', './app'])
        self.assertIs(args.minimize, True)


@unittest.skipIf(skip_test, skip_msg)
class TestBuildObjective(unittest.TestCase):
    """C2/C3: build_objective validation and canonical-model construction."""

    def test_section4_builds_canonical_model(self):
        """The §4 flags build three metrics, maximize fom, two constraints."""
        spec = optimizer.build_objective(
            _SECTION4_METRICS, 'fom', None, _SECTION4_CONSTRAINTS)

        self.assertEqual(set(spec.metric_map), {'fom', 'power', 'energy'})
        self.assertEqual(spec.objective_expr, 'fom')
        self.assertEqual(spec.label, 'fom')
        self.assertFalse(spec.minimize)
        self.assertTrue(spec.requires_fom)
        self.assertTrue(spec.needs_session)

        by_name = {c.name: c for c in spec.constraints}
        self.assertEqual(set(by_name), {'power', 'energy'})
        self.assertEqual(by_name['power'].op, '<=')
        self.assertEqual(by_name['power'].value, 250.0)
        self.assertEqual(by_name['energy'].op, '<=')
        self.assertEqual(by_name['energy'].value, 5000.0)

    def test_constraint_unit_suffix_is_normalized(self):
        """A unit-suffixed constraint value is normalized to canonical units."""
        spec = optimizer.build_objective(
            ['power=signal:CPU_POWER@board:mean'], None, None,
            ['power <= 0.25kW'])
        self.assertEqual(spec.constraints[0].value, 250.0)

    def test_default_objective_minimizes_time(self):
        """With no objective flags the default is to minimize wall-clock time."""
        spec = optimizer.build_objective([], None, None, None)
        self.assertEqual(spec.objective_expr, 'time')
        self.assertEqual(spec.label, 'time')
        self.assertTrue(spec.minimize)

    def test_minimize_name_selects_objective(self):
        """--minimize NAME selects that metric as the minimized objective."""
        spec = optimizer.build_objective(
            ['energy=signal:CPU_ENERGY@board:delta'], None, 'energy', None)
        self.assertEqual(spec.objective_expr, 'energy')
        self.assertTrue(spec.minimize)

    def test_expr_objective_expands_to_expression(self):
        """A derived objective metric expands to its expression."""
        spec = optimizer.build_objective(
            ["fom=regex:GFLOPS: ([0-9.]+)",
             "power=signal:CPU_POWER@board:mean",
             "eff=expr:fom / power"],
            'eff', None, None)
        self.assertEqual(spec.objective_expr, 'fom / power')

    def test_expr_reference_to_unmeasured_power_raises(self):
        """A derived metric referencing unsampled power fails at parse time."""
        with self.assertRaises(ValueError) as context:
            optimizer.build_objective(
                ["fom=regex:GFLOPS: ([0-9.]+)", "eff=expr:fom / power"],
                'eff', None, None)
        message = str(context.exception)
        self.assertIn('eff', message)
        self.assertIn('power', message)

    def test_objective_on_unmeasured_power_raises(self):
        """Selecting unsampled power as the objective fails at parse time."""
        with self.assertRaises(ValueError) as context:
            optimizer.build_objective(
                ["fom=regex:GFLOPS: ([0-9.]+)"], 'power', None, None)
        message = str(context.exception)
        self.assertIn('objective', message)
        self.assertIn('power', message)

    def test_constraint_on_unmeasured_power_raises(self):
        """A constraint on unsampled power fails at parse time."""
        with self.assertRaises(ValueError) as context:
            optimizer.build_objective(
                ["fom=regex:GFLOPS: ([0-9.]+)"], 'fom', None,
                ['power <= 250'])
        message = str(context.exception)
        self.assertIn('constraint', message)
        self.assertIn('power', message)

    def test_fom_reference_without_regex_metric_raises(self):
        """Referencing fom with no regex: metric fails at parse time."""
        with self.assertRaises(ValueError) as context:
            optimizer.build_objective(
                ["power=signal:CPU_POWER@board:mean", "eff=expr:fom / power"],
                'eff', None, None)
        message = str(context.exception)
        self.assertIn('eff', message)
        self.assertIn('fom', message)

    def test_both_directions_raises(self):
        """Specifying both --maximize and --minimize NAME is an error."""
        with self.assertRaises(ValueError) as context:
            optimizer.build_objective(
                ["fom=regex:GFLOPS: ([0-9.]+)"], 'fom', 'fom', None)
        self.assertIn('mutually exclusive', str(context.exception))

    def test_undefined_objective_raises(self):
        """Selecting an undefined objective metric is an error."""
        with self.assertRaises(ValueError) as context:
            optimizer.build_objective(
                ["fom=regex:GFLOPS: ([0-9.]+)"], 'missing', None, None)
        self.assertIn('missing', str(context.exception))

    def test_expr_undefined_reference_raises(self):
        """A derived metric referencing an undefined metric is an error."""
        with self.assertRaises(ValueError) as context:
            optimizer.build_objective(
                ["bad=expr:undefined_name + 1"], 'bad', None, None)
        self.assertIn('undefined', str(context.exception))

    def test_duplicate_metric_raises(self):
        """Defining the same metric name twice is an error."""
        with self.assertRaises(ValueError) as context:
            optimizer.build_objective(
                ["fom=regex:A: ([0-9.]+)", "fom=regex:B: ([0-9.]+)"],
                'fom', None, None)
        self.assertIn('more than once', str(context.exception))

    def test_constraint_undefined_metric_raises(self):
        """A constraint on an undefined metric is an error."""
        with self.assertRaises(metrics.MetricSpecError):
            optimizer.build_objective(
                ["fom=regex:GFLOPS: ([0-9.]+)"], 'fom', None, ['bogus <= 5'])


@unittest.skipIf(skip_test, skip_msg)
class TestMainGeneralInterface(unittest.TestCase):
    """C2: main() dispatch between the legacy and general interfaces."""

    def test_legacy_and_general_flags_are_mutually_exclusive(self):
        """Mixing --metric-regex with --metric aborts with an error."""
        argv = ['optimizer.py', '--sweep', 'cpu-freq@package',
                '--metric-regex', 'FOM: ([0-9.]+)',
                '--metric', 'fom=regex:FOM: ([0-9.]+)', '--maximize', 'fom',
                '--', 'echo', 'hi']
        with patch('sys.argv', argv), \
                patch('geopmdpy.optimizer.pio') as mock_pio:
            mock_pio.save_control = MagicMock()
            mock_pio.restore_control = MagicMock()
            result = optimizer.main()
        self.assertEqual(result, 1)
        mock_pio.restore_control.assert_called_once()

    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    @patch('geopmdpy.optimizer.ApplicationEvaluator')
    def test_general_interface_drives_optimizer(self, mock_eval_class,
                                                mock_optimizer_class,
                                                mock_grid_class, mock_pio):
        """A --metric/--maximize invocation builds and runs the general spec."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()
        mock_grid = MagicMock()
        mock_grid.control_name = ['cpu_frequency']
        mock_grid.get_grid_data.return_value = [
            {"control": "CPU_FREQUENCY_MAX_CONTROL", "domain": "package",
             "domain_idx": 0, "settings": [1000000, 2000000]}
        ]
        mock_grid_class.return_value = mock_grid
        mock_optimizer = MagicMock()
        mock_optimizer.optimize.return_value = {
            'best_metric': 1.0, 'best_coordinate': [1],
            'best_config': 'CPU_FREQUENCY_MAX_CONTROL package 0 2000000',
            'n_evaluations': 10,
        }
        mock_optimizer_class.return_value = mock_optimizer

        argv = ['optimizer.py', '--sweep', 'cpu-freq@package',
                '--metric', 'fom=regex:FOM: ([0-9.]+)', '--maximize', 'fom',
                '--', 'echo', 'FOM: 1.0']
        with patch('sys.argv', argv), patch('builtins.print'):
            result = optimizer.main()

        self.assertEqual(result, 0)
        # The general spec, not the legacy one, drives the optimizer.
        _, optimize_kwargs = mock_optimizer.optimize.call_args
        spec = optimize_kwargs['spec']
        self.assertEqual(spec.objective_expr, 'fom')
        self.assertFalse(spec.minimize)
        self.assertEqual(set(spec.metric_map), {'fom'})
        # The regex metric's pattern is wired into the evaluator.
        _, eval_kwargs = mock_eval_class.call_args
        self.assertEqual(eval_kwargs['metric_regex'], 'FOM: ([0-9.]+)')


@unittest.skipIf(skip_test, skip_msg)
class TestListMetrics(unittest.TestCase):
    """D1/D2: the --list-metrics discovery mode and formatter."""

    def test_flag_defaults_false(self):
        """--list-metrics defaults off and sets its mode when present."""
        parser = optimizer.get_parser()
        off = parser.parse_args(['--sweep', 'cpu-freq@board', 'echo', 'hi'])
        self.assertFalse(off.list_metrics)
        on = parser.parse_args(['--list-metrics'])
        self.assertTrue(on.list_metrics)

    def test_lists_reserved_metrics_with_units(self):
        """The listing header names each reserved canonical metric and unit."""
        text = optimizer.list_metrics_str()
        self.assertIn('METRIC', text)
        self.assertIn('UNIT', text)
        for name, unit in metrics.RESERVED_UNITS.items():
            self.assertRegex(text, rf'{name}\s+{re.escape(unit)}')

    @patch('geopmdpy.optimizer.metrics.signal_behavior')
    def test_monotone_signal_shows_delta(self, mock_behavior):
        """A referenced monotone signal defaults to the delta aggregation."""
        mock_behavior.return_value = metrics.BEHAVIOR_MONOTONE
        text = optimizer.list_metrics_str(['energy=signal:CPU_ENERGY@board'])
        self.assertRegex(text, r'CPU_ENERGY\b.*monotone.*delta')

    @patch('geopmdpy.optimizer.metrics.signal_behavior')
    def test_variable_signal_shows_mean(self, mock_behavior):
        """A referenced variable signal defaults to the mean aggregation."""
        mock_behavior.return_value = metrics.BEHAVIOR_VARIABLE
        text = optimizer.list_metrics_str(['power=signal:CPU_POWER@board'])
        self.assertRegex(text, r'CPU_POWER\b.*variable.*mean')

    @patch('geopmdpy.optimizer.metrics.signal_behavior')
    def test_unavailable_signal_renders_na(self, mock_behavior):
        """A signal unavailable on the platform renders as n/a."""
        mock_behavior.side_effect = RuntimeError('signal unavailable')
        text = optimizer.list_metrics_str(['gpu=signal:GPU_ENERGY@gpu'])
        self.assertRegex(text, r'GPU_ENERGY\b.*n/a')

    @patch('geopmdpy.optimizer.metrics.signal_behavior')
    def test_explicit_aggregation_is_preserved(self, mock_behavior):
        """An explicit aggregation overrides the behavior-derived default."""
        mock_behavior.return_value = metrics.BEHAVIOR_MONOTONE
        text = optimizer.list_metrics_str(['power=signal:CPU_POWER@board:mean'])
        self.assertRegex(text, r'CPU_POWER\b.*monotone.*mean')

    @patch('sys.argv', ['optimizer.py', '--list-metrics'])
    @patch('geopmdpy.optimizer.pio')
    def test_main_list_metrics_early_return(self, mock_pio):
        """--list-metrics prints and exits without touching controls."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()
        with patch('builtins.print') as mock_print:
            result = optimizer.main()
        self.assertEqual(result, 0)
        mock_pio.save_control.assert_not_called()
        mock_pio.restore_control.assert_not_called()
        mock_print.assert_called()


if __name__ == '__main__':
    unittest.main()
