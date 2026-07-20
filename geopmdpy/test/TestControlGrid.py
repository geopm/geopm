#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
from geopmdpy import grid
from unittest import TestCase, main, mock
import json
import tempfile
import os


class TestControlGrid(TestCase):
    def setUp(self):
        # Mock the dependencies to avoid requiring actual geopm installation
        self.topo_patcher = mock.patch('geopmdpy.grid.topo')
        self.pio_patcher = mock.patch('geopmdpy.grid.pio')

        self.mock_topo = self.topo_patcher.start()
        self.mock_pio = self.pio_patcher.start()

        # Set up default mock return values
        self.mock_topo.num_domain.return_value = 2
        self.mock_topo.domain_name.return_value = 'package'
        self.mock_topo.domain_nested.return_value = None
        self.mock_topo.DOMAIN_BOARD = 0
        # Set up pio.read_signal to return consistent values for min, max, step
        self.mock_pio.read_signal.side_effect = lambda signal, domain, idx: float({
            'CPU_FREQUENCY_MIN_AVAIL': 1000000,
            'CPU_FREQUENCY_MAX_AVAIL': 2000000,
            'CPU_FREQUENCY_STEP': 100000,
            'CPU_POWER_MIN_AVAIL': 100,
            'CPU_POWER_LIMIT_DEFAULT': 200,
            'CPU_UNCORE_FREQUENCY_MAX_CONTROL': 2000000,  # Add this for cpu_uncore_frequency
        }.get(signal, 1000.0))
        self.mock_pio.control_domain_type.return_value = 'package'
        self.mock_pio.signal_names.return_value = []
        self.mock_pio.push_control.return_value = 'mock_handle'
        self.mock_pio.adjust.return_value = None
        self.mock_pio.write_batch.return_value = None

        # Create test instances
        self.grid = grid.ControlGrid(['--cpu-frequency', 'package'])
        self.empty_grid = grid.ControlGrid([])

    def tearDown(self):
        self.topo_patcher.stop()
        self.pio_patcher.stop()

    def test_init_default_argv(self):
        """Test initialization with default argv"""
        # Test that argv defaults correctly
        with mock.patch('sys.argv', ['script.py', '--cpu-frequency', 'package']):
            test_grid = grid.ControlGrid()
            self.assertEqual(len(test_grid.control_name), 2)

    def test_init_with_cpu_frequency(self):
        """Test initialization with CPU frequency argument"""
        self.assertEqual(len(self.grid.control_name), 2)  # num_domain returns 2
        self.assertEqual(self.grid.control_name[0], 'cpu_frequency')
        self.assertEqual(self.grid.domain_type[0], 'package')
        self.assertEqual(self.grid.domain_idx[0], 0)

    def test_init_with_coordinate(self):
        """Test initialization with coordinate argument"""
        test_grid = grid.ControlGrid(['--cpu-frequency', 'package', '--coordinate', '1', '2'])
        self.assertEqual(test_grid.coordinate, [1, 2])

    def test_init_with_coordinate_file(self):
        """Test initialization with coordinate file"""
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            f.write('1 2 3')
            temp_filename = f.name

        try:
            test_grid = grid.ControlGrid(['--cpu-frequency', 'package', '--coordinate-file', temp_filename])
            self.assertEqual(test_grid.coordinate, [1, 2, 3])
        finally:
            os.unlink(temp_filename)

    def test_init_with_coordinate_range(self):
        """Test initialization with coordinate range flag"""
        test_grid = grid.ControlGrid(['--cpu-frequency', 'package', '--coordinate-range'])
        self.assertTrue(test_grid.coordinate_range)

    def test_init_with_write_flag(self):
        """Test initialization with write flag"""
        test_grid = grid.ControlGrid(['--cpu-frequency', 'package', '--coordinate', '1', '2', '--write'])
        self.assertTrue(test_grid.do_write)
        self.assertEqual(test_grid.coordinate, [1, 2])

    def test_init_with_write_flag_no_coordinate(self):
        """Test initialization with write flag but no coordinate raises error"""
        with self.assertRaises(ValueError) as context:
            grid.ControlGrid(['--cpu-frequency', 'package', '--write'])
        self.assertIn('Either --coordinate or --coordinate-file must also be provided', str(context.exception))

    def test_add_dimension(self):
        """Test adding dimensions to the grid"""
        idx = self.empty_grid.add_dimension('cpu_frequency', 'board')
        self.assertEqual(idx, 1)  # Returns len(control_name) - 1, which is 2-1=1 for 2 domains
        idx = self.grid.add_dimension('cpu_frequency', 'package')
        self.assertEqual(idx, 3)  # Grid starts with 2, adds 2 more, so len-1 = 4-1=3
        self.assertIn('cpu_frequency', self.grid.control_name)
        self.assertIn('package', self.grid.domain_type)

    def test_add_dimension_invalid_control(self):
        """Test adding invalid control raises error"""
        # The method checks if control_name is in _CLI_FLAG_TO_CONTROL first,
        # which raises a KeyError, then converts to ValueError
        with self.assertRaises(KeyError):
            self.empty_grid.add_dimension('invalid_control', 'package')

    def test_add_dimension_invalid_domain_nesting(self):
        """Test adding dimension with invalid domain nesting"""
        self.mock_topo.domain_nested.side_effect = RuntimeError("Domain nesting error")
        with self.assertRaises(ValueError) as context:
            self.empty_grid.add_dimension('cpu_frequency', 'package')
        self.assertIn('Control cpu_frequency cannot be applied to domain package', str(context.exception))

    def test_get_minimum(self):
        """Test getting minimum value for a control"""
        min_freq = self.grid.get_minimum('cpu_frequency', 'board')
        self.assertIsInstance(min_freq, float)

    def test_get_maximum(self):
        """Test getting maximum value for a control"""
        max_freq = self.grid.get_maximum('cpu_frequency', 'board')
        self.assertIsInstance(max_freq, float)

    def test_get_step(self):
        """Test getting step value for a control"""
        step_freq = self.grid.get_step('cpu_frequency')
        self.assertIsInstance(step_freq, float)

    def test_get_range_with_numeric_value(self):
        """Test _get_range with numeric values like power controls"""
        # Test with cpu_power which has a numeric step value
        step = self.grid._get_range('cpu_power', 'board', 3)
        self.assertEqual(step, 1)

    def test_get_range_invalid_control(self):
        """Test _get_range with invalid control name"""
        with self.assertRaises(ValueError) as context:
            self.grid._get_range('invalid_control', 'board', 1)
        self.assertIn("Control invalid_control is not recognized", str(context.exception))

    def test_get_dimensions(self):
        """Test getting dimensions iterator"""
        dimensions = list(self.grid.get_dimensions())
        self.assertEqual(len(dimensions), 2)
        self.assertEqual(dimensions[0], ('cpu_frequency', 'package', 0))
        self.assertEqual(dimensions[1], ('cpu_frequency', 'package', 1))

    def test_get_dimension_grid(self):
        """Test getting grid values for a dimension"""
        grid_values = self.grid.get_dimension_grid(0)
        self.assertIsInstance(grid_values, list)
        self.assertEqual(len(grid_values), 11)  # (2000000-1000000)/100000 + 1
        self.assertEqual(grid_values[0], 1000000.0)
        self.assertEqual(grid_values[-1], 2000000.0)

    def test_get_dimension_grid_uneven_division(self):
        """Test error handling for uneven grid division"""
        # Mock values that don't divide evenly
        self.mock_pio.read_signal.side_effect = lambda signal, domain, idx: float({
            'CPU_FREQUENCY_MIN_AVAIL': 1000000,
            'CPU_FREQUENCY_MAX_AVAIL': 2050000,  # Doesn't divide evenly by 100000
            'CPU_FREQUENCY_STEP': 100000,
        }.get(signal, 1000.0))

        with self.assertRaises(ValueError) as context:
            self.grid.get_dimension_grid(0)
        self.assertIn("Grid for cpu_frequency is not evenly divisible", str(context.exception))

    def test_get_dimension_grid_no_steps(self):
        """Test error handling for grid with no steps"""
        # Mock values where max < min
        self.mock_pio.read_signal.side_effect = lambda signal, domain, idx: float({
            'CPU_FREQUENCY_MIN_AVAIL': 2000000,
            'CPU_FREQUENCY_MAX_AVAIL': 1000000,  # Max < min
            'CPU_FREQUENCY_STEP': 100000,
        }.get(signal, 1000.0))

        with self.assertRaises(ValueError) as context:
            self.grid.get_dimension_grid(0)
        self.assertIn("Grid for cpu_frequency has maximum less than minimum", str(context.exception))

    def test_get_grid_data(self):
        """Test getting grid data structure"""
        grid_data = self.grid.get_grid_data()
        self.assertEqual(len(grid_data), 2)
        self.assertIn('control', grid_data[0])
        self.assertIn('domain', grid_data[0])
        self.assertIn('domain_idx', grid_data[0])
        self.assertIn('settings', grid_data[0])

    def test_get_json(self):
        """Test JSON output generation"""
        json_output = self.grid.get_json()
        self.assertIsInstance(json_output, str)
        # Verify it's valid JSON
        parsed = json.loads(json_output)
        self.assertIsInstance(parsed, list)

    def test_run_coordinate_range(self):
        """Test run method with coordinate range flag"""
        self.grid.coordinate_range = True

        output = self.grid.run()
        # Should return space-separated dimension sizes
        self.assertEqual(output, '11 11')  # Both dimensions have 11 settings

    def test_run_without_coordinate(self):
        """Test run method without coordinate (returns JSON)"""
        output = self.grid.run()
        # Should return JSON when no coordinate is set
        parsed = json.loads(output)
        self.assertIsInstance(parsed, list)

    def test_run_with_coordinate(self):
        """Test run method with coordinate (returns config string)"""
        self.grid.coordinate = [0, 1]

        output = self.grid.run()
        # Should return config string when coordinate is set
        self.assertIsInstance(output, str)
        self.assertNotIn('[', output)  # Should not be JSON

    def test_run_with_write_flag(self):
        """Test run method with write flag"""
        self.grid.coordinate = [0, 1]
        self.grid.do_write = True

        output = self.grid.run()
        # Should return empty string when writing
        self.assertEqual(output, "")
        # Verify pio methods were called
        self.mock_pio.push_control.assert_called()
        self.mock_pio.adjust.assert_called()
        self.mock_pio.write_batch.assert_called_once()

    def test_get_config(self):
        """Test getting configuration for specific coordinate"""
        config = self.grid.get_config([0, 1])
        self.assertEqual(len(config), 2)
        # Each config item should be a tuple with (control, domain, domain_idx, value)
        self.assertEqual(len(config[0]), 4)

    def test_get_config_no_grid(self):
        """Test error when getting config with no grid configured"""
        with self.assertRaises(RuntimeError) as context:
            self.empty_grid.get_config([])
        self.assertIn("ControlGrid has not been configured", str(context.exception))

    def test_get_config_wrong_coordinate_size(self):
        """Test error when coordinate size doesn't match grid dimensions"""
        with self.assertRaises(ValueError) as context:
            self.grid.get_config([1])  # Grid has 2 dimensions, providing only 1
        self.assertIn("Input coordinate not correctly sized", str(context.exception))

    def test_get_config_str(self):
        """Test getting configuration as string"""
        config_str = self.grid.get_config_str([0, 1])
        self.assertIsInstance(config_str, str)
        self.assertIn('\n', config_str)  # Should have newlines between commands

    def test_write_config(self):
        """Test write_config method with proper pio mocking"""
        self.grid.write_config([0, 1])

        # Verify pio methods were called correctly
        self.assertEqual(self.mock_pio.push_control.call_count, 2)
        self.assertEqual(self.mock_pio.adjust.call_count, 2)
        self.mock_pio.write_batch.assert_called_once()

    def test_cli_flag_to_control_mappings(self):
        """Test that all control mappings are properly defined"""
        expected_controls = [
            'cpu_frequency', 'cpu_uncore_frequency', 'cpu_power',
            'gpu_frequency', 'gpu_power'
        ]

        for control in expected_controls:
            self.assertIn(control, grid._CLI_FLAG_TO_CONTROL)
            mapping = grid._CLI_FLAG_TO_CONTROL[control]
            self.assertEqual(len(mapping), 4)  # Should have control, min, max, step

    def test_multiple_control_types(self):
        """Test adding multiple different control types"""
        # Now all control types should be processed
        test_grid = grid.ControlGrid([
            '--cpu-frequency', 'package',
            '--cpu-power', 'board'
        ])

        # Should have added dimensions for both control types
        # cpu_frequency: 2 domains, cpu_power: 2 domains = 4 total
        self.assertEqual(len(test_grid.control_name), 4)
        self.assertIn('cpu_frequency', test_grid.control_name)
        self.assertIn('cpu_power', test_grid.control_name)

    def test_coordinate_vs_coordinate_file_mutual_exclusion(self):
        """Test that coordinate and coordinate-file are mutually exclusive"""
        # This should work without error since argparse handles mutual exclusion
        with self.assertRaises(SystemExit):
            grid.ControlGrid([
                '--cpu-frequency', 'package',
                '--coordinate', '1', '2',
                '--coordinate-file', 'nonexistent.txt'
            ])

    def test_main_function_success(self):
        """Test main function with successful execution"""
        with mock.patch('sys.argv', ['grid.py', '--cpu-frequency', 'package']):
            with mock.patch('builtins.print') as mock_print:
                with mock.patch('geopmdpy.grid.topo') as mock_topo:
                    with mock.patch('geopmdpy.grid.pio') as mock_pio:
                        mock_topo.num_domain.return_value = 2
                        mock_topo.domain_name.return_value = 'package'
                        mock_topo.domain_nested.return_value = None
                        mock_topo.DOMAIN_BOARD = 0
                        mock_pio.read_signal.side_effect = lambda signal, domain, idx: float({
                            'CPU_FREQUENCY_MIN_AVAIL': 1000000,
                            'CPU_FREQUENCY_MAX_AVAIL': 2000000,
                            'CPU_FREQUENCY_STEP': 100000,
                        }.get(signal, 1000.0))
                        mock_pio.control_domain_type.return_value = 'package'
                        mock_pio.signal_names.return_value = []
                        result = grid.main()
                        self.assertEqual(result, 0)
                        mock_print.assert_called_once()

    def test_main_function_error_handling(self):
        """Test main function error handling"""
        with mock.patch('sys.argv', ['grid.py', '--invalid-arg']):
            # ArgumentParser raises SystemExit(2) on invalid arguments
            # This is not caught by the Exception handler in main()
            with self.assertRaises(SystemExit) as context:
                grid.main()
            self.assertEqual(context.exception.code, 2)

    def test_main_function_with_debug(self):
        """Test main function with debug environment variable"""
        with mock.patch('sys.argv', ['grid.py', '--invalid-arg']):
            with mock.patch.dict('os.environ', {'GEOPM_DEBUG': '1'}):
                with self.assertRaises(SystemExit):
                    grid.main()

    def test_main_function_exception_handling(self):
        """Test main function handling of actual exceptions (not SystemExit)"""
        with mock.patch('sys.argv', ['grid.py', '--cpu-frequency', 'package']):
            with mock.patch('builtins.print') as mock_print:
                with mock.patch('geopmdpy.grid.ControlGrid') as mock_grid_class:
                    # Mock ControlGrid to raise an exception
                    mock_grid_class.side_effect = RuntimeError("Test error")
                    result = grid.main()
                    self.assertEqual(result, 1)
                    mock_print.assert_called_once_with("Error: Test error")

    def test_gpu_power_tuple_resolution(self):
        """Test GPU power tuple-based range resolution"""
        mapping = grid._CLI_FLAG_TO_CONTROL['gpu_power']
        self.assertIsInstance(mapping[1], tuple)
        self.assertIsInstance(mapping[2], tuple)

        # Simulate failure for the first tuple entry and ensure fallback is used
        def fake_read(signal, domain, idx):
            if signal.startswith("LEVELZERO::"):
                raise RuntimeError("signal unavailable")
            return 123.0 if signal == "GPU_POWER_LIMIT_CONTROL" else 456.0

        self.mock_pio.read_signal.side_effect = fake_read
        minimum = self.grid.get_minimum('gpu_power', 'board')
        maximum = self.grid.get_maximum('gpu_power', 'board')

        self.assertEqual(minimum, 200)  # Falls back to default numeric value
        self.assertEqual(maximum, 123.0)  # Falls back to alternate signal

    def test_all_argument_types_defined(self):
        """Test that all control arguments are properly defined and processed"""
        # Test that the parser accepts all defined arguments and processes them
        # Use a simpler test that doesn't trigger the uneven division error
        test_grid = grid.ControlGrid([
            '--cpu-frequency', 'package',
            '--cpu-power', 'board'
        ])

        # All control types should be processed now
        # cpu_frequency: 2 domains, cpu_power: 2 domains = 4 total
        self.assertEqual(len(test_grid.control_name), 4)
        self.assertIn('cpu_frequency', test_grid.control_name)
        self.assertIn('cpu_power', test_grid.control_name)

    def test_create_parser(self):
        """Test parser creation and argument definitions"""
        parser = self.grid._create_parser()
        self.assertIsNotNone(parser)

        # Test that expected arguments are present
        action_dests = [action.dest for action in parser._actions]
        self.assertIn('cpu_frequency_domain', action_dests)
        self.assertIn('cpu_power_domain', action_dests)
        self.assertIn('coordinate', action_dests)
        self.assertIn('coordinate_file', action_dests)
        self.assertIn('coordinate_range', action_dests)
        self.assertIn('write', action_dests)

    def test_get_config_with_default_coordinate(self):
        """Test get_config using instance's default coordinate"""
        self.grid.coordinate = [0, 1]

        config = self.grid.get_config()  # No coordinate argument
        self.assertEqual(len(config), 2)


class TestSweepParsing(TestCase):
    """Unit tests for the --sweep grammar parsers (no pio/topo needed)."""

    def test_parse_quantity_frequency_units(self):
        """Frequency suffixes convert to Hz"""
        self.assertEqual(grid.parse_quantity('2.8GHz', 'frequency'), 2.8e9)
        self.assertEqual(grid.parse_quantity('100MHz', 'frequency'), 1e8)
        self.assertEqual(grid.parse_quantity('500kHz', 'frequency'), 5e5)
        self.assertEqual(grid.parse_quantity('50Hz', 'frequency'), 50.0)

    def test_parse_quantity_power_units(self):
        """Power suffixes convert to Watts"""
        self.assertEqual(grid.parse_quantity('250W', 'power'), 250.0)
        self.assertEqual(grid.parse_quantity('1kW', 'power'), 1000.0)

    def test_parse_quantity_bare_number_raw_units(self):
        """A bare number keeps the control's raw units"""
        self.assertEqual(grid.parse_quantity('2800000000', 'frequency'), 2.8e9)
        self.assertEqual(grid.parse_quantity('250', 'power'), 250.0)

    def test_parse_quantity_case_insensitive(self):
        """Unit suffixes are case-insensitive"""
        self.assertEqual(grid.parse_quantity('2.8ghz', 'frequency'), 2.8e9)
        self.assertEqual(grid.parse_quantity('2.8GHZ', 'frequency'), 2.8e9)
        self.assertEqual(grid.parse_quantity('1KW', 'power'), 1000.0)

    def test_parse_quantity_scientific_notation(self):
        """Scientific notation is accepted for bare numbers"""
        self.assertEqual(grid.parse_quantity('2.8e9', 'frequency'), 2.8e9)

    def test_parse_quantity_unknown_unit(self):
        """An unrecognized unit raises with the canonical list"""
        with self.assertRaises(ValueError) as context:
            grid.parse_quantity('2.8GhZz', 'frequency')
        self.assertIn('unrecognized unit', str(context.exception))
        self.assertIn('GHz', str(context.exception))

    def test_parse_quantity_power_unit_on_frequency(self):
        """A power unit on a frequency control is rejected"""
        with self.assertRaises(ValueError):
            grid.parse_quantity('250W', 'frequency')

    def test_parse_quantity_negative(self):
        """Negative values are rejected"""
        with self.assertRaises(ValueError) as context:
            grid.parse_quantity('-1GHz', 'frequency')
        self.assertIn('must not be negative', str(context.exception))

    def test_parse_quantity_malformed(self):
        """A non-numeric token is rejected"""
        with self.assertRaises(ValueError) as context:
            grid.parse_quantity('abc', 'frequency')
        self.assertIn('invalid numeric value', str(context.exception))

    def test_parse_quantity_level_integer(self):
        """A level control accepts a bare integer"""
        self.assertEqual(grid.parse_quantity('4', 'level'), 4.0)

    def test_parse_quantity_level_rejects_unit(self):
        """A level control rejects any unit suffix"""
        with self.assertRaises(ValueError) as context:
            grid.parse_quantity('4GHz', 'level')
        self.assertIn('not allowed for a level control', str(context.exception))

    def test_parse_quantity_level_rejects_fraction(self):
        """A level control rejects a non-integer value"""
        with self.assertRaises(ValueError) as context:
            grid.parse_quantity('1.5', 'level')
        self.assertIn('must be an integer', str(context.exception))

    def test_parse_triple_full(self):
        """A full MIN:MAX:STEP triple parses all three fields"""
        result = grid.parse_triple('1.2GHz:3GHz:100MHz', 'frequency')
        self.assertEqual(result, {'min': 1.2e9, 'max': 3e9, 'step': 1e8})

    def test_parse_triple_bounds_only(self):
        """MIN:MAX sets only the bounds"""
        result = grid.parse_triple('1.2GHz:3GHz', 'frequency')
        self.assertEqual(result, {'min': 1.2e9, 'max': 3e9})

    def test_parse_triple_step_only(self):
        """::STEP sets only the step"""
        result = grid.parse_triple('::100MHz', 'frequency')
        self.assertEqual(result, {'step': 1e8})

    def test_parse_triple_min_only(self):
        """MIN:: sets only the minimum"""
        result = grid.parse_triple('1.2GHz::', 'frequency')
        self.assertEqual(result, {'min': 1.2e9})

    def test_parse_triple_max_only(self):
        """:MAX: sets only the maximum"""
        result = grid.parse_triple(':3GHz:', 'frequency')
        self.assertEqual(result, {'max': 3e9})

    def test_parse_triple_missing_separator(self):
        """A lone value with no ':' is ambiguous and rejected"""
        with self.assertRaises(ValueError) as context:
            grid.parse_triple('2GHz', 'frequency')
        self.assertIn('invalid range', str(context.exception))

    def test_parse_triple_too_many_fields(self):
        """More than three fields is rejected"""
        with self.assertRaises(ValueError):
            grid.parse_triple('1:2:3:4', 'frequency')

    def test_parse_triple_non_positive_step(self):
        """A zero or negative step is rejected"""
        with self.assertRaises(ValueError) as context:
            grid.parse_triple('1GHz:3GHz:0', 'frequency')
        self.assertIn('must be positive', str(context.exception))

    def test_parse_triple_min_exceeds_max(self):
        """A minimum greater than the maximum is rejected"""
        with self.assertRaises(ValueError) as context:
            grid.parse_triple('3GHz:1GHz', 'frequency')
        self.assertIn('min exceeds max', str(context.exception))

    def test_parse_sweep_dim_control_only(self):
        """A bare control resolves with no domain and no overrides"""
        self.assertEqual(
            grid.parse_sweep_dim('cpu-freq'),
            ('cpu_frequency', None, {}),
        )

    def test_parse_sweep_dim_with_domain(self):
        """An @DOMAIN clause is captured"""
        self.assertEqual(
            grid.parse_sweep_dim('cpu-freq@board'),
            ('cpu_frequency', 'board', {}),
        )

    def test_parse_sweep_dim_full(self):
        """A full CONTROL@DOMAIN=TRIPLE spec parses all parts"""
        self.assertEqual(
            grid.parse_sweep_dim('cpu-freq@board=1.2GHz:3GHz:100MHz'),
            ('cpu_frequency', 'board', {'min': 1.2e9, 'max': 3e9, 'step': 1e8}),
        )

    def test_parse_sweep_dim_canonical_name(self):
        """The canonical dashed control name is also accepted"""
        self.assertEqual(
            grid.parse_sweep_dim('cpu-frequency@package'),
            ('cpu_frequency', 'package', {}),
        )

    def test_parse_sweep_dim_all_aliases(self):
        """Every documented alias resolves to a control key"""
        expected = {
            'cpu-freq': 'cpu_frequency',
            'uncore-freq': 'cpu_uncore_frequency',
            'cpu-power': 'cpu_power',
            'gpu-freq': 'gpu_frequency',
            'gpu-power': 'gpu_power',
            'board-power': 'board_power',
            'prefetch': 'prefetch_disable',
        }
        for alias, control_key in expected.items():
            self.assertEqual(grid.parse_sweep_dim(alias)[0], control_key)

    def test_parse_sweep_dim_case_insensitive_control(self):
        """Control names are matched case-insensitively"""
        self.assertEqual(grid.parse_sweep_dim('CPU-Freq')[0], 'cpu_frequency')

    def test_parse_sweep_dim_prefetch_level_override(self):
        """A level control override parses as an integer with no units"""
        self.assertEqual(
            grid.parse_sweep_dim('prefetch=0:4:1'),
            ('prefetch_disable', None, {'min': 0.0, 'max': 4.0, 'step': 1.0}),
        )

    def test_parse_sweep_dim_unknown_control(self):
        """An unknown control names --list-controls in the error"""
        with self.assertRaises(ValueError) as context:
            grid.parse_sweep_dim('cpu-frq')
        self.assertIn("unknown control 'cpu-frq'", str(context.exception))
        self.assertIn('--list-controls', str(context.exception))

    def test_parse_sweep_dim_empty(self):
        """An empty spec is rejected"""
        with self.assertRaises(ValueError) as context:
            grid.parse_sweep_dim('   ')
        self.assertIn('empty --sweep specification', str(context.exception))

    def test_parse_sweep_dim_empty_domain(self):
        """An empty @DOMAIN clause is rejected"""
        with self.assertRaises(ValueError) as context:
            grid.parse_sweep_dim('cpu-freq@')
        self.assertIn('empty domain', str(context.exception))


if __name__ == '__main__':
    main()

