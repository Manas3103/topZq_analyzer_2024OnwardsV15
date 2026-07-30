import ROOT
import os
import argparse
import time


def is_data_sample(filename, extra_flag):
    if extra_flag.lower() == "data":
        return True
    if "data" in filename.lower():
        return True
    return False


def get_sumw_branch(columns):
    """Priority: sumGenWeight first, then genEventSumw, else None."""
    if "sumGenWeight" in columns:
        return "sumGenWeight"
    elif "genEventSumw" in columns:
        return "genEventSumw"
    return None


def calculate_yield(filename, cross_section, luminosity, tree_name, extra_flag):

    start_time = time.time()
    ROOT.EnableImplicitMT()

    df = ROOT.RDataFrame(tree_name, filename)
    columns = df.GetColumnNames()

    is_data = is_data_sample(filename, extra_flag)
    print("Running on DATA sample:", is_data)

    if is_data:
        # -----------------------------------
        # DATA: unit weight -> raw event count
        # -----------------------------------
        df = df.Define("total_weight", "1.0")

    else:
        # -----------------------------------
        # MC: (genWeight / sumGenWeight) * xsec * lumi
        # -----------------------------------
        has_gen = "genWeight" in columns
        sumw_branch = get_sumw_branch(columns)

        if not (has_gen and sumw_branch):
            print("Warning: required weight branches (genWeight / sumGenWeight"
                  " or genEventSumw) missing, cannot compute normalized yield.")
            return

        normalization_factor = luminosity * cross_section
        print("Using sumw branch:", sumw_branch)
        print("Normalization Factor:", normalization_factor)

        df = df.Define(
            "total_weight",
            f"(genWeight / {sumw_branch}) * {normalization_factor}"
        )

    # -----------------------------------
    # Book a 1-bin histogram holding the yield
    # (bin center at 0.5, range 0-1 -> single inclusive bin)
    # -----------------------------------
    yield_hist = df.Histo1D(
        ("yield", "yield", 1, 0.0, 1.0),
        "total_weight" if False else "total_weight",  # dummy x-value column
        "total_weight"
    )
    # Note: since we just want ONE number in ONE bin, we instead fill using
    # a constant dummy x-value of 0.5 for every event, weighted by total_weight.
    df = df.Define("yield_x", "0.5")
    yield_hist = df.Histo1D(
        ("yield", "yield", 1, 0.0, 1.0),
        "yield_x",
        "total_weight"
    )

    # -----------------------------------
    # Output file
    # -----------------------------------
    outputFileName = os.path.basename(filename).replace(".root", "_hist.root")
    outputFile = ROOT.TFile(outputFileName, "RECREATE")
    yield_hist.Write()
    outputFile.Close()

    yield_value = yield_hist.Integral()

    end_time = time.time()

    print("\n=========== SUMMARY ===========")
    print("File:", filename)
    print("Type:", "DATA" if is_data else "MC")
    print("Yield:", yield_value)
    print("Output file:", outputFileName)
    print("Execution time:", round(end_time - start_time, 2), "seconds")
    print("================================")

    return yield_value


if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument("--filename", required=True)
    parser.add_argument("--cross_section", type=float, default=1.0)
    parser.add_argument("--luminosity", type=float, default=1.0)
    parser.add_argument("--tree_name", default="outputTree")
    parser.add_argument("--extra", default="mc",
                        help="Use 'data' to force no weighting")

    args = parser.parse_args()

    calculate_yield(
        filename=args.filename,
        cross_section=args.cross_section,
        luminosity=args.luminosity,
        tree_name=args.tree_name,
        extra_flag=args.extra
    )
