#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import torch
from torch import nn
import torch.utils.data as data
from random import shuffle
import numpy as np
import sys
import os
import math

import pandas as pd
import argparse

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
                        help='Reload the model specified and try to convert to script.')
    parser.add_argument('--layer-width', type=int,
                        help='Specify model layer width (hidden dimensions for LSTM or RNN).')
    parser.add_argument('--layer-count', type=int,
                        help='Specify model layer count')
    parser.add_argument('--net-types', nargs='+', default=['FF'], type=str,
                        choices=['FF', 'LSTM', 'RNN'],
                        help='Space seperated list of NN to train against.')
    parser.add_argument('--scheduler-metric', default='accuracy', type=str,
                        choices=['loss', 'accuracy'],
                        help='Specify scheduler metric.')
    parser.add_argument('--train-hp-signals', action='store_true',
                        help='Specify scheduler metric.')
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
        model = FFNet(width_input=len(X_columns), layer_width=args.layer_width, layer_count=args.layer_count)
        model.load_state_dict(torch.load(args.convert_model))
        model_to_script(model, args.output)
        sys.exit(0)

    if args.train_hp_signals:
        X_nophi = X_columns[:-1]
        shuffle(X_nophi)
        signal_choice = list(np.array_split(np.array(X_nophi), 2))

        for choice in signal_choice:
            np.append(choice, X_columns[-1])
        signal_choice.append(X_columns)
    else:
        signal_choice = [X_columns]

    #import code
    #code.interact(local=locals())

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
            df_test_list = []
            for app_config in app_config_list:
                #If we exclude it from training we should save it for testing
                df_test_list.append(df_traces.loc[df_traces['app-config'] == app_config])

                #exclude from training
                df_traces = df_traces.loc[df_traces['app-config'] != app_config]

            #If using leave one out only use those cases for the test case
            df_test = pd.concat(df_test_list)

    seq_length = None
    net_type = None
    if args.train_hyperparams:
        #TODO: evaluate sample mechanism used for each setting
        config = {
            #"net_type" : tune.choice(['FF','LSTM']),
            "net_type" : tune.choice(args.net_types),
            "criterion" : tune.choice([nn.MSELoss(), nn.L1Loss()]),
            # If batch size is not evenly divisible by sequence length then LSTM training time will significanly
            # suffer.  See the mini_batch function
            "batch_size": tune.sample_from(lambda _: 2**np.random.randint(8, 13)),
            "seq_length" : tune.sample_from(lambda spec:
                2**np.random.randint(5, math.log2(spec.config.batch_size)) if
                spec.config.net_type in ['RNN','LSTM'] else None),
            "layer_width" : tune.sample_from(lambda _: 2**np.random.randint(2, 8)), #TODO: change based on NN type?
            "layer_count" : tune.sample_from(lambda spec:
                2**np.random.randint(2, 5) if spec.config.net_type=="FF" else np.random.randint(2,4)),
            "lr" : tune.loguniform(1e-3, 1e-3), #TODO: move to 1e-5 to 1e-1 range, per raytune FAQ recommendations
            "input_names": tune.grid_search(signal_choice)
        }

        scheduler_metric = args.scheduler_metric
        if(scheduler_metric == "accuracy"):
            scheduler_mode = "max"
        elif(scheduler_metric == "loss"):
            scheduler_mode = "min"
        else:
            print('Error: Specified scheduler metric {} does not have a mode'.format(scheduler_metric))
            sys.exit(1)

        scheduler = ASHAScheduler(
            metric=scheduler_metric,
            mode=scheduler_mode,
            #max_t=100, #TODO: define a maximum time for trials
            grace_period=1,
            reduction_factor=2)

        reporter = CLIReporter(
            metric_columns=["loss", "accuracy"],
            parameter_columns =['net_type', 'criterion', 'batch_size',
                                'seq_length', 'layer_width',
                                'layer_count', 'lr'])

        result = tune.run(
            tune.with_parameters(training_loop, traces=df_traces, output_control=y_columns),
            #resources_per_trial={"cpu":, "gpu":}, #TODO: specifying CPU availability as a command line option
            config = config,
            num_samples=args.train_hp_samples,
            scheduler = scheduler,
            progress_reporter=reporter,
            checkpoint_at_end=True
            )

        best_trial = result.get_best_trial(scheduler_metric, scheduler_mode, "last")
        print("Best config: {}".format(best_trial.config))
        print("\tValidation Set Accuracy: {:.2f}%".format(100*best_trial.last_result["accuracy"]))
        print("\tValidation Set Loss: {:.2f}%".format(100*best_trial.last_result["loss"]))

        net_type = best_trial.config['net_type']
        if(net_type == 'FF'):
            best_model = FFNet(width_input=len(X_columns),
                layer_width=best_trial.config["layer_width"],
                layer_count=best_trial.config["layer_count"])
        elif(net_type == 'LSTM'):
            best_model = LSTMNet(width_input=len(X_columns),
                hidden_dim=best_trial.config["layer_width"],
                layer_count=best_trial.config["layer_count"])
        elif(net_type == 'RNN'):
            best_model = RecNet(width_input=len(X_columns),
                hidden_dim=best_trial.config["layer_width"],
                layer_count=best_trial.config["layer_count"])

        criterion = best_trial.config['criterion']
        seq_length = best_trial.config['seq_length']

        best_checkpoint_dir = best_trial.checkpoint.value
        model_state, optimizer_state = torch.load(os.path.join(
            best_checkpoint_dir, "checkpoint"))
        best_model.load_state_dict(model_state)

        batch_size = best_trial.config['batch_size']
    else:
        # Default case
        batch_size = 1024
        learning_rate = 1e-3
        criterion = nn.MSELoss()
        seq_length = None
        net_type = "FF"

        train_set, val_set = data_prep(df_traces, X_columns, y_columns)

        train_loader = torch.utils.data.DataLoader(dataset = train_set, batch_size = batch_size, shuffle = False)
        val_loader = torch.utils.data.DataLoader(dataset = val_set, batch_size = batch_size, shuffle = False)

        layer_width = len(X_columns)
        layer_count = 2
        if args.layer_width is not None:
            layer_width = args.layer_width
        if args.layer_count is not None:
            layer_count = args.layer_count

        best_model = FFNet(width_input=len(X_columns), layer_width=layer_width, layer_count=layer_count)
        optimizer = torch.optim.Adam(best_model.parameters(), lr=learning_rate)

        print("batch_size:{}, epoch_count:{}, learning_rate={}".format(batch_size, EPOCH_SIZE, learning_rate))
        for epoch in range(EPOCH_SIZE):
            print("\tepoch:{}".format(epoch))
            train(best_model, optimizer, criterion, train_loader, seq_length)
            val_loss, val_accuracy, pred_min, pred_max = evaluate(best_model, val_loader, criterion, seq_length, rounding=1)

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
        val_loss, val_accuracy, pred_min, pred_max = evaluate(
            best_model, test_loader, criterion, seq_length=seq_length, rounding=1)
        print('\tAccuracy vs Test Set: {:.2f}%.'.format(val_accuracy*100))
        print('\tLoss vs Test Set: {:.2f}%.'.format(val_loss*100))
        print('\tModel Min Prediction vs Test Set: {:.2f} GHz.'.format(pred_min))
        print('\tModel Max Prediction vs Test Set: {:.2f} GHz.'.format(pred_max))

    # Our libtorch C++ implementation only supports FF nets currently
    if (net_type == "FF"):
        model_to_script(best_model, args.output)


def model_to_script(model, output):
    model.eval()
    model_scripted = torch.jit.script(model)
    model_scripted.save(output)
    print("Model saved to {}".format(output))

def data_prep(input_trace, input_names, output_name):
    # Setup training and validation sets
    train_size = int(0.8 * len(input_trace))
    val_size = len(input_trace) - train_size

    df_train = input_trace
    df_x_train = df_train[input_names]
    df_y_train = df_train[output_name]
    df_y_train /= 1e9

    x_train = torch.tensor(df_x_train.to_numpy()).float()
    y_train = torch.tensor(df_y_train.to_numpy()).float()
    x_train = x_train.to(device)
    y_train = y_train.to(device)

    train_set = torch.utils.data.TensorDataset(x_train, y_train)
    train_set, val_set = data.random_split(train_set, [train_size, val_size])

    return train_set, val_set

def training_loop(config, traces, output_control):
    train_set, val_set = data_prep(traces, config['input_names'], output_control)
    input_size = len(config['input_names'])

    train_loader = torch.utils.data.DataLoader(dataset = train_set, batch_size = config['batch_size'], shuffle = False)
    val_loader = torch.utils.data.DataLoader(dataset = val_set, batch_size = config['batch_size'], shuffle = False)


    net_type=config['net_type']
    if(net_type == 'Feed-Forward'):
        model = FFNet(width_input=input_size, fc_width=config["fc_width"], fc_depth=config["fc_depth"])
    if(net_type == 'FF'):
        model = FFNet(width_input=input_size, layer_width=config["layer_width"], layer_count=config["layer_count"])
    elif(net_type == 'LSTM'):
        assert(config['batch_size'] % config['seq_length'] == 0), "LSTM batch size must be divisibly by sequence length"
        model = LSTMNet(width_input=input_size, hidden_dim=config["layer_width"], layer_count=config["layer_count"])
    elif(net_type == 'RNN'):
        assert(config['batch_size'] % config['seq_length'] == 0), "LSTM batch size must be divisibly by sequence length"
        model = RecNet(width_input=input_size, hidden_dim=config["layer_width"], layer_count=config["layer_count"])
    else:
        print('Error: Invalid net type specified: {}'.format(net_type))
        sys.exit(1)
    model.to(device)

    criterion=config['criterion']

    learning_rate = config["lr"]
    optimizer = torch.optim.Adam(model.parameters(), lr=learning_rate)

    seq_length = config['seq_length']

    for epoch in range(EPOCH_SIZE):
        train(model, optimizer, criterion, train_loader, seq_length)

        # Prediction min and max aren't used here, they're for test sets
        val_loss, val_accuracy, _, _ = evaluate(model, val_loader, criterion, seq_length, rounding=1);

        with tune.checkpoint_dir(epoch) as checkpoint_dir:
            path = os.path.join(checkpoint_dir, "checkpoint")
            torch.save((model.state_dict(), optimizer.state_dict()), path)

        # Send the current training result back to Tune
        tune.report(loss=val_loss, accuracy=val_accuracy)

def train(model, optimizer, criterion, train_loader, seq_length=None):
    flag = False
    for idx, (inputs, target_control) in enumerate(train_loader):
        # Check if mini batches are required (i.e. RNN, LSTM)
        if seq_length is not None:
            inputs, target_control = mini_batch(inputs, target_control, inputs.size(1), seq_length)

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

# This function is used to convert batches into mini_batches of the form
# [MINI_BATCH_SIZE, SEQUENCE_LENGTH, # SIGNALS], as required by the
# LSTM Net.  tensor.view() is the function responsible for this, but it requires
# that the input tensor size (which is BATCH_SIZE * # Signals) be exactly convertible
# to the new 3D tensor form.  As such BATCH_SIZE * # Signals must be evenly divisible
# by MINI_BATCH_SIZE * SEQUENCE_LENGTH * # Signals.  As such if batch_size is not
# easily divisible by seq_length torch.cat will be called often and slow down training
def mini_batch(input_batch, target_batch, input_size, seq_length):
    if(input_batch.size(0) % seq_length == 0):
        inputs = input_batch.view(-1, seq_length, input_size)
        target = target_batch

    #TODO: eventually this should move into a custom data loader for
    #      the RNN style models.
    #      This is likely a computationally expensive solution compared
    #      to a custom dataloader due to the usage of torch.cat()
    elif(input_batch.size(0) > seq_length):
        padding = (seq_length - input_batch.size(0)%seq_length)
        inputs = torch.cat((input_batch, input_batch[0:padding]))
        inputs = inputs.view(-1, seq_length, input_size)
        target = torch.cat((target_batch, target_batch[0:padding]))
    else:
        padding = (seq_length-input_batch.size(0))
        repeat = math.ceil(padding/input_batch.size(0)) + 1
        inputs = input_batch.repeat(repeat,1)
        inputs = inputs[0:seq_length]
        inputs = inputs.view(-1, seq_length, input_size)

        target = target_batch.repeat(repeat,1)
        target = target[0:seq_length]

    return inputs, target

def evaluate(model, test_loader, criterion, seq_length=None, rounding=None):
    prediction_total = 0
    prediction_correct = 0
    prediction_min = 0
    prediction_max = 0
    val_loss = 0
    val_steps = 0
    model.eval()
    with torch.no_grad():
        for idx, (inputs, target_control) in enumerate(test_loader):
            # Check if mini batches are used (i.e. RNN, LSTM)
            if seq_length is not None:
                inputs, target_control = mini_batch(inputs, target_control, inputs.size(1), seq_length)

            prediction_total += target_control.size(0)

            # Run inputs through model, save prediction
            predicted_control = model(inputs)

            if rounding is not None:
                # Round to number of decimals specified
                predicted_control = np.round(predicted_control, rounding)

            # Generate stats
            prediction_correct += (target_control == predicted_control).sum().item()
            loss = criterion(predicted_control, target_control)
            val_loss += loss.numpy()
            val_steps += 1;

            # Stats for test case
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
    def __init__(self, width_input, layer_width, layer_count):
        super(FFNet, self).__init__()
        self.layer_count = layer_count

        # Normalization and input layer
        layer_list = [
            nn.BatchNorm1d(width_input),
            nn.Linear(width_input, layer_width),
            nn.Sigmoid()
            ]

        # Hidden layers
        for d in range(1, self.layer_count):
            layer_list.append(nn.Linear(layer_width, layer_width))
            layer_list.append(nn.Sigmoid())

        # Output layer
        layer_list.append(nn.Linear(layer_width, 1))

        self.p3Model = nn.Sequential(
            *layer_list
        )

    def forward(self, x):
        return self.p3Model(x)

class LSTMNet(nn.Module):
    def __init__(self, width_input, hidden_dim, layer_count):
        super(LSTMNet, self).__init__()

        # Hyper parameters
        self.hidden_dim = hidden_dim
        self.layer_count = layer_count

        #TODO: Normalization Layer?

        # LSTM Definition
        self.lstm = nn.LSTM(width_input, hidden_dim, layer_count, batch_first=True)

        # Output layer definition
        self.fc_out = nn.Linear(hidden_dim, 1)

    def forward(self, x):
        batch_size = x.size(0)
        # Initialize hidden state
        hidden = torch.zeros(self.layer_count, batch_size, self.hidden_dim)
        c = torch.zeros(self.layer_count, batch_size, self.hidden_dim)

        # Passing batch input and hidden state into the LSTM
        x, hidden = self.lstm(x, (hidden, c))

        # Output layer
        x = x.contiguous().view(-1, self.hidden_dim)
        x = self.fc_out(x)

        return x

class RecNet(nn.Module):
    def __init__(self, width_input, hidden_dim, layer_count):
        super(RecNet, self).__init__()

        # Hyper parameters
        self.hidden_dim = hidden_dim
        self.layer_count = layer_count

        #TODO: Normalization Layer?

        # RNN Definition
        self.rnn = nn.RNN(width_input, hidden_dim, layer_count, batch_first=True)

        # Output layer definition
        self.fc_out = nn.Linear(hidden_dim, 1)

    def forward(self, x):
        batch_size = x.size(0)
        # Initialize hidden state
        hidden = torch.zeros(self.layer_count, batch_size, self.hidden_dim)

        # Passing batch input and hidden state into the RNN
        x, hidden = self.rnn(x, hidden)

        # Output layer
        x = x.contiguous().view(-1, self.hidden_dim)
        x = self.fc_out(x)

        return x

if __name__ == "__main__":
    main()
