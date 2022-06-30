# Python Agent Tutorial

This tutorial demonstrates how to implement a python agent in geopm. Specifically,
the tutorial shows how to create an agent that can use a tensorflow model to
select frequency controls on a system with CPUs and GPUs.

The following steps are required to use this agent:
1. Perform frequency sweeps to generate data for model training.
2. Process the frequency sweeps to condition the data for model training.
3. Train a neural network model using the processed trace data.
4. Execute the agent with a trained model.

## 1) Perform frequency sweeps
Use GEOPM to run a suite of workloads with varying performance and energy
responses to GPU frequency settings. Be sure to include at least the following
signals in your traces and reports:
 * NVML::FREQUENCY@board\_accelerator
 * NVML::TOTAL\_ENERGY\_CONSUMPTION@board\_accelerator
 * NVML::POWER@board\_accelerator
 * NVML::UTILIZATION\_ACCELERATOR@board\_accelerator
 * DCGM::SM\_ACTIVE@board\_accelerator
 * DCGM::DRAM\_ACTIVE@board\_accelerator

Ensure each file in the sweep contains a period-separated set of properties
that identify each step in the sweep, as in:

```
<appname>.gpufreq_<freq_in_GHz>.memratio_<mem_ratio>.precision_<precision>.trial_<trial>.report
<appname>.gpufreq_<freq_in_GHz>.memratio_<mem_ratio>.precision_<precision>.trial_<trial>.trace-<hostname>
```

The following are possible fields in the name:
 * appname: A name that identifies the application. E.g., stream or dgemm
 * freq\_in\_GHz: GPU core frequency limit applied on this report or trace
 * memratio: mem ratio for runs of the mixbench benchmark, or NaN.
 * precision: precision for runs of the mixbench benchmark, or NaN.
 * trial: number of the trial, for repeated executions.
 * hostname: name of the server associated with the trace file.

The GEOPM integration infrastructure has built-in support for frequency sweeps.
You can also use the agent in this tutorial in a shell script to generate a
frequency sweep. For example:

```bash
out_dir=${HOME}/my_sweep_$$
mkdir -p "${out_dir}"
host=$(hostname)
APPNAME=myappname
EXEC=/path/to/your/workload

supported_freqs_mhz=$(nvidia-smi -q -d SUPPORTED_CLOCKS -i 0 | awk -e '/Graphics/ { print $3 }')
max_cpu_freq=$(geopmread FREQUENCY_MAX board 0)

for trial in {1..3}
do
    for gpu_freq in $supported_freqs_mhz
    do
        agent='monitor'
        set -x
        ./ml_agent.py --freq-gpu=${gpu_freq}e6 --freq-core=${max_cpu_freq} \
            --report="${out_dir}/agent_${agent}.${APPNAME}.gpufreq_${gpu_freq}.trial_${trial}.report" \
            --trace="${out_dir}/agent_${agent}.${APPNAME}.gpufreq_${gpu_freq}.trial_${trial}.trace-${host}" \
            --control-period=0.05 \
            -- \
            ${EXEC}
        set +x
        sleep 10
    done
done
```

Try generating sweeps over multiple applications, or over multiple
configurations of one application (such as different inputs to a benchmark).

## 2) Process the frequency sweeps
After finishing the previous step, you should have access to one or more
directories that contain GEOPM traces and reports from your frequency sweeps.
Feed those directories to the process\_gpu\_frequency\_sweep.py file to obtain
a single file with all results. Example:

```bash
./process_gpu_frequency_sweep.py $(hostname) mysweepdata ${HOME}/my_sweep_1234
```

## 3) Train a model
Now you should have a .h5 file containing all of your frequency sweep data,
along with some additional derived data. Use that file as an input to the
train\_gpu\_model.py script to create your model. This may take a long time,
depending on how much trace data is available for training. The script will
display time estimates for each of its 5 iterations over the training data.

```bash
./train_gpu_model.py ./mysweepdata.h5 mygpumodel
```

## 4) Execute the agent with a trained model
It's time to use your model! Try running the ml\_agent.py script as a wrapper
for an application you want to execute. Be sure to provide your trained model!
Example:

```bash
./ml_agent.py \
    --phi=0 \
    --gpu-model ./mygpumodel \
    --freq-gpu 1.536e9 \
    --freq-core 3.7e9 \
    --report tutorial.report \
    --control-period 0.05 \
    -- \
    /run/your/application with its args here
```

Try it out with different values of phi, from 0 to 100. Take a look inside
tutorial.report to see how the agent is impacting execution time, energy,
and selected frequencies.
