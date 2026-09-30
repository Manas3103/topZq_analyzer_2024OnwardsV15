#!/usr/bin/env python3
"""
compute_sumgenweight_from_das_list.py

Reads a filelist where each line's FIRST field is a DAS dataset path, e.g.:

/TZQB-Zto2L-4FS_..._realistic_v2-v2/NANOAODSIM top_zq.root top_zq.out

For each DAS path:
  1. Runs `dasgoclient -query="file dataset=<path>"` to get the actual LFNs
  2. Prepends an xrootd redirector
  3. Sums genEventSumw from the 'Runs' tree over all files (RDataFrame)
  4. Writes "<das_path>  <sumGenWeight>  <n_files_ok>/<n_files_total>" to an output file

Usage:
    cmsenv
    voms-proxy-init -voms cms
    python3 compute_sumgenweight_from_das_list.py General.txt -o sumw_output.txt
"""

import argparse
import subprocess
import sys

import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gErrorIgnoreLevel = ROOT.kWarning


def get_das_path(line):
    """Take only the first whitespace-separated field of a filelist line."""
    line = line.strip()
    if not line or line.startswith("#"):
        return None
    return line.split()[0]


def query_das_files(das_path, redirector):
    """Run dasgoclient to resolve a DAS dataset path into LFNs, then prepend redirector."""
    try:
        result = subprocess.run(
            ["dasgoclient", "-query", f"file dataset={das_path}"],
            capture_output=True, text=True, check=True, timeout=120
        )
    except subprocess.CalledProcessError as e:
        print(f"  ERROR: dasgoclient failed for {das_path}: {e.stderr.strip()}")
        return []
    except FileNotFoundError:
        sys.exit("ERROR: 'dasgoclient' not found. Run 'cmsenv' and 'voms-proxy-init -voms cms' first.")

    lfns = [l.strip() for l in result.stdout.splitlines() if l.strip()]
    files = []
    for lfn in lfns:
        if lfn.startswith("root://"):
            files.append(lfn)
        else:
            host = redirector.rstrip("/")
            path = "/" + lfn.lstrip("/")
            files.append(host + "/" + path)
    return files


def check_files_openable(files, tree_name, branch_name):
    good, bad = [], []
    for f in files:
        try:
            tf = ROOT.TFile.Open(f)
        except OSError as e:
            bad.append((f, f"could not open file ({e})"))
            continue
        if not tf or tf.IsZombie():
            bad.append((f, "could not open file"))
            if tf:
                tf.Close()
            continue
        tree = tf.Get(tree_name)
        if not tree or not tree.GetBranch(branch_name):
            bad.append((f, f"no '{branch_name}' in {tree_name}"))
            tf.Close()
            continue
        good.append(f)
        tf.Close()
    return good, bad


def sum_branch(files, tree_name, branch_name):
    if not files:
        return 0.0
    df = ROOT.RDataFrame(tree_name, files)
    return float(df.Sum(branch_name).GetValue())


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("filelist", help="Text file; first column of each line is a DAS dataset path")
    ap.add_argument("-o", "--output", default="sumgenweight_summary.txt",
                    help="Output txt file (default: sumgenweight_summary.txt)")
    ap.add_argument("--redirector", default="root://cmsxrootd.fnal.gov/",
                    help="xrootd redirector for resolved LFNs")
    args = ap.parse_args()

    with open(args.filelist) as fh:
        das_paths = [get_das_path(line) for line in fh]
    das_paths = [p for p in das_paths if p]

    if not das_paths:
        sys.exit(f"No DAS paths found in {args.filelist}")

    results = []
    for i, das_path in enumerate(das_paths, 1):
        print(f"[{i}/{len(das_paths)}] {das_path}")
        files = query_das_files(das_path, args.redirector)
        if not files:
            print("  WARNING: no files resolved, skipping")
            results.append((das_path, None, 0, 0))
            continue

        good, bad = check_files_openable(files, "Runs", "genEventSumw")
        sumw = sum_branch(good, "Runs", "genEventSumw")
        print(f"  sumGenWeight = {sumw:.6g}   [{len(good)}/{len(files)} files OK]")
        if bad:
            for f, reason in bad:
                print(f"    - skipped {f}: {reason}")
        results.append((das_path, sumw, len(good), len(files)))

    with open(args.output, "w") as out:
        for das_path, sumw, n_good, n_total in results:
            if sumw is None:
                out.write(f"{das_path}  FAILED  0/{n_total}\n")
            else:
                out.write(f"{das_path}  {sumw:.6g}  {n_good}/{n_total}\n")

    print(f"\nWrote summary for {len(results)} dataset(s) to {args.output}")


if __name__ == "__main__":
    main()
