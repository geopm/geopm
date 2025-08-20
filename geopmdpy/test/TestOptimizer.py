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

        # Should return negative value for maximization
        self.assertEqual(result, -123.45)
        mock_grid.write_config.assert_called_once_with(coordinate)
        mock_run.assert_called_once_with(
            self.launch_command,
            capture_output=True,
            text=True,
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

        # Should return positive value for minimization
        self.assertEqual(result, 456.78)

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

            self.assertEqual(result, -789.01)
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

    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.gp_minimize')
    def test_optimize_with_efficiency(self, mock_gp_minimize, mock_pio):
        """Test optimization with efficiency calculation."""
        # Mock optimization result
        mock_result = MagicMock()
        mock_result.x = [1]
        mock_result.fun = -50.0  # Efficiency metric
        mock_result.func_vals = [-50.0]
        mock_gp_minimize.return_value = mock_result

        # Mock time and energy readings
        mock_pio.read_signal.side_effect = [1000.0, 1010.0]  # 10 second runtime
        mock_pio.signal_names.return_value = ["CPU_ENERGY", "TIME"]

        # Mock get_energy function
        with patch('geopmdpy.optimizer.get_energy') as mock_get_energy:
            mock_get_energy.side_effect = [500.0, 600.0]  # Start and end energy

            # Mock evaluator to return metric
            self.mock_evaluator.evaluate.return_value = 1000.0  # Base metric

            opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)

            result = opt.optimize(
                trials=1,
                n_initial_points=1,
                use_efficiency=1,  # Maximizing efficiency
                efficiency_domain='cpu'
            )

            # Just verify the optimize method was called without errors
            self.assertIsNotNone(result)
            self.assertIn('best_coordinate', result)


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

    def test_parser_required_arguments(self):
        """Test parser with missing required arguments."""
        parser = optimizer.get_parser()

        # Missing metric-regex should raise SystemExit
        with self.assertRaises(SystemExit):
            parser.parse_args(['--cpu-frequency', 'package', 'echo', 'test'])


@unittest.skipIf(skip_test, skip_msg)
class TestOptimizationError(unittest.TestCase):
    def test_optimization_error(self):
        """Test OptimizationError exception."""
        error = optimizer.OptimizationError("Test error message")
        self.assertEqual(str(error), "Test error message")
        self.assertIsInstance(error, Exception)


if __name__ == '__main__':
    unittest.main()
