#!/usr/bin/env python3
"""
check_sumgenweight_cmssw.py

CMSSW-native version: uses PyROOT / RDataFrame instead of uproot, so it needs
NO extra pip packages (no uproot, no awkward, no fsspec-xrootd). Just run it
inside `cmsenv` (CMSSW_13_x ships ROOT with PyROOT and xrootd support built in),
and root:// URLs work out of the box.

Computes sum of generator weights over all ROOT files under a path (or from a
DAS-style file list), cross-checked two ways:

  1. sum(genEventSumw) from the 'Runs' tree  -> standard NanoAOD bookkeeping
  2. sum(Generator_weight) from the 'Events' tree, event-by-event -> cross-check

If they disagree, or if some files fail to open, that's your normalization bug.

Usage:
    cmsenv
    python3 check_sumgenweight_cmssw.py /path/to/sample/ --pattern "*.root"
    python3 check_sumgenweight_cmssw.py --filelist tzq_files.txt --xsec 4.67 --lumi 110000

To build a filelist from DAS:
    voms-proxy-init -voms cms
    dasgoclient -query="file dataset=/YourDataset/.../NANOAODSIM" > tzq_files.txt
"""

import argparse
import glob
import os
import sys

import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gErrorIgnoreLevel = ROOT.kWarning  # show real errors (e.g. xrootd auth/TLS failures) instead of hiding them


def find_root_files(path, pattern="*.root"):
    if os.path.isfile(path):
        return [path]
    return sorted(glob.glob(os.path.join(path, "**", pattern), recursive=True))


def load_filelist(filelist_path, redirector=None):
    """Read a plain-text list of LFNs/paths (e.g. from dasgoclient) and,
    optionally, prepend an xrootd redirector to each line so files can be
    opened remotely without staging them locally first.

    xrootd URLs require a DOUBLE slash between the host and the path
    (root://host//store/...) -- the first slash ends the authority part,
    the second is the start of the absolute path. Dropping this produces
    a "relative path ... is disallowed" error from strict redirectors."""
    files = []
    with open(filelist_path) as fh:
        for line in fh:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if redirector and not line.startswith("root://"):
                host = redirector.rstrip("/")           # e.g. root://cms-xrd-global.cern.ch
                lfn = "/" + line.lstrip("/")             # ensure exactly one leading slash, e.g. /store/...
                line = host + "/" + lfn                  # -> root://host//store/...
            files.append(line)
    return files


def check_files_openable(files, tree_name, branch_name):
    """Open each file individually with TFile::Open to find out exactly which
    ones are bad and why -- RDataFrame itself won't tell you per-file errors.
    Note: some ROOT builds raise OSError on a failed Open() instead of
    returning None/a zombie TFile, so we must catch that explicitly."""
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
        if not tree:
            bad.append((f, f"no '{tree_name}' tree"))
            tf.Close()
            continue
        if not tree.GetBranch(branch_name):
            bad.append((f, f"no '{branch_name}' branch in {tree_name}"))
            tf.Close()
            continue
        good.append(f)
        tf.Close()
    return good, bad


def sum_branch(files, tree_name, branch_name):
    """Sum a branch over a list of files using RDataFrame (fast, vectorized)."""
    if not files:
        return 0.0, 0
    df = ROOT.RDataFrame(tree_name, files)
    total = df.Sum(branch_name).GetValue()
    n = df.Count().GetValue()
    return float(total), int(n)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", nargs="?", default=None,
                    help="Directory (searched recursively) or single ROOT file. "
                         "Omit if using --filelist instead.")
    ap.add_argument("--pattern", default="*.root", help="Glob pattern for ROOT files (default: *.root)")
    ap.add_argument("--filelist", default=None,
                    help="Text file with one ROOT file path/LFN per line (e.g. from "
                         "'dasgoclient -query=\"file dataset=...\"'). Use instead of 'path'.")
    ap.add_argument("--redirector", default="root://cmsxrootd.fnal.gov/",
                    help="xrootd redirector prepended to LFNs from --filelist that don't "
                         "already start with root:// (default: root://cmsxrootd.fnal.gov/, "
                         "good default from LPC). Pass '' to disable.")
    ap.add_argument("--weight-branch", default=None,
                    help="Override branch name for the Events-tree cross-check "
                         "(default: auto-detect genWeight / Generator_weight)")
    ap.add_argument("--xsec", type=float, default=None, help="Cross section in pb (optional)")
    ap.add_argument("--lumi", type=float, default=None, help="Luminosity in pb^-1 (optional)")
    ap.add_argument("--tolerance", type=float, default=0.01,
                    help="Relative tolerance between the two sums before warning (default 1%%)")
    args = ap.parse_args()

    if args.filelist:
        files = load_filelist(args.filelist, redirector=args.redirector or None)
        source_desc = f"filelist {args.filelist}"
    elif args.path:
        files = find_root_files(args.path, args.pattern)
        source_desc = f"path {args.path}"
    else:
        sys.exit("Provide either a path or --filelist.")

    if not files:
        sys.exit(f"No ROOT files found for: {source_desc}")

    print(f"Found {len(files)} ROOT file(s) from {source_desc}\n")

    # --- Method 1: Runs tree, genEventSumw ---
    print("== Method 1: sum(genEventSumw) from 'Runs' tree ==")
    good_runs, bad_runs = check_files_openable(files, "Runs", "genEventSumw")
    sumw_runs, _ = sum_branch(good_runs, "Runs", "genEventSumw")
    print(f"  sumGenWeights (Runs tree) = {sumw_runs:.6g}   [{len(good_runs)}/{len(files)} files OK]")
    if bad_runs:
        print(f"  WARNING: {len(bad_runs)} file(s) had problems and were SKIPPED:")
        for f, reason in bad_runs:
            print(f"    - {f}: {reason}")

    # --- Method 2: Events tree, Generator_weight / genWeight ---
    weight_branch = args.weight_branch
    if weight_branch is None:
        # figure out which branch name this dataset actually uses, from the first good file
        weight_branch = "Generator_weight"
        for f in files:
            try:
                tf = ROOT.TFile.Open(f)
            except OSError:
                continue
            if tf and not tf.IsZombie():
                tree = tf.Get("Events")
                if tree:
                    if tree.GetBranch("genWeight"):
                        weight_branch = "genWeight"
                        tf.Close()
                        break
                    elif tree.GetBranch("Generator_weight"):
                        weight_branch = "Generator_weight"
                        tf.Close()
                        break
                tf.Close()

    print(f"\n== Method 2: sum({weight_branch}) from 'Events' tree (cross-check) ==")
    good_events, bad_events = check_files_openable(files, "Events", weight_branch)
    sumw_events, nevents = sum_branch(good_events, "Events", weight_branch)
    print(f"  sumGenWeights (Events tree) = {sumw_events:.6g}  (over {nevents} events)"
          f"   [{len(good_events)}/{len(files)} files OK]")
    if bad_events:
        print(f"  WARNING: {len(bad_events)} file(s) had problems and were SKIPPED:")
        for f, reason in bad_events:
            print(f"    - {f}: {reason}")

    # --- Cross-check ---
    print("\n== Cross-check ==")
    if sumw_runs != 0:
        rel_diff = abs(sumw_runs - sumw_events) / abs(sumw_runs)
        print(f"  Relative difference between methods: {rel_diff:.4%}")
        if rel_diff > args.tolerance:
            print("  ==> MISMATCH beyond tolerance! Likely cause: skipped/corrupted files, "
                  "or a mix of files processed with different genWeight conventions.")
        else:
            print("  ==> Consistent within tolerance.")
    else:
        print("  Runs-tree sum is zero; cannot compute relative difference.")

    skipped_runs = {f for f, _ in bad_runs}
    skipped_events = {f for f, _ in bad_events}
    if skipped_runs != skipped_events:
        print("  NOTE: the set of skipped files differs between the two methods -- "
              "inspect those files individually before trusting either sum.")

    if args.xsec is not None and args.lumi is not None and sumw_runs:
        print("\n== Normalization preview ==")
        norm_weight = (args.xsec * args.lumi) / sumw_runs
        print(f"  xsec = {args.xsec} pb, lumi = {args.lumi} pb^-1")
        print(f"  per-event MC weight scale = xsec*lumi/sumGenWeights = {norm_weight:.6g}")


if __name__ == "__main__":
    main()
