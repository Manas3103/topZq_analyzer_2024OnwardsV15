#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
split_dataset.py

Resolves a DAS dataset into its ROOT file list, splits it into fixed-size
chunks (N files per job), writes one filelist .txt per chunk, and writes a
per-dataset queue file (input_dir output_file stderr_file) that can be fed
directly into a condor `queue ... from` line.

Usage:
    python3 split_dataset.py <DAS_dataset> <files_per_job> <tag> [--xrootd-prefix PREFIX] [--outdir DIR]

Example:
    python3 split_dataset.py /EGamma0/Run2024H-MINIv6NANOv15-v2/NANOAOD 15 EGamma0_Era_H_Run24

Produces:
    filelists/EGamma0_Era_H_Run24_chunk0.txt
    filelists/EGamma0_Era_H_Run24_chunk1.txt
    ...
    queues/EGamma0_Era_H_Run24_queue.txt
"""
import os
import sys
import subprocess
import argparse


def get_das_files(dataset, xrootd_prefix="root://cmsxrootd.fnal.gov/"):
    """Query dasgoclient for the list of files in a dataset, return full xrootd paths."""
    cmd = f"dasgoclient --query='file dataset={dataset}'"
    try:
        out = subprocess.check_output(cmd, shell=True, text=True)
    except subprocess.CalledProcessError as e:
        print(f"ERROR: dasgoclient query failed for {dataset}: {e.output}")
        sys.exit(1)

    files = [line.strip() for line in out.strip().split("\n") if line.strip()]
    if not files:
        print(f"ERROR: dasgoclient returned zero files for dataset {dataset}. "
              f"Check the dataset name and that you have a valid proxy (voms-proxy-init).")
        sys.exit(1)

    return [xrootd_prefix + f for f in files]


def chunk_list(lst, n):
    for i in range(0, len(lst), n):
        yield lst[i:i + n]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", help="DAS dataset path, e.g. /EGamma0/Run2024H-MINIv6NANOv15-v2/NANOAOD")
    parser.add_argument("files_per_job", type=int, help="Number of ROOT files per job (e.g. 15)")
    parser.add_argument("tag", help="Short name used for output/filelist naming, e.g. EGamma0_Era_H_Run24")
    parser.add_argument("--xrootd-prefix", default="root://cmsxrootd.fnal.gov/",
                         help="XRootD redirector prefix to prepend to each file (default: FNAL global redirector)")
    parser.add_argument("--filelist-dir", default="filelists",
                         help="Directory to physically write per-chunk filelists to, relative to where this script is run")
    parser.add_argument("--queue-dir", default="queues",
                         help="Directory to physically write the per-dataset queue file to")
    parser.add_argument("--path-prefix", default="",
                         help="Prefix to prepend ONLY to the filelist path string recorded in the queue file "
                              "(e.g. 'DataMC_txt' if condor_submit will be run one directory above this script). "
                              "Does NOT affect where the filelist is actually written on disk.")
    args = parser.parse_args()

    # Guard against a tag that accidentally includes an extension (e.g. copy-pasted
    # from an old output_file column like "EGamma1_Era_H_Run24.root"). Strip it so
    # we never produce double-extension names like "..._chunk4.root_chunk4.root".
    if args.tag.endswith(".root"):
        cleaned_tag = args.tag[: -len(".root")]
        print(f"WARNING: tag '{args.tag}' ends in .root - stripping to '{cleaned_tag}'")
        args.tag = cleaned_tag

    os.makedirs(args.filelist_dir, exist_ok=True)
    os.makedirs(args.queue_dir, exist_ok=True)

    print(f"Resolving files for dataset: {args.dataset}")
    files = get_das_files(args.dataset, args.xrootd_prefix)
    print(f"  -> {len(files)} files found")

    queue_rows = []
    nchunks = 0
    for i, group in enumerate(chunk_list(files, args.files_per_job)):
        listfile_name = f"{args.tag}_chunk{i}.txt"
        listfile_path = os.path.join(args.filelist_dir, listfile_name)   # where the file is ACTUALLY written
        with open(listfile_path, "w") as f:
            f.write("\n".join(group) + "\n")

        outname = f"{args.tag}_chunk{i}.root"
        stderrname = f"{args.tag}_chunk{i}.out"

        # Path recorded in the queue file - may differ from listfile_path if
        # --path-prefix is set (e.g. condor_submit runs one dir above this script)
        queue_path = os.path.join(args.path_prefix, listfile_path) if args.path_prefix else listfile_path

        queue_rows.append(f"{queue_path} {outname} {stderrname}")
        nchunks += 1

    queue_path = os.path.join(args.queue_dir, f"{args.tag}_queue.txt")
    with open(queue_path, "w") as f:
        f.write("\n".join(queue_rows) + "\n")

    print(f"Wrote {nchunks} chunk filelists to {args.filelist_dir}/")
    print(f"Wrote queue file: {queue_path}")


if __name__ == "__main__":
    main()
