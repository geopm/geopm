#!/usr/bin/env python3
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import argparse
import json
import numpy as np
import pandas as pd
import sys
import torch
from torch import nn
import torch.utils.data as data

def model_to_json(model, X_columns, y_columns, describe_net):
    def parse_signal(signal_name):
        if signal_name in ['TIME', 'DRAM_POWER', 'DRAM_ENERGY']:
            return [signal_name, 0, 0]
        component_list=["board", "package", "core", "cpu", "memory", "package_integrated_memory", "nic", "package_integrated_nic", "gpu", "package_integrated_gpu", "gpu_chip"]
        signal_list = signal_name.split('-')
        return [signal_list[0], component_list.index(signal_list[1].lower()), int(signal_list[2])]

    layers = [[]]

    for module in model.modules():
        name = str(type(module)).split('.')[-1][:-2]
        if hasattr(module, 'original_name'):
            name = module.original_name
        if name == 'BatchNorm1d':
            net_weight = module.weight/(module.running_var+module.eps)**0.5
            net_bias = -module.weight*module.running_mean/(module.running_var+module.eps)**0.5 + module.bias
            layers[-1].append((torch.diag(net_weight), net_bias))
        elif name == 'Linear':
            weight, bias = list(module.parameters())
            layers[-1].append((weight, bias))
        elif name == 'Sigmoid':
            layers.append([])

    rval = {
        'description': describe_net,
        'delta_inputs': [],
        'policy_inputs': [],
        'signal_inputs': [],
        'trace_outputs': [],
        'control_outputs': [],
        'layers': [],
    }

    for input_col in X_columns:
        if input_col.startswith("delta_"):
            num, den = input_col.split("/")
            num = num[len("delta_"):]
            den = den[len("delta_"):]
            rval['delta_inputs'].append([parse_signal(num), parse_signal(den)])
        elif input_col.startswith("policy_"):
            rval['policy_inputs'].append(input_col[len("policy_"):])
        else:
            rval['signal_inputs'].append(parse_signal(input_col))

    for output_col in y_columns:
        if output_col.startswith("trace_"):
            rval['trace_outputs'].append(output_col[len("trace_"):])
        else:
            rval['control_outputs'].append(parse_signal(output_col))

    for sublayers in layers:
        if len(sublayers) == 0:
            continue
        acc_weight, acc_bias = sublayers[0]
        for weight, bias in sublayers[1:]:
            acc_weight = np.matmul(weight, acc_weight)
            acc_bias = np.matmul(weight, acc_bias) + bias
        rval['layers'].append([acc_weight.tolist(), \
                               acc_bias.tolist()])

    return rval



# TODO: Remove trace lines transitioning between two different regions
#       i.e. if REGION_HASH != prev line REGION_HASH, delete.

def data_prep(input_trace, input_names, output_name):
    # Setup training and validation sets
    train_size = int(0.8 * len(input_trace))
    val_size = len(input_trace) - train_size

    df_train = input_trace
    df_x_train = df_train[input_names]
    df_y_train = df_train[output_name]

    x_train = torch.tensor(df_x_train.to_numpy()).float()
    y_train = torch.tensor(df_y_train.to_numpy()).long()

    train_set = torch.utils.data.TensorDataset(x_train, y_train)
    train_set, val_set = data.random_split(train_set, [train_size, val_size])

    return train_set, val_set

def train_model(df_traces, X_columns, y_columns, log=print):
    train_set, val_set = data_prep(df_traces, X_columns, y_columns)
    num_outputs = len(df_traces[y_columns[0]].unique())

    model = nn.Sequential(
        nn.BatchNorm1d(len(X_columns)),
        nn.Linear(len(X_columns), 64),
        nn.Sigmoid(),
        nn.Linear(64, 64),
        nn.Sigmoid(),
        nn.Linear(64, 64),
        nn.Sigmoid(),
        nn.Linear(64, num_outputs)
    )

    loss_fn = nn.CrossEntropyLoss()
    optimizer = torch.optim.Adam(model.parameters(), lr=1e-3)

    # TODONT write a custom loss function which is based
    #        on the energy loss rather than the frequency
    #        parking lot

    n_samples = len(train_set)
    n_test_samples = len(val_set)


    bs = 50000
    for epoch in range(100):
        model.train = True
        train_loss = 0
        for i in range((n_samples-1)//bs + 1):
            start_i = i*bs
            end_i = start_i + bs
            xb, yb = train_set[start_i:end_i]
            loss = loss_fn(model(xb), yb[:,0])
            train_loss += loss * len(xb)
            train_loss.detach()

            loss.backward()
            optimizer.step()
            optimizer.zero_grad()
        model.train = False

        total_loss = 0
        with torch.no_grad():
            for i in range((n_test_samples-1)//bs + 1):
                start_i = i*bs
                end_i = start_i + bs
                xb, yb = val_set[start_i:end_i]
                total_loss += loss_fn(model(xb), yb[:,0]) * len(xb)
    #TODO add softmax accuracy
        log(epoch, train_loss/n_samples, total_loss/n_test_samples)

    return model

def main(input_list, output_name="nnet", describe_net="A neural net.", region_ignore=None):
    #Regions to ignore for training
    if region_ignore == None:
        region_ignore = []
    else:
        region_ignore.split(",")
    region_ignore = ['NAN', '0x725e8066', '0x644f9787'] + region_ignore

    dfs = []

    region_ids = []

    #TODO: Test using config_name instead
    y_columns = ['region-id']
    X_columns_domain = {
            'cpu':['CPU_POWER-package-0',
                'CPU_FREQUENCY_STATUS-package-0',
                'MSR::UNCORE_PERF_STATUS:FREQ-package-0',
                'MSR::QM_CTR_SCALED_RATE-package-0'],
            'gpu':['GPU_CORE_FREQUENCY_STATUS-gpu-0',
                'GPU_POWER-gpu-0',
                'GPU_UTILIZATION-gpu-0',
                'GPU_CORE_ACTIVITY-gpu-0',
                'GPU_UNCORE_ACTIVITY-gpu-0']}
    ratios_domain = {
            'cpu': [['CPU_INSTRUCTIONS_RETIRED-package-0', 'TIME'],
            ['CPU_CYCLES_THREAD-package-0', 'TIME'],
            ['CPU_ENERGY-package-0', 'TIME'],
            ['MSR::APERF:ACNT-package-0', 'MSR::MPERF:MCNT-package-0'],
            ['MSR::PPERF:PCNT-package-0', 'MSR::MPERF:MCNT-package-0'],
            ['MSR::PPERF:PCNT-package-0', 'MSR::APERF:ACNT-package-0'],
            ],
            'gpu':[]}

    if type(input_list) == str:
        input_list = [input_list]

    for inp in input_list:
        #TODO: Check if hdf, if not, convert
        df = pd.read_hdf(inp)
        if 'app-config' not in  df:
            sys.stderr.write('<geopm> Error: No app-config in input data. Have you used gen_hdf_from_fsweep.py to create this HDF?\n')
            sys.exit(1)

        df["region-id"] = df["app-config"]

        #TODO: Confirm we can delete the following
        # for parres, the region hash can't be determined by GEOPM
        #df = df[~df['REGION_HASH'].isna()]
        #df = df[~df['REGION_HASH'].isin(region_ignore)]
        #df["region_id"] = df["REGION_HASH"] + suffix

        dfs.append(df)

    df_traces = pd.concat(dfs)

    domains_to_train = []

# Check traces to determine which domains we can train on
    for domain in X_columns_domain:
        add_domain = True
        for signal in set(X_columns_domain[domain] + sum(ratios_domain[domain], [])):
            if signal not in df_traces.columns:
                print (f"Missing signal {signal} is required for training {domain}.")
                add_domain = False
                break
        if add_domain:
            print (f"Training on {domain}")
            domains_to_train.append(domain)

    if len(domains_to_train) == 0:
        sys.stderr.write('<geopm> Error: No training domain is complete\n')
        sys.exit(1)

    print("Training to identify these regions:")
    region_ids = sorted(list(df_traces["region-id"].unique()))
    print(", ".join(region_ids))
    mapping = {region_id: region_ids.index(region_id) for region_id in region_ids}
    df_traces["region-id"] = df_traces['region-id'].map(mapping)

    for domain in domains_to_train:
        for num,den in ratios_domain[domain]:
            name = 'delta_{}/delta_{}'.format(num, den)
            X_columns_domain[domain].append(name)
            df_traces[name] = df_traces[num].diff() / df_traces[den].diff()

        df_traces.replace([np.inf, -np.inf], np.nan, inplace=True)
        is_missing_data = df_traces[X_columns_domain[domain] + y_columns].isna().sum(axis=1) > 0
        df_traces_domain = df_traces.loc[~is_missing_data]

        model = train_model(df_traces_domain, X_columns_domain[domain], y_columns)

        model_scripted = torch.jit.script(model)
        model_scripted.save(f"{output_name}_{domain}.pt")

        model_out = open(f"{output_name}_{domain}.json", "w")
        model.train = False
        with torch.no_grad():
            json.dump(model_to_json(model, X_columns_domain[domain], ["trace_" + region_id  for region_id in region_ids], describe_net), model_out)
        model_out.close()


if __name__ == '__main__':

     parser = argparse.ArgumentParser()
     parser.add_argument('--output',
                         help='Prefix of the output json file(s).',
                         dest="output_name")
     parser.add_argument('--description',
                        help='Description of the neural net, between quotes.',
                        dest="describe_net",
                        default="A neural net.")
     parser.add_argument('--ignore',
                        help='Comma-separated hashes of any regions to ignore.',
                        dest="region_ignore",
                        default=None)
     parser.add_argument('--data',
                        nargs='+',
                        help='Data files to train on')
     args = parser.parse_args()

     main(args.data, args.output_name, args.describe_net, args.region_ignore)

