#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
"""Monte Carlo simulation: non-uniform vs uniform power-capping improvement.

Samples random N-host subsets from a model host pool, computes the
worst-node slowdown improvement of non-uniform over uniform capping,
and plots the distribution as a violin plot across power budgets.

Usage (quadratic model only):
  ./simulate_nonuniform.py \\
      /path/to/model.json \\
      model_hosts \\
      --outliers hosts_fom_outliers.txt \\
      --output nonuniform_projection.png

Usage (real data + hybrid evaluation of quadratic model, parsing reports):
  ./simulate_nonuniform.py \\
      /path/to/model.json \\
      model_hosts \\
      --outliers hosts_fom_outliers.txt \\
      --partial hosts_partial_power_limits.txt \\
      --missing-fom hosts_missing_fom.txt \\
      --real-data \\
      --sweep /path/to/12345678_60_2400 /path/to/12345679_60_2600 ... \\
      --outliers-rules "3200,lt,1e6" "3200,gt,11e6" "3800,lt,41e6" \\
      --output nonuniform_projection.png

Usage (real data, using pre-built HDF5 caches):
  ./simulate_nonuniform.py \\
      /path/to/model.json \\
      model_hosts \\
      --outliers hosts_fom_outliers.txt \\
      --partial hosts_partial_power_limits.txt \\
      --missing-fom hosts_missing_fom.txt \\
      --real-data \\
      --cache-dir .compare_cache \\
      --outliers-rules "3200,lt,1e6" "3200,gt,11e6" "3800,lt,41e6" \\
      --output nonuniform_projection.png
"""

import argparse
import json
import os
import random
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from unittest import mock

import numpy as np

# Add the PBS hook directory so geopm_power_limit_compute can be found
_hook_dir = str(Path(__file__).resolve().parent.parent)
if _hook_dir not in sys.path:
    sys.path.insert(0, _hook_dir)

# Mock PBS so we can import the compute module
_pbs_mock = mock.MagicMock()
_pbs_mock.hook_config_filename = None
mock.patch.dict("sys.modules", pbs=_pbs_mock).start()

# Ensure geopmdpy can be imported before loading the compute hook module.
try:
    import geopmdpy  # noqa: F401
except ImportError:
    _geopmdpy_mock = mock.MagicMock()
    sys.modules.setdefault("geopmdpy", _geopmdpy_mock)
    sys.modules.setdefault("geopmdpy.pio", _geopmdpy_mock)
    sys.modules.setdefault("geopmdpy.system_files", _geopmdpy_mock)
    sys.modules.setdefault("geopmdpy.topo", _geopmdpy_mock)

import geopm_power_limit_compute as compute_hook

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

# Ensure geopmpy can be imported (plot.py / compare.py import it at module level).
try:
    import geopmpy  # noqa: F401
except ImportError:
    _geopmpy_mock = mock.MagicMock()
    sys.modules.setdefault("geopmpy", _geopmpy_mock)
    sys.modules.setdefault("geopmpy.io", _geopmpy_mock)


def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("model", help="Path to the model JSON file")
    p.add_argument("hosts", help="File listing all model hosts (one per line)")
    p.add_argument("--outliers", default=None,
                   help="File listing outlier hosts to exclude (one per line)")
    p.add_argument("--partial", default=None,
                   help="File listing hosts with partial power-limit data "
                        "to exclude (hosts_partial_power_limits.txt)")
    p.add_argument("--missing-fom", default=None,
                   help="File listing hosts missing FOM data "
                        "to exclude (hosts_missing_fom.txt)")
    p.add_argument("--job-type", default="nekbone",
                   help="Profile name in the model JSON (default: nekbone)")
    p.add_argument("--num-nodes", type=int, default=128,
                   help="Nodes to sample per iteration (default: 128)")
    p.add_argument("--iterations", type=int, default=1000,
                   help="Monte Carlo iterations per power budget (default: 1000)")
    p.add_argument("--power-min", type=int, default=2400,
                   help="Min per-node power budget in watts (default: 2400)")
    p.add_argument("--power-max", type=int, default=4000,
                   help="Max per-node power budget in watts (default: 4000)")
    p.add_argument("--power-step", type=int, default=200,
                   help="Power budget step in watts (default: 200)")
    p.add_argument("--output", "-o", required=True,
                   help="Output file for the violin plot")
    p.add_argument("--seed", type=int, default=None,
                   help="Random seed for reproducibility")
    p.add_argument("--title",
                   default="Projected FOM Improvement: "
                           "Non-Uniform vs Uniform Power Capping\n",
                   help="Plot title")
    p.add_argument("--real-data", action="store_true",
                   help="Use real power-sweep data (piecewise linear "
                        "interpolation) for both power allocation and "
                        "slowdown evaluation, replacing the quadratic "
                        "model entirely.")
    p.add_argument("--sweep", nargs="+", default=None,
                   help="(requires --real-data) Paths to power-sweep dataset "
                        "directories.  Parses reports and creates HDF5 caches. "
                        "If omitted, loads pre-built caches from --cache-dir.")
    p.add_argument("--cache-dir", default=".compare_cache",
                   help="(requires --real-data) Directory for HDF5 caches "
                        "(default: ./.compare_cache)")
    p.add_argument("--outliers-rules", nargs="+", default=None,
                   metavar="POWER,OP,THRESH",
                   help="(requires --real-data) FOM outlier rules "
                        "(same format as plot.py --outliers)")
    p.add_argument("--publication", action="store_true", default=False,
                   help="Publication mode: remove titles, prune extreme "
                        "power budgets from violin plots.")
    p.add_argument("--min-trials", type=int, default=None,
                   help="(requires --real-data) Exclude hosts whose minimum "
                        "trial count across all power levels is below this "
                        "threshold.  Helps remove noisy single-sample hosts.")
    p.add_argument("--plot-range", default=None,
                   help="Power range to show in violin plots as 'LOW,HIGH' "
                        "(e.g. '2800,3800').  Budgets outside this range are "
                        "still simulated but hidden from the plot.  "
                        "Applied in both normal and publication modes.")
    p.add_argument("--load-sim", default=None,
                   help="Path to a previously saved simulation CSV file.  "
                        "Skips the Monte Carlo loop entirely and re-plots "
                        "from the saved data.  Accepts a glob pattern to "
                        "load both main and hybrid CSVs.")
    return p.parse_args(argv)


def load_model(model_path: str, job_type: str) -> tuple:
    """Load model JSON; return (max_power, model_type, host_models, host_curves).

    For original-quadratic:
        host_models = {host: {x0, A, B, C}}, host_curves = None
    For piecewise-linear:
        host_models = {}, host_curves = {host: (power_list, fom_list)}
    """
    with open(model_path) as f:
        config = json.load(f)

    max_power = config["max_power"]
    profile = config["profiles"][job_type]
    model_type = profile.get("model_type", "original-quadratic")

    if model_type == "piecewise-linear":
        host_curves = {}
        for host_name, host_data in profile.get("hosts", {}).items():
            pairs = sorted((float(k), float(v))
                           for k, v in host_data["model"].items())
            powers = [p for p, _ in pairs]
            foms = [f for _, f in pairs]
            # Enforce monotonicity (non-decreasing FOM with increasing power)
            for i in range(1, len(foms)):
                if foms[i] < foms[i - 1]:
                    foms[i] = foms[i - 1]
            host_curves[host_name] = (
                np.array(powers, dtype=float),
                np.array(foms, dtype=float),
            )
        return max_power, model_type, {}, host_curves
    else:
        hosts = {}
        for host_name, host_data in profile.get("hosts", {}).items():
            m = host_data["model"]
            hosts[host_name] = {
                "x0": float(m["x0"]),
                "A": float(m["A"]),
                "B": float(m["B"]),
                "C": float(m["C"]),
            }
        return max_power, model_type, hosts, None


# ---------------------------------------------------------------------------
# Real-data helpers
# ---------------------------------------------------------------------------

def load_real_data(args: argparse.Namespace) -> pd.DataFrame:
    """Load sweep data from --sweep dirs or pre-built HDF5 caches."""
    from plot import load_cached_data, validate_sweep_dataset, find_fom_outliers

    cache_root = Path(args.cache_dir).expanduser().resolve()
    if args.sweep:
        from compare import load_raw_host_data
        sweep_dirs = [Path(d).expanduser().resolve() for d in args.sweep]
        sections = load_raw_host_data(sweep_dirs, "sweep", cache_root)
    else:
        sections = load_cached_data(str(cache_root))

    if "totals" not in sections or sections["totals"].empty:
        raise RuntimeError("No totals data found.")

    df = validate_sweep_dataset(sections["totals"])

    if args.outliers_rules:
        outlier_hosts = find_fom_outliers(df, args.outliers_rules)
        if outlier_hosts:
            df = df[~df["host"].isin(outlier_hosts)].reset_index(drop=True)
            print(f"Removed {len(outlier_hosts)} rule-based outlier host(s)")

    return df


def build_host_curves(
    df: pd.DataFrame,
) -> Tuple[Dict[str, Tuple[np.ndarray, np.ndarray]], List[int]]:
    """Build per-host FOM(power) lookup arrays from averaged sweep data.

    Returns
    -------
    host_curves : dict
        ``{hostname: (power_array, fom_array)}`` sorted by power ascending.
    measured_levels : list of int
        Sorted list of all measured BOARD_POWER_LIMIT_CONTROL values.
    """
    col = "BOARD_POWER_LIMIT_CONTROL"
    metric = "FOM"

    df = df.copy()
    df[col] = df[col].astype(int)
    df = df.dropna(subset=[metric])
    avg = df.groupby(["host", col], as_index=False)[metric].mean()

    measured_levels = sorted(avg[col].unique())
    host_curves: Dict[str, Tuple[np.ndarray, np.ndarray]] = {}

    for host, hdf in avg.groupby("host"):
        hdf = hdf.sort_values(col)
        power_arr = hdf[col].values.astype(float)
        fom_arr = hdf[metric].values.astype(float)
        if np.any(np.isnan(fom_arr)):
            print(f"  WARNING: dropping {host} — NaN in FOM after averaging")
            continue
        # Enforce monotonicity: dips are measurement noise
        fom_arr = np.maximum.accumulate(fom_arr)
        host_curves[str(host)] = (power_arr, fom_arr)

    # --- Data completeness check ------------------------------------------
    n_levels = len(measured_levels)
    incomplete = {}
    for host, (powers, _foms) in host_curves.items():
        host_levels = set(int(p) for p in powers)
        missing = sorted(set(measured_levels) - host_levels)
        if missing:
            incomplete[host] = missing
    if incomplete:
        report_path = "hosts_incomplete_curves.txt"
        with open(report_path, "w") as fh:
            fh.write(f"# {len(incomplete)} host(s) with incomplete "
                     f"power-level coverage\n")
            fh.write(f"# Expected {n_levels} levels: {measured_levels}\n")
            for h in sorted(incomplete):
                fh.write(f"{h}  missing: {incomplete[h]}\n")
        print(f"WARNING: {len(incomplete)} host(s) have incomplete "
              f"power-level coverage — details in {report_path}")

    return host_curves, measured_levels


def fom_at_power(power: float, curve: Tuple[np.ndarray, np.ndarray]) -> float:
    """Piecewise-linear interpolation of FOM at a given power level."""
    return float(np.interp(power, curve[0], curve[1]))


def power_at_fom(target_fom: float, curve: Tuple[np.ndarray, np.ndarray]) -> float:
    """Inverse piecewise-linear interpolation: minimum power to achieve target_fom.

    Curves are already monotonized at build time, so FOM is non-decreasing.
    When a host is saturated (FOM plateaus), returns the lowest power that
    reaches the target — no power is wasted on a saturated host.
    """
    powers, foms = curve
    if target_fom <= foms[0]:
        return float(powers[0])
    if target_fom >= foms[-1]:
        return float(powers[-1])
    idx = int(np.searchsorted(foms, target_fom, side="left"))
    idx = max(1, min(idx, len(foms) - 1))  # safety clamp
    f0, f1 = foms[idx - 1], foms[idx]
    p0, p1 = powers[idx - 1], powers[idx]
    if f1 == f0:
        return float(p0)  # flat (saturated) segment: minimum power
    t = (target_fom - f0) / (f1 - f0)
    return float(p0 + t * (p1 - p0))


def allocate_nonuniform(
    host_names: List[str],
    host_curves: Dict[str, Tuple[np.ndarray, np.ndarray]],
    avg_power: float,
) -> Tuple[float, List[float]]:
    """Bisect to find equal-FOM allocation for a total power budget.

    Returns (target_fom, power_by_host).
    """
    num_nodes = len(host_names)
    total_budget = num_nodes * avg_power
    curves = [host_curves[h] for h in host_names]

    peak_fom = [float(c[1].max()) for c in curves]
    min_fom = [float(c[1].min()) for c in curves]

    fom_upper = min(peak_fom)
    fom_lower = min(min_fom)

    for _ in range(60):
        mid = (fom_lower + fom_upper) / 2.0
        powers = [power_at_fom(mid, c) for c in curves]
        total = sum(powers)
        if total > total_budget + 0.1:
            fom_upper = mid
        elif total < total_budget - 0.1:
            fom_lower = mid
        else:
            break

    target_fom = (fom_lower + fom_upper) / 2.0
    power_by_host = [power_at_fom(target_fom, c) for c in curves]
    return target_fom, power_by_host


# ---------------------------------------------------------------------------
# Simulation
# ---------------------------------------------------------------------------

def simulate_one(
    host_names: List[str],
    host_models: dict,
    max_node_power: float,
    avg_power_per_node: int,
    host_curves: Optional[Dict[str, Tuple[np.ndarray, np.ndarray]]] = None,
) -> float:
    """Return the worst-node FOM improvement (%) for one random sample.

    When *host_curves* is provided, both the allocation and evaluation use
    piecewise-linear interpolation of real measured FOM data.  Otherwise the
    quadratic model is used throughout.
    """
    num_nodes = len(host_names)
    job_budget = num_nodes * avg_power_per_node

    if host_curves is not None:
        # --- Real-data path: piecewise-linear model -----------------------
        # Non-uniform: bisection to equalize FOM
        _target_fom, power_by_node = allocate_nonuniform(
            host_names, host_curves, float(avg_power_per_node),
        )
        fom_nu = [fom_at_power(p, host_curves[h])
                  for h, p in zip(host_names, power_by_node)]

        # Uniform: every host gets avg_power_per_node
        fom_uniform = [fom_at_power(avg_power_per_node, host_curves[h])
                       for h in host_names]

        # Job performance is gated by the worst (long-pole) node
        worst_nu = min(fom_nu)
        worst_uniform = min(fom_uniform)
        if worst_uniform <= 0:
            return 0.0
        improvement = (worst_nu - worst_uniform) / worst_uniform * 100

        if improvement < -0.01:
            # Diagnostic: find the problematic hosts
            idx_worst_nu = fom_nu.index(worst_nu)
            idx_worst_uni = fom_uniform.index(worst_uniform)
            total_assigned = sum(power_by_node)
            total_budget = num_nodes * avg_power_per_node
            print(f"\n  DEBUG negative improvement at {avg_power_per_node}W: "
                  f"{improvement:.4f}%")
            print(f"    target_fom={_target_fom:.2f}  "
                  f"worst_nu={worst_nu:.2f} (host {host_names[idx_worst_nu]}, "
                  f"assigned {power_by_node[idx_worst_nu]:.1f}W)  "
                  f"worst_uniform={worst_uniform:.2f} "
                  f"(host {host_names[idx_worst_uni]})")
            print(f"    total_assigned={total_assigned:.1f}  "
                  f"budget={total_budget:.1f}  "
                  f"diff={total_assigned - total_budget:.1f}")
            # Check round-trip consistency for the worst non-uniform host
            h_bad = host_names[idx_worst_nu]
            p_assigned = power_by_node[idx_worst_nu]
            c_bad = host_curves[h_bad]
            print(f"    worst_nu host curve: powers={c_bad[0]}, "
                  f"foms={c_bad[1]}")
            print(f"    power_at_fom({_target_fom:.4f}, curve)="
                  f"{power_at_fom(_target_fom, c_bad):.4f}  "
                  f"fom_at_power({p_assigned:.4f}, curve)="
                  f"{fom_at_power(p_assigned, c_bad):.4f}")

        return improvement
    else:
        # --- Quadratic-model path -----------------------------------------
        x0 = [host_models[h]["x0"] for h in host_names]
        A = [host_models[h]["A"] for h in host_names]
        B = [host_models[h]["B"] for h in host_names]
        C = [host_models[h]["C"] for h in host_names]

        _slowdown_nu, power_by_node = compute_hook.allocate_budget_to_nodes(
            job_budget, max_node_power, x0, A, B, C,
        )
        normalized_power = [p / max_node_power for p in power_by_node]
        slowdown_by_node_nu = [
            An * (x0n - pn) ** 2 + Bn * (x0n - pn) + Cn
            for x0n, An, Bn, Cn, pn in zip(x0, A, B, C, normalized_power)
        ]
        norm_uniform = avg_power_per_node / max_node_power
        slowdown_by_node_uniform = [
            An * (x0n - norm_uniform) ** 2 + Bn * (x0n - norm_uniform) + Cn
            for x0n, An, Bn, Cn in zip(x0, A, B, C)
        ]

        # Job performance is gated by the worst (long-pole) node
        return (max(slowdown_by_node_uniform) - max(slowdown_by_node_nu)) * 100


def simulate_one_hybrid(
    host_names: List[str],
    host_models: dict,
    max_node_power: float,
    avg_power_per_node: int,
    host_curves: Dict[str, Tuple[np.ndarray, np.ndarray]],
    measured_range: Tuple[float, float] = (2400.0, 4000.0),
) -> float:
    """Quadratic-model allocation evaluated with real measured data.

    The quadratic model decides power assignments, but FOM is looked up
    from the piecewise-linear real-data curves.  This reveals how well
    the model's decisions perform in practice.

    If any host is assigned a power limit outside the measured data
    range, returns 0.0 — we have no real data to evaluate that
    assignment.
    """
    num_nodes = len(host_names)
    job_budget = num_nodes * avg_power_per_node

    x0 = [host_models[h]["x0"] for h in host_names]
    A = [host_models[h]["A"] for h in host_names]
    B = [host_models[h]["B"] for h in host_names]
    C = [host_models[h]["C"] for h in host_names]

    _slowdown_nu, power_by_node = compute_hook.allocate_budget_to_nodes(
        job_budget, max_node_power, x0, A, B, C,
    )

    # If any assignment falls outside the measured data range, we cannot
    # reliably evaluate — return no improvement.
    p_min, p_max = measured_range
    if any(p < p_min - 0.1 or p > p_max + 0.1 for p in power_by_node):
        return 0.0

    # Evaluate with real data
    fom_nu = [fom_at_power(p, host_curves[h])
              for h, p in zip(host_names, power_by_node)]
    fom_uniform = [fom_at_power(avg_power_per_node, host_curves[h])
                   for h in host_names]

    worst_nu = min(fom_nu)
    worst_uniform = min(fom_uniform)
    if worst_uniform <= 0:
        return 0.0
    return (worst_nu - worst_uniform) / worst_uniform * 100


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)

    if args.seed is None:
        args.seed = random.randint(0, 2**31 - 1)
    random.seed(args.seed)
    print(f"Random seed: {args.seed}")

    # -- Load model --------------------------------------------------------
    max_power, model_type, host_models, model_curves = load_model(
        args.model, args.job_type)
    print(f"Model: {args.model}  (max_power={max_power}, type={model_type})")

    # -- Load real data (if requested) -------------------------------------
    host_curves = None
    if model_type == "piecewise-linear" and model_curves is not None:
        # Use curves loaded directly from the piecewise-linear JSON model
        host_curves = model_curves
        all_power_levels = set()
        for _pw, _fm in host_curves.values():
            all_power_levels.update(int(p) for p in _pw)
        measured_levels = sorted(all_power_levels)
        print(f"Piecewise-linear model: {len(host_curves)} hosts at "
              f"{len(measured_levels)} power levels: {measured_levels}")
    if args.real_data:
        df_real = load_real_data(args)
        host_curves, measured_levels = build_host_curves(df_real)
        print(f"Real data: {len(host_curves)} hosts at "
              f"{len(measured_levels)} power levels: {measured_levels}")

        # -- Per-host trial count diagnostic --------------------------------
        col = "BOARD_POWER_LIMIT_CONTROL"
        metric = "FOM"
        df_diag = df_real.copy()
        df_diag[col] = df_diag[col].astype(int)
        trial_counts = (
            df_diag.dropna(subset=[metric])
            .groupby(["host", col])[metric]
            .count()
            .unstack(fill_value=0)
        )
        # Flag hosts with zero trials at any level
        hosts_zero = trial_counts[(trial_counts == 0).any(axis=1)]
        if not hosts_zero.empty:
            zero_report = "hosts_zero_fom_trials.txt"
            with open(zero_report, "w") as fh:
                fh.write(hosts_zero.to_string())
                fh.write("\n")
            print(f"WARNING: {len(hosts_zero)} host(s) have ZERO FOM "
                  f"trials at one or more power levels — "
                  f"details in {zero_report}")
        # Show summary: min/median/max trial count per power level
        print("Trial counts per power level (across all hosts):")
        print(f"  {'Power':>7s}  {'Min':>5s}  {'Med':>5s}  {'Max':>5s}  "
              f"{'Hosts':>5s}")
        for pw in sorted(trial_counts.columns):
            vals = trial_counts[pw]
            print(f"  {pw:>7d}  {vals.min():>5d}  {int(vals.median()):>5d}  "
                  f"{vals.max():>5d}  {(vals > 0).sum():>5d}")

        # Distribution of per-host minimum trial counts
        min_trials_per_host = trial_counts.min(axis=1)
        print("\nDistribution of per-host min trial count:")
        for count_val in sorted(min_trials_per_host.unique()):
            n = (min_trials_per_host == count_val).sum()
            print(f"  min_trials={count_val:>3d}: {n:>6d} host(s)")

        # Filter hosts below --min-trials threshold
        if args.min_trials is not None:
            low_trial_hosts = set(
                min_trials_per_host[min_trials_per_host < args.min_trials].index
            )
            if low_trial_hosts:
                report_path = "hosts_low_trials.txt"
                with open(report_path, "w") as fh:
                    fh.write(f"# Hosts with min trial count < "
                             f"{args.min_trials}\n")
                    for h in sorted(low_trial_hosts):
                        fh.write(f"{h}  min_trials="
                                 f"{int(min_trials_per_host[h])}\n")
                print(f"\n--min-trials={args.min_trials}: removing "
                      f"{len(low_trial_hosts)} host(s) — "
                      f"details in {report_path}")
                # Remove from curves so they are excluded from the pool
                for h in low_trial_hosts:
                    host_curves.pop(h, None)
                print(f"Host curves after min-trials filter: "
                      f"{len(host_curves)}")
        print()

    # -- Load host pool ----------------------------------------------------
    with open(args.hosts) as f:
        all_hosts = [line.strip() for line in f if line.strip()]

    exclude_files = [
        (args.outliers, "outlier"),
        (args.partial, "partial-data"),
        (args.missing_fom, "missing-FOM"),
    ]
    for filepath, label in exclude_files:
        if filepath is None:
            continue
        with open(filepath) as f:
            exclude_set = {line.split()[0] for line in f
                           if line.strip() and not line.startswith("#")}
        before = len(all_hosts)
        all_hosts = [h for h in all_hosts if h not in exclude_set]
        print(f"Excluded {before - len(all_hosts)} {label} host(s) "
              f"via {filepath}, {len(all_hosts)} remaining")

    # Drop any host not present in the model
    if host_models:
        missing = [h for h in all_hosts if h not in host_models]
        if missing:
            print(f"WARNING: {len(missing)} host(s) not in model, excluding them")
            all_hosts = [h for h in all_hosts if h in host_models]

    # If using real data, also drop hosts without measured curves
    if host_curves is not None:
        no_data = [h for h in all_hosts if h not in host_curves]
        if no_data:
            print(f"WARNING: {len(no_data)} host(s) in model but not in "
                  f"real data, excluding them")
            all_hosts = [h for h in all_hosts if h in host_curves]

    if len(all_hosts) < args.num_nodes:
        print(f"ERROR: only {len(all_hosts)} hosts available, "
              f"need {args.num_nodes}")
        return 1

    print(f"Host pool: {len(all_hosts)} hosts")
    print(f"Sampling {args.num_nodes} hosts × {args.iterations} iterations")
    print(f"Power range: {args.power_min}–{args.power_max} W "
          f"(step {args.power_step} W)\n")

    # -- FOM spread diagnostic (real-data mode) ----------------------------
    if host_curves is not None:
        print("FOM spread across host pool at each power level:")
        print(f"  {'Power':>7s}  {'Min FOM':>12s}  {'Mean FOM':>12s}  "
              f"{'Max FOM':>12s}  {'Std FOM':>12s}  {'Spread%':>8s}  "
              f"{'Worst Host'}")
        power_budgets_diag = list(range(args.power_min,
                                        args.power_max + 1,
                                        args.power_step))
        for pw in power_budgets_diag:
            foms = [(fom_at_power(float(pw), host_curves[h]), h)
                    for h in all_hosts]
            fom_vals = [f for f, _ in foms]
            fmin = min(fom_vals)
            fmean = np.mean(fom_vals)
            fmax = max(fom_vals)
            fstd = np.std(fom_vals)
            worst_host = min(foms, key=lambda x: x[0])[1]
            spread_pct = (fmax - fmin) / fmean * 100 if fmean > 0 else 0
            print(f"  {pw:>7d}  {fmin:>12.1f}  {fmean:>12.1f}  "
                  f"{fmax:>12.1f}  {fstd:>12.1f}  {spread_pct:>7.1f}%  "
                  f"{worst_host}")
        print()

    # -- Monte Carlo -------------------------------------------------------
    power_budgets = list(range(args.power_min,
                               args.power_max + 1,
                               args.power_step))

    if args.load_sim:
        print(f"Loading simulation data from {args.load_sim}")
        df = pd.read_csv(args.load_sim)
        # Try to load a matching hybrid CSV
        hybrid_csv = args.load_sim.replace("_sim.", "_sim_hybrid.")
        df_hybrid_loaded = None
        if Path(hybrid_csv).exists():
            df_hybrid_loaded = pd.read_csv(hybrid_csv)
            print(f"Loaded hybrid simulation data from {hybrid_csv}")
    else:
        records: list = []

        for power in power_budgets:
            print(f"  {power} W ...", end="", flush=True)
            for _ in range(args.iterations):
                sample = random.sample(all_hosts, args.num_nodes)
                improvement = simulate_one(
                    sample, host_models, max_power, power,
                    host_curves=host_curves,
                )
                records.append({
                    "power_budget": power,
                    "improvement": improvement,
                })
            print(" done")

        df = pd.DataFrame(records)
        df_hybrid_loaded = None

    # -- Plot --------------------------------------------------------------
    if args.publication:
        plt.style.use("seaborn-v0_8-whitegrid")
        plt.rcParams.update({
            'font.size': 14,
            'axes.labelsize': 16,
            'xtick.labelsize': 12,
            'ytick.labelsize': 12,
            'legend.fontsize': 12,
            'savefig.bbox': 'tight',
            'savefig.pad_inches': 0.02,
        })
    else:
        plt.style.use("seaborn-v0_8-darkgrid")
    fig, ax = plt.subplots(figsize=(12, 6))

    # Drop first and last power budgets (flat extrapolation artifacts)
    if args.publication and not args.plot_range and len(power_budgets) > 2:
        plot_budgets = power_budgets[1:-1]
    else:
        plot_budgets = power_budgets
    # Apply --plot-range filter
    if args.plot_range:
        lo, hi = (int(x.strip()) for x in args.plot_range.split(","))
        plot_budgets = [p for p in plot_budgets if lo <= p <= hi]
    order = [str(p) for p in plot_budgets]
    df = df[df["power_budget"].isin(plot_budgets)]
    df["power_budget"] = df["power_budget"].astype(str)

    sns.violinplot(
        data=df,
        x="power_budget",
        y="improvement",
        hue="power_budget",
        ax=ax,
        order=order,
        legend=False,
        inner="box",
    )

    model_label = "Linear Interpolation" if host_curves is not None else "Quadratic Model"
    if not args.publication:
        ax.set_title(f"{args.title}{model_label} | {args.num_nodes} Node Samples | {args.iterations} Iterations | Profile: {args.job_type} | {len(all_hosts)} Nodes in Pool")
    ax.set_xlabel("Per-Node Power Budget (W)")
    ax.set_ylabel("Worst-Node FOM Improvement (%)")
    plt.tight_layout()

    base, ext = args.output.rsplit(".", 1)
    pub_tag = "_publication" if args.publication else ""
    output_path = f"{base}_{args.num_nodes}_{args.job_type}{pub_tag}.{ext}"
    fig.savefig(output_path, dpi=150)
    print(f"\nSaved figure to {output_path}")

    # Save simulation data for later re-plotting
    if not args.load_sim:
        sim_csv = f"{base}_{args.num_nodes}_{args.job_type}_seed{args.seed}_sim.csv"
        df.to_csv(sim_csv, index=False)
        print(f"Saved simulation data to {sim_csv}")

    # -- Hybrid plot: quadratic allocation, real-data evaluation -----------
    if args.load_sim and df_hybrid_loaded is not None:
        df_hybrid = df_hybrid_loaded
    elif not args.load_sim and host_curves is not None and host_models:
        print("\n--- Hybrid: Quadratic Allocation + Linear Evaluation ---")
        hybrid_records: list = []
        for power in power_budgets:
            print(f"  {power} W ...", end="", flush=True)
            for _ in range(args.iterations):
                sample = random.sample(all_hosts, args.num_nodes)
                improvement = simulate_one_hybrid(
                    sample, host_models, max_power, power, host_curves,
                    measured_range=(float(measured_levels[0]),
                                    float(measured_levels[-1])),
                )
                hybrid_records.append({
                    "power_budget": power,
                    "improvement": improvement,
                })
            print(" done")

        df_hybrid = pd.DataFrame(hybrid_records)

        # Save hybrid simulation data
        hybrid_csv = f"{base}_{args.num_nodes}_{args.job_type}_seed{args.seed}_sim_hybrid.csv"
        df_hybrid.to_csv(hybrid_csv, index=False)
        print(f"Saved hybrid simulation data to {hybrid_csv}")
    else:
        df_hybrid = None

    if df_hybrid is not None:
        fig_h, ax_h = plt.subplots(figsize=(12, 6))
        # Use same pruned order as the main plot
        df_hybrid = df_hybrid[df_hybrid["power_budget"].isin(plot_budgets)]
        df_hybrid["power_budget"] = df_hybrid["power_budget"].astype(str)

        sns.violinplot(
            data=df_hybrid,
            x="power_budget",
            y="improvement",
            hue="power_budget",
            ax=ax_h,
            order=order,
            legend=False,
            inner="box",
        )

        if not args.publication:
            ax_h.set_title(
                f"{args.title}Quadratic Allocation, Linear Evaluation | "
                f"{args.num_nodes} Node Samples | "
                f"{args.iterations} Iterations | "
                f"Profile: {args.job_type} | "
                f"{len(all_hosts)} Nodes in Pool"
            )
        ax_h.set_xlabel("Per-Node Power Budget (W)")
        ax_h.set_ylabel("Worst-Node FOM Improvement (%)")
        plt.tight_layout()

        hybrid_path = f"{base}_{args.num_nodes}_{args.job_type}_hybrid{pub_tag}.{ext}"
        fig_h.savefig(hybrid_path, dpi=150)
        print(f"\nSaved hybrid figure to {hybrid_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
