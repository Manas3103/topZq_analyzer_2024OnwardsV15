#!/usr/bin/env python3
"""
make_queue_list.py

Converts a DataMC json (produced by your dataset-fetching script) into:
  1) one plain-text file list per batch, one ROOT file per line
     (these become the "input_dir" argument for runjob_24.sh)
  2) a single flat queue_list.txt for use with HTCondor's
     `queue input_dir, output_file, stderr_file from queue_list.txt`

Usage:
    ./make_queue_list.py DataMC_txt/test.json
    ./make_queue_list.py DataMC_txt/test.json --outdir DataMC_txt/batch_lists --queue-file DataMC_txt/queue_list.txt

For every dataset in the json and every batch within it, this writes:
    <outdir>/<dataset>_batch<batch_index>.txt      (list of root:// file paths)

And appends a line to the queue file:
    <outdir>/<dataset>_batch<batch_index>.txt,<dataset>_batch<batch_index>.root,<dataset>_batch<batch_index>.stderr
"""

import argparse
import json
import os
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("json_file", help="Path to the DataMC json (e.g. DataMC_txt/test.json)")
    parser.add_argument("--outdir", default="DataMC_txt/batch_lists",
                         help="Directory to write per-batch file-list txt files (default: %(default)s)")
    parser.add_argument("--queue-file", default="DataMC_txt/queue_list.txt",
                         help="Path to write the flat condor queue file (default: %(default)s)")
    parser.add_argument("--only-ok", action="store_true", default=True,
                         help="Skip datasets whose status is not 'ok' (default: True)")
    args = parser.parse_args()

    if not os.path.isfile(args.json_file):
        sys.exit(f"ERROR: json file not found: {args.json_file}")

    with open(args.json_file) as f:
        data = json.load(f)

    datasets = data.get("datasets", {})
    if not datasets:
        sys.exit("ERROR: no 'datasets' key found in json")

    os.makedirs(args.outdir, exist_ok=True)

    n_jobs = 0
    with open(args.queue_file, "w") as qf:
        for ds_name, ds_info in datasets.items():
            if args.only_ok and ds_info.get("status") != "ok":
                print(f"Skipping dataset '{ds_name}' (status={ds_info.get('status')})")
                continue

            batches = ds_info.get("batches", [])
            for batch in batches:
                idx = batch["batch_index"]
                files = batch["files"]

                list_name = f"{ds_name}_batch{idx}.txt"
                list_path = os.path.join(args.outdir, list_name)
                with open(list_path, "w") as lf:
                    lf.write("\n".join(files) + "\n")

                output_file = f"{ds_name}_batch{idx}.root"
                stderr_file = f"{ds_name}_batch{idx}.stderr"

                qf.write(f"{list_path},{output_file},{stderr_file}\n")
                n_jobs += 1

    print(f"Wrote {n_jobs} batch file-lists into: {args.outdir}/")
    print(f"Wrote condor queue file: {args.queue_file}")
    print("\nIn your submit file, use:")
    print(f"    queue input_dir, output_file, stderr_file from {args.queue_file}")


if __name__ == "__main__":
    main()
