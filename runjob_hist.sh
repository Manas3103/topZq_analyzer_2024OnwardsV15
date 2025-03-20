#!/bin/bash

# Read input parameters
root_file=$1         # ROOT file path
cross_section=$2     # Cross section value
sum_genweight=$3     # Sum of gen weights

# EOS Output Directory
eos_output_dir="/eos/uscms/store/user/msahoo/"

echo "==================== JOB STARTED ===================="
echo "Processing ROOT file: $root_file"
echo "Cross Section: $cross_section"
echo "Sum Gen Weight: $sum_genweight"
echo "EOS Output Directory: $eos_output_dir"

# Convert to xrootd format (if needed)
if [[ "$root_file" == /eos/* ]]; then
    xrootd_file="root://cmseos.fnal.gov/$root_file"
else
    xrootd_file="$root_file"
fi

echo "Using xrootd path: $xrootd_file"

# Check if the ROOT file exists
# Extract only the relative EOS path (removing root:// prefix)
if [[ "$root_file" == root://cmseos.fnal.gov//* ]]; then
    eos_path="${root_file#root://cmseos.fnal.gov/}"
else
    eos_path="$root_file"
fi

# Debug print
echo "Checking file existence with: xrdfs root://cmseos.fnal.gov stat $eos_path"

if [ $? -ne 0 ]; then
    echo "Error: ROOT file $root_file not found!"
    exit 1
fi

# Setup CMSSW environment
if [ -z "${_CONDOR_SCRATCH_DIR}" ]; then
    echo "Running Interactively"
else
    echo "Running in Batch (HTCondor)"
    cd ${_CONDOR_SCRATCH_DIR}
    source /cvmfs/cms.cern.ch/cmsset_default.sh
    export SCRAM_ARCH=el8_amd64_gcc10

    if [ ! -d "CMSSW_12_3_4" ]; then
        eval `scramv1 project CMSSW CMSSW_12_3_4`
    fi

    cd CMSSW_12_3_4/src
    cmsenv
    eval `scramv1 runtime -sh`
    cd - ;
fi

# Define output file
output_file="$(basename "$root_file" .root)_hist.root"


# Run ROOT macro to create histogram
root -l -q "create_normalized_histogram.C(\"${xrootd_file}\", ${cross_section}, ${sum_genweight})"


#ls -lh
echo "Checking if $output_file exists..."
# Copy output to EOS
if [ -f "$output_file" ]; then
    echo "Copying $output_file to EOS..."
    xrdcp -d 3 -f "$output_file" "root://cmseos.fnal.gov/${eos_output_dir}/$(basename "$output_file")"

    # Verify transfer
    xrdfs root://cmseos.fnal.gov/ stat "${eos_output_dir}/$(basename "$output_file")"
    if [ $? -eq 0 ]; then
        echo "✅ Successfully copied: ${eos_output_dir}/$(basename "$output_file")"
    else
        echo "❌ ERROR: Failed to copy $output_file to EOS!"
    fi
else
    echo "⚠️ Warning: Expected output file $output_file not found."
fi

echo "==================== JOB FINISHED ===================="
exit 0

