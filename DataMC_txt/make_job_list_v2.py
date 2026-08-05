#!/usr/bin/env python3
"""
make_joblist.py

Reads the DataMC_txt/*.json (from get_das_files_batched.py) and writes a
flat 'joblist.txt' that HTCondor's `queue ... from` directive can parse:
one line per (dataset, batch_index) pair. That's all condor needs now --
processnanoaod_new.py reads the JSON directly at runtime via its
--dataset/--batch filter, so there's no per-batch file-splitting step.

Usage:
    ./make_joblist.py DataMC_txt/test.json joblist.txt
"""
import json
import sys


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_json> <output_joblist.txt>")
        sys.exit(1)

    json_path = sys.argv[1]
    out_path = sys.argv[2]

    with open(json_path) as f:
        data = json.load(f)

    n_jobs = 0
    with open(out_path, "w") as out:
        for dataset_name, dset in data.get("datasets", {}).items():
            if dset.get("status") != "ok":
                print(f"Skipping dataset '{dataset_name}' (status={dset.get('status')})")
                continue
            for batch in dset.get("batches", []):
                out.write(f"{dataset_name},{batch['batch_index']}\n")
                n_jobs += 1

    print(f"Wrote {n_jobs} job rows to {out_path}")


if __name__ == "__main__":
    main()
