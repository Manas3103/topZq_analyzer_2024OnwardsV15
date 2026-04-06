#!/bin/bash

# Input validation
if [ $# -ne 2 ]; then
    echo "Error: Required arguments missing"
    echo "Usage: $0 <filename> <crossection> "
    exit 1
fi

# Input arguments
filename=$1
crossection=$2


# Convert EOS path to xrootd URL if needed
xrootd_filename="root://cmseos.fnal.gov/${filename}"

echo "Processing histogram with the following parameters:"
echo "File name: $xrootd_filename"
echo "crossection: $crossection"
#echo "sum of genweight: $sum_genweight"

# Define EOS output directory
eos_output_dir="root://cmseos.fnal.gov//store/user/msahoo/"

# Function to handle errors
handle_error() {
    echo "Error: $1"
    exit 1
}

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
    export SCRAM_ARCH=el9_amd64_gcc10

    # Use an existing CMSSW release if available
    if [ ! -d "CMSSW_13_3_3" ]; then
        eval `scramv1 project CMSSW CMSSW_13_3_3`
    fi

    cd CMSSW_13_3_3/src
    cmsenv
    eval `scramv1 runtime -sh`
    cd - ;

    echo "CMSSW environment setup done."
    log_file="${_CONDOR_SCRATCH_DIR}/${stderr_file}"
fi


# Create output directory
output_dir="Analysed"
mkdir -p "${output_dir}" || handle_error "Failed to create output directory ${output_dir}"
echo "Output directory ${output_dir} ready"

# Run the analysis job with xrootd path
root -l -q "create_normalized_histogram.C(\"${xrootd_filename}\",${crossection})" || handle_error "Processing failed"

# List directory contents
#ls -alh

# Transfer output to EOS
HIST_FILE=$(ls -t _hist.root 2>/dev/null | head -n1)
echo ${HIST_FILE}
if [ -n "${HIST_FILE}" ]; then
    echo "Copying file ${HIST_FILE} to EOS..."
    xrdcp -f "${HIST_FILE}" "${eos_output_dir}" || handle_error "Failed to copy file to EOS"

    # Verify transfer
    xrdfs root://cmseos.fnal.gov/ stat "${eos_output_dir}${HIST_FILE}" || handle_error "Failed to verify file on EOS"
    echo "File successfully copied to EOS: ${eos_output_dir}${HIST_FILE}"
else
    handle_error "No output root file found"
fi

# Cleanup
if [ -n "${_CONDOR_SCRATCH_DIR}" ]; then
    echo "Cleaning up scratch directory..."
    rm -rf "${_CONDOR_SCRATCH_DIR}"/* || echo "Warning: Cleanup failed"
    echo "Job completed and scratch directory cleaned."
else
    echo "Running locally, no cleanup needed."
fi

echo "Job Completed."
