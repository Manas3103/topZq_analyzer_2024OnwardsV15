#!/bin/bash

# Input validation
if [ $# -ne 2 ]; then
    echo "Error: Required arguments missing"
    echo "Usage: $0 <filename> <crosssection> "
    exit 1
fi

# Input arguments
filename=$1
crosssection=$2
luminosity=110.0


start_time=$(date +%s)


# Convert EOS path to xrootd URL if needed
xrootd_filename="root://cmseos.fnal.gov/${filename}"

echo "Processing histogram with the following parameters:"
echo "File name: $xrootd_filename"
echo "Cross section: $crosssection"
echo "Luminosity: $luminosity"

# Define EOS output directory
eos_output_dir="root://cmseos.fnal.gov//store/user/msahoo/2024_Analysed_hist/"
handle_error() {
    echo "Error: $1"
    exit 1
}

# Determine execution environment
if [ -z "${_CONDOR_SCRATCH_DIR}" ] ; then
    echo "Running Interactively"
else
    echo "Running in Batch (HTCondor)"
    cd ${_CONDOR_SCRATCH_DIR}

    source /cvmfs/cms.cern.ch/cmsset_default.sh
    export SCRAM_ARCH=el9_amd64_gcc10

    if [ ! -d "CMSSW_13_3_3" ]; then
        eval `scramv1 project CMSSW CMSSW_13_3_3`
    fi  

    cd CMSSW_13_3_3/src
    cmsenv
    eval `scramv1 runtime -sh`
    cd -
fi

# Create output directory
output_dir="Analysed"
mkdir -p "${output_dir}" || handle_error "Failed to create output directory"

# Determine if DATA or MC
if [ "$crosssection" = "1" ]; then
    extra_flag="data"
else
    extra_flag="mc"
fi

#Run Python RDF version
# python3 create_hist_rdf_modified.py\
python3 create_hist_rdf.py \
    --filename "${xrootd_filename}" \
    --cross_section ${crosssection} \
    --luminosity ${luminosity} \
    --tree_name outputTree \
    --config hist_config.json \
    --extra ${extra_flag} \
    || handle_error "Processing failed"

# Find output file
HIST_FILE=$(ls -t *_hist.root 2>/dev/null | head -n1)

if [ -n "${HIST_FILE}" ]; then
    echo "Moving file to Analysed/ directory..."
    mv -f "${HIST_FILE}" "${output_dir}/" || handle_error "Failed to move file"

    HIST_FILE_PATH="${output_dir}/${HIST_FILE}"

    echo "Copying file ${HIST_FILE_PATH} to EOS..."
    xrdcp -f "${HIST_FILE_PATH}" "${eos_output_dir}" || handle_error "Failed to copy file to EOS"

    # Extract pure EOS path (without root://...)
    EOS_PATH="/store/user/msahoo/2024_Analysed_hist/${HIST_FILE}"

    echo "Verifying file on EOS..."
    xrdfs root://cmseos.fnal.gov stat "${EOS_PATH}" || handle_error "Verification failed"

    echo "File successfully copied to EOS: ${eos_output_dir}${HIST_FILE}"

else
    handle_error "No output root file found"
fi

echo "Job Completed."

end_time=$(date +%s)
runtime=$((end_time - start_time))

echo "-----------------------------------------"
echo "Total Runtime: ${runtime} seconds"
echo "Total Runtime: $((${runtime}/60)) minutes"
echo "-----------------------------------------"
