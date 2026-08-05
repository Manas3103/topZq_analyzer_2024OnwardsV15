#!/bin/bash
# resubmit_missing.sh
#
# Filters an original queue file down to only the rows whose output_file
# was reported MISSING or PRESENT_BUT_BAD by check_job_status.py, and
# writes them to a new queue file ready for resubmission.
#
# Usage:
#   ./resubmit_missing.sh <job_status_report.csv> <original_queue.txt> <resubmit_queue_out.txt>
#
# Example:
#   ./resubmit_missing.sh job_status_report.csv queue_all.txt resubmit_queue.txt

set -e

REPORT=${1:?"Usage: $0 <job_status_report.csv> <original_queue.txt> <resubmit_queue_out.txt>"}
ORIGINAL_QUEUE=${2:?"Need original queue file"}
RESUBMIT_QUEUE=${3:?"Need output resubmit queue filename"}

# Pull output filenames whose status is not OK
awk -F, 'NR>1 && $2!="OK" {print $1}' "${REPORT}" > /tmp/_missing_outputs.$$.txt

count_missing=$(wc -l < /tmp/_missing_outputs.$$.txt)
echo "Found ${count_missing} non-OK outputs in ${REPORT}"

# Match those output filenames (2nd column) against the original queue file
: > "${RESUBMIT_QUEUE}"
while read -r fname; do
    [[ -z "$fname" ]] && continue
    grep -P "^\S+\s+${fname}\s+\S+\s*$" "${ORIGINAL_QUEUE}" >> "${RESUBMIT_QUEUE}" || true
done < /tmp/_missing_outputs.$$.txt

rm -f /tmp/_missing_outputs.$$.txt

n=$(wc -l < "${RESUBMIT_QUEUE}")
echo "Wrote ${n} rows to ${RESUBMIT_QUEUE}"
echo "Point a .submit file at it with: queue input_dir, output_file, stderr_file from ${RESUBMIT_QUEUE}"
