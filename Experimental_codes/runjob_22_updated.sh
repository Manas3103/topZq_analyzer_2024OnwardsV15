#!/bin/bash

# ==========================
#   Input arguments
# ==========================
input_dir=$1
output_file=$2  # Output ROOT file
stderr_file=$3  # Log file
nJobs=${4:-1}   # Default 1 if not provided

# ==========================
#   Configuration variables
# ==========================
jobconfmod="jobconfiganalysis_2022"
eos_output_dir="/eos/uscms/store/user/msahoo/test/"

echo "==================== JOB STARTED ===================="
echo "Running analysis with the following parameters:"
echo "Input directory: $input_dir"
echo "Output file: $output_file"
echo "EOS Output Directory: $eos_output_dir"

# ==========================
#   Environment setup
# ==========================
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

# ==========================
#   Handle special splitting
# ==========================
if [ "$nJobs" -gt 1 ]; then
    echo "Special case: splitting $input_dir into $nJobs jobs"

    # List all files in the directory
    allfiles=($(cat "$input_dir"))
    total=${#allfiles[@]}
    batch_size=$(( (total + nJobs - 1) / nJobs ))  # ceil division

    echo "Total files: $total | Batch size: $batch_size per job"

    for ((i=0;i<nJobs;i++)); do
        start=$((i*batch_size))
        end=$((start+batch_size-1))
        if [ $end -ge $total ]; then end=$((total-1)); fi

        batch_files=("${allfiles[@]:$start:$((end-start+1))}")
        batch_output="${output_file%.root}_part$((i+1)).root"
        batch_log="${stderr_file%.log}_part$((i+1)).log"

        echo "Running sub-job $((i+1)) on files: ${batch_files[@]}"
        ./processnanoaod_v.py "${batch_files[@]}" "Analyzed/$batch_output" "$jobconfmod" > "$batch_log" 2>&1 &
    done

    wait
    echo "All sub-jobs finished."
    exit 0
fi

# ==========================
#   Normal single-job flow
# ==========================
echo "Listing all files before execution:"
ls -alh

# Ensure output directory exists
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

# ==========================
#   Post-processing
# ==========================
echo "Listing files after execution:"
ls -lh

output_files=("$local_output_path" "$stderr_file" "${log_file}")

for file in "${output_files[@]}"; do
    if [ -f "$file" ]; then
        echo "Copying $file to EOS..."
        xrdcp -d 3 -f "$file" "root://cmseos.fnal.gov/${eos_output_dir}/$(basename "$file")"

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

# ==========================
#   Cleanup
# ==========================
if [ -n "${_CONDOR_SCRATCH_DIR}" ]; then
    echo "Cleaning up scratch directory..."
    rm -rf ${_CONDOR_SCRATCH_DIR}/*
    echo "Job Completed and scratch directory cleaned."
else
    echo "Running locally, no cleanup needed."
fi

echo "==================== JOB FINISHED ===================="
exit 0

