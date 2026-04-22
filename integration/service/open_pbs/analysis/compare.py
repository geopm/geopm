#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
"""
Usage:
  ./compare.py \\
    --baseline /path/to/12459695_60 /path/to/12459696_60 \\
    --capped /path/to/12459765_60_3500 /path/to/12459766_60_3000 /path/to/12459767_60_2500
"""

import argparse
import re
from pathlib import Path
from typing import Dict, List, Optional

import pandas as pd

from geopmpy import io as geopm_io


# Matches the last _<digits> before .report (with optional -<hostname> suffix).
# Examples:
#   hacc-monitor_0.report                                     -> trial 0
#   nekbone-geopm-power-sweep_4000_2.report-x1922c6s1b0n0     -> trial 2
TRIAL_RE = re.compile(r"_(?P<trial>\d+)\.report(?:-\S+)?$")


def _parse_trial_from_report_file(filename: str) -> int:
    """Extract the trial number from a report filename string."""
    m = TRIAL_RE.search(filename)
    if not m:
        raise ValueError(
            f"Report filename does not match '*_<TRIAL>.report[-<host>]': {filename}"
        )
    return int(m.group("trial"))


def _add_trial_column(df: pd.DataFrame) -> pd.DataFrame:
    """Derive trial number from the report_file column."""
    if "report_file" not in df.columns:
        raise KeyError("Expected 'report_file' column in DataFrame")
    df = df.copy()
    df["trial"] = df["report_file"].apply(_parse_trial_from_report_file)
    return df


def load_report_data(dir_path: Path, cache_dir: Path) -> Dict[str, pd.DataFrame]:
    """Batch-load all report sections from all reports in *dir_path*.

    Returns a dict of DataFrames keyed by section name:
      - ``'totals'``  — Application Totals (one row per host per trial)
      - ``'<region>'`` — per-region data for each unique region name
    """
    # Collect both multi-host reports (*.report) and per-host reports (*.report-<hostname>)
    report_files = sorted(
        set(dir_path.glob("*.report")) | set(dir_path.glob("*.report-*"))
    )
    if not report_files:
        raise FileNotFoundError(f"No *.report or *.report-* files in {dir_path}")

    names = [rp.name for rp in report_files]
    rrc = geopm_io.RawReportCollection(
        names,
        dir_name=str(dir_path),
        dir_cache=str(cache_dir),
        verbose=False,
        do_cache=True,
    )

    result = {}  # type: Dict[str, pd.DataFrame]

    # Application Totals
    app_df = _add_trial_column(rrc.get_app_df())
    result["totals"] = app_df

    # Per-region data
    region_df = rrc.get_df()
    if region_df is not None and not region_df.empty:
        region_df = _add_trial_column(region_df)
        for region_name, rdf in region_df.groupby("region", sort=True):
            result[str(region_name)] = rdf.reset_index(drop=True)

    return result


def load_raw_host_data(
    dirs: List[Path], label: str, cache_root: Path
) -> Dict[str, pd.DataFrame]:
    """Load and tag raw host-level data for all reports in *dirs*.

    Returns a dict of DataFrames (same keys as :func:`load_report_data`)
    with ``label`` and ``directory`` columns added to each.
    """
    combined = {}  # type: Dict[str, List[pd.DataFrame]]
    for dir_path in dirs:
        cache_dir = cache_root / dir_path.name
        cache_dir.mkdir(parents=True, exist_ok=True)

        sections = load_report_data(dir_path, cache_dir)
        for key, df in sections.items():
            df = df.copy()
            df["label"] = label
            df["directory"] = dir_path.name
            combined.setdefault(key, []).append(df)

    return {
        key: pd.concat(dfs, ignore_index=True) for key, dfs in combined.items()
    }


def common_arg_parser() -> argparse.ArgumentParser:
    """Return an :class:`ArgumentParser` with the shared dataset flags.

    Can be used directly or as a *parent* for tool-specific parsers::

        parent = common_arg_parser()
        p = argparse.ArgumentParser(parents=[parent], ...)
    """
    p = argparse.ArgumentParser(add_help=False)
    p.add_argument(
        "--baseline", nargs="+", default=None,
        help="Paths to one or more baseline (unconstrained) dataset directories",
    )
    p.add_argument(
        "--capped", nargs="+", default=None,
        help="Paths to one or more power-capped dataset directories",
    )
    p.add_argument(
        "--cache-dir", default=None,
        help="Directory for HDF5 caches (default: ./.compare_cache)",
    )
    return p


def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        parents=[common_arg_parser()],
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "--interactive", action="store_true", default=False,
        help="Drop into an interactive Python REPL after loading data.",
    )
    return p.parse_args(argv)


def load_data(args: argparse.Namespace) -> Dict[str, pd.DataFrame]:
    """Load and merge report data based on CLI *args*.

    Expects *args* to carry ``baseline``, ``capped``, and ``cache_dir``
    attributes (as produced by :func:`common_arg_parser`).

    Returns a dict of DataFrames keyed by section name
    (``'totals'``, region names, etc.).
    """
    cache_root = (
        Path(args.cache_dir).expanduser().resolve()
        if args.cache_dir
        else Path(".compare_cache").resolve()
    )

    loaded = []  # type: List[Dict[str, pd.DataFrame]]
    if args.baseline:
        baseline_dirs = [Path(d).expanduser().resolve() for d in args.baseline]
        loaded.append(load_raw_host_data(baseline_dirs, "baseline", cache_root))
    if args.capped:
        capped_dirs = [Path(d).expanduser().resolve() for d in args.capped]
        loaded.append(load_raw_host_data(capped_dirs, "capped", cache_root))

    if not loaded:
        print("Nothing to load \u2014 supply --baseline and/or --capped.")
        return {}

    all_keys = set().union(*(d.keys() for d in loaded))
    raw = {}  # type: Dict[str, pd.DataFrame]
    for key in sorted(all_keys):
        parts = [d[key] for d in loaded if key in d]
        raw[key] = pd.concat(parts, ignore_index=True)

    return raw


def main(argv: Optional[List[str]] = None) -> int:
    pd.set_option("display.width", 250)
    pd.set_option("display.max_columns", 40)
    pd.set_option("display.float_format", "{:.4f}".format)
    pd.set_option("display.max_rows", None)

    args = parse_args(argv)
    raw = load_data(args)
    if not raw:
        return 0

    print(f"Available sections: {list(raw.keys())}")

    raw['totals']['sync_minus_mpi_time'] = (
        raw['totals']['sync-runtime (s)'] - raw['totals']['MPI startup (s)']
    )

    # Per-trial
    tdf = raw['totals'].groupby(['label', 'directory', 'trial'], sort=True)
    #  tdf['runtime (s)'].describe()
    #  b2 = df.get_group(('baseline', '12459695_60', 2))
    #  u1 = df.get_group(('capped', '12459765_60_3500', 1))

    # Speed up percentage
    # (b2['sync-runtime (s)'].mean() - u1['sync-runtime (s)'].mean()) / b2['sync-runtime (s)'].mean()
    # BOARD_POWER percentage
    # (b2['BOARD_POWER'].mean() - u1['BOARD_POWER'].mean()) / b2['BOARD_POWER'].mean()
    # BOARD_ENERGY percentage
    # (b2['BOARD_ENERGY'].mean() - u1['BOARD_ENERGY'].mean()) / b2['BOARD_ENERGY'].mean()

    # cols = ['sync-runtime (s)', 'BOARD_ENERGY', 'BOARD_POWER']

    # All trials
    adf = raw['totals'].groupby(['label', 'directory'], sort=True)
    #  base = adf.get_group(('baseline', '12459695_60'))
    #  cap_3500 = adf.get_group(('capped', '12459765_60_3500'))
    #  cap_3000 = adf.get_group(('capped', '12459766_60_3000'))
    #  cap_2500 = adf.get_group(('capped', '12459767_60_2500'))
    #  cap_210000 = adf.get_group(('capped', '12461211_60_210000'))
    #  cap_150000 = adf.get_group(('capped', '12461212_60_150000'))

    # FOM analysis
    print(raw['totals'].groupby('BOARD_POWER_LIMIT_CONTROL')['FOM'].describe())
    print()
    print((raw['totals'].groupby('BOARD_POWER_LIMIT_CONTROL')['FOM'].max() / raw['totals'].groupby('BOARD_POWER_LIMIT_CONTROL')['FOM'].min()) - 1)

    a = raw['totals'].groupby(['BOARD_POWER_LIMIT_CONTROL', 'trial'])['FOM']
    b = raw['totals'].groupby('BOARD_POWER_LIMIT_CONTROL')

    # Average FOM per trial per host, sorted
    host_foms = b.get_group(3000).groupby('host')['FOM'].mean().sort_values()
    print(host_foms)

    if args.interactive:
        import code
        code.interact(local=dict(globals(), **locals()))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
