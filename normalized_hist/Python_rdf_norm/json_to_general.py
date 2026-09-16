#!/usr/bin/env python3

import json
import os
import sys


def main():

    if len(sys.argv) != 3:
        print("Usage:")
        print("  python3 json_to_general.py <input.json> <output.txt>")
        sys.exit(1)

    input_json = sys.argv[1]
    output_txt = sys.argv[2]

    # -----------------------------------------
    # Read JSON
    # -----------------------------------------
    with open(input_json, "r") as f:
        samples = json.load(f)

    errors = []
    valid_samples = []

    # -----------------------------------------
    # Validate samples
    # -----------------------------------------
    for sample_name, sample in samples.items():

        filename = sample.get("file")
        cross_section = sample.get("cross_section")
        sum_gen_weight = sample.get("sum_gen_weight")
        sample_type = sample.get("type")

        # Basic checks
        if filename is None:
            errors.append(
                f"{sample_name}: missing 'file'"
            )
            continue

        if sample_type not in ("mc", "data"):
            errors.append(
                f"{sample_name}: 'type' must be 'mc' or 'data'"
            )
            continue

        # -----------------------------------------
        # DATA
        # -----------------------------------------
        if sample_type == "data":

            # Data doesn't need cross section
            # or sum of generator weights.
            valid_samples.append(
                (filename, 1.0, 0.0, "data")
            )

        # -----------------------------------------
        # MC
        # -----------------------------------------
        else:

            if cross_section is None:
                errors.append(
                    f"{sample_name}: MC sample has no cross_section"
                )
                continue

            if sum_gen_weight is None:
                errors.append(
                    f"{sample_name}: MC sample has no sum_gen_weight"
                )
                continue

            valid_samples.append(
                (
                    filename,
                    cross_section,
                    sum_gen_weight,
                    "mc"
                )
            )

    # -----------------------------------------
    # Print validation errors
    # -----------------------------------------
    if errors:

        print("\n========================================")
        print("CONFIGURATION ERRORS")
        print("========================================")

        for error in errors:
            print("ERROR:", error)

        print("\nThese samples will NOT be written to General.txt.")

    # -----------------------------------------
    # Write General.txt
    # -----------------------------------------
    with open(output_txt, "w") as f:

        # Header
        f.write(
            "# filename crosssection sum_gen_weight type\n"
        )

        for filename, cross_section, sum_gen_weight, sample_type in valid_samples:

            f.write(
                f"{filename} "
                f"{cross_section} "
                f"{sum_gen_weight} "
                f"{sample_type}\n"
            )

    # -----------------------------------------
    # Summary
    # -----------------------------------------
    mc_count = sum(
        1 for x in valid_samples if x[3] == "mc"
    )

    data_count = sum(
        1 for x in valid_samples if x[3] == "data"
    )

    print("\n========================================")
    print("JSON → General.txt conversion complete")
    print("========================================")
    print(f"Input file      : {input_json}")
    print(f"Output file     : {output_txt}")
    print(f"Valid MC samples: {mc_count}")
    print(f"Valid DATA      : {data_count}")
    print(f"Total valid     : {len(valid_samples)}")
    print(f"Errors          : {len(errors)}")
    print("========================================")

    if errors:
        print(
            "\nWARNING: Fix the above configuration errors "
            "before submitting those samples."
        )


if __name__ == "__main__":
    main()
