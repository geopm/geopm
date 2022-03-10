#!/usr/bin/env python3
#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import torch
from torch import nn
from random import uniform
import sys

import pandas as pd
import argparse
import code

def main():
    parser = argparse.ArgumentParser(
        description='Run ML training based on CPU frequency sweep data.')
    parser.add_argument('input', help='HDF file containing the training data, '
                                      'generated by process_cpu_frequency_sweep.py.')
    parser.add_argument('output', help='Output directory for the tensorflow model.')
    parser.add_argument('--leave-app-out',
                        help='Leave the named app out of the training set')
    args = parser.parse_args()

    df_traces = pd.read_hdf(args.input)

    y_columns = ['phi-freq']
    X_columns = ['POWER_PACKAGE',
                 'POWER_DRAM',
                 'FREQUENCY',
                 'TEMPERATURE_CORE',
                 'MSR::UNCORE_PERF_STATUS:FREQ-package-0',
                 'MSR::UNCORE_PERF_STATUS:FREQ-package-1',
                 'QM_CTR_SCALED_RATE-package-0',
                 'QM_CTR_SCALED_RATE-package-1']

    ratios = [['ENERGY_DRAM', 'TIME'],
              ['INSTRUCTIONS_RETIRED-package-0', 'TIME'],
              ['INSTRUCTIONS_RETIRED-package-1', 'TIME'],
              ['ENERGY_PACKAGE-package-0', 'TIME'],
              ['ENERGY_PACKAGE-package-1', 'TIME'],
              ['MSR::APERF:ACNT-package-0', 'MSR::MPERF:MCNT-package-0'],
              ['MSR::APERF:ACNT-package-1', 'MSR::MPERF:MCNT-package-1'],
              ['MSR::PPERF:PCNT-package-0', 'MSR::MPERF:MCNT-package-0'],
              ['MSR::PPERF:PCNT-package-1', 'MSR::MPERF:MCNT-package-1'],
              ['MSR::PPERF:PCNT-package-0', 'MSR::APERF:ACNT-package-0'],
              ['MSR::PPERF:PCNT-package-1', 'MSR::APERF:ACNT-package-1']]

    for num,den in ratios:
        name = 'delta_{}/delta_{}'.format(num, den)
        X_columns.append(name)
        df_traces[name] = df_traces[num].diff() / df_traces[den].diff()
    X_columns.append("phi")

    #Print phi to phi-freq mapping
    print(df_traces.pivot_table('phi-freq', 'phi', 'app-config'))

    # Exclude rows missing data in any of the columns of interest. Otherwise,
    # NaN values propagate into every weight in the model.
    is_missing_data = df_traces[X_columns + y_columns].isna().sum(axis=1) > 0
    df_traces = df_traces.loc[~is_missing_data]

    # Ignore applications that are requested to be ignored by the user. This
    # may be useful for a case where the training data includes many
    # application sweeps. Then, a single sweep output can be re-used for many
    # models, and each model can ignore one application. When evaluating the
    # model's performance on an applcation, we should use a model that excludes
    # that application from the training set so we can get a better idea about
    # how the model might generalize to unseen workloads.
    if args.leave_app_out is not None:
        if args.leave_app_out not in df_traces['app-config'].unique():
            print('Error: {args.leave_app_out} not in the available training sets')
            exit(1)
        df_traces = df_traces.loc[df_traces['app-config'] != args.leave_app_out]

    df_train = df_traces
    df_x_train = df_train[X_columns]
    df_y_train = df_train[y_columns]
    df_y_train /= 1e9

    model = nn.Sequential(
                    nn.BatchNorm1d(len(X_columns)),
                    nn.Linear(len(X_columns), len(X_columns)),
                    nn.Sigmoid(),
                    nn.Linear(len(X_columns), len(X_columns)),
                    nn.Sigmoid(),
                    nn.Linear(len(X_columns), len(X_columns)),
                    nn.Sigmoid(),
                    nn.Linear(len(X_columns), 1)
            )

    learning_rate = 1e-3
    optimizer = torch.optim.Adam(model.parameters(), lr=learning_rate)

    x_train = torch.tensor(df_x_train.to_numpy()).float()
    y_train = torch.tensor(df_y_train.to_numpy()).float()

    batch_size = 1000
    epoch_count = 5

    loss_fn = nn.MSELoss()

    train_tensor = torch.utils.data.TensorDataset(x_train, y_train)
    train_loader = torch.utils.data.DataLoader(dataset = train_tensor, batch_size = batch_size, shuffle = True)

    message_interval = round((len(df_x_train)/batch_size)/5)
    print("batch_size:{}, epoch_count:{}, learning_rate={}, message_interval={}".format(batch_size, epoch_count, learning_rate, message_interval))
    for epoch in range(epoch_count):
        train_loss = 0
        for idx, (inputs, target_control) in enumerate(train_loader):
            model.train()
            # Clear gradient
            optimizer.zero_grad()
            # Get model output
            predicted_control = model(inputs)
            # loss calculation vs target
            loss = loss_fn(predicted_control, target_control)
            loss.backward()

            # update model weights
            optimizer.step()
            # print statistics
            train_loss += loss.item()

            if (idx % message_interval == message_interval-1):
                print("\te:{}, idx:{} - loss: {:.3f}".format(epoch, idx, train_loss/(message_interval)))
                train_loss = 0.0

        #model.eval()
        #with torch.no_grad():
        #    print("\tEvaluate vs semi-random inputs:".format(epoch, idx, train_loss/(message_interval)))
        #    eval_gpu_freq = round(uniform(0,2)*1e9,0)
        #    eval_gpu_power = round(uniform(0,300),0)
        #    eval_gpu_util = round(uniform(0,1),2)
        #    eval_gpu_ca = round(uniform(0,1),2)
        #    eval_gpu_ma = round(uniform(0,1),2)
        #    for phi in [0, 0.5, 1.0]:
        #        output = model(torch.tensor([[eval_gpu_freq, eval_gpu_power, eval_gpu_util, eval_gpu_ca, eval_gpu_ma, phi]]))
        #        print('\t\tphi:{} -> recommended: {}'.format(phi, output[0]))

    model.eval()
    model_scripted = torch.jit.script(model)
    model_scripted.save(args.output)

if __name__ == "__main__":
    main()
