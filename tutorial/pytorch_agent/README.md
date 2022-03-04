# Pytorch C++ Agent Tutorial

This tutorial demonstrates how to implement a liborch based C++ agent in GEOPM.
Specifically, the tutorial shows how to create a C++ agent that can use a pytorch model
to select frequency controls on a system with CPUs and GPUs.

The following steps are required to use this agent:
1. Acquire libtorch with CX11 ABI
2. Build the GEOPM C++ TorchAgent
3. Perform frequency sweeps to generate data for model training.
4. Process the frequency sweeps to condition the data for model training.
5. Train a neural network model using the processed trace data.
6. Execute the agent with a trained model.

## 1) Acquire libtorch
The latest install information can be found here: https://pytorch.org/get-started/locally/
Generally you'll need to the following:
```
wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.10.2%2Bcpu.zip
```

## 2) Build the GEOPM C++ TorchAgent
Assuming the libtorch bundle has been unziped into $HOME/libtorch, you should be able to use the
provided makefile.

## 3-5) Follow the existing GEOPM ML Tutorial.
Use the process_gpu_sweeps.py and train_gpu_model.py scripts in this tutorial as you would for
the existing ML tutorial.
TODO: Actually write this

## 6) Execute the agent with a trained model
TODO: build this into the integration infrastructure!
