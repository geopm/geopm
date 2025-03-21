#!/bin/bash
set -e
source ~/.geopmrc
HACC_RUN_DIR="${HACC_RUN_DIR?-Set HACC_RUN_DIR to the location of run_mpiexec.sh}"
GEOPM_SOURCE="${GEOPM_SOURCE?-Set GEOPM_SOURCE to the location of geopm source code}"
source "${GEOPM_SOURCE}/integration/config/run_env.sh"
PROFILE_NAME='hacc'
EXPERIMENT_NAME="${PROFILE_NAME}-geopm-power-sweep"
TRIAL_COUNT="${TRIAL_COUNT:-5}"
MIN_POWER="${MIN_POWER:-2400}"
MAX_POWER="${MAX_POWER:-4000}"
POWER_STEP="${POWER_STEP:-100}"
TRIAL_COUNT="${TRIAL_COUNT:-5}"
SWEEP_OUTPUT_DIR="${SWEEP_OUTPUT_DIR:-${HACC_RUN_DIR}/power_sweep}"

for ((p="$MIN_POWER"; p<="$MAX_POWER"; p=p+"$POWER_STEP")); do
    printf 'MSR::PLATFORM_POWER_LIMIT:PL1_POWER_LIMIT board 0 %s\nMSR::PLATFORM_POWER_LIMIT:PL1_TIME_WINDOW board 0 0.013\nMSR::PLATFORM_POWER_LIMIT:PL1_LIMIT_ENABLE board 0 1\nMSR::PLATFORM_POWER_LIMIT:PL1_CLAMP_ENABLE board 0 1\n' "${p}" > "/tmp/power_sweep_controls_${p}.txt"
done

cd "$HACC_RUN_DIR"
mkdir -p "$SWEEP_OUTPUT_DIR"
echo "Writing logs and reports to ${SWEEP_OUTPUT_DIR}"
for ((t=0; t<"$TRIAL_COUNT"; t++)); do
  for ((p="$MIN_POWER"; p<="$MAX_POWER"; p=p+"$POWER_STEP")); do
    echo "================= Trial $t, $p W power cap ================="
    GEOPM_SIGNALS="BOARD_POWER@board,BOARD_POWER_LIMIT_CONTROL@board,BOARD_ENERGY@board"
    geopmlaunch pals \
      -n 1 -ppn 1 \
      -hosts="${HOSTNAME}" \
      --geopm-init-control="/tmp/power_sweep_controls_${p}.txt" \
      --geopm-ctl=application \
      --geopm-preload \
      --geopm-profile="${PROFILE_NAME}" \
      --geopm-report="${SWEEP_OUTPUT_DIR}/${EXPERIMENT_NAME}_${p}_${t}.report-${HOSTNAME}" \
      --geopm-report-signals=${GEOPM_SIGNALS} \
      --geopm-program-filter=hacc_tpm.CORE-AVX512.omp_offload.hostreg.mpich \
      -- /bin/bash ./run_mpiexec.sh -hosts=${HOSTNAME} -fcm=4 -hr=1 -n=1 -rpn=96 --ranks_per_socket=48 -ng=912 -g=6x4x4 2>&1 \
      > "${SWEEP_OUTPUT_DIR}/${EXPERIMENT_NAME}_${p}_${t}.log-${HOSTNAME}"

    sleep 5
    # Extract the figure of merit from the app log into the GEOPM report
    awk -e '/^step/ { steptime=$3 } /\^3 grid$/ {particles=int($1)^3} END {print "Figure of Merit: " particles/steptime}' \
      "${SWEEP_OUTPUT_DIR}/${EXPERIMENT_NAME}_${p}_${t}.log-${HOSTNAME}" \
      >> "${SWEEP_OUTPUT_DIR}/${EXPERIMENT_NAME}_${p}_${t}.report-${HOSTNAME}"
    sleep 45
  done
done
