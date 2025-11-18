#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import sys
import json
import os
import math
from . import pio
from . import topo
from argparse import ArgumentParser
from typing import Optional, Union, List

"""ControlGrid class for managing geopmwrite configurations."""

# Constants that are used to define the control grid
_DEFAULT_POWER_MAX = 6000
_DEFAULT_POWER_MIN = 200
_DEFAULT_POWER_STEP = 1
_CLI_FLAG_TO_CONTROL = {
    "cpu_frequency": (
        "CPU_FREQUENCY_MAX_CONTROL",
        "CPU_FREQUENCY_MIN_AVAIL",
        "CPU_FREQUENCY_MAX_AVAIL",
        "CPU_FREQUENCY_STEP",
    ),
    "cpu_uncore_frequency": (
        "CPU_UNCORE_FREQUENCY_MAX_CONTROL",
        "CPU_FREQUENCY_MIN_AVAIL",
        "CPU_UNCORE_FREQUENCY_MAX_CONTROL", # The current limit is read dynamically and used as the maximum value because it reflects the hardware's current configuration.
        "CPU_FREQUENCY_STEP",
    ),
    "cpu_power": (
        "POWERCAP::CPU_POWER_LIMIT",
        "CPU_POWER_MIN_AVAIL",
        "CPU_POWER_LIMIT_DEFAULT",
        _DEFAULT_POWER_STEP,
    ),
    "gpu_frequency": (
        "GPU_CORE_FREQUENCY_MAX_CONTROL",
        "GPU_CORE_FREQUENCY_MIN_AVAIL",
        "GPU_CORE_FREQUENCY_MAX_AVAIL",
        "GPU_CORE_FREQUENCY_STEP",
    ),
    "gpu_power": (
        "GPU_POWER_LIMIT_CONTROL",
        ("LEVELZERO::GPU_POWER_LIMIT_MIN_AVAIL", _DEFAULT_POWER_MIN),
        ("LEVELZERO::GPU_POWER_LIMIT_DEFAULT", "GPU_POWER_LIMIT_CONTROL"),
        _DEFAULT_POWER_STEP,
    ),
    "board_power": (
        "BOARD_POWER_LIMIT_CONTROL",
        _DEFAULT_POWER_MIN,
        _DEFAULT_POWER_MAX,
        _DEFAULT_POWER_STEP,
    ),
}

def add_grid_cli_arguments(parser: ArgumentParser) -> None:
    """Register control-domain and override arguments on a parser."""
    for flag, control in _CLI_FLAG_TO_CONTROL.items():
        flag_dash = flag.replace('_', '-')
        parser.add_argument(
            f"--{flag_dash}",
            default=None,
            dest=f'{flag}_domain',
            help=f"Provide a grid over {control[0]} for the given domain.",
        )
        parser.add_argument(
            f"--{flag_dash}-min",
            type=float,
            default=None,
            dest=f'{flag}_min',
            help=f"Override the minimum value used when constructing the {control[0]} grid.",
        )
        parser.add_argument(
            f"--{flag_dash}-max",
            type=float,
            default=None,
            dest=f'{flag}_max',
            help=f"Override the maximum value used when constructing the {control[0]} grid.",
        )
        parser.add_argument(
            f"--{flag_dash}-step",
            type=float,
            default=None,
            dest=f'{flag}_step',
            help=f"Override the step size used when constructing the {control[0]} grid.",
        )

class ControlGrid:
    """
    ControlGrid class to manage control grid configurations.

    This class allows users to define a grid of control parameters for
    geopmwrite configurations. It supports various control types and
    domains, and can generate configurations based on specified
    coordinates within the grid.
    """
    def __init__(self, argv: Optional[List[str]] = None):
        """Initialize the ControlGrid with command line arguments.

        Args:
            argv (list[str], optional): List of command line arguments.
            If None, uses sys.argv[1:] by default.
        """
        if argv is None:
            argv = sys.argv[1:]
        self.control_name = []
        self.domain_type = []
        self.domain_idx = []
        self.coordinate = None
        self.coordinate_range = False
        self.parser = self._create_parser()
        self.do_write = False

        args = self.parser.parse_args(argv)
        self.range_overrides = self._collect_range_overrides(args)

        # Process all control type arguments dynamically
        for control_key in _CLI_FLAG_TO_CONTROL.keys():
            domain = getattr(args, f'{control_key}_domain', None)
            if domain is not None:
                self.add_dimension(control_key, domain)

        self.grid_data = self._get_grid_data()
        if args.coordinate is not None:
            self.coordinate = args.coordinate
        elif args.coordinate_file is not None:
            with open(args.coordinate_file) as fid:
                content = fid.read()
                self.coordinate = [int(tok) for tok in content.split()]
        elif args.coordinate_range:
            self.coordinate_range = True
        if args.write:
            if self.coordinate is None:
                raise ValueError('Either --coordinate or --coordinate-file must also be provided when using --write option')
            self.do_write = True

    def _create_parser(self) -> ArgumentParser:
        """Create the argument parser for ControlGrid.
        Returns:
            ArgumentParser: Configured argument parser.
        """
        parser = ArgumentParser(description="Define an N dimensional control grid")
        add_grid_cli_arguments(parser)
        grp = parser.add_mutually_exclusive_group()
        grp.add_argument(
            "--coordinate",
            nargs="+",
            type=int,
            help="Generate a geopmwrite configuration for a grid point",
        )
        grp.add_argument(
            "--coordinate-file",
            default=None,
            help="Generate a geopmwrite configuration for a grid point stored in file",
        )
        grp.add_argument(
            "--coordinate-range",
            action="store_true",
            default=False,
            help="Print the size of each dimension of the grid"
        )
        parser.add_argument(
            "--write",
            action="store_true",
            help="Write configuration to the platform"
        )
        return parser

    def _collect_range_overrides(self, args) -> dict:
        overrides = {}
        for control_key in _CLI_FLAG_TO_CONTROL.keys():
            min_override = getattr(args, f'{control_key}_min', None)
            max_override = getattr(args, f'{control_key}_max', None)
            step_override = getattr(args, f'{control_key}_step', None)
            control_overrides = {}
            if min_override is not None:
                control_overrides['min'] = float(min_override)
            if max_override is not None:
                control_overrides['max'] = float(max_override)
            if step_override is not None:
                if step_override <= 0:
                    raise ValueError(f"Step override for {control_key} must be positive")
                control_overrides['step'] = float(step_override)
            if control_overrides:
                overrides[control_key] = control_overrides
        return overrides

    def add_dimension(self, control_name: str, domain: Union[int, str]) -> int:
        """Add a dimension to the control grid.

        Args:
            control_name (str): Name of the control to add.
            domain (int or str): Domain type or name where the control applies.
        Returns:
            int: Index of the added dimension.
        """
        domain = topo.domain_name(domain)
        num_domain = topo.num_domain(domain)
        native_domain = pio.control_domain_type(_CLI_FLAG_TO_CONTROL[control_name][0])
        try:
            topo.domain_nested(native_domain, domain, 0)
        except RuntimeError:
            raise ValueError(f"Control {control_name} cannot be applied to domain {domain}.")
        if control_name not in _CLI_FLAG_TO_CONTROL:
            raise ValueError(f"Control {control_name} is not recognized.")
        for domain_idx in range(num_domain):
            self.control_name.append(control_name)
            self.domain_type.append(domain)
            self.domain_idx.append(domain_idx)
        return len(self.control_name) - 1

    def get_minimum(self, control_name: str, domain: str) -> float:
        """Get the minimum value for a control parameter.
        Args:
            control_name (str): Name of the control to query.
            domain (str): Name of domain where the control applies.
        Returns:
            float: Minimum value for the specified control.
        """
        return self._get_range(control_name, domain, 1)

    def get_maximum(self, control_name: str, domain: str) -> float:
        """Get the maximum value for a control parameter.
        Args:
            control_name (str): Name of the control to query.
            domain (str): Name of domain where the control applies.
        Returns:
            float: Maximum value for the specified control.
        """
        return self._get_range(control_name, domain, 2)

    def get_step(self, control_name: str) -> float:
        """Get the step size for a control parameter.
        Args:
            control_name (str): Name of the control to query.
        Returns:
            float: Step size for the specified control.
        """
        return self._get_range(control_name, 'board', 3)

    def _get_range(self, control_name: str, domain: str, index: int) -> float:
        """Get a specific range value for a control parameter.
        Args:
            control_name (str): Name of the control to query.
            domain (str): Name of domain where the control applies.
            index (int): Index of the range value to retrieve (1: min, 2: max, 3: step).
        Returns:
            float: The requested range value for the specified control.
        """
        if control_name not in _CLI_FLAG_TO_CONTROL:
            raise ValueError(f"Control {control_name} is not recognized.")
        override = self.range_overrides.get(control_name, {})
        if index == 1 and 'min' in override:
            return override['min']
        if index == 2 and 'max' in override:
            return override['max']
        if index == 3 and 'step' in override:
            return override['step']
        key = _CLI_FLAG_TO_CONTROL[control_name][index]
        return self._resolve_range_value(key, domain)

    def _resolve_range_value(self, key, domain: Union[int, str]) -> float:
        """Resolve a range value, supporting tuples of fallbacks."""
        if isinstance(key, tuple):
            last_error = None
            for candidate in key:
                try:
                    return self._resolve_range_value(candidate, domain)
                except RuntimeError as err:
                    last_error = err
                    continue
            if last_error is not None:
                raise last_error
            raise ValueError("No valid value found for range tuple")
        if isinstance(key, (int, float)):
            return key
        if isinstance(key, str):
            try:
                return pio.read_signal(key, domain, 0)
            except RuntimeError:
                return pio.read_signal(key, 0, 0)
        raise TypeError(f"Unsupported range key type: {type(key)}")

    def get_dimensions(self) -> List[tuple]:
        """Get the dimensions of the control grid.
        Returns:
            list[tuple]: A list of tuples containing control name, domain type, and domain index"""
        return zip(self.control_name, self.domain_type, self.domain_idx)

    def get_dimension_grid(self, dimension_idx: int) -> List[float]:
        """Get the grid values for a specific dimension.
        Args:
            dimension_idx (int): Index of the dimension to retrieve.
        Returns:
            list[float]: List of grid values for the specified dimension.
        """
        control = self.control_name[dimension_idx]
        domain = self.domain_type[dimension_idx]
        minimum = self.get_minimum(control, domain)
        maximum = self.get_maximum(control, domain)
        step = self.get_step(control)
        if step <= 0:
            raise ValueError(f"Grid for {control} has a non-positive step size.")
        if maximum < minimum:
            raise ValueError(f"Grid for {control} has maximum less than minimum.")
        span = maximum - minimum
        if math.isclose(span, 0.0, rel_tol=1e-9, abs_tol=1e-12):
            # Zero-span: return a single value grid
            return [minimum]
        steps_float = span / step
        steps_int = int(round(steps_float))
        if not math.isclose(steps_float, steps_int, rel_tol=1e-9, abs_tol=1e-12):
            raise ValueError(f"Grid for {control} is not evenly divisible by step size.")
        num_step = steps_int + 1
        if num_step <= 0:
            raise ValueError(f"Grid for {control} has no steps.")
        return [minimum + idx * step for idx in range(num_step)]

    def _get_grid_data(self) -> List[dict]:
        """Get the grid data for all dimensions.
        Returns:
            list[dict]: A list of dictionaries containing control, domain type, domain index, and
                        settings for each dimension.
        """
        grid_data = [
            {
                "control": _CLI_FLAG_TO_CONTROL[control][0],
                "domain": domain_type,
                "domain_idx": domain_idx,
                "settings": self.get_dimension_grid(idx)
            }
            for idx, (control, domain_type, domain_idx) in enumerate(self.get_dimensions())
        ]
        return grid_data

    def get_grid_data(self) -> List[dict]:
        return self.grid_data

    def get_json(self) -> str:
        """Get the grid data in JSON format.
        Returns:
            str: JSON string representation of the grid data.
        """
        grid_data = self.get_grid_data()
        return json.dumps(grid_data, indent=4)

    def run(self) -> str:
        """Get the standard output for the command line tool.
        Returns:
            str: Output suitable for the command line arguments provided.
        """
        if self.coordinate_range:
            return ' '.join([str(len(dim["settings"])) for dim in self.get_grid_data()])
        elif self.coordinate is None:
            return self.get_json()
        elif self.do_write:
            self.write_config()
            return ""
        else:
            return self.get_config_str()

    def get_config(self, coordinate: Optional[List[int]] = None) -> List[tuple]:
        """Get the configuration for the specified coordinate.

        If coordinate is None, it uses the instance's coordinate.
        If the coordinate is not provided or does not match the grid dimensions,
        it raises a ValueError.
        Args:
            coordinate (list[int], optional): List of indices for each control dimension.
        Returns:
            list[tuple]: A list of tuples containing control name, domain type, domain index,
                         and settings for each dimension.
        """
        if coordinate is None:
            coordinate = self.coordinate
        if len(self.control_name) == 0:
            raise RuntimeError(f"Called get_config(), but ControlGrid has not been configured")
        if coordinate is None or len(coordinate) != len(self.control_name):
            raise ValueError(f"Input coordinate not correctly sized, must be length {len(self.control_name)}")
        result = []
        for idx, dim in enumerate(self.get_grid_data()):
            result.append((dim["control"], dim["domain"], dim["domain_idx"], dim["settings"][coordinate[idx]]))
            min_control = dim["control"].replace("_MAX_", "_MIN_")
            if min_control != dim["control"] and min_control in pio.control_names() and min_control != "CPU_FREQUENCY_MIN_CONTROL":
                result.append((min_control, dim["domain"], dim["domain_idx"], dim["settings"][coordinate[idx]]))
        return result

    def get_config_str(self, coordinate: Optional[List[int]] = None):
        """Get the geopmwrite configuration string for the specified coordinate.

        If coordinate is None, it uses the instance's coordinate.
        Args:
            coordinate (list[int], optional): List of indices for each control dimension.
        Returns:
            str: A string representation of the configuration commands to configure geopmwrite.
        """
        config = self.get_config(coordinate)
        result = []
        for cmd in config:
            cmd = [str(cc) for cc in cmd]
            result.append(' '.join(cmd))
        return '\n'.join(result)

    def write_config(self, coordinate=None):
        config = self.get_config(coordinate)
        handles = []
        for cmd in config:
            handles.append(pio.push_control(*cmd[:3]))
        for idx, control in enumerate(config):
            pio.adjust(handles[idx], control[3])
        pio.write_batch()

def main():
    """Main function to run the ControlGrid command line tool.

    This function initializes the ControlGrid with command line arguments
    and prints the output based on the provided options.
    It handles exceptions and prints error messages if any issues arise.

    Returns:
        int: Exit code, 0 for success, 1 for failure.
    """
    try:
        grid = ControlGrid(sys.argv[1:])
        stdout = grid.run()
        if stdout:
            print(stdout)
    except Exception as e:
        if "GEOPM_DEBUG" in os.environ:
            raise
        print(f"Error: {e}")
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())
