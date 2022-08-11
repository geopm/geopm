#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import torch
from torch import nn
from random import uniform
import numpy as np
import sys

import pandas as pd
import argparse

def main(input_file, output_model, leave_app_out):
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print("Using {} device for training".format(device))

    df_traces = pd.read_hdf(input_file)

    # Only train from the estimated region of interest
    df_traces = df_traces.loc[df_traces['is-roi']]

    X_columns = [
            'GPU_CORE_FREQUENCY_STATUS-gpu-0',
            'GPU_POWER-gpu-0',
            'GPU_UTILIZATION-gpu-0',
            'GPU_CORE_ACTIVITY-gpu-0',
            'GPU_UNCORE_ACTIVITY-gpu-0',
            'phi',
            ]

    y_columns = ['phi-freq']

    #Print phi to phi-freq mapping
    print(df_traces.pivot_table('phi-freq', 'phi', 'app-config'))

    # Exclude rows missing data in any of the columns of interest. Otherwise,
    # NaN values propagate into every weight in the model.
    # And replace Inf with NaN
    df_traces.replace([np.inf, -np.inf], np.nan, inplace=True)
    is_missing_data = df_traces[X_columns + y_columns].isna().sum(axis=1) > 0
    df_traces = df_traces.loc[~is_missing_data]

    # Assume no test traces to start.  Testing will be handled via running
    # applications with the gpu_torch agent
    df_test = None

    # Ignore applications that are requested to be ignored by the user. This
    # may be useful for a case where the training data includes many
    # application sweeps. Then, a single sweep output can be re-used for many
    # models, and each model can ignore one application. When evaluating the
    # model's performance on an applcation, we should use a model that excludes
    # that application from the training set so we can get a better idea about
    # how the model might generalize to unseen workloads.
    if leave_app_out is not None:
        app_config_list = [e for e in df_traces['app-config'].unique() if leave_app_out in e]
        if app_config_list is None:
            print('Error: {leave_app_out} not in the available training sets')
            exit(1)
        else:
            df_test_list = []
            for app_config in app_config_list:
                #If we exclude it from training we should save it for testing
                df_test_list.append(df_traces.loc[df_traces['app-config'] == app_config])

                #exclude from training
                df_traces = df_traces.loc[df_traces['app-config'] != app_config]

            #If using leave one out only use those cases for the test case
            df_test = pd.concat(df_test_list)

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

    model.to(device)

    learning_rate = 1e-3
    optimizer = torch.optim.Adam(model.parameters(), lr=learning_rate)

    x_train = torch.tensor(df_x_train.to_numpy()).float()
    y_train = torch.tensor(df_y_train.to_numpy()).float()
    # TODO: test this vs sending input and target
    #       this might be more performant overall, or easier
    x_train = x_train.to(device)
    y_train = y_train.to(device)

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
            # TODO: test this vs sending the entire tensor.
            #       this likely isn't as performant due to a lot of data transfers
            #inputs = inputs.to(device)
            #target_control = target_control.to(device)

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

    # Inference is going to be handled on the CPU
    # using a pytorch GEOPM agent.
    model.to(torch.device('cpu'))
    model.eval()
    model_scripted = torch.jit.script(model)
    model_scripted.save(output_model)
    print("Model saved to {}".format(output_model))

    # Testing should be handled on GPUs if available
    model.to(device)
    if df_test is not None:
        print("Beginning testing")
        df_x_test = df_test[X_columns]
        df_y_test = df_test[y_columns]
        df_y_test /= 1e9

        x_test = torch.tensor(df_x_test.to_numpy()).float()
        y_test = torch.tensor(df_y_test.to_numpy()).float()
        x_test = x_test.to(device)
        y_test = y_test.to(device)

        test_tensor = torch.utils.data.TensorDataset(x_test, y_test)
        test_loader = torch.utils.data.DataLoader(dataset = test_tensor, batch_size = batch_size, shuffle = True)

        prediction_correct = 0
        prediction_total = 0
        with torch.no_grad():
            for idx, (inputs, target_control) in enumerate(test_loader):
                # TODO: test this vs sending the entire tensor.
                #       this likely isn't as performant due to a lot of data transfers
                #inputs = inputs.to(device)
                #target_control = target_control.to(device)

                prediction_total += inputs.size(0)

                # Run inputs through model, save prediction
                predicted_control = model(inputs)
                # Round to nearest 100 MHz increment
                predicted_control = np.round(predicted_control.cpu(),1)
                target_control = np.round(target_control.cpu(),1)

                prediction_correct = (target_control == predicted_control).sum().item()

        print('Total Predictions: {}.  Accurate Prediction: {}.  Accuracy: {:.2f}%'.format(prediction_total, prediction_correct, 100*(prediction_correct/prediction_total)))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Run ML training based on GPU frequency sweep data.')
    parser.add_argument('input', help='HDF file containing the training data, '
                                      'generated by process_gpu_frequency_sweep.py.')
    parser.add_argument('output', help='Output directory for the tensorflow model.')
    parser.add_argument('--leave-app-out',
                        help='Leave the named app out of the training set')
    args = parser.parse_args()

    main(args.input, args.output, args.leave_app_out)
