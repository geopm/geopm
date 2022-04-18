#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import torch
from torch import nn
import torch.utils.data as data
from random import uniform
import numpy as np
import sys
import os

import pandas as pd
import argparse
import code

from ray import tune
from ray.tune import CLIReporter
from ray.tune.schedulers import ASHAScheduler

from functools import partial

EPOCH_SIZE = 5

def main():
    parser = argparse.ArgumentParser(
        description='Run ML training based on CPU frequency sweep data.')
    parser.add_argument('input', help='HDF file containing the training data, '
                                      'generated by process_cpu_frequency_sweep.py.')
    parser.add_argument('output', help='Output directory for the tensorflow model.')
    parser.add_argument('--leave-app-out',
                        help='Leave the named app out of the training set')
    parser.add_argument('--train-hyperparams', action='store_true',
                        help='Train model hyper parameters')
    parser.add_argument('--train-hp-samples', type=int, default=20,
                        help='Numer of hyperparameter samples (tasks to execute).  '
                             'Only used when --train-hyperparams is specified.')
    parser.add_argument('--convert-model', type=str,
                        help='Reload the model specified and try to convert to script')
    parser.add_argument('--width', type=int,
                        help='Specify model width')
    parser.add_argument('--depth', type=int,
                        help='Specify model depth')
    args = parser.parse_args()

    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print("Using {} device for training".format(device))

    df_traces = pd.read_hdf(args.input)

    y_columns = ['phi-freq']
    X_columns = ['CPU_POWER-package-0',
                 'CPU_FREQUENCY_STATUS-package-0',
                 'CPU_PACKAGE_TEMPERATURE-package-0',
                 'MSR::UNCORE_PERF_STATUS:FREQ-package-0',
                 'QM_CTR_SCALED_RATE-package-0']

    ratios = [['CPU_INSTRUCTIONS_RETIRED-package-0', 'CPU_CYCLES_THREAD-package-0'],
              ['CPU_INSTRUCTIONS_RETIRED-package-0', 'CPU_ENERGY-package-0'],
              ['MSR::APERF:ACNT-package-0', 'MSR::MPERF:MCNT-package-0'],
              ['MSR::PPERF:PCNT-package-0', 'MSR::MPERF:MCNT-package-0'],
              ['MSR::PPERF:PCNT-package-0', 'MSR::APERF:ACNT-package-0']]

    for num,den in ratios:
        name = 'delta_{}/delta_{}'.format(num, den)
        X_columns.append(name)
        df_traces[name] = df_traces[num].diff() / df_traces[den].diff()
    X_columns.append("phi")

    #Print phi to phi-freq mapping
    print(df_traces.pivot_table('phi-freq', 'phi', 'app-config'))

    if args.convert_model is not None:
        model = FFNet(width_input=len(X_columns), fc_width=args.width, fc_depth=args.depth)
        model.load_state_dict(torch.load(args.convert_model))
        model_to_script(model, args.output)
        sys.exit(0)

    # Exclude rows missing data in any of the columns of interest. Otherwise,
    # NaN values propagate into every weight in the model.
    # And replace Inf with NaN
    df_traces.replace([np.inf, -np.inf], np.nan, inplace=True)
    is_missing_data = df_traces[X_columns + y_columns].isna().sum(axis=1) > 0
    df_traces = df_traces.loc[~is_missing_data]

    # Assume no test traces to start
    df_test = None

    # Ignore applications that are requested to be ignored by the user. This
    # may be useful for a case where the training data includes many
    # application sweeps. Then, a single sweep output can be re-used for many
    # models, and each model can ignore one application. When evaluating the
    # model's performance on an applcation, we should use a model that excludes
    # that application from the training set so we can get a better idea about
    # how the model might generalize to unseen workloads.
    if args.leave_app_out is not None:
        app_config_list = [e for e in df_traces['app-config'].unique() if args.leave_app_out in e]
        if len(app_config_list) == 0:
            print('Error: {args.leave_app_out} not in the available training sets')
            sys.exit(1)
        else:
            df_test_list = [df_test]
            for app_config in app_config_list:
                #If we exclude it from training we should save it for testing
                df_test_list.append(df_traces.loc[df_traces['app-config'] == app_config])

                #exclude from training
                df_traces = df_traces.loc[df_traces['app-config'] != app_config]

            #If using leave one out only use those cases for the test case
            df_test = pd.concat(df_test_list)

    # Setup training and validation sets
    train_size = int(0.8 * len(df_traces))
    val_size = len(df_traces) - train_size

    df_train = df_traces
    df_x_train = df_train[X_columns]
    df_y_train = df_train[y_columns]
    df_y_train /= 1e9

    x_train = torch.tensor(df_x_train.to_numpy()).float()
    y_train = torch.tensor(df_y_train.to_numpy()).float()

    # TODO: test this vs sending input and target
    #       this might be more performant overall, or easier
    x_train = x_train.to(device)
    y_train = y_train.to(device)

    train_set = torch.utils.data.TensorDataset(x_train, y_train)
    train_set, val_set = data.random_split(train_set, [train_size, val_size])

    if args.train_hyperparams:
        #TODO: evaluate sample mechanism used for each setting
        config = {
            "fc_width" : tune.sample_from(lambda _: 2**np.random.randint(2, 7)),
            "fc_depth" : tune.randint(2,11),
            "batch_size": tune.randint(500,3000),
            "net_type" : tune.choice(['Feed-Forward']),
            "criterion" : tune.choice([nn.MSELoss(), nn.L1Loss()]),
            "lr" : tune.loguniform(1e-3, 1e-3)
        }

        scheduler = ASHAScheduler(
            #TODO: Evaluate using metric="loss", mode="min" vs metric="accuracy", mode="max"
            metric="accuracy",
            mode="max",
            #max_t=100, #TODO: define a maximum time for trials
            grace_period=1,
            reduction_factor=2)

        reporter = CLIReporter(
            metric_columns=["loss", "accuracy"])

        result = tune.run(
            tune.with_parameters(training_loop, input_size=len(X_columns), train_set=train_set, val_set=val_set),
            #resources_per_trial={"cpu":, "gpu":}, #TODO: specifying CPU availability as a command line option
            config = config,
            num_samples=args.train_hp_samples,
            scheduler = scheduler,
            progress_reporter=reporter,
            checkpoint_at_end=True
            )

        best_trial = result.get_best_trial("accuracy", "max", "last")
        print("Best config: {}".format(best_trial.config))
        print("\tValidation Set Accuracy: {:.2f}%".format(100*best_trial.last_result["accuracy"]))
        print("\tValidation Set Loss: {:.2f}%".format(100*best_trial.last_result["loss"]))
        best_model = FFNet(width_input=len(X_columns), fc_width=best_trial.config["fc_width"], fc_depth=best_trial.config["fc_depth"])
        criterion = best_trial.config['criterion']

        best_checkpoint_dir = best_trial.checkpoint.value
        model_state, optimizer_state = torch.load(os.path.join(
            best_checkpoint_dir, "checkpoint"))
        best_model.load_state_dict(model_state)

        batch_size = best_trial.config['batch_size']
    else:
        # Default case
        batch_size = 1000
        learning_rate = 1e-3
        criterion = nn.MSELoss()

        train_loader = torch.utils.data.DataLoader(dataset = train_set, batch_size = batch_size, shuffle = False)
        val_loader = torch.utils.data.DataLoader(dataset = val_set, batch_size = batch_size, shuffle = False)

        width = len(X_columns)
        depth = 2
        if args.width is not None:
            width = args.width
        if args.depth is not None:
            depth = args.depth

        best_model = FFNet(width_input=len(X_columns), fc_width=width, fc_depth=depth)
        optimizer = torch.optim.Adam(best_model.parameters(), lr=learning_rate)

        print("batch_size:{}, epoch_count:{}, learning_rate={}".format(batch_size, EPOCH_SIZE, learning_rate))
        for epoch in range(EPOCH_SIZE):
            print("\tepoch:{}".format(epoch))
            train(best_model, optimizer, criterion, train_loader)
            val_loss, val_accuracy, pred_min, pred_max = evaluate(best_model, val_loader, criterion);

    print('Saving model (non-scripted and in training mode) as a precaution here: {}'.format(args.output + '-pre-torchscript'))
    torch.save(best_model.state_dict(), "{}".format(args.output + '-pre-torchscript'))

    best_model.eval()
    if df_test is not None:
        df_x_test = df_test[X_columns]
        df_y_test = df_test[y_columns]
        df_y_test /= 1e9

        x_test = torch.tensor(df_x_test.to_numpy()).float()
        y_test = torch.tensor(df_y_test.to_numpy()).float()
        x_test = x_test.to(device)
        y_test = y_test.to(device)

        test_set = torch.utils.data.TensorDataset(x_test, y_test)
        test_loader = torch.utils.data.DataLoader(dataset = test_set, batch_size = batch_size , shuffle = False)

        print("Begin Testing:")
        val_loss, val_accuracy, pred_min, pred_max = evaluate(best_model, test_loader, criterion);
        print('\tAccuracy vs Test Set: {:.2f}%.'.format(val_accuracy*100))
        print('\tLoss vs Test Set: {:.2f}%.'.format(val_loss*100))
        print('\tModel Min Prediction vs Test Set: {:.2f} GHz.'.format(pred_min))
        print('\tModel Max Prediction vs Test Set: {:.2f} GHz.'.format(pred_max))

    for retry in range(3):
        try:
            model_to_script(best_model, args.output)
            break
        except Exception as e:
            print("Exception on convertion to torch.jit.script")
            print("{}".format(e))
            print("Retrying {}/3".format(retry+1))


def model_to_script(model, output):
    model.eval()
    model_scripted = torch.jit.script(model)
    model_scripted.save(output)
    print("Model saved to {}".format(output))

def training_loop(config, input_size, train_set, val_set):
    train_loader = torch.utils.data.DataLoader(dataset = train_set, batch_size = config['batch_size'], shuffle = False)
    val_loader = torch.utils.data.DataLoader(dataset = val_set, batch_size = config['batch_size'], shuffle = False)

    net_type=config['net_type']
    if(net_type == 'Feed-Forward'):
        model = FFNet(width_input=input_size, fc_width=config["fc_width"], fc_depth=config["fc_depth"])
        model.to(device)
    else:
        #TODO: Add additional NN types of interest!
        print('Error: Invalid net type specified: {}'.format(net_type))
        sys.exit(1)

    criterion=config['criterion']

    learning_rate = config["lr"]
    optimizer = torch.optim.Adam(model.parameters(), lr=learning_rate)

    for epoch in range(EPOCH_SIZE):
        if(net_type == 'Feed-Forward'):
            train(model, optimizer, criterion, train_loader)
            # Prediction min and max aren't used here, they're for test sets
            val_loss, val_accuracy, _, _ = evaluate(model, val_loader, criterion);
        else:
            #TODO: Add additional NN types of interest!
            print('Error: Invalid net type specified: {}'.format(net_type))
            sys.exit(1)

        with tune.checkpoint_dir(epoch) as checkpoint_dir:
            path = os.path.join(checkpoint_dir, "checkpoint")
            torch.save((model.state_dict(), optimizer.state_dict()), path)

        # Send the current training result back to Tune
        tune.report(loss=val_loss, accuracy=val_accuracy)

def train(model, optimizer, criterion, train_loader):
    for idx, (inputs, target_control) in enumerate(train_loader):
        model.train()
        # Clear gradient
        optimizer.zero_grad()
        # Get model output
        predicted_control = model(inputs)
        # loss calculation vs target
        loss = criterion(predicted_control, target_control)
        loss.backward()
        # update model weights
        optimizer.step()

def evaluate(model, test_loader, criterion):
    prediction_total = 0
    prediction_correct = 0
    prediction_min = 0
    prediction_max = 0
    val_loss = 0
    val_steps = 0
    model.eval()
    with torch.no_grad():
        for idx, (inputs, target_control) in enumerate(test_loader):
            prediction_total += inputs.size(0)
            # Run inputs through model, save prediction
            predicted_control = model(inputs)
            # Round to nearest 100 MHz increment
            predicted_control = np.round(predicted_control.cpu(),1)
            target_control = np.round(target_control.cpu(),1)

            # Generate stats
            prediction_correct += (target_control == predicted_control).sum().item()
            loss = criterion(predicted_control, target_control)
            val_loss += loss.numpy()
            val_steps += 1;

            if (idx == 0):
                prediction_max = predicted_control.max()
                prediction_min = predicted_control.min()
            else:
                if (prediction_max < predicted_control.max()):
                    prediction_max = predicted_control.max()
                if (prediction_min > predicted_control.min()):
                    prediction_min = predicted_control.min()

    return val_loss/val_steps, prediction_correct/prediction_total, prediction_min, prediction_max

class FFNet(nn.Module):
    def __init__(self, width_input, fc_width, fc_depth):
        super(FFNet, self).__init__()
        self.fc_depth = fc_depth

        # Normalization and input layer
        layer_list = [
            nn.BatchNorm1d(width_input),
            nn.Linear(width_input, fc_width),
            nn.Sigmoid()
            ]

        # Hidden layers
        for d in range(1, self.fc_depth):
            layer_list.append(nn.Linear(fc_width, fc_width))
            layer_list.append(nn.Sigmoid())

        # Output layer
        layer_list.append(nn.Linear(fc_width, 1))

        self.p3Model = nn.Sequential(
            *layer_list
        )

    def forward(self, x):
        return self.p3Model(x)

if __name__ == "__main__":
    main()
