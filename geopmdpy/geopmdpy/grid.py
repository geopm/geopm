#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import sys
import json
import os
import math
import re
from . import pio
from . import topo
from argparse import ArgumentParser
from typing import Optional, Union, List, Tuple

"""ControlGrid class for managing geopmwrite configurations."""

# Constants that are used to define the control grid
_DEFAULT_POWER_MAX = 6000
_DEFAULT_POWER_MIN = 200
_DEFAULT_POWER_STEP = 1
_PREFETCHER_CONTROL_SEQUENCE = (
    "MSR::MISC_FEATURE_CONTROL:DCU_HW_PREFETCHER_DISABLE",
    "MSR::MISC_FEATURE_CONTROL:L2_HW_PREFETCHER_DISABLE",
    "MSR::MISC_FEATURE_CONTROL:DCU_IP_PREFETCHER_DISABLE",
    "MSR::MISC_FEATURE_CONTROL:L2_ADJACENT_PREFETCHER_DISABLE",
)
# _MAX_PREFETCH_DISABLE_LEVEL represents the number of available prefetchers.
#  Selecting the max value disables all prefetchers.
_MAX_PREFETCH_DISABLE_LEVEL = len(_PREFETCHER_CONTROL_SEQUENCE)
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
    "prefetch_disable": (
        "prefetch_disable",
        0,
        _MAX_PREFETCH_DISABLE_LEVEL,
        1,
    ),
}

# Mapping of user-facing --sweep control names (short aliases and the canonical
# dashed spellings) to the _CLI_FLAG_TO_CONTROL keys.
_CONTROL_ALIASES = {
    "cpu-freq": "cpu_frequency",
    "cpu-frequency": "cpu_frequency",
    "uncore-freq": "cpu_uncore_frequency",
    "cpu-uncore-frequency": "cpu_uncore_frequency",
    "cpu-power": "cpu_power",
    "gpu-freq": "gpu_frequency",
    "gpu-frequency": "gpu_frequency",
    "gpu-power": "gpu_power",
    "board-power": "board_power",
    "prefetch": "prefetch_disable",
    "prefetch-disable": "prefetch_disable",
}
# Category of each control, which selects the allowed unit suffixes.
_CONTROL_CATEGORY = {
    "cpu_frequency": "frequency",
    "cpu_uncore_frequency": "frequency",
    "cpu_power": "power",
    "gpu_frequency": "frequency",
    "gpu_power": "power",
    "board_power": "power",
    "prefetch_disable": "level",
}
# Unit suffix (lower-cased) to raw-unit multiplier for each category.
_UNIT_TABLE = {
    "frequency": {"hz": 1.0, "khz": 1e3, "mhz": 1e6, "ghz": 1e9},
    "power": {"w": 1.0, "kw": 1e3},
}
# Canonical unit spellings for each category, used in error messages.
_UNIT_CANONICAL = {
    "frequency": ("Hz", "kHz", "MHz", "GHz"),
    "power": ("W", "kW"),
}
# Preferred short alias for each canonical control key, used by --list-controls.
_PREFERRED_ALIAS = {
    "cpu_frequency": "cpu-freq",
    "cpu_uncore_frequency": "uncore-freq",
    "cpu_power": "cpu-power",
    "gpu_frequency": "gpu-freq",
    "gpu_power": "gpu-power",
    "board_power": "board-power",
    "prefetch_disable": "prefetch",
}

# Matches a numeric value with an optional sign and alphabetic unit suffix.
_QUANTITY_RE = re.compile(
    r"^\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][+-]?[0-9]+)?)\s*([A-Za-z]*)\s*$"
)


def parse_quantity(text: str, category: str) -> float:
    """Parse a numeric grid value with an optional unit suffix.

    Args:
        text: The value token, for example "2.8GHz", "250W", or "4".
        category: The control category ("frequency", "power", or "level")
            that determines which unit suffixes are accepted.

    Returns:
        float: The value converted to the control's raw unit (Hz for
        frequency, Watts for power, and an integer level for "level").

    Raises:
        ValueError: If the token is malformed, negative, or carries an
            unrecognized or disallowed unit suffix.
    """
    match = _QUANTITY_RE.match(text)
    if match is None:
        raise ValueError(f"invalid numeric value '{text}'")
    number, suffix = match.group(1), match.group(2)
    value = float(number)
    if value < 0:
        raise ValueError(f"value '{text}' must not be negative")
    if category == "level":
        if suffix:
            raise ValueError(
                f"unit '{suffix}' is not allowed for a level control"
            )
        if not value.is_integer():
            raise ValueError(f"level value '{text}' must be an integer")
        return value
    units = _UNIT_TABLE.get(category)
    if units is None:
        raise ValueError(f"no units defined for category '{category}'")
    if not suffix:
        return value
    multiplier = units.get(suffix.lower())
    if multiplier is None:
        canonical = ", ".join(_UNIT_CANONICAL[category])
        raise ValueError(
            f"unrecognized unit '{suffix}'; use one of {canonical}"
        )
    return value * multiplier


def parse_triple(text: str, category: str) -> dict:
    """Parse a MIN:MAX:STEP override triple using slice semantics.

    Any field may be omitted: "1.2GHz:3GHz" sets only the bounds, "::100MHz"
    sets only the step, and "1.2GHz::" sets only the minimum. At least one ':'
    separator must be present so a lone value is rejected as ambiguous.

    Args:
        text: The triple token following '=' in a --sweep specification.
        category: The control category used to parse each field's units.

    Returns:
        dict: A dictionary containing only the supplied 'min', 'max', and
        'step' keys mapped to their parsed float values.

    Raises:
        ValueError: If the triple has the wrong number of fields, a
            non-positive step, or a minimum that exceeds the maximum.
    """
    parts = text.split(":")
    if len(parts) < 2 or len(parts) > 3:
        raise ValueError(
            f"invalid range '{text}'; expected MIN:MAX[:STEP] with ':' separators"
        )
    overrides = {}
    for key, part in zip(("min", "max", "step"), parts):
        part = part.strip()
        if part:
            overrides[key] = parse_quantity(part, category)
    if "step" in overrides and overrides["step"] <= 0:
        raise ValueError(f"step in '{text}' must be positive")
    if "min" in overrides and "max" in overrides and overrides["min"] > overrides["max"]:
        raise ValueError(f"min exceeds max in '{text}'")
    return overrides


def parse_sweep_dim(spec: str) -> Tuple[str, Optional[str], dict]:
    """Parse a --sweep specification: CONTROL[@DOMAIN][=MIN:MAX:STEP].

    Args:
        spec: A single --sweep token, for example "cpu-freq",
            "cpu-freq@board", or "cpu-freq@board=1.2GHz:3GHz:100MHz".

    Returns:
        Tuple[str, Optional[str], dict]: The resolved _CLI_FLAG_TO_CONTROL
        key, the domain string (or None when '@DOMAIN' was omitted), and a
        dictionary of any 'min'/'max'/'step' overrides.

    Raises:
        ValueError: If the specification is empty, names an unknown control,
            or has an empty domain.
    """
    text = spec.strip()
    if not text:
        raise ValueError("empty --sweep specification")
    control_part = text
    triple = None
    if "=" in control_part:
        control_part, triple = control_part.split("=", 1)
    domain = None
    if "@" in control_part:
        control_part, domain = control_part.split("@", 1)
        domain = domain.strip()
        if not domain:
            raise ValueError(f"empty domain in --sweep '{spec}'")
    alias = control_part.strip()
    control_key = _CONTROL_ALIASES.get(alias.lower())
    if control_key is None:
        raise ValueError(f"unknown control '{alias}'; see --list-controls")
    overrides = {}
    if triple is not None:
        overrides = parse_triple(triple, _CONTROL_CATEGORY[control_key])
    return control_key, domain, overrides


# Human-readable unit label per category for the --list-controls table.
_CATEGORY_DISPLAY_UNIT = {
    "frequency": "Hz",
    "power": "W",
    "level": "level",
}


def native_domain(control_key: str) -> str:
    """Resolve the native domain name for a control.

    Args:
        control_key: A key from _CLI_FLAG_TO_CONTROL.

    Returns:
        str: The name of the domain on which the control natively applies,
        used when a --sweep specification omits an explicit '@DOMAIN'.
    """
    signal_key = _CLI_FLAG_TO_CONTROL[control_key][0]
    if control_key == "prefetch_disable":
        signal_key = _PREFETCHER_CONTROL_SEQUENCE[0]
    return topo.domain_name(pio.control_domain_type(signal_key))


def _format_range_value(getter) -> str:
    """Format a range value for display, substituting 'n/a' on failure.

    Args:
        getter: A zero-argument callable returning a numeric range value.

    Returns:
        str: The value formatted with '%g', or 'n/a' if the getter raised
        (for example when the underlying signal is unavailable).
    """
    try:
        return f"{getter():g}"
    except Exception:
        return "n/a"


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

def prefetch_settings(level: int) -> List[Tuple[str, int]]:
    """Derive MSR disable values for a prefetch optimization level.

    Args:
        level: Desired prefetch disable level produced by the grid. Higher
            values disable deeper layers of hardware prefetchers.

    Returns:
        List[Tuple[str, int]]: Ordered pairs mapping each prefetch MSR control
        name to the disable value that should be written.
    """
    settings: List[Tuple[str, int]] = []
    for idx, control_name in enumerate(_PREFETCHER_CONTROL_SEQUENCE):
        disable = 1 if level > idx else 0
        settings.append((control_name, disable))
    return settings

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
        control_key = _CLI_FLAG_TO_CONTROL[control_name][0]
        if control_name == "prefetch_disable":
            control_key = _PREFETCHER_CONTROL_SEQUENCE[0]
        native_domain = pio.control_domain_type(control_key)
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
        """
        Resolve a range value for a control parameter, supporting multiple fallback mechanisms.

        Args:
            key (Union[str, int, float, tuple]): The identifier for the range value. This can be:
                - A string: The name of a signal to read via pio.read_signal.
                - An int or float: The value is returned directly.
                - A tuple: A sequence of fallback keys. Each candidate is tried in order until one succeeds.
                  If all candidates fail (raise RuntimeError), the last error is raised.
            domain (Union[int, str]): The domain identifier or type to use when reading a signal.

        Returns:
            float: The resolved range value for the control parameter.

        Fallback behavior for tuple-type keys:
            If `key` is a tuple, each element is treated as a candidate key. The method recursively attempts
            to resolve each candidate in order. The first candidate that does not raise a RuntimeError is used,
            and its value is returned. If all candidates fail, the last RuntimeError is raised. If the tuple is empty,
            a ValueError is raised.

        Raises:
            RuntimeError: If all tuple candidates fail with a RuntimeError.
            ValueError: If no valid value is found for a tuple key.
            TypeError: If the key is of an unsupported type.
        """
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

    def list_controls_str(self) -> str:
        """Render the catalog of available controls as a fixed-width table.

        For each control the table lists its preferred --sweep alias, native
        domain, display units, and the auto-detected minimum, maximum, and step.
        Values that cannot be resolved (for example an unavailable signal) are
        shown as 'n/a' so a single missing control does not abort the listing.

        Returns:
            str: A multi-line, fixed-width table suitable for printing.
        """
        header = (f"{'CONTROL':<14}{'DOMAIN':<10}{'UNITS':<8}"
                  f"{'MIN':<14}{'MAX':<14}{'STEP':<14}")
        lines = [header]
        for control_key in _CLI_FLAG_TO_CONTROL:
            alias = _PREFERRED_ALIAS[control_key]
            units = _CATEGORY_DISPLAY_UNIT[_CONTROL_CATEGORY[control_key]]
            try:
                domain = native_domain(control_key)
            except Exception:
                domain = "n/a"
            minimum = _format_range_value(
                lambda ck=control_key, dm=domain: self.get_minimum(ck, dm))
            maximum = _format_range_value(
                lambda ck=control_key, dm=domain: self.get_maximum(ck, dm))
            step = _format_range_value(
                lambda ck=control_key: self.get_step(ck))
            lines.append(f"{alias:<14}{domain:<10}{units:<8}"
                         f"{minimum:<14}{maximum:<14}{step:<14}")
        return "\n".join(lines)

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
            value = dim["settings"][coordinate[idx]]
            if dim["control"] == "prefetch_disable":
                level = int(round(value))
                level = max(0, min(_MAX_PREFETCH_DISABLE_LEVEL, level))
                for control_name, setting in prefetch_settings(level):
                    result.append((control_name, dim["domain"], dim["domain_idx"], setting))
                continue
            result.append((dim["control"], dim["domain"], dim["domain_idx"], value))
            min_control = dim["control"].replace("_MAX_", "_MIN_")
            if min_control != dim["control"] and min_control in pio.control_names() and min_control != "CPU_FREQUENCY_MIN_CONTROL":
                result.append((min_control, dim["domain"], dim["domain_idx"], value))
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
