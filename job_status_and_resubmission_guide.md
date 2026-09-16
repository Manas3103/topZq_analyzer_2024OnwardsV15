# Job Output Checking and Resubmission Guide

This document explains how the two scripts work together:

1. `check_job_status.py`
2. `resubmit_missing_job.sh`

The purpose is to identify missing or invalid ROOT output files and create a new queue file containing only the jobs that need to be resubmitted.

---

## Overall Workflow

```text
Original queue file
       |
       v
check_job_status.py
       |
       v
job_status_report.csv
       |
       v
resubmit_missing_job.sh
       |
       v
resubmit_queue.txt
       |
       v
Resubmit only failed jobs
```

---

# 1. `check_job_status.py`

## Purpose

`check_job_status.py` compares the output ROOT files expected from a queue file with the files actually present in an EOS directory.

It can also perform a deeper check by opening each ROOT file to identify corrupted or zombie files.

## Expected Queue File Format

The script expects each queue entry to have at least two columns:

```text
input_dir    output_file    stderr_file
```

For example:

```text
/path/to/input1   output_001.root   error_001.txt
/path/to/input2   output_002.root   error_002.txt
/path/to/input3   output_003.root   error_003.txt
```

The script uses the **second column** as the expected output filename.

Conceptually, it does:

```python
expected.append(parts[1])
```

Therefore, make sure the ROOT output filename is in column 2.

## Usage

Basic usage:

```bash
python3 check_job_status.py <queue_file.txt> <eos_dir>
```

Example:

```bash
python3 check_job_status.py     queue_all.txt     /store/user/msahoo/2024_processed
```

By default, the report is written to:

```text
job_status_report.csv
```

## Deep ROOT File Check

To check not only whether files exist but also whether ROOT can open them, use `--deep`:

```bash
python3 check_job_status.py     queue_all.txt     /store/user/msahoo/2024_processed     --deep
```

The script constructs remote ROOT URLs similar to:

```text
root://cmseos.fnal.gov//store/user/msahoo/2024_processed/output_001.root
```

and attempts to open them using:

```python
ROOT.TFile.Open(...)
```

It checks whether the file is a ROOT zombie file.

## Custom Report Name

You can choose the report filename with `--report`:

```bash
python3 check_job_status.py     DataMC_txt/queue_all.txt     /store/user/msahoo/2024_test  --deep  --report my_job_report.csv
```

## Possible Status Values

### `OK`

The expected output file exists.

When `--deep` is used, the file must also open successfully with ROOT.

### `MISSING`

The output file is expected according to the queue file but is not present in the EOS directory.

### `PRESENT_BUT_BAD`

This status is only possible with `--deep`.

The file exists on EOS but could not be opened correctly, or ROOT identifies it as a zombie file.

## Example Report

The generated CSV may look like:

```csv
output_file,status,detail
output_001.root,OK,
output_002.root,MISSING,
output_003.root,PRESENT_BUT_BAD,zombie or failed to open
output_004.root,OK,
```

## Example Summary

The script may print:

```text
---------------------------------------------------
OK               : 950
MISSING          : 45
PRESENT_BUT_BAD  : 5
Report written to: job_status_report.csv
---------------------------------------------------
```

## Useful Commands

View the report:

```bash
cat job_status_report.csv
```

View only jobs that are not OK:

```bash
awk -F, 'NR>1 && $2!="OK"' job_status_report.csv
```

Count missing or bad jobs:

```bash
awk -F, 'NR>1 && $2!="OK" {count++} END {print count}' job_status_report.csv
```

## Recommended Command

A typical command is:

```bash
python3 check_job_status.py     queue_all.txt     /store/user/msahoo/2024_processed     --deep     --report job_status_report.csv
```

---

# 2. `resubmit_missing_job.sh`

## Purpose

`resubmit_missing_job.sh` reads the CSV report produced by `check_job_status.py` and creates a new queue file containing only jobs whose output status is not `OK`.

This includes:

- `MISSING`
- `PRESENT_BUT_BAD`

The resulting queue can then be used to resubmit only the failed jobs.

## Usage

```bash
./resubmit_missing_job.sh     <job_status_report.csv>     <original_queue.txt>     <resubmit_queue_out.txt>
```

Example:

```bash
./resubmit_missing_job.sh     job_status_report.csv     queue_all.txt     resubmit_queue.txt
```

The three arguments are:

| Argument | Description |
|---|---|
| `job_status_report.csv` | Report created by `check_job_status.py` |
| `queue_all.txt` | Original queue containing all jobs |
| `resubmit_queue.txt` | New queue file created for failed jobs |

## What the Script Does

### Step 1: Find non-OK output files

The script uses:

```bash
awk -F, 'NR>1 && $2!="OK" {print $1}'
```

This means:

- Skip the CSV header.
- Look at the second column, `status`.
- Select rows where the status is not `OK`.
- Print the first column, `output_file`.

For example, from:

```csv
output_file,status,detail
job_1.root,OK,
job_2.root,MISSING,
job_3.root,PRESENT_BUT_BAD,zombie or failed to open
job_4.root,OK,
```

it extracts:

```text
job_2.root
job_3.root
```

### Step 2: Match These Files Against the Original Queue

Suppose the original queue is:

```text
/input/A job_1.root job_1.err
/input/B job_2.root job_2.err
/input/C job_3.root job_3.err
/input/D job_4.root job_4.err
```

The script searches for the entries whose second column matches the failed output filenames.

### Step 3: Create the Resubmission Queue

The resulting `resubmit_queue.txt` will contain:

```text
/input/B job_2.root job_2.err
/input/C job_3.root job_3.err
```

Only those jobs need to be submitted again.

## Making the Script Executable

If necessary:

```bash
chmod +x resubmit_missing_job.sh
```

Then run:

```bash
./resubmit_missing_job.sh     job_status_report.csv     queue_all.txt     resubmit_queue.txt
```

## Check the Result

Count the number of jobs:

```bash
wc -l resubmit_queue.txt
```

View the contents:

```bash
cat resubmit_queue.txt
```

---

# Complete Workflow

## Step 1: Check that the EOS directory is accessible

```bash
xrdfs cmseos.fnal.gov ls /store/user/msahoo/2024_processed | head
```

## Step 2: Run the job status checker

```bash
python3 check_job_status.py     queue_all.txt     /store/user/msahoo/2024_processed     --deep     --report job_status_report.csv
```

This creates:

```text
job_status_report.csv
```

## Step 3: Inspect non-OK jobs

```bash
awk -F, 'NR>1 && $2!="OK"' job_status_report.csv
```

## Step 4: Create a resubmission queue

```bash
chmod +x resubmit_missing_job.sh

./resubmit_missing_job.sh     job_status_report.csv     queue_all.txt     resubmit_queue.txt
```

This creates:

```text
resubmit_queue.txt
```

## Step 5: Check the number of jobs to resubmit

```bash
wc -l resubmit_queue.txt
```

## Step 6: Submit the new queue

Use your existing Condor submit configuration, but point the queue input to:

```text
resubmit_queue.txt
```

instead of the original full queue file.

---

# Summary

The two scripts work together as follows:

```text
queue_all.txt
     |
     |  Expected output filenames
     v
check_job_status.py
     |
     |  Checks EOS for missing files
     |  Optional: checks whether ROOT files can be opened
     v
job_status_report.csv
     |
     |  Selects MISSING and PRESENT_BUT_BAD jobs
     v
resubmit_missing_job.sh
     |
     v
resubmit_queue.txt
     |
     v
Resubmit only failed jobs
```

In short:

- **`check_job_status.py`** finds expected ROOT output files that are missing or invalid.
- **`resubmit_missing_job.sh`** creates `resubmit_queue.txt` containing only the corresponding failed jobs from the original queue.
