import ROOT
import os
import argparse
import json
import time
import re 

def load_config(config_file):
    with open(config_file, "r") as f:
        return json.load(f)

# ---------- NEW HELPER FUNCTION 1 ----------
def make_default_title(branch):
    title = branch.replace("_", " ")
    title = re.sub(r"(?<!^)(?=[A-Z])", " ", title)
    title = re.sub(r"\s+", " ", title).strip()
    return title


# ---------- NEW HELPER FUNCTION 2 ----------
def make_default_xlabel(branch):
    variable_labels = {
        "pt": "p_{T} [GeV]",
        "eta": "#eta",
        "phi": "#phi",
        "mass": "Mass [GeV]",
        "nJets": "Number of Jets",
        "nbJets": "Number of b-Jets",
        "muon_multiplicity": "Muon Multiplicity"
    }

    for key, label in variable_labels.items():
        if branch.endswith(key):
            return label

    return make_default_title(branch)


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
                                    sum_gen_weight=None,
                                    extra_flag="mc"):

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

        # -----------------------------------
        # Determine Sum of Generator Weights
        # -----------------------------------

        if sum_gen_weight is not None:
            # Use value provided by user
            sumw = float(sum_gen_weight)
            print(f"Using input SumGenWeight = {sumw}")

        else:
            # Look for branch inside ROOT tree
            if "sumGenWeight" in columns:
                sumw = float(df.Max("sumGenWeight").GetValue())
                print(f"Using branch 'sumGenWeight' = {sumw}")

            elif "genEventSumw" in columns:
                sumw = float(df.Max("genEventSumw").GetValue())
                print(f"Using branch 'genEventSumw' = {sumw}")

            else:
                raise RuntimeError(
                    "MC sample detected but no SumGenWeight provided "
                    "and neither 'sumGenWeight' nor 'genEventSumw' branch exists."
                )

        # -----------------------------------
        # Normalization
        # -----------------------------------

        normalization_factor = luminosity * cross_section / sumw

        print(f"Cross section       : {cross_section}")
        print(f"Luminosity          : {luminosity}")
        print(f"Normalization factor: {normalization_factor}")


        weight_terms = []

        # Always include normalization
        weight_terms.append(str(normalization_factor))

        # Check available branches
        has_pu = "pugenWeight" in columns
        has_ele_sf = "ele_SF_central" in columns
        has_muon_sf = "muon_SF_central" in columns
        has_muon_iso_sf = "muon_SF_iso_nominal" in columns
        has_muon_id_sf = "muon_SF_id_nominal" in columns
        has_btag_lf = "btag_SF_lflav_vector" in columns
        has_btag_bc = "btag_SF_bcflav_vector" in columns

        # Print availability
        print("\n========== AVAILABLE WEIGHT BRANCHES ==========")

        if has_pu:
            print("pugenWeight                  : AVAILABLE")
            weight_terms.append("pugenWeight")
        else:
            print("pugenWeight                  : MISSING")

        if has_ele_sf:
            print("ele_SF_central               : AVAILABLE")
            weight_terms.append("ele_SF_central")
        else:
            print("ele_SF_central               : MISSING")

        # if has_muon_sf:
        #     print("muon_SF_central              : AVAILABLE")
        #     weight_terms.append("muon_SF_central")
        # else:
        #     print("muon_SF_central              : MISSING")

        if has_muon_iso_sf:
            print("muon_SF_iso_nominal              : AVAILABLE")
            weight_terms.append("muon_SF_iso_nominal")
        else:
            print("muon_SF_iso_nominal              : MISSING")

        if has_muon_id_sf:
            print("muon_SF_id_nominal              : AVAILABLE")
            weight_terms.append("muon_SF_id_nominal")
        else:
            print("muon_SF_id_nominal              : MISSING")

        if has_btag_lf:
            print("btag_SF_lflav_vector[0]      : AVAILABLE")
            weight_terms.append("btag_SF_lflav_vector[0]")
        else:
            print("btag_SF_lflav_vector[0]      : MISSING")

        if has_btag_bc:
            print("btag_SF_bcflav_vector[0]     : AVAILABLE")
            weight_terms.append("btag_SF_bcflav_vector[0]")
        else:
            print("btag_SF_bcflav_vector[0]     : MISSING")

        print("===============================================")

        # -----------------------------------
        # Construct total weight
        # -----------------------------------

        weight_expression = " * ".join(weight_terms)

        print(f"Final weight expression: {weight_expression}")

        df = df.Define(
            "total_weight",
            weight_expression
        )

    else:
        print("DATA detected -> Using unit weights")

        df = df.Define(
            "total_weight",
            "1.0"
        )


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

        hist_title = config.get("title", make_default_title(branch))
        x_axis_title = config.get("xlabel", make_default_xlabel(branch))
        y_axis_title = config.get("ylabel", "Events")

        root_title = f"{hist_title};{x_axis_title};{y_axis_title}"

        histograms[branch] = df.Histo1D(
            (branch, root_title, bins, xmin, xmax),
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
    parser.add_argument(
            "--sum_gen_weight",
            type=float,
            default=None,
            help="Optional total sum of generator weights for MC samples.")

    args = parser.parse_args()

    create_normalized_histogram_rdf(
        filename=args.filename,
        cross_section=args.cross_section,
        luminosity=args.luminosity,
        tree_name=args.tree_name,
        config_file=args.config,
        sum_gen_weight=args.sum_gen_weight,
        extra_flag=args.extra
    )
