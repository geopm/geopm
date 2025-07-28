#!/usr/bin/env python3
"""
Unit tests for the Bayesian optimizer.
"""

import unittest
from unittest.mock import patch, MagicMock, mock_open
import subprocess
import tempfile
import os

# Mock scikit-optimize if not available
try:
    import skopt
except ImportError:
    # Create mock skopt module
    skopt = MagicMock()
    skopt.gp_minimize = MagicMock()
    skopt.space.Integer = MagicMock()
    skopt.utils.use_named_args = lambda x: lambda f: f

from geopmdpy import optimizer
from geopmdpy.grid import ControlGrid


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


class TestBayesianOptimizer(unittest.TestCase):
    def setUp(self):
        # Mock the control grid
        self.mock_grid = MagicMock(spec=ControlGrid)
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
        self.mock_evaluator = MagicMock(spec=optimizer.ApplicationEvaluator)
        self.mock_evaluator.maximize = True

    def test_init_with_skopt(self):
        """Test initialization when scikit-optimize is available."""
        opt = optimizer.BayesianOptimizer(self.mock_grid, self.mock_evaluator)
        self.assertEqual(opt.control_grid, self.mock_grid)
        self.assertEqual(opt.evaluator, self.mock_evaluator)
        self.assertEqual(len(opt.space), 1)
        self.assertEqual(len(opt.evaluation_history), 0)

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


class TestOptimizerMain(unittest.TestCase):
    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--trials', '10',
                       'echo', 'Performance: 123.45'])
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

        result = optimizer.main()

        self.assertEqual(result, 0)
        mock_pio.save_control.assert_called_once()
        mock_pio.restore_control.assert_called_once()

    @patch('geopmdpy.optimizer.pio')
    @patch('sys.argv', ['optimizer.py', '--metric-regex', 'test'])
    def test_main_no_launch_command(self, mock_pio):
        """Test main function with no launch command."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()

        result = optimizer.main()
        self.assertEqual(result, 1)
        mock_pio.restore_control.assert_called_once()

    @patch('geopmdpy.optimizer.pio')
    @patch('sys.argv', ['optimizer.py', '--metric-regex', 'test', 'echo', 'hello'])
    def test_main_no_control_parameters(self, mock_pio):
        """Test main function with no control parameters."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()

        result = optimizer.main()
        self.assertEqual(result, 1)

    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--application-timeout', '600',
                       '--print-stdout',
                       '--verbosity', '3',
                       'echo', 'Performance: 123.45'])
    def test_main_with_custom_options(self, mock_grid_class, mock_pio):
        """Test main function with custom timeout and print-stdout options."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()

        # Mock ControlGrid to raise an error for testing
        mock_grid_class.side_effect = ValueError("Test error")

        result = optimizer.main()
        self.assertEqual(result, 1)

    @patch('geopmdpy.optimizer.pio')
    @patch('builtins.open', mock_open())
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--output-file', 'test_output.conf',
                       'echo', 'Performance: 123.45'])
    def test_main_with_output_file(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """Test main function with output file option."""
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

        result = optimizer.main()
        self.assertEqual(result, 0)

    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--efficiency',
                       'echo', 'Performance: 123.45'])
    def test_main_with_efficiency_option(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """Test main function with --efficiency option."""
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

        result = optimizer.main()
        self.assertEqual(result, 0)
        mock_pio.save_control.assert_called_once()
        mock_pio.restore_control.assert_called_once()

    @patch('geopmdpy.optimizer.pio')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', '[invalid', 'echo', 'Performance: 123.45'])
    def test_main_invalid_metric_regex(self, mock_pio):
        """Test main with invalid metric regex pattern."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()
        result = optimizer.main()
        self.assertEqual(result, 1)
        mock_pio.restore_control.assert_called_once()

    @patch('geopmdpy.optimizer.pio')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)'])
    def test_main_empty_launch_command(self, mock_pio):
        """Test main with empty launch command."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()
        result = optimizer.main()
        self.assertEqual(result, 1)
        mock_pio.restore_control.assert_called_once()

    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--verbosity', '5',
                       'echo', 'Performance: 123.45'])
    def test_main_verbosity_option(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """Test main function with verbosity option."""
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
        result = optimizer.main()
        self.assertEqual(result, 0)

    @patch('geopmdpy.optimizer.pio')
    @patch('builtins.open', mock_open())
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--output-file', '-',
                       'echo', 'Performance: 123.45'])
    def test_main_output_file_stdout(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """Test main function with output file set to stdout."""
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
        result = optimizer.main()
        self.assertEqual(result, 0)

    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--random-seed', '999',
                       'echo', 'Performance: 123.45'])
    def test_main_random_seed_option(self, mock_optimizer_class, mock_grid_class, mock_pio):
        """Test main function with random seed option."""
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
        result = optimizer.main()
        self.assertEqual(result, 0)
        mock_optimizer.optimize.assert_called_once()
        args, kwargs = mock_optimizer.optimize.call_args
        self.assertEqual(kwargs.get('random_state'), 999)

    @patch('geopmdpy.optimizer.pio')
    @patch('geopmdpy.optimizer.ControlGrid')
    @patch('geopmdpy.optimizer.BayesianOptimizer')
    @patch('sys.argv', ['optimizer.py', '--cpu-frequency', 'package',
                       '--metric-regex', 'Performance: ([0-9.]+)',
                       '--minimize',
                       'echo', 'Performance: 123.45'])
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
        result = optimizer.main()
        self.assertEqual(result, 0)
        # Check that maximize is False in evaluator
        args, kwargs = mock_optimizer_class.call_args
        evaluator = args[1]
        self.assertFalse(evaluator.maximize)

    @patch('geopmdpy.optimizer.pio')
    @patch('sys.argv', ['optimizer.py', '--unknown-option'])
    def test_main_unknown_argument(self, mock_pio):
        """Test main with unknown argument."""
        mock_pio.save_control = MagicMock()
        mock_pio.restore_control = MagicMock()
        with self.assertRaises(SystemExit):
            optimizer.main()

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

    def test_parser_with_efficiency_option(self):
        """Test parser with --efficiency option."""
        parser = optimizer.get_parser()
        args = parser.parse_args([
            '--cpu-frequency', 'package',
            '--metric-regex', 'Performance: ([0-9.]+)',
            '--efficiency',
            'echo', 'test'
        ])
        self.assertTrue(args.efficiency)
        self.assertEqual(args.cpu_frequency_domain, 'package')
        self.assertEqual(args.metric_regex, 'Performance: ([0-9.]+)')
        self.assertEqual(args.launch, ['echo', 'test'])


if __name__ == '__main__':
    unittest.main()
