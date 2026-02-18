#!/bin/bash

# Input arguments
input_dir=$1
output_file=$2  # Output ROOT file
stderr_file=$3  # Log file

# Configuration variables
jobconfmod="jobconfiganalysis_17"
eos_output_dir="/eos/uscms/store/user/msahoo/test/"

echo "==================== JOB STARTED ===================="
echo "Running analysis with the following parameters:"
echo "Input directory: $input_dir"
echo "Output file: $output_file"
echo "EOS Output Directory: $eos_output_dir"

# Determine execution environment
if [ -z "${_CONDOR_SCRATCH_DIR}" ] ; then
    start_time=$(date +%s)
    echo "Running Interactively"
    log_file="./${stderr_file}"  # Local execution log file
else
    echo "Running in Batch (HTCondor)"
    cd ${_CONDOR_SCRATCH_DIR}
    echo "Condor Scratch Directory: ${_CONDOR_SCRATCH_DIR}"

    source /cvmfs/cms.cern.ch/cmsset_default.sh
    export SCRAM_ARCH=el8_amd64_gcc10

    # Use an existing CMSSW release if available
    if [ ! -d "CMSSW_12_3_4" ]; then
        eval `scramv1 project CMSSW CMSSW_12_3_4`
    fi

    cd CMSSW_12_3_4/src
    cmsenv
    eval `scramv1 runtime -sh`
    cd - ;

    echo "CMSSW environment setup done."
    log_file="${_CONDOR_SCRATCH_DIR}/${stderr_file}"
fi

# List all files in current directory
echo "Listing all files before execution:"
ls -alh

# Ensure the output file directory exists
output_dir="Analyzed"
if [ ! -d "${output_dir}" ]; then
    echo "Output directory ${output_dir} does not exist, creating it..."
    mkdir -p "${output_dir}" || { echo "Failed to create directory ${output_dir}"; exit 1; }
else
    echo "Output directory ${output_dir} exists, proceeding with job..."
fi

local_output_path="${output_dir}/${output_file}"
echo "Final output file path: ${local_output_path}"

# Run the analysis job
echo "Running analysis script..."
./processnanoaod_v.py "$input_dir" "$local_output_path" "$jobconfmod" > "$log_file" 2>&1 || { echo "Error: Processing failed"; exit 1; }

echo "==================== JOB COMPLETED ===================="
cat "$log_file"

# Check if the output file exists before copying
echo "Listing files after execution:"
ls -lh

# Define all files that need to be copied
output_files=("$local_output_path" "$stderr_file" "${log_file}")

for file in "${output_files[@]}"; do
    if [ -f "$file" ]; then
        echo "Copying $file to EOS..."
        xrdcp -d 3 -f "$file" "root://cmseos.fnal.gov/${eos_output_dir}/$(basename "$file")"

        # Verify transfer
        xrdfs root://cmseos.fnal.gov/ stat "${eos_output_dir}/$(basename "$file")"
        if [ $? -eq 0 ]; then
            echo "✅ Successfully copied: ${eos_output_dir}/$(basename "$file")"
        else
            echo "❌ ERROR: Failed to copy $file to EOS!"
        fi
    else
        echo "⚠️ Warning: Expected output file $file not found."
    fi
done

# Cleanup if running in Condor
if [ -n "${_CONDOR_SCRATCH_DIR}" ]; then
    echo "Cleaning up scratch directory..."
    rm -rf ${_CONDOR_SCRATCH_DIR}/*
    echo "Job Completed and scratch directory cleaned."
else
    echo "Running locally, no cleanup needed."
fi

echo "==================== JOB FINISHED ===================="
exit 0

