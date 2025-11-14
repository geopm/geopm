#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import argparse
from unittest import mock
import matplotlib.pyplot as plt
import json
import os
from matplotlib.ticker import PercentFormatter
import numpy as np


# Import the PBS hook so we can plot the output of its power-balancing routines
# Mock out the GEOPM and PBS stuff since this plots what-if scenarios from the
# hook config data and we don't intend to interact with actual system controls.
pbs_mock=mock.MagicMock()
pbs_mock.hook_config_filename = None
mock.patch.dict("sys.modules", pbs=pbs_mock, geopmdpy=mock.MagicMock()).start()
import geopm_power_limit_compute


parser = argparse.ArgumentParser()
parser.add_argument('model_params', help='Path to json file containing model parameters')
parser.add_argument('power_caps',
                    type=float,
                    nargs='+',
                    help='Which average power cap levels to evaluate.')
parser.add_argument('--job-type', help='Which job model to use. Default: The one specified by node_profile_name in the model params.')
parser.add_argument('--plot-dir',
                    default='./plots',
                    help='Where to save plots. Creates the directory if it does not exist. Default: %(default)s')
parser.add_argument('--plot-ext',
                    default='png',
                    help='File extension to use for plots. Default: %(default)s')

args = parser.parse_args()

with open(args.model_params) as f:
    hook_config = json.load(f)

if args.job_type is None:
    job_type = hook_config['node_profile_name']
else:
    job_type = args.job_type
host_models = geopm_power_limit_compute.get_model_from_config(hook_config, job_type, per_host=True)
max_node_power = host_models['max_power']

vnode_names = list(hook_config['profiles'][job_type]['hosts'])

x0 = [host_models[host]['x0'] for host in vnode_names]
A = [host_models[host]['A'] for host in vnode_names]
B = [host_models[host]['B'] for host in vnode_names]
C = [host_models[host]['C'] for host in vnode_names]


os.makedirs(args.plot_dir, exist_ok=True)
plt.style.use('seaborn')

average_caps = args.power_caps
budget_allocations = [
    geopm_power_limit_compute.allocate_budget_to_nodes(average_cap * len(vnode_names), max_node_power, x0, A, B, C)
    for average_cap in average_caps
]

# Worst/slowest node if uniform power caps are applied
uniform_power_slowdowns = [
    geopm_power_limit_compute.slowdown_at_power(average_cap / max_node_power, x0, A, B, C)
    for average_cap in average_caps
]
non_uniform_power_slowdowns = [
    slowdown
    for slowdown, node_caps in budget_allocations
]

fig, ax = plt.subplots(figsize=(6, 3))
bins = np.histogram(np.hstack([node_caps for slowdown, node_caps in budget_allocations]), bins=40)[1]
for average_cap, (slowdown, power_by_node) in zip(average_caps, budget_allocations):
    if slowdown < 0:
        slowdown = 0
    ax.hist(power_by_node, bins=bins, label=f'{average_cap} W, Slowdown={slowdown:.0%}')
ax.set_ylabel('Node Count')
ax.set_xlabel('Node Power Cap')

ax.legend(loc='upper left')
fig.tight_layout()
fig.savefig(os.path.join(args.plot_dir, f"{job_type}_node_power_dist.{args.plot_ext}"))


fig, ax = plt.subplots(figsize=(6, 3))
bins = np.histogram(np.hstack([slowdowns for slowdowns in uniform_power_slowdowns]), bins=40)[1]
linestyle = [0, (1, 3)]
text_offset = 0.003
text_ha = 'left'
for average_cap, non_uniform_slowdown, slowdowns in zip(average_caps, non_uniform_power_slowdowns, uniform_power_slowdowns):
    n, bins, patches = ax.hist(slowdowns, bins=bins, label=f'{average_cap} W')
    color = patches[0].get_facecolor()
    ax.axvline(non_uniform_slowdown, linestyle=tuple(linestyle), c=color)
    if non_uniform_slowdown < 0:
        non_uniform_slowdown = 0
    ax.text(non_uniform_slowdown + text_offset, 1.001, f'{non_uniform_slowdown:.1%}',
            transform=ax.get_xaxis_transform(),
            ha=text_ha, va='bottom', rotation=0, color=color, size=8)
    linestyle[0] += 2  # Shift the offset so overlapping dotted lines are visible
    text_offset *= -1
    text_ha = 'right' if text_ha == 'left' else 'left'
ax.set_ylabel('Node Count')
ax.set_xlabel('Slowdown')
ax.xaxis.set_major_formatter(PercentFormatter(1))

ax.legend(loc='upper right')
fig.tight_layout()
fig.savefig(os.path.join(args.plot_dir, f"{job_type}_node_slowdown_dist.{args.plot_ext}"))
