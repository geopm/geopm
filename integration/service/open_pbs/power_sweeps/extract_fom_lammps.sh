#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
# Extract the figure of merit (FOM) from LAMMPS application
# logs and append to the corresponding GEOPM report files.
#
# Usage:
#   extract_fom_lammps.sh <sweep_output_dir>
#       Process all .log-* files in the given directory.
#
#   extract_fom_lammps.sh <log_file> <report_file>
#       Process a single log/report file pair.
#
set -e

extract_fom() {
    local log_file="$1"
    local report_file="$2"

    if [[ ! -f "$log_file" ]]; then
        echo "WARNING: Log file not found: ${log_file}" >&2
        return 1
    fi

    local fom
    fom=$(awk -F, '/^FOM,/ { print $2 }' "$log_file")

    if [[ -z "$fom" ]]; then
        echo "WARNING: Could not extract FOM from ${log_file}" >&2
        return 1
    fi

    echo "Figure of Merit: ${fom}" >> "$report_file"
    # echo "${log_file} -> FOM: ${fom}"
}

if [[ $# -eq 2 ]]; then
    # Single file mode
    extract_fom "$1" "$2"
elif [[ $# -eq 1 ]]; then
    # Directory mode: process all log files
    sweep_dir="$1"
    if [[ ! -d "$sweep_dir" ]]; then
        echo "ERROR: ${sweep_dir} is not a directory" >&2
        exit 1
    fi

    shopt -s nullglob
    log_files=("${sweep_dir}"/*.log-*)
    shopt -u nullglob

    if [[ ${#log_files[@]} -eq 0 ]]; then
        echo "ERROR: No .log-* files found in ${sweep_dir}" >&2
        exit 1
    fi

    for log_file in "${log_files[@]}"; do
        # Derive report file: replace .log- with .report-
        report_file="${log_file/.log-/.report-}"
        if [[ ! -f "$report_file" ]]; then
            echo "WARNING: Report file not found for ${log_file}, skipping" >&2
            continue
        fi
        extract_fom "$log_file" "$report_file" || true
    done
else
    echo "Usage: $(basename "$0") <sweep_output_dir>" >&2
    echo "       $(basename "$0") <log_file> <report_file>" >&2
    exit 1
fi
