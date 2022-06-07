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

This tutorial also shows how to create a C++ agent that uses a pytorch model to select frequency
controls for a CPU enabled system based on the following signals:
```
POWER_PACKAGE
CPU_FREQUENCY_STATUS
TEMPERATURE_CORE
MSR::UNCORE_PERF_STATUS:FREQ
QM_CTR_SCALED_RATE
INSTRUCTIONS_RETIRED
CYCLES_THREAD
INSTRUCTIONS_RETIRED
ENERGY_PACKAGE
MSR::APERF:ACNT
MSR::MPERF:MCNT
MSR::PPERF:PCNT
```
An additioanl user input knob "CPU_PHI" is also taken as an input.  It operates similarly to the
"GPU_PHI" signal discussed above.  Some of the signals may be modified through basic diffing or
division prior to training or use in an agent.

This tutorial is intended to show how this type of agent may be implemented, compiled, trained,
and used alongside a workload.  For this purpose two GPU benchmarks integrated into the GEOPM
experiment infrastructure, PARRES DGEMM and PARRES NSTREAM, are sufficient for training.  For the
CPU work the geopmbench benchmark is sufficient for training, though the integrated
arithmetic_intensity benchmark may also be used.

For a fully realized pytorch agent an expanded workload list should be used.  Similarly an
expanded telemetry list may be beneficial.

The following steps are required to build and use this agent:
1. Acquire libtorch with CX11 ABI and other dependencies
2. Build the GEOPM C++ GPUTorchAgent
3. Perform frequency sweeps to generate data for model training.
4. Process the frequency sweeps to condition the data for model training.
5. Train a neural network model using the processed trace data.
6. Execute the agent with a trained model.

## 1) Acquire libtorch and other dependencies - CPU or GPU Agent
The latest install information can be found here: https://pytorch.org/get-started/locally/

```
wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.10.2%2Bcpu.zip
```

Some python packages are also required if they are not already provided by the administrator
```
python3 -m pip install --user ray torch torchvision tabulate numpy
```

## 2) Build the GEOPM C++ Torch Agents
Assuming the libtorch bundle has been unziped into $HOME/libtorch and a $HOME/.geopmrc file is setup
as specified in the general GEOPM documentation you should be able to use the provided tutorial
makefile after building GEOPM.

```
source $HOME/.geopmrc
export TORCH_ROOT=$HOME/libtorch
make
```

This will build both the CPU and GPU Torch Agent.

## 3a) Perform GPU frequency sweeps to generate data for model training.
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

## 3b) Perform CPU frequency sweeps to generate data for model training.
Use the ${GEOPM_SOURCE}/integration/experiments/gen_slurm.sh script.  Example:

```
./gen_slurm.sh 1 dgemm frequency_sweep
./gen_slurm.sh 1 arithmetic_intensity frequency_sweep

```

Modify the output test.sbatch to enable traces, set frequency range, and set per package CPU signals
```
    --enable-traces \
    --step-frequency=100000000 \
    --min-frequency=1000000000 \
    --max-frequency=2800000000 \
    --trial-count=3 \
    --geopm-trace-signals=POWER_PACKAGE@package,POWER_DRAM,CPU_FREQUENCY_STATUS@package,TEMPERATURE_CORE@package,MSR::UNCORE_PERF_STATUS:FREQ@package,QM_CTR_SCALED_RATE@package,INSTRUCTIONS_RETIRED@package,CYCLES_THREAD@package,ENERGY_PACKAGE@package,MSR::APERF:ACNT@package,MSR::MPERF:MCNT@package,MSR::PPERF:PCNT@package,TIME@package,ENERGY_DRAM\

```
Min and max frequency may vary from system to system.

Additionally modify the test.sbatch script to set-up the QM signals:
```
geopmwrite MSR::PQR_ASSOC:RMID board 0 0
geopmwrite MSR::QM_EVTSEL:RMID board 0 0
geopmwrite MSR::QM_EVTSEL:EVENT_ID board 0 2
```

## 4a) Process the GPU frequency sweeps to condition the data for model training.
For the GPU Agent use the geopm/tutorial/ml/process_gpu_sweeps.py to process the dgemm and stream frequency sweeps
```
./process_gpu_frequency_sweep.py <node_name> --domain node processed_gpu_sweep training_data/dgemm/ training_data/stream/
```
This will result in a processed_gpu_sweep.h5 output.

## 4b) Process the CPU frequency sweeps to condition the data for model training.
For the CPU Agent use the local process_cpu_sweeps.py on the sweep folders (paths may vary):
```
./process_cpu_frequency_sweep.py <node_name> processed_cpu_sweep ${GEOPM_WORKDIR}/dgemm_frequency_sweep ${GEOPM_WORKDIR}/arithmetic_intensity_frequency_sweep/
```
This will result in a processed_cpu_sweep.h5 output.

## 5a) Train a neural network model for the GPU using the processed trace data.
For GPU:
    Use the geopm/tutorial/pytorch_agent/train_gpu_model-pytorch.py to create a model with the name gpu_control.pt
    ```
     ./train_gpu_model-pytorch.py processed_gpu_sweep.h5 gpu_control.pt
    ```

## 5b) Train a neural network model for the CPU using the processed trace data.
For CPU:
    Use the geopm/tutorial/pytorch_agent/train_gpu_model-pytorch.py to create a model with the name gpu_control.pt
    ```
     ./train_cpu_model-pytorch.py processed_cpu_sweep.h5 cpu_control.pt
    ```

## 6a) Directly execute the GPU agent with a trained model
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

## 6b) Execute the CPU agent with a trained model
The NN model must be in the directory the job is being launched from and must be named cpu_control.pt
A geopmbench config of interest should be provided as bench.config.  See GEOPM documentation on how to generate this.
Ranks has been left empty in the example code below and should be provided by the user.
```
EXE=geopmbench
APPOPTS=bench.config
RANKS=

export GEOPM_PLUGIN_PATH=${GEOPM_SOURCE}/tutorial/pytorch_agent
geopmagent -a cpu_torch -pNAN,NAN,0 > phi0.policy
geopmlaunch impi -ppn ${RANKS} -n ${RANKS} --geopm-policy=phi0.policy --geopm-ctl=process --geopm-report=dgemm-cpu-torch-phi0.report --geopm-agent=cpu_torch -- $EXE $APPOPTS

geopmagent -a cpu_torch -pNAN,NAN,0.4 > phi40.policy
geopmlaunch impi -ppn ${RANKS} -n ${RANKS} --geopm-policy=phi40.policy --geopm-ctl=process --geopm-report=dgemm-cpu-torch-phi40.report --geopm-agent=cpu_torch -- $EXE $APPOPTS
```

## 7a) Use the GEOPM integration infrastructure to execute the agents with a trained model
First setup the user environment

```
export GEOPM_PLUGIN_PATH=${GEOPM_SOURCE}/tutorial/pytorch_agent
export PYTHONPATH=$PYTHONPATH:$PWD/python_experiment/
export GEOPM_CPU_NN_PATH=$PWD/cpu_control.pt
```

Then generate a run script
```
$GEOPM_SOURCE/integration/experiment/gen_slurm.sh 1 parres_dgemm gpu_torch
```

Next manually update the output test.sbatch scripte to point to the tutorial experiment
scripts.  The final line should resemble
```
${GEOPM_SOURCE}/tutorial/pytorch_agent/pytorch_experiment/run_gpu_torch_parres_dgemm.py \
```

Finally run the experiment using sbatch
```
sbatch test.sbatch
```
