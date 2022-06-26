# Pytorch C++ Agent Tutorial

This tutorial demonstrates how to implement a libtorch based C++ agent in GEOPM.
Specifically, the tutorial shows how to create a C++ agent that can use a pytorch model
to select frequency controls for a GPU enabled system based on the following signals:
```
GPU_FREQUENCY_STATUS
GPU_POWER
GPU_UTILIZATION
GPU_COMPUTE_ACTIVITY
GPU_MEMORY_ACTIVITY
```
An additional user input knob "GPU_PHI" is also taken as an input.  This control is a unitless
input ranging from 0 to 1 with 0 skewing the agent toward performance oriented frequency selection
and 1 skewing the agent towards energy efficient frequency selection.

This tutorial is intended to show how this type of agent may be implemented, compiled, trained,
and used alongside a workload.  For this purpose two GPU benchmarks integrated into the GEOPM
experiment infrastructure, PARRES DGEMM and PARRES NSTREAM, are sufficient for training.
For a fully realized pytorch agent an expanded workload list  should be used.  Similarly an
expanded telemetry list may be beneficial.


The following steps are required to build and use this agent:
1. Acquire libtorch with CX11 ABI
2. Build the GEOPM C++ GPUTorchAgent
3. Perform frequency sweeps to generate data for model training.
4. Process the frequency sweeps to condition the data for model training.
5. Train a neural network model using the processed trace data.
6. Execute the agent with a trained model.


## 1) Acquire libtorch
The latest install information can be found here: https://pytorch.org/get-started/locally/

```
wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.10.2%2Bcpu.zip
```

## 2) Build the GEOPM C++ Torch Agent
Assuming the libtorch bundle has been unziped into $HOME/libtorch and a $HOME/.geopmrc file is setup
as specified in the general GEOPM documentation you should be able to use the provided tutorial
makefile after building GEOPM.

```
source $HOME/.geopmrc
export TORCH_ROOT=$HOME/libtorch
make
```

This will build the GPU Torch Agent.

## 3) Perform GPU frequency sweeps to generate data for model training.
Run the parres dgemm and stream frequency sweeps using the geopm/integration/experiment/
infrastructure as covered in PR 2024.

Use the ${GEOPM_SOURCE}/integration/experiments/gen_slurm.sh script.  Example:
```
./gen_slurm.sh 1 parres_dgemm gpu_frequency_sweep
./gen_slurm.sh 1 parres_nstream gpu_frequency_sweep

```

Modify the output test.sbatch to enable traces, set frequency range, and set per GPU signals
```
    --enable-traces \
    --geopm-trace-signals=GPU_FREQUENCY_STATUS-board_accelerator-0,GPU_POWER-board_accelerator-0,GPU_UTILIZATION-board_accelerator-0,GPU_COMPUTE_ACTIVITY-board_accelerator-0,GPU_MEMORY_ACTIVITY-board_accelerator-0\
```
Min and max frequency may vary from system to system

## 4) Process the GPU frequency sweeps to condition the data for model training.
For the GPU Agent use the geopm/tutorial/ml/process_gpu_sweeps.py to process the dgemm and stream frequency sweeps
```
./process_gpu_frequency_sweep.py <node_name> --domain node processed_gpu_sweep training_data/dgemm/ training_data/stream/
```
This will result in a processed_gpu_sweep.h5 output.


## 5) Train a neural network model for the GPU using the processed trace data.
For GPU:
    Use the geopm/tutorial/pytorch_agent/train_gpu_model-pytorch.py to create a model with the name gpu_control.pt
    ```
     ./train_gpu_model-pytorch.py processed_gpu_sweep.h5 gpu_control.pt
    ```

## 6a) Execute the GPU agent with a trained model
The NN model must be in the directory the job is being launched from and must be named gpu_control.pt

```
EXE=${GEOPM_SOURCE}/integration/apps/parres/Kernels/Cxx11/dgemm-mpi-cublas
APPOPTS="10 16000"
GPUS=4 #used as number of ranks

export GEOPM_PLUGIN_PATH=${GEOPM_SOURCE}/tutorial/pytorch_agent
geopmagent -a gpu_torch -pNAN,NAN,0 > phi0.policy
geopmlaunch impi -ppn ${GPUS} -n ${GPUS} --geopm-policy=phi0.policy --geopm-ctl=process --geopm-report=dgemm-16000-torch-phi0.report --geopm-agent=gpu_torch -- $EXE $APPOPTS

geopmagent -a gpu_torch -pNAN,NAN,0.4 > phi40.policy
geopmlaunch impi -ppn ${GPUS} -n ${GPUS} --geopm-policy=phi40.policy --geopm-ctl=process --geopm-report=dgemm-16000-torch-phi40.report --geopm-agent=gpu_torch -- $EXE $APPOPTS
```
