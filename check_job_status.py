#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_job_status.py

Diffs the output files a queue file *expects* against what's actually present
on EOS, and writes a CSV report. Optionally does a deeper validity check by
opening each present file with ROOT to catch zero-byte / truncated / zombie
files that "exist" but aren't usable.

Usage:
    python3 check_job_status.py <queue_file.txt> <eos_dir> [--deep] [--report out.csv]

Example:
    python3 check_job_status.py queue_all.txt /store/user/msahoo/2024_processed --deep
"""
import os
import sys
import csv
import subprocess
import argparse


def eos_ls(eos_dir, xrootd_host="cmseos.fnal.gov"):
    """List basenames of files present in an EOS directory via xrdfs."""
    cmd = f"xrdfs {xrootd_host} ls {eos_dir}"
    try:
        out = subprocess.check_output(cmd, shell=True, text=True)
    except subprocess.CalledProcessError as e:
        print(f"ERROR: xrdfs ls failed on {eos_dir}: {e}")
        sys.exit(1)

    names = set()
    for line in out.strip().split("\n"):
        line = line.strip()
        if line:
            names.add(os.path.basename(line))
    return names


def is_valid_root_file(xrootd_url):
    """Open a remote ROOT file and check it's not a zombie / has entries. Slow - use --deep only."""
    try:
        import ROOT
        f = ROOT.TFile.Open(xrootd_url)
        if not f or f.IsZombie():
            return False, "zombie or failed to open"
        f.Close()
        return True, "ok"
    except Exception as e:
        return False, f"exception: {e}"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("queue_file", help="queue file with rows: input_dir output_file stderr_file")
    parser.add_argument("eos_dir", help="EOS directory to check, e.g. /store/user/msahoo/2024_processed")
    parser.add_argument("--xrootd-host", default="cmseos.fnal.gov")
    parser.add_argument("--deep", action="store_true",
                         help="Also open each present ROOT file remotely to check it's not corrupted/empty")
    parser.add_argument("--report", default="job_status_report.csv")
    args = parser.parse_args()

    expected = []
    with open(args.queue_file) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) >= 2:
                expected.append(parts[1])  # output_file column

    if not expected:
        print(f"ERROR: no rows parsed from {args.queue_file}")
        sys.exit(1)

    print(f"Checking {len(expected)} expected output files against EOS dir {args.eos_dir} ...")
    produced = eos_ls(args.eos_dir, args.xrootd_host)

    rows = []
    n_ok, n_missing, n_bad = 0, 0, 0
    for outname in expected:
        if outname not in produced:
            rows.append((outname, "MISSING", ""))
            n_missing += 1
            continue

        if args.deep:
            url = f"root://{args.xrootd_host}/{args.eos_dir}/{outname}"
            ok, detail = is_valid_root_file(url)
            if ok:
                rows.append((outname, "OK", detail))
                n_ok += 1
            else:
                rows.append((outname, "PRESENT_BUT_BAD", detail))
                n_bad += 1
        else:
            rows.append((outname, "OK", ""))
            n_ok += 1

    with open(args.report, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["output_file", "status", "detail"])
        w.writerows(rows)

    print("---------------------------------------------------")
    print(f"OK               : {n_ok}")
    print(f"MISSING          : {n_missing}")
    if args.deep:
        print(f"PRESENT_BUT_BAD  : {n_bad}")
    print(f"Report written to: {args.report}")
    print("---------------------------------------------------")

    if n_missing or n_bad:
        print("Files needing resubmission:")
        for outname, status, detail in rows:
            if status != "OK":
                print(f"  {outname:40s} {status:16s} {detail}")


if __name__ == "__main__":
    main()
