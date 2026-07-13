import ROOT
import os
import argparse
import json
import time


def load_config(config_file):
    with open(config_file, "r") as f:
        return json.load(f)


def is_data_sample(filename, extra_flag):
    if extra_flag.lower() == "data":
        return True
    if "data" in filename.lower():
        return True
    return False


def create_normalized_histogram_rdf(filename,
                                    cross_section,
                                    luminosity,
                                    tree_name,
                                    config_file,
                                    extra_flag):

    start_time = time.time()

    ROOT.EnableImplicitMT()

    df = ROOT.RDataFrame(tree_name, filename)
    columns = df.GetColumnNames()

    # -----------------------------------
    # Determine if DATA
    # -----------------------------------
    is_data = is_data_sample(filename, extra_flag)

    print("Running on DATA sample:", is_data)

    # -----------------------------------
    # Weight definition
    # -----------------------------------
    if not is_data:
        normalization_factor = luminosity * cross_section
        print("Normalization Factor:", normalization_factor)

        has_pu   = "pugenWeight" in columns
        has_gen   = "genWeight" in columns
        has_ev   = "evWeight" in columns
        has_muon_sf = "muon_SF_central" in columns
        has_ev_new = "evWeight_with_em_SF_Only" in columns
        has_sumw = "sumGenWeight" in columns or "genEventSumw" in columns

        if has_pu and has_sumw and has_ev and has_gen and has_muon_sf:
            sumw_branch = "sumGenWeight" if "sumGenWeight" in columns else "genEventSumw"
            df = df.Define(
                "total_weight",
                f"((pugenWeight)/{sumw_branch}) * {normalization_factor}"  
            )#has done with evWeight
        else:
            print("Warning: weight branches missing, using normalization only")
            df = df.Define("total_weight", f"{normalization_factor}")
    else:
        print("DATA detected → Using unit weights")
        df = df.Define("total_weight", "1.0")

    # -----------------------------------
    # Load histogram configuration
    # -----------------------------------
    branch_ranges = load_config(config_file)

    # -----------------------------------
    # Output file
    # -----------------------------------
    outputFileName = os.path.basename(filename).replace(".root", "_hist.root")
    outputFile = ROOT.TFile(outputFileName, "RECREATE")

    # -----------------------------------
    # Create histograms (lazy)
    # -----------------------------------
    histograms = {}

    for branch, config in branch_ranges.items():

        if branch not in columns:
            print(f"Warning: Branch {branch} not found, skipping.")
            continue

        bins = int(config["bins"])
        xmin = float(config["xmin"])
        xmax = float(config["xmax"])

        histograms[branch] = df.Histo1D(
            (branch, branch, bins, xmin, xmax),
            branch,
            "total_weight"
        )

    # -----------------------------------
    # Trigger event loop (single pass)
    # -----------------------------------
    for hist in histograms.values():
        hist.Write()

    outputFile.Close()

    end_time = time.time()

    print("\n=========== SUMMARY ===========")
    print("Output file:", outputFileName)
    print("Histograms created:", len(histograms))
    print("Execution time:", round(end_time - start_time, 2), "seconds")
    print("================================")


if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument("--filename", required=True)
    parser.add_argument("--cross_section", type=float, default=1.0)
    parser.add_argument("--luminosity", type=float, default=1.0)
    parser.add_argument("--tree_name", default="outputTree")
    parser.add_argument("--config", required=True,
                        help="JSON config file for histogram definitions")
    parser.add_argument("--extra", default="mc",
                        help="Use 'data' to force no weighting")

    args = parser.parse_args()

    create_normalized_histogram_rdf(
        filename=args.filename,
        cross_section=args.cross_section,
        luminosity=args.luminosity,
        tree_name=args.tree_name,
        config_file=args.config,
        extra_flag=args.extra
    )
