#!/bin/bash
# prepare_all_datasets.sh
#
# Runs split_dataset.py for every dataset listed in a dataset-list file
# (format: "DAS_dataset  tag" per line, '#' comments allowed), then
# concatenates all the resulting per-dataset queue files into one
# master queue file for condor.
#
# Usage:
#   ./prepare_all_datasets.sh <datasets_list.txt> <files_per_job> <master_queue_out.txt>
#
# Example:
#   ./prepare_all_datasets.sh datasets_2024.txt 15 queue_all.txt

set -e

DATASETS_FILE=${1:?"Usage: $0 <datasets_list.txt> <files_per_job> <master_queue_out.txt>"}
FILES_PER_JOB=${2:?"Need files_per_job, e.g. 15"}
MASTER_QUEUE=${3:?"Need output master queue filename, e.g. queue_all.txt"}
# Optional: set PATH_PREFIX=DataMC_txt before calling this script if you plan to
# condor_submit from one directory above where this script/filelists/ live.
# Leave unset (default empty) if you condor_submit from this same directory.
PATH_PREFIX=${PATH_PREFIX:-}

echo "==================================================="
echo "Datasets file : ${DATASETS_FILE}"
echo "Files per job : ${FILES_PER_JOB}"
echo "Master queue  : ${MASTER_QUEUE}"
echo "Path prefix   : ${PATH_PREFIX:-<none>}"
echo "==================================================="

# Clear/create the master queue file
: > "${MASTER_QUEUE}"

while read -r line; do
    # Skip blank lines and comments
    [[ -z "$line" ]] && continue
    [[ "$line" =~ ^# ]] && continue

    dataset=$(echo "$line" | awk '{print $1}')
    tag=$(echo "$line" | awk '{print $2}')

    if [[ -z "$dataset" || -z "$tag" ]]; then
        echo "WARNING: skipping malformed line: $line"
        continue
    fi

    # Guard against a tag copy-pasted with a .root extension still attached
    # (e.g. from an old output_file column) - avoids "..._chunk4.root_chunk4.root"
    if [[ "$tag" == *.root ]]; then
        echo "WARNING: tag '${tag}' ends in .root - stripping extension"
        tag="${tag%.root}"
    fi

    echo ""
    echo "--- Processing dataset: ${dataset}  (tag=${tag}) ---"
    if [[ -n "${PATH_PREFIX}" ]]; then
        python3 split_dataset.py "${dataset}" "${FILES_PER_JOB}" "${tag}" --path-prefix "${PATH_PREFIX}"
    else
        python3 split_dataset.py "${dataset}" "${FILES_PER_JOB}" "${tag}"
    fi

    queue_file="queues/${tag}_queue.txt"
    if [[ -f "${queue_file}" ]]; then
        cat "${queue_file}" >> "${MASTER_QUEUE}"
    else
        echo "ERROR: expected queue file ${queue_file} not found, skipping"
    fi
done < "${DATASETS_FILE}"

nlines=$(wc -l < "${MASTER_QUEUE}")
echo ""
echo "==================================================="
echo "Done. ${MASTER_QUEUE} contains ${nlines} job rows total."
echo "Point your .submit file at: queue input_dir, output_file, stderr_file from ${MASTER_QUEUE}"
echo "==================================================="
