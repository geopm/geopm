# Pytorch C++ Agent Tutorial

This tutorial demonstrates how to implement a libtorch based C++ agent in GEOPM.
Specifically, the tutorial shows how to create a C++ agent that can use a pytorch model
controls for CPUs based on the following signals:
```
```
CPU_POWER
CPU_FREQUENCY_STATUS
CPU_PACKAGE_TEMPERATURE
CPU_UNCORE_FREQUENCY_STATUS
MSR::QM_CTR_SCALED_RATE
CPU_INSTRUCTIONS_RETIRED
CPU_CYCLES_THREAD
CPU_ENERGY
MSR::APERF:ACNT
MSR::MPERF:MCNT
MSR::PPERF:PCNT
```

An additional user input knob "CPU_PHI" is also taken as an input.  This control is a unitless
input ranging from 0 to 1 with 0 skewing the agent toward performance oriented frequency selection
and 1 skewing the agent towards energy efficient frequency selection.

This tutorial is intended to show how this type of agent may be implemented, compiled, trained,
and used alongside a workload. For the CPU work the geopmbench benchmark is sufficient for training,
though the integrated arithmetic_intensity benchmark may also be used as shown in the integration
test..

For a fully realized pytorch agent an expanded workload list should be used.  Similarly an
expanded telemetry list may be beneficial.

The following steps are required to build and use this agent:
1. Acquire libtorch with CX11 ABI
2. Build the GEOPM C++ CPUTorchAgent
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

## 3) Perform CPU frequency sweeps to generate data for model training.
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
    --geopm-trace-signals=CPU_POWER@package,DRAM_POWER,CPU_FREQUENCY_STATUS@package,CPU_CORE_TEMPERATURE@package,CPU_PACKAGE_TEMPERATURE@package,CPU_UNCORE_FREQUENCY_STATUS@package,MSR::QM_CTR_SCALED_RATE@package,INSTRUCTIONS_RETIRED@package,CYCLES_THREAD@package,CPU_ENERGY@package,MSR::APERF:ACNT@package,MSR::MPERF:MCNT@package,MSR::PPERF:PCNT@package,TIME@package,MSR::CPU_SCALABILITY_RATIO@package,DRAM_ENERGY \

```
Min and max frequency may vary from system to system.

Additionally modify the test.sbatch script to set-up the QM signals:
```
geopmwrite MSR::PQR_ASSOC:RMID board 0 0
geopmwrite MSR::QM_EVTSEL:RMID board 0 0
geopmwrite MSR::QM_EVTSEL:EVENT_ID board 0 2
```

## 4b) Process the CPU frequency sweeps to condition the data for model training.
For the CPU Agent use the local process_cpu_sweeps.py on the sweep folders (paths may vary):
```
./process_cpu_frequency_sweep.py <node_name> processed_cpu_sweep ${GEOPM_WORKDIR}/dgemm_frequency_sweep ${GEOPM_WORKDIR}/arithmetic_intensity_frequency_sweep/
```
This will result in a processed_cpu_sweep.h5 output.

## 5) Train a neural network model for the CPU using the processed trace data.

Use the ./train_cpu_model-pytorch.py to create a model with the name cpu_control.pt
```
 ./train_cpu_model-pytorch.py processed_cpu_sweep.h5 cpu_control.pt
```

## 6b) Execute the CPU agent with a trained model
The NN model must be in the directory the job is being launched from and must be named cpu_control.pt or the user
must specify the neural net via the GEOPM_CPU_NN_PATH variable.

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

##7) Running Pytorch Agent Integration tests
Assuming the user enviroment has been setup for experiment & integration testing as covered in GEOPM documentation, the
Arithmetic Intensity Benchmark & MiniFE workload have been built in integration/apps, and the the TORCH_ROOT env var is setup
then the following command may be used to train and test a basic CPU-NN agent:
```
PYTHONPATH=${PWD}:${GEOPM_SOURCE}:${PYTHONPATH} /home/lhlawson/geopm_public_lhlawson-pytorch/tutorial/pytorch_agent/test/test_cpu_torch_agent.py
```
