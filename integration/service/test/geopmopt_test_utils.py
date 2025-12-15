#!/usr/bin/env python3
"""Utility helpers for geopmopt integration tests.

This module provides small helpers that compute the figures of merit (FoM)
that the geopmopt integration tests use as optimization targets.  Keeping the
logic in one place makes it simpler to maintain consistent behaviour across
all of the test variants.
"""

from __future__ import annotations

import argparse
import glob
import sys
from pathlib import Path
from typing import List, Optional

try:
    from geopmpy import io as geopm_io
    _GEOPMPY_IO_ERROR = None
except ImportError as err:  # pragma: no cover - dependency guard
    geopm_io = None
    _GEOPMPY_IO_ERROR = err
from yaml import safe_load


class FoMError(RuntimeError):
    """Raised when a FoM computation cannot be completed."""


def _load_session_reports(pattern: str) -> List[dict]:
    """Load geopmsession-style reports that match *pattern*."""
    matches = sorted(glob.glob(pattern))
    if not matches:
        raise FoMError(f'No reports found matching pattern "{pattern}"')

    reports: List[dict] = []
    for match in matches:
        path = Path(match)
        if not path.is_file():
            continue
        with path.open(encoding='utf-8') as fid:
            reports.append(safe_load(fid))

    if not reports:
        raise FoMError(f'Unable to load any reports matching pattern "{pattern}"')
    return reports


def _load_geopmctl_reports(pattern: str) -> List[geopm_io.RawReport]:
    """Load geopmctl-style reports using geopmpy.io.RawReport."""
    if geopm_io is None:
        raise FoMError(f'geopmpy.io module not available: {_GEOPMPY_IO_ERROR}')
    matches = sorted(glob.glob(pattern))
    if not matches:
        raise FoMError(f'No reports found matching pattern "{pattern}"')

    reports: List[geopm_io.RawReport] = []
    for match in matches:
        path = Path(match)
        if not path.is_file():
            continue
        try:
            reports.append(geopm_io.RawReport(str(path)))
        except Exception as err:  # pragma: no cover - defensive guard
            raise FoMError(f'Unable to parse report "{path}": {err}') from err

    if not reports:
        raise FoMError(f'Unable to load any reports matching pattern "{pattern}"')
    return reports


def region_energy_product(pattern: str, region_index: int) -> float:
    """Average region energy (runtime * power) across all hosts."""
    reports = _load_geopmctl_reports(pattern)
    total = 0.0
    count = 0

    for report in reports:
        host_names = report.host_names()
        for host_name in host_names:
            region_names = report.region_names(host_name)
            if region_names is None or len(region_names) <= region_index:
                raise FoMError('Region data missing from report; cannot compute FoM')
            region_name = region_names[region_index]
            try:
                region = report.raw_region(host_name, region_name)
            except RuntimeError as err:
                raise FoMError(f'Unable to access region data: {err}') from err
            try:
                total += region['runtime (s)'] * region['power (W)']
            except KeyError as err:
                raise FoMError(f'Missing key {err!s} in region data; cannot compute FoM')
            count += 1

    if count == 0:
        raise FoMError('No region data found while computing region energy product')
    return total / count


def epoch_runtime_average(pattern: str) -> float:
    """Average epoch runtime across all hosts."""
    reports = _load_geopmctl_reports(pattern)
    total = 0.0
    count = 0

    for report in reports:
        for host_name in report.host_names():
            try:
                epoch_totals = report.raw_epoch(host_name)
            except KeyError as err:
                raise FoMError('Epoch Totals missing from report; cannot compute FoM') from err
            try:
                total += epoch_totals['runtime (s)']
            except KeyError as err:
                raise FoMError(f'Missing key {err!s} in epoch totals; cannot compute FoM')
            count += 1

    if count == 0:
        raise FoMError('No epoch data found while computing average runtime')
    return total / count


def session_duration(pattern: str) -> float:
    """Average session duration across all reports."""
    reports = _load_session_reports(pattern)
    total = 0.0
    count = 0

    for report in reports:
        metrics = report.get('metrics')
        if metrics is None or 'TIME' not in metrics:
            raise FoMError('Time metrics missing from session report; cannot compute FoM')
        time_metric = metrics['TIME']
        try:
            total += time_metric['last'] - time_metric['first']
        except KeyError as err:
            raise FoMError(f'Missing key {err!s} in time metrics; cannot compute FoM')
        count += 1

    if count == 0:
        raise FoMError('No session data found while computing duration')
    return total / count


def _print_fom(value: float) -> None:
    print(f'GEOPMOPT-FOM: {value}')


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(description='Compute FoM values for geopmopt tests')
    subparsers = parser.add_subparsers(dest='command', required=True)

    parser_region = subparsers.add_parser('region-energy', help='Average runtime*power for a region')
    parser_region.add_argument('--report-pattern', required=True, help='Glob pattern for report files')
    parser_region.add_argument('--region-index', type=int, default=0, help='Index of the region to use')

    parser_epoch = subparsers.add_parser('epoch-runtime', help='Average epoch runtime across all hosts')
    parser_epoch.add_argument('--report-pattern', required=True, help='Glob pattern for report files')

    parser_session = subparsers.add_parser('session-duration', help='Average session duration across reports')
    parser_session.add_argument('--report-pattern', required=True, help='Glob pattern for report files')

    args = parser.parse_args(argv)

    try:
        if args.command == 'region-energy':
            _print_fom(region_energy_product(args.report_pattern, args.region_index))
        elif args.command == 'epoch-runtime':
            _print_fom(epoch_runtime_average(args.report_pattern))
        elif args.command == 'session-duration':
            _print_fom(session_duration(args.report_pattern))
        else:
            raise FoMError(f'Unknown command "{args.command}"')
        return 0
    except FoMError as err:
        print(f'Error: {err}', file=sys.stderr)
        return 1


if __name__ == '__main__':  # pragma: no cover - script entry point
    sys.exit(main())
