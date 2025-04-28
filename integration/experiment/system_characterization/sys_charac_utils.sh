set -e
set -x

if [[ -z "$SWEEP_TYPE" ]]; then
  echo "Please set all variables under set_vars.sh"
  exit 1
fi

INIT_CONTROLS_LIST="${EMPTY_SWEEP_OUTPUT_DIR}/init_controls-${HOSTNAME}.lst"
CORE_SKIP_CNT=`echo -e ${CORE_SKIP_LIST//,/\\n} | wc -l`
CPU_PHYSICAL_CORE_CNT=$(geopmread -d | grep core | awk '{print $2}')
CPU_PHYSICAL_CORE_CNT_AVAILABLE=$(( ${CPU_PHYSICAL_CORE_CNT} - ${CORE_SKIP_CNT} ))

mkdir -p $EMPTY_SWEEP_OUTPUT_DIR
cd $EMPTY_SWEEP_OUTPUT_DIR
python3 -c "from integration.experiment import machine; machine.Machine().save()"
cd -
echo "Writing logs and reports to ${EMPTY_SWEEP_OUTPUT_DIR}"


set_rank_bind_list () {

         SEP=","
         RANK_BIND_LIST=""
         NEXT_CORE_AVAILABLE=0
         for  (( rank=0; rank<$RANK_COUNT; rank++)); do
         
            # Searching for an available core
            while [[ "${SEP}${CORE_SKIP_LIST}${SEP}" =~ "${SEP}${NEXT_CORE_AVAILABLE}${SEP}" ]]; do
               NEXT_CORE_AVAILABLE=$(( ${NEXT_CORE_AVAILABLE} + 1 ))
            done
         
            ## Assigning rank to core# NEXT_CORE_AVAILABLE"
            if (( NEXT_CORE_AVAILABLE >= CPU_PHYSICAL_CORE_CNT )); then
               echo "*** Warning: Insufficient number of available cores ***"
               break
            else
               RANK_BIND_LIST="${RANK_BIND_LIST}:${NEXT_CORE_AVAILABLE}"
            fi
         
            NEXT_CORE_AVAILABLE=$(( ${NEXT_CORE_AVAILABLE} + 1 ))
         done

         # remove the first occurring of ":"
         RANK_BIND_LIST="${RANK_BIND_LIST/:/}"
}


launch_sweep () {

          geopmlaunch pals \
            -n ${RANK_COUNT} -ppn ${RANK_COUNT} \
            --cpu-bind list:${RANK_BIND_LIST} \
            --geopm-init-control=$INIT_CONTROLS_LIST \
            --geopm-ctl=application \
            --geopm-preload \
            --geopm-profile="${PROGRAM_NAME}" \
            --geopm-report=${REPORT_FILE_PATH} \
            --geopm-report-signals=${GEOPM_SIGNALS} \
            --geopm-program-filter=${PROGRAM_NAME} \
            -- ${BINARY_PATH_PLUS_FLAGS} \
            &> ${LOG_FILE_PATH}

          sleep 5
}


launch_cpu_sweep () {

                RANK_COUNT=${CPU_PHYSICAL_CORE_CNT_AVAILABLE} # using all the available cores
                CORE_MIN_FREQ=$(geopmread CPU_FREQUENCY_MIN_AVAIL board 0)
                CORE_MAX_FREQ=$(geopmread CPU_FREQUENCY_MAX_AVAIL board 0)
                CORE_FREQ_STEP=$(geopmread CPU_FREQUENCY_STEP board 0)
                UNCORE_MIN_FREQ=$(geopmread CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0)
                UNCORE_MAX_FREQ=$(geopmread CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0)
                UNCORE_FREQ_STEP=100000000
                GEOPM_SIGNALS="${EXTRA_SIGNALS},MSR::QM_CTR_SCALED_RATE@package,CPU_UNCORE_FREQUENCY_STATUS@package,MSR::CPU_SCALABILITY_RATIO@package,CPU_FREQUENCY_MIN_CONTROL@package,CPU_UNCORE_FREQUENCY_MIN_CONTROL@package"
        
                set_rank_bind_list

                for ((p="$CORE_MIN_FREQ"; p<="$CORE_MAX_FREQ"; p=p+"$CORE_FREQ_STEP")); do
                    for ((u="$UNCORE_MIN_FREQ"; u<="$UNCORE_MAX_FREQ"; u=u+"$UNCORE_FREQ_STEP")); do
                
                      printf "${EXTRA_CONTROLS}" > $INIT_CONTROLS_LIST
                      printf "%s\n" \
                             "MSR::PQR_ASSOC:RMID board 0 0" \
                             "MSR::QM_EVTSEL:RMID board 0 0" \
                             "MSR::QM_EVTSEL:EVENT_ID board 0 2" \
                             "CPU_FREQUENCY_MIN_CONTROL board 0 ${p}" \
                             "CPU_FREQUENCY_MAX_CONTROL board 0 ${p}" \
                             "CPU_UNCORE_FREQUENCY_MIN_CONTROL board 0 ${u}" \
                             "CPU_UNCORE_FREQUENCY_MAX_CONTROL board 0 ${u}" \
                             >> $INIT_CONTROLS_LIST
        
                      for ((t=0; t<"$TRIAL_COUNT"; t++)); do
                
                          echo "=== CPU POWER-FREQ SWEEP: Trial $t, CPU CORE $p, CPU UNCORE $u, BOARD POWER $l==="
                          REPORT_FILE_PATH="${EMPTY_SWEEP_OUTPUT_DIR}/${PROGRAM_NAME}_boardcap_${l}_core_${p}_uncore_${u}_trial_${t}_cpufreqsweep-${HOSTNAME}.report"
                          LOG_FILE_PATH="${EMPTY_SWEEP_OUTPUT_DIR}/${PROGRAM_NAME}_boardcap_${l}_core_${p}_uncore_${u}_trial_${t}_cpufreqsweep-${HOSTNAME}.log"

                          launch_sweep
                
                      done
                    done
                done

}


launch_gpu_sweep () {

                RANK_COUNT=1 # 1 CPU process is sufficient to launch sweeps across all gpus
                GPU_CORE_MIN_FREQ=$(geopmread GPU_CORE_FREQUENCY_MIN_AVAIL board 0)
                GPU_CORE_MAX_FREQ=$(geopmread GPU_CORE_FREQUENCY_MAX_AVAIL board 0)
                GPU_CORE_FREQ_STEP=$(geopmread GPU_CORE_FREQUENCY_STEP board 0)
                GEOPM_SIGNALS="${EXTRA_SIGNALS},GPU_CORE_FREQUENCY_STATUS@board,GPU_CORE_FREQUENCY_MIN_CONTROL@board"
                
                set_rank_bind_list

                for ((p="$GPU_CORE_MIN_FREQ"; p<="$GPU_CORE_MAX_FREQ"; p=p+"$GPU_CORE_FREQ_STEP")); do
        
                      printf "${EXTRA_CONTROLS}" > $INIT_CONTROLS_LIST
                      printf "%s\n" \
                             "GPU_CORE_FREQUENCY_MIN_CONTROL board 0 ${p}" \
                             "GPU_CORE_FREQUENCY_MAX_CONTROL board 0 ${p}" \
                             >> $INIT_CONTROLS_LIST
                
                      for ((t=0; t<"$TRIAL_COUNT"; t++)); do
                          echo "=== GPU POWER-FREQ SWEEP: Trial $t, GPU CORE $p, BOARD POWER $l==="
                          REPORT_FILE_PATH="${EMPTY_SWEEP_OUTPUT_DIR}/${PROGRAM_NAME}_boardcap_${l}_gpucore_${p}_trial_${t}_gpufreqsweep-${HOSTNAME}.report"
                          LOG_FILE_PATH="${EMPTY_SWEEP_OUTPUT_DIR}/${PROGRAM_NAME}_boardcap_${l}_gpucore_${p}_trial_${t}_gpufreqsweep-${HOSTNAME}.log"

                          launch_sweep

                      done
                done


}

