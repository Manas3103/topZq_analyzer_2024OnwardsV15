#!/usr/bin/env python3
"""
list_dataset_files.py
----------------------
Query DAS for all files belonging to a dataset and print them as full
xrootd URLs (redirector-prefixed), one per line.

Requires `dasgoclient` on PATH and a valid voms proxy, exactly as with any
other DAS query:

    source /cvmfs/cms.cern.ch/cmsset_default.sh
    voms-proxy-init --voms cms

Usage:
    python3 list_dataset_files.py DATASET [--redirector REDIRECTOR] [--output OUTPUT_FILE]

Example:
    python3 list_dataset_files.py \\
        "/TZQB-Zto2L-4FS_Bin-MLL-30_TuneCP5_13p6TeV_amcatnlo-pythia8/RunIII2024Summer24NanoAODv15-Madgraph_2_6_5_150X_mcRun3_2024_realistic_v2-v2/NANOAODSIM"
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from typing import List


def query_das_files(dataset: str) -> List[str]:
    """Run dasgoclient and return the list of LFNs for the given dataset."""
    if shutil.which("dasgoclient") is None:
        print(
            "ERROR: dasgoclient not found on PATH. "
            "Did you 'source /cvmfs/cms.cern.ch/cmsset_default.sh'?",
            file=sys.stderr,
        )
        sys.exit(1)

    cmd = ["dasgoclient", "--query", f"file dataset={dataset}"]
    print(f"[INFO] Querying DAS for dataset:\n  {dataset}", file=sys.stderr)

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"ERROR: dasgoclient failed:\n{result.stderr.strip()}", file=sys.stderr)
        sys.exit(1)

    files = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if not files:
        print("ERROR: No files returned by DAS. Check the dataset name.", file=sys.stderr)
        sys.exit(1)

    print(f"[INFO] Found {len(files)} files.", file=sys.stderr)
    return files


def build_url(lfn: str, redirector: str) -> str:
    """Join a redirector and an LFN into a full xrootd URL.

    XRootD URLs need the protocol slash *and* the LFN's own leading slash,
    e.g. root://host//store/mc/...  (double slash is required, not a typo).
    """
    redirector = redirector.rstrip("/") + "/"
    if not lfn.startswith("/"):
        lfn = "/" + lfn
    return f"{redirector}{lfn}"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="List all files in a CMS dataset as full xrootd URLs."
    )
    parser.add_argument("dataset", help="Full DAS dataset name, e.g. /A/B/NANOAODSIM")
    parser.add_argument(
        "--redirector",
        default="root://cmsxrootd.fnal.gov/",
        help="XRootD redirector to prefix each file with (default: %(default)s)",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Write URLs to this file instead of stdout (one per line)",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    lfns = query_das_files(args.dataset)
    urls = [build_url(lfn, args.redirector) for lfn in lfns]

    if args.output:
        with open(args.output, "w") as f:
            f.write("\n".join(urls) + "\n")
        print(f"[INFO] Wrote {len(urls)} URLs to {args.output}", file=sys.stderr)
    else:
        for url in urls:
            print(url)


if __name__ == "__main__":
    main()
