#!/bin/bash

source ${GEOPM_SOURCE}/integration/config/run_env.sh
OUTPUT_DIR=${GEOPM_WORKDIR}/parres_dgemm_freq_sweep

${GEOPM_SOURCE}/integration/experiment/gpu_frequency_sweep/run_gpu_frequency_sweep_parres_dgemm.py \
    --node-count=1 \
    --output-dir=${OUTPUT_DIR} \
    --trial-count=2 \
    --enable-traces \
    --min-uncore-frequency=NAN \
    --min-frequency=2.4e9 \
    --max-frequency=2.5e9 \
    --step-frequency=1e8 \
    --max-gpu-frequency=1530e6 \
    --min-gpu-frequency=420e6 \
    --geopm-trace-signals=NVML::FREQUENCY@board_accelerator,NVML::POWER@board_accelerator,NVML::UTILIZATION_ACCELERATOR@board_accelerator,NVML::TOTAL_ENERGY_CONSUMPTION@board_accelerator,NVML::UTILIZATION_MEMORY@board_accelerator,POWER_PACKAGE@package,POWER_DRAM@board_memory,FREQUENCY@core,TEMPERATURE_CORE@core,MSR::UNCORE_PERF_STATUS:FREQ@package,QM_CTR_SCALED_RATE@package,ENERGY_DRAM@board_memory,ENERGY_PACKAGE@package,MSR::PPERF:PCNT@package,MSR::MPERF:MCNT@package,MSR::APERF:ACNT@package,INSTRUCTIONS_RETIRED@core,CYCLES_THREAD@core,MSR::IA32_PMC0:PERFCTR@core,MSR::IA32_PMC1:PERFCTR@core,MSR::IA32_PMC2:PERFCTR@core,MSR::IA32_PMC3:PERFCTR@core,NVML::CPU_ACCELERATOR_ACTIVE_AFFINITIZATION@core 

mkdir -p ${OUTPUT_DIR}/results
python3 gpu_frequency_sweep/gen_region_summary.py --output-dir ${OUTPUT_DIR} --analysis-dir ${OUTPUT_DIR}/results


