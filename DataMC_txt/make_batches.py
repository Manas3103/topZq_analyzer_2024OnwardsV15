#!/usr/bin/env python3
"""
get_das_files_batched.py

Reads a filelist where each line's FIRST field is a DAS dataset path, e.g.:

/TZQB-Zto2L-4FS_..._realistic_v2-v2/NANOAODSIM top_zq.root top_zq.out

For each DAS path:
  1. Runs `dasgoclient -query="file dataset=<path>"` to get the actual LFNs
  2. Prepends an xrootd redirector
  3. Splits the resolved files into fixed-size batches (default: 20 files/batch)
  4. Writes ALL results into a single structured JSON file
     (and, optionally, one plain-text filelist per batch)

No file-opening checks and no genEventSumw / event-count computation are
done here -- this script only resolves and batches file paths.

Usage:
    cmsenv
    voms-proxy-init -voms cms
    python3 get_das_files_batched.py General.txt -o das_files_batched.json
    python3 get_das_files_batched.py General.txt -o das_files_batched.json --write-txt --txt-outdir batches/
"""

import argparse
import json
import os
import subprocess
import sys
import time


def parse_filelist_line(line):
    """Fields (whitespace-separated), only the first is mandatory:
        1. das_path   (required)
        2. tag        (optional, e.g. 'top_zq.root' -> 'top_zq'; used as the
                       dataset's key in the output JSON / as the batch-file
                       basename)

    Anything after field 2 is ignored (sample_type / batch_size columns from
    older filelists are simply not used by this script).
    """
    line = line.strip()
    if not line or line.startswith("#"):
        return None
    fields = line.split()
    das_path = fields[0]
    tag = fields[1].replace(".root", "") if len(fields) > 1 else None
    return das_path, tag


def query_das_files(das_path, redirector):
    """Run dasgoclient to resolve a DAS dataset path into LFNs, then prepend redirector."""
    try:
        result = subprocess.run(
            ["dasgoclient", "-query", f"file dataset={das_path}"],
            capture_output=True, text=True, check=True, timeout=120
        )
    except subprocess.CalledProcessError as e:
        return [], f"dasgoclient query failed: {e.stderr.strip()}"
    except FileNotFoundError:
        sys.exit("ERROR: 'dasgoclient' not found. Run 'cmsenv' and 'voms-proxy-init -voms cms' first.")
    except subprocess.TimeoutExpired:
        return [], "dasgoclient query timed out"

    lfns = [l.strip() for l in result.stdout.splitlines() if l.strip()]
    files = []
    for lfn in lfns:
        if lfn.startswith("root://"):
            files.append(lfn)
        else:
            host = redirector.rstrip("/")
            path = "/" + lfn.lstrip("/")
            files.append(host + "/" + path)
    return files, None


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("filelist", help="Text file; first column of each line is a DAS dataset path")
    ap.add_argument("-o", "--output", default="das_files_batched.json",
                    help="Output JSON file (default: das_files_batched.json)")
    ap.add_argument("--redirector", default="root://cmsxrootd.fnal.gov/",
                    help="xrootd redirector for resolved LFNs")
    ap.add_argument("--batch-size", type=int, default=20,
                    help="Number of files per batch (default: 20)")
    ap.add_argument("--write-txt", action="store_true",
                    help="Also write one plain-text filelist per batch "
                         "(one root file path per line)")
    ap.add_argument("--txt-outdir", default="batches",
                    help="Directory to write per-batch .txt filelists into "
                         "when --write-txt is given (default: batches/)")
    args = ap.parse_args()

    with open(args.filelist) as fh:
        parsed = [parse_filelist_line(line) for line in fh]
    entries = [p for p in parsed if p]

    if not entries:
        sys.exit(f"No DAS paths found in {args.filelist}")

    if args.write_txt:
        os.makedirs(args.txt_outdir, exist_ok=True)

    datasets = {}
    n_ok, n_failed = 0, 0

    for i, (das_path, tag) in enumerate(entries, 1):
        print(f"[{i}/{len(entries)}] {das_path}")
        key = tag if tag else das_path

        files, query_error = query_das_files(das_path, args.redirector)

        if query_error:
            print(f"  ERROR: {query_error}")
            datasets[key] = {
                "das_path": das_path,
                "status": "failed",
                "error": query_error,
                "n_files_total": 0,
                "n_batches": 0,
                "batches": [],
            }
            n_failed += 1
            continue

        if not files:
            print("  WARNING: no files resolved, skipping")
            datasets[key] = {
                "das_path": das_path,
                "status": "no_files",
                "error": "dasgoclient returned zero files",
                "n_files_total": 0,
                "n_batches": 0,
                "batches": [],
            }
            n_failed += 1
            continue

        chunks = [files[j:j + args.batch_size] for j in range(0, len(files), args.batch_size)]

        batches = []
        for b_idx, chunk in enumerate(chunks):
            print(f"  batch {b_idx}: {len(chunk)} file(s)")
            batches.append({
                "batch_index": b_idx,
                "n_files": len(chunk),
                "files": chunk,
            })

            if args.write_txt:
                txt_path = os.path.join(args.txt_outdir, f"{key}_batch{b_idx}.txt")
                with open(txt_path, "w") as tf:
                    tf.write("\n".join(chunk) + "\n")

        datasets[key] = {
            "das_path": das_path,
            "status": "ok",
            "error": None,
            "n_files_total": len(files),
            "n_batches": len(batches),
            "batches": batches,
        }
        n_ok += 1

    output = {
        "metadata": {
            "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "source_filelist": args.filelist,
            "redirector": args.redirector,
            "batch_size": args.batch_size,
            "n_datasets_total": len(entries),
            "n_datasets_ok": n_ok,
            "n_datasets_failed": n_failed,
        },
        "datasets": datasets,
    }

    with open(args.output, "w") as out:
        json.dump(output, out, indent=2)

    print(f"\nWrote batched file lists for {len(entries)} dataset(s) to {args.output}")
    print(f"  OK: {n_ok}   Failed/no-files: {n_failed}")


if __name__ == "__main__":
    main()
