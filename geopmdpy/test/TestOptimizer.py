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

# Skip test if skopt not available
skip_test = False
skip_msg = 'The skopt module is required to test optimizer'
try:
    import skopt
    from geopmdpy import optimizer
except ImportError:
    skip_test = True

@unittest.skipIf(skip_test, skip_msg)
class TestApplicationEvaluator(unittest.TestCase):
    def setUp(self):
        self.launch_command = ["echo", "Performance: 123.45"]
        self.metric_regex = "Performance: ([0-9.]+)"
        self.evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, self.metric_regex, maximize=True, timeout=300
        )

    def test_init(self):
        """Test ApplicationEvaluator initialization."""
        self.assertEqual(self.evaluator.launch_command, self.launch_command)
        self.assertEqual(self.evaluator.metric_regex, self.metric_regex)
        self.assertTrue(self.evaluator.maximize)
        self.assertEqual(self.evaluator.timeout, 300)
        self.assertFalse(self.evaluator.print_stdout)

    def test_init_with_print_stdout(self):
        """Test ApplicationEvaluator initialization with print_stdout enabled."""
        evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, self.metric_regex, maximize=False,
            timeout=600, print_stdout=True
        )
        self.assertFalse(evaluator.maximize)
        self.assertEqual(evaluator.timeout, 600)
        self.assertTrue(evaluator.print_stdout)

    def test_compile_regex(self):
        """Test regex compilation."""
        # Test direct regex
        evaluator = optimizer.ApplicationEvaluator(
            ["echo", "test"], "Performance: ([0-9.]+)", maximize=True
        )
        self.assertIsNotNone(evaluator.regex)

        # Test regex with special characters
        evaluator = optimizer.ApplicationEvaluator(
            ["echo", "test"], r"Score:\s+([0-9]*\.?[0-9]+)", maximize=True
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
            self.launch_command, self.metric_regex, maximize=False
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
        """Test evaluation with failed subprocess."""
        mock_result = MagicMock()
        mock_result.returncode = 1
        mock_result.stderr = "Error occurred"
        mock_result.stdout = "Some output"
        mock_run.return_value = mock_result

        mock_grid = MagicMock()
        coordinate = [0]

        with self.assertRaises(optimizer.OptimizationError) as context:
            self.evaluator.evaluate(mock_grid, coordinate)

        self.assertIn("Application failed", str(context.exception))

    @patch('subprocess.run')
    def test_evaluate_timeout(self, mock_run):
        """Test evaluation with subprocess timeout."""
        mock_run.side_effect = subprocess.TimeoutExpired("cmd", 300)

        mock_grid = MagicMock()
        coordinate = [0]

        with self.assertRaises(optimizer.OptimizationError) as context:
            self.evaluator.evaluate(mock_grid, coordinate)

        self.assertIn("timed out after 300 seconds", str(context.exception))

    @patch('subprocess.run')
    def test_evaluate_custom_timeout(self, mock_run):
        """Test evaluation with custom timeout."""
        evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, self.metric_regex, timeout=600
        )

        mock_run.side_effect = subprocess.TimeoutExpired("cmd", 600)
        mock_grid = MagicMock()

        with self.assertRaises(optimizer.OptimizationError) as context:
            evaluator.evaluate(mock_grid, [0])

        self.assertIn("timed out after 600 seconds", str(context.exception))

    @patch('subprocess.run')
    def test_evaluate_no_output(self, mock_run):
        """Test evaluation with no stdout."""
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = ""
        mock_run.return_value = mock_result

        mock_grid = MagicMock()

        with self.assertRaises(optimizer.OptimizationError) as context:
            self.evaluator.evaluate(mock_grid, [0])

        self.assertIn("produced no output", str(context.exception))

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
        """A non-zero return code still raises even without a metric regex."""
        evaluator = optimizer.ApplicationEvaluator(
            self.launch_command, metric_regex=None
        )
        mock_result = MagicMock()
        mock_result.returncode = 1
        mock_result.stderr = "boom"
        mock_result.stdout = ""
        mock_run.return_value = mock_result

        mock_grid = MagicMock()
        with self.assertRaises(optimizer.OptimizationError) as context:
            evaluator.evaluate(mock_grid, [0])
        self.assertIn("Application failed", str(context.exception))

    def test_extract_metric_success(self):
        """Test successful metric extraction."""
        stdout = "Application output\nPerformance: 456.78 GFLOPS\nMore output"
        metric = self.evaluator._extract_metric(stdout)
        self.assertEqual(metric, 456.78)

    def test_extract_metric_no_capture_group(self):
        """Test metric extraction without capture group."""
        evaluator = optimizer.ApplicationEvaluator(
            ["echo", "test"], r"[0-9]+\.[0-9]+", maximize=True
        )
        stdout = "Score is 123.45 points"
        metric = evaluator._extract_metric(stdout)
        self.assertEqual(metric, 123.45)

    def test_extract_metric_no_match(self):
        """Test metric extraction with no match."""
        stdout = "Application output without metric"

        with self.assertRaises(optimizer.OptimizationError) as context:
            self.evaluator._extract_metric(stdout)

        self.assertIn("Could not extract metric", str(context.exception))

    def test_extract_metric_invalid_value(self):
        """Test metric extraction with invalid numeric value."""
        evaluator = optimizer.ApplicationEvaluator(
            ["echo", "test"], "Performance: ([a-z]+)", maximize=True
        )
        stdout = "Performance: invalid"

        with self.assertRaises(optimizer.OptimizationError) as context:
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
        self.mock_evaluator.maximize = True

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

    def test_score_result_maximize(self):
        """Maximize converts the raw FoM to its negation (minimization space)."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        trial = optimizer.TrialResult(fom=123.45)
        self.assertEqual(
            opt._score_result(trial, optimizer.ObjectiveMode.RAW_METRIC), -123.45)

    def test_score_result_minimize(self):
        """Minimize keeps the raw FoM sign."""
        self.mock_evaluator.maximize = False
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        trial = optimizer.TrialResult(fom=123.45)
        self.assertEqual(
            opt._score_result(trial, optimizer.ObjectiveMode.RAW_METRIC), 123.45)

    def test_score_result_efficiency_maximize(self):
        """Maximizing efficiency divides the negated FoM by average power."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        trial = optimizer.TrialResult(fom=1000.0, average_power=100.0)
        self.assertEqual(
            opt._score_result(trial, optimizer.ObjectiveMode.EFFICIENCY), -10.0)

    def test_score_result_efficiency_minimize(self):
        """Minimizing efficiency multiplies the FoM by average power."""
        self.mock_evaluator.maximize = False
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        trial = optimizer.TrialResult(fom=10.0, average_power=5.0)
        self.assertEqual(
            opt._score_result(trial, optimizer.ObjectiveMode.EFFICIENCY), 50.0)

    def test_score_result_nonpositive_power_raises(self):
        """Non-positive average power raises OptimizationError."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        trial = optimizer.TrialResult(fom=1.0, average_power=0.0)
        with self.assertRaises(optimizer.OptimizationError) as context:
            opt._score_result(trial, optimizer.ObjectiveMode.EFFICIENCY)
        self.assertIn("non-positive", str(context.exception))

    def test_score_result_runtime(self):
        """The runtime objective minimizes wall-clock time directly."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        trial = optimizer.TrialResult(runtime=12.5)
        self.assertEqual(
            opt._score_result(trial, optimizer.ObjectiveMode.RUNTIME), 12.5)

    def test_score_result_energy(self):
        """The energy objective minimizes measured energy directly."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        trial = optimizer.TrialResult(energy=987.0)
        self.assertEqual(
            opt._score_result(trial, optimizer.ObjectiveMode.ENERGY), 987.0)

    def test_score_result_energy_bounded_feasible(self):
        """A feasible energy-bounded trial scores its plain energy."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._metric_bound = 100.0
        opt._penalty_weight = 1.0
        # maximize (default): feasible when fom >= bound
        trial = optimizer.TrialResult(fom=120.0, energy=500.0)
        self.assertEqual(
            opt._score_result(trial, optimizer.ObjectiveMode.ENERGY_BOUNDED),
            500.0)

    def test_score_result_energy_bounded_infeasible(self):
        """An infeasible energy-bounded trial scores strictly above its energy."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._metric_bound = 100.0
        opt._penalty_weight = 1.0
        # violation = (100 - 50) / 100 = 0.5; energy_scale = 500;
        # penalty = 1.0 * 500 * 0.5 = 250
        trial = optimizer.TrialResult(fom=50.0, energy=500.0)
        score = opt._score_result(trial, optimizer.ObjectiveMode.ENERGY_BOUNDED)
        self.assertGreater(score, 500.0)
        self.assertEqual(score, 750.0)

    def test_score_result_energy_bounded_minimize(self):
        """When minimizing the FoM the bound is a ceiling."""
        self.mock_evaluator.maximize = False
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._metric_bound = 100.0
        opt._penalty_weight = 1.0
        # feasible when fom <= bound
        feasible = optimizer.TrialResult(fom=80.0, energy=500.0)
        self.assertEqual(
            opt._score_result(feasible, optimizer.ObjectiveMode.ENERGY_BOUNDED),
            500.0)
        # infeasible when fom > bound: violation = (150 - 100) / 100 = 0.5
        infeasible = optimizer.TrialResult(fom=150.0, energy=500.0)
        score = opt._score_result(
            infeasible, optimizer.ObjectiveMode.ENERGY_BOUNDED)
        self.assertEqual(score, 750.0)

    def test_build_bounded_result_prefers_feasible(self):
        """The best feasible point is chosen even if infeasible energy is lower."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._metric_bound = 100.0
        opt.evaluation_history = [
            {'coordinate': [0], 'metric': 100.0, 'energy': 100.0,
             'fom': 50.0, 'feasible': False},
            {'coordinate': [1], 'metric': 300.0, 'energy': 300.0,
             'fom': 120.0, 'feasible': True},
            {'coordinate': [2], 'metric': 250.0, 'energy': 250.0,
             'fom': 110.0, 'feasible': True},
        ]
        mock_result = MagicMock()
        mock_result.func_vals = [1, 2, 3]
        out = opt._build_bounded_result(mock_result)
        self.assertTrue(out['bound_satisfied'])
        self.assertEqual(out['best_coordinate'], [2])
        self.assertEqual(out['best_metric'], 250.0)
        self.assertEqual(out['best_fom'], 110.0)

    def test_build_bounded_result_all_infeasible(self):
        """With no feasible point the least-penalized trial is reported."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._metric_bound = 100.0
        opt.evaluation_history = [
            {'coordinate': [0], 'metric': 900.0, 'energy': 400.0,
             'fom': 50.0, 'feasible': False},
            {'coordinate': [1], 'metric': 600.0, 'energy': 500.0,
             'fom': 80.0, 'feasible': False},
        ]
        mock_result = MagicMock()
        mock_result.func_vals = [1, 2]
        out = opt._build_bounded_result(mock_result)
        self.assertFalse(out['bound_satisfied'])
        # The trial with the lowest penalized metric (600.0) is reported.
        self.assertEqual(out['best_coordinate'], [1])
        self.assertEqual(out['best_fom'], 80.0)

    def test_summarize_energy_bounded_satisfied(self):
        """The summary reports bound feasibility and the best figure of merit."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._metric_bound = 100.0
        result = {
            'best_config': 'CONFIG', 'best_metric': 250.0,
            'best_fom': 110.0, 'bound_satisfied': True,
        }
        summary = opt.summarize(result, optimizer.ObjectiveMode.ENERGY_BOUNDED)
        self.assertIn('satisfied', summary)
        self.assertIn('110.0', summary)
        self.assertNotIn('NOT satisfied', summary)

    def test_summarize_energy_bounded_violated(self):
        """The summary flags a violated bound."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._metric_bound = 100.0
        result = {
            'best_config': 'CONFIG', 'best_metric': 250.0,
            'best_fom': 80.0, 'bound_satisfied': False,
        }
        summary = opt.summarize(result, optimizer.ObjectiveMode.ENERGY_BOUNDED)
        self.assertIn('NOT satisfied', summary)

    def test_summarize_non_bounded(self):
        """Non-bounded objectives summarize just the best configuration."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        result = {'best_config': 'CONFIG', 'best_metric': 1.0}
        summary = opt.summarize(result, optimizer.ObjectiveMode.RUNTIME)
        self.assertEqual(summary, 'Best configuration:\nCONFIG')

    def test_record_history(self):
        """History records the coordinate, metric, and config commands."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._record_history([1], 42.0)
        self.assertEqual(len(opt.evaluation_history), 1)
        self.assertEqual(opt.evaluation_history[0]['coordinate'], [1])
        self.assertEqual(opt.evaluation_history[0]['metric'], 42.0)

    def test_record_history_energy_bounded_feasibility(self):
        """Energy-bounded history entries record feasibility, fom, and energy."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        opt._metric_bound = 100.0
        feasible = optimizer.TrialResult(fom=120.0, energy=500.0)
        opt._record_history([0], 500.0, feasible,
                            optimizer.ObjectiveMode.ENERGY_BOUNDED)
        infeasible = optimizer.TrialResult(fom=50.0, energy=400.0)
        opt._record_history([1], 700.0, infeasible,
                            optimizer.ObjectiveMode.ENERGY_BOUNDED)
        self.assertTrue(opt.evaluation_history[0]['feasible'])
        self.assertEqual(opt.evaluation_history[0]['fom'], 120.0)
        self.assertEqual(opt.evaluation_history[0]['energy'], 500.0)
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

        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        result = opt.optimize(trials=4, n_initial_points=2, random_state=42)

        # Check results
        self.assertEqual(result['best_coordinate'], [2])
        self.assertEqual(result['best_metric'], 123.45)  # Should be positive for maximization
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

        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)

        result = opt.optimize(
            trials=1,
            n_initial_points=1,
            objective_mode=optimizer.ObjectiveMode.EFFICIENCY
        )

        # Just verify the optimize method was called without errors
        self.assertIsNotNone(result)
        self.assertIn('best_coordinate', result)


@unittest.skipIf(skip_test, skip_msg)
class TestSessionStrategy(unittest.TestCase):
    """Tests for the geopmsession-wrapped energy evaluation strategy."""

    def _make_energy_evaluator(self, domain='cpu', signals=('CPU_ENERGY',),
                               mode=None, sample_period=0.01, metric_regex=None):
        """Construct an energy-objective evaluator with preflight mocked out."""
        if mode is None:
            mode = optimizer.ObjectiveMode.EFFICIENCY
        with patch('geopmdpy.optimizer.shutil.which',
                   return_value='/usr/bin/geopmsession'), \
             patch('geopmdpy.optimizer.pio') as mock_pio:
            mock_pio.signal_names.return_value = list(signals)
            evaluator = optimizer.ApplicationEvaluator(
                ["app"], metric_regex=metric_regex, maximize=True,
                objective_mode=mode, efficiency_domain=domain,
                sample_period=sample_period)
        return evaluator

    def test_needs_session_for_energy_modes(self):
        """Energy objectives route through geopmsession; others stay direct."""
        for mode in (optimizer.ObjectiveMode.EFFICIENCY,
                     optimizer.ObjectiveMode.ENERGY,
                     optimizer.ObjectiveMode.ENERGY_BOUNDED):
            evaluator = self._make_energy_evaluator(mode=mode)
            self.assertTrue(evaluator.needs_session)
        for mode in (optimizer.ObjectiveMode.RAW_METRIC,
                     optimizer.ObjectiveMode.RUNTIME):
            evaluator = optimizer.ApplicationEvaluator(
                ["app"], objective_mode=mode)
            self.assertFalse(evaluator.needs_session)

    def test_init_missing_geopmsession_raises(self):
        """A missing geopmsession is caught at construction time."""
        with patch('geopmdpy.optimizer.shutil.which', return_value=None), \
             patch('geopmdpy.optimizer.pio'):
            with self.assertRaises(optimizer.OptimizationError) as ctx:
                optimizer.ApplicationEvaluator(
                    ["app"], objective_mode=optimizer.ObjectiveMode.EFFICIENCY,
                    efficiency_domain='cpu')
            self.assertIn("geopmsession", str(ctx.exception))

    def test_init_missing_yaml_raises(self):
        """A missing pyyaml module is caught at construction time."""
        with patch('geopmdpy.optimizer.yaml', None), \
             patch('geopmdpy.optimizer.shutil.which',
                   return_value='/usr/bin/geopmsession'), \
             patch('geopmdpy.optimizer.pio'):
            with self.assertRaises(optimizer.OptimizationError) as ctx:
                optimizer.ApplicationEvaluator(
                    ["app"], objective_mode=optimizer.ObjectiveMode.EFFICIENCY,
                    efficiency_domain='cpu')
            self.assertIn("pyyaml", str(ctx.exception))

    def test_init_non_energy_mode_skips_preflight(self):
        """Non-energy objectives do not require geopmsession or pyyaml."""
        with patch('geopmdpy.optimizer.shutil.which', return_value=None):
            evaluator = optimizer.ApplicationEvaluator(
                ["app"], objective_mode=optimizer.ObjectiveMode.RUNTIME)
        self.assertFalse(evaluator.needs_session)
        self.assertIsNone(evaluator._energy_signals)

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
        """A non-positive runtime is rejected."""
        evaluator = self._make_energy_evaluator(domain='cpu',
                                                signals=('CPU_ENERGY',))
        report = {'metrics': {
            'TIME': {'first': 5.0, 'last': 5.0},
            'CPU_ENERGY': {'first': 0.0, 'last': 1.0},
        }}
        with self.assertRaises(optimizer.OptimizationError):
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
            with self.assertRaises(optimizer.OptimizationError):
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


@unittest.skipIf(skip_test, skip_msg)
class TestOptimizerMain(unittest.TestCase):
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
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

    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
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

    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
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
        

    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', '[invalid', 'echo', 'Performance: 123.45'])
    @patch('geopmdpy.optimizer.pio')
    def test_main_invalid_metric_regex(self, mock_pio):
        """Test main with invalid metric regex pattern."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()
        result = optimizer.main()
        self.assertEqual(result, 1)
        mock_pio.restore_control.assert_called_once()

    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--minimize',
                       'echo', 'Performance: 123.45'])
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
        # Check that maximize is False in evaluator
        args, kwargs = mock_optimizer_class.call_args
        evaluator = args[1]
        self.assertFalse(evaluator.maximize)

    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
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
        # The evaluator has no metric regex and optimize gets the runtime mode.
        args, kwargs = mock_optimizer_class.call_args
        evaluator = args[1]
        self.assertIsNone(evaluator.metric_regex)
        _, optimize_kwargs = mock_optimizer.optimize.call_args
        self.assertEqual(optimize_kwargs['objective_mode'],
                         optimizer.ObjectiveMode.RUNTIME)


@unittest.skipIf(skip_test, skip_msg)
class TestGetParser(unittest.TestCase):
    def test_get_parser(self):
        """Test argument parser creation."""
        parser = optimizer.get_parser()
        self.assertIsNotNone(parser)

        # Test that basic arguments are present
        args = parser.parse_args(['--cpu-frequency', 'package',
                                 '--metric-regex', 'test',
                                 'echo', 'hello'])
        self.assertEqual(args.cpu_frequency_domain, 'package')
        self.assertEqual(args.metric_regex, 'test')
        self.assertEqual(args.launch, ['echo', 'hello'])

    def test_parser_with_all_options(self):
        """Test parser with all available options."""
        parser = optimizer.get_parser()

        args = parser.parse_args([
            '--cpu-frequency', 'package',
            '--cpu-power', 'board',
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
            'echo', 'test'
        ])

        self.assertEqual(args.cpu_frequency_domain, 'package')
        self.assertEqual(args.cpu_power_domain, 'board')
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
        self.assertEqual(args.launch, ['echo', 'test'])

    def test_parser_defaults(self):
        """Test parser default values."""
        parser = optimizer.get_parser()

        args = parser.parse_args([
            '--cpu-frequency', 'package',
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

    def test_parser_metric_regex_optional(self):
        """--metric-regex is optional and defaults to None."""
        parser = optimizer.get_parser()

        # Omitting --metric-regex is allowed; it defaults to None.
        args = parser.parse_args(['--cpu-frequency', 'package', 'echo', 'test'])
        self.assertIsNone(args.metric_regex)
        self.assertEqual(args.launch, ['echo', 'test'])


@unittest.skipIf(skip_test, skip_msg)
class TestResolveObjectiveMode(unittest.TestCase):
    def test_raw_metric(self):
        """Regex without efficiency selects the raw figure of merit."""
        mode = optimizer.resolve_objective_mode(
            'FOM: ([0-9.]+)', None, None, False)
        self.assertEqual(mode, optimizer.ObjectiveMode.RAW_METRIC)

    def test_efficiency(self):
        """Regex with efficiency selects the efficiency objective."""
        mode = optimizer.resolve_objective_mode(
            'FOM: ([0-9.]+)', 'cpu', None, False)
        self.assertEqual(mode, optimizer.ObjectiveMode.EFFICIENCY)

    def test_runtime(self):
        """No regex and no efficiency selects the runtime objective."""
        mode = optimizer.resolve_objective_mode(None, None, None, False)
        self.assertEqual(mode, optimizer.ObjectiveMode.RUNTIME)

    def test_energy(self):
        """Efficiency without a regex selects the energy objective."""
        mode = optimizer.resolve_objective_mode(None, 'cpu', None, False)
        self.assertEqual(mode, optimizer.ObjectiveMode.ENERGY)

    def test_energy_bounded(self):
        """A metric bound with regex and efficiency selects energy-bounded."""
        mode = optimizer.resolve_objective_mode(
            'FOM: ([0-9.]+)', 'cpu', 100.0, False)
        self.assertEqual(mode, optimizer.ObjectiveMode.ENERGY_BOUNDED)

    def test_direction_does_not_change_mode(self):
        """The minimize flag does not change the resolved mode."""
        for minimize in (True, False):
            self.assertEqual(
                optimizer.resolve_objective_mode('FOM: ([0-9.]+)', None, None, minimize),
                optimizer.ObjectiveMode.RAW_METRIC)
            self.assertEqual(
                optimizer.resolve_objective_mode(None, None, None, minimize),
                optimizer.ObjectiveMode.RUNTIME)

    def test_metric_bound_without_regex_raises(self):
        """A metric bound requires a metric regex."""
        with self.assertRaises(ValueError) as context:
            optimizer.resolve_objective_mode(None, 'cpu', 100.0, False)
        self.assertIn('--metric-regex', str(context.exception))

    def test_metric_bound_without_efficiency_raises(self):
        """A metric bound requires an efficiency domain."""
        with self.assertRaises(ValueError) as context:
            optimizer.resolve_objective_mode('FOM: ([0-9.]+)', None, 100.0, False)
        self.assertIn('--efficiency', str(context.exception))


@unittest.skipIf(skip_test, skip_msg)
class TestOptimizationError(unittest.TestCase):
    def test_optimization_error(self):
        """Test OptimizationError exception."""
        error = optimizer.OptimizationError("Test error message")
        self.assertEqual(str(error), "Test error message")
        self.assertIsInstance(error, Exception)


if __name__ == '__main__':
    unittest.main()
