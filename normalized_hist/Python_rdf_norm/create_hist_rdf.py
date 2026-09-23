import ROOT
import os
import argparse
import json
import time
import re

def load_config(config_file):
    with open(config_file, "r") as f:
        return json.load(f)

# ---------- HELPER FUNCTION 1 ----------
def make_default_title(branch):
    title = branch.replace("_", " ")
    title = re.sub(r"(?<!^)(?=[A-Z])", " ", title)
    title = re.sub(r"\s+", " ", title).strip()
    return title


# ---------- HELPER FUNCTION 2 ----------
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


def resolve_branch_expr(branch, columns):
    """
    Resolve a requested branch name to an actual RDataFrame column
    expression. If the exact branch name exists, use it as-is.
    If it doesn't exist but a vector/array version does (e.g. the
    user asked for 'leadJet_eta' but the tree only has 'Jet_eta'),
    fall back to index [0] on the array branch (the leading object).
    """
    if branch in columns:
        return branch

    # common case: user wants the "leading" object but tree stores
    # a vector/array branch instead of a dedicated lead-object branch
    candidates = []

    lower = branch.lower()
    for prefix in ("leadjet_", "lead_jet_", "leading_jet_"):
        if lower.startswith(prefix):
            suffix = branch[len(prefix):]
            candidates.append(f"Jet_{suffix}")
            candidates.append(f"jet_{suffix}")

    for cand in candidates:
        if cand in columns:
            return f"{cand}[0]"

    # last resort: if branch itself is a known vector-style column,
    # just index it directly
    if branch in columns:
        return branch

    return None


def create_2d_histograms(df, columns, config_2d):
    """
    Build lazy 2D histograms (e.g. eta vs phi of the leading jet)
    from a config dict of the form:

    {
      "leadJet_eta_phi": {
          "xbranch": "leadJet_eta",
          "ybranch": "leadJet_phi",
          "xbins": 50, "xmin": -3.0, "xmax": 3.0,
          "ybins": 50, "ymin": -3.2, "ymax": 3.2,
          "title": "Leading Jet #eta vs #phi",
          "xlabel": "#eta",
          "ylabel": "#phi"
      }
    }
    """
    hist2d = {}

    for name, cfg in config_2d.items():

        xbranch_raw = cfg["xbranch"]
        ybranch_raw = cfg["ybranch"]

        xexpr = resolve_branch_expr(xbranch_raw, columns)
        yexpr = resolve_branch_expr(ybranch_raw, columns)

        if xexpr is None or yexpr is None:
            print(f"Warning: could not resolve branch(es) "
                  f"'{xbranch_raw}' / '{ybranch_raw}' for 2D hist "
                  f"'{name}', skipping.")
            continue

        xbins = int(cfg["xbins"])
        xmin = float(cfg["xmin"])
        xmax = float(cfg["xmax"])
        ybins = int(cfg["ybins"])
        ymin = float(cfg["ymin"])
        ymax = float(cfg["ymax"])

        title = cfg.get("title", f"{xbranch_raw} vs {ybranch_raw}")
        xlabel = cfg.get("xlabel", make_default_xlabel(xbranch_raw))
        ylabel = cfg.get("ylabel", make_default_xlabel(ybranch_raw))

        root_title = f"{title};{xlabel};{ylabel}"

        print(f"Booking 2D histogram '{name}': x={xexpr}, y={yexpr}")

        hist2d[name] = df.Histo2D(
            (name, root_title, xbins, xmin, xmax, ybins, ymin, ymax),
            xexpr,
            yexpr,
            "total_weight"
        )

    return hist2d


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
            sumw = float(sum_gen_weight)
            print(f"Using input SumGenWeight = {sumw}")

        else:
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
        weight_terms.append(str(normalization_factor))

        has_pu = "pugenWeight" in columns
        has_ele_sf = "ele_SF_central" in columns
        has_muon_sf = "muon_SF_central" in columns
        has_muon_iso_sf = "muon_SF_iso_nominal" in columns
        has_muon_id_sf = "muon_SF_id_nominal" in columns
        has_btag_lf = "btag_SF_lflav_vector" in columns
        has_btag_bc = "btag_SF_bcflav_vector" in columns

        print("\n========== AVAILABLE WEIGHT BRANCHES ==========")

        if has_pu:
            print("pugenWeight                  : AVAILABLE")
            weight_terms.append("pugenWeight")
        else:
            print("pugenWeight                  : MISSING")

        # if has_ele_sf:
        #     print("ele_SF_central               : AVAILABLE")
        #     weight_terms.append("ele_SF_central")
        # else:
        #     print("ele_SF_central               : MISSING")

        # if has_muon_sf:
        #     print("muon_SF_central              : AVAILABLE")
        #     weight_terms.append("muon_SF_central")
        # else:
        #     print("muon_SF_central              : MISSING")

        # if has_muon_iso_sf:
        #     print("muon_SF_iso_nominal              : AVAILABLE")
        #     weight_terms.append("muon_SF_iso_nominal")
        # else:
        #     print("muon_SF_iso_nominal              : MISSING")

        # if has_muon_id_sf:
        #     print("muon_SF_id_nominal              : AVAILABLE")
        #     weight_terms.append("muon_SF_id_nominal")
        # else:
        #     print("muon_SF_id_nominal              : MISSING")

        # if has_btag_lf:
        #     print("btag_SF_lflav_vector[0]      : AVAILABLE")
        #     weight_terms.append("btag_SF_lflav_vector[0]")
        # else:
        #     print("btag_SF_lflav_vector[0]      : MISSING")

        # if has_btag_bc:
        #     print("btag_SF_bcflav_vector[0]     : AVAILABLE")
        #     weight_terms.append("btag_SF_bcflav_vector[0]")
        # else:
        #     print("btag_SF_bcflav_vector[0]     : MISSING")

        print("===============================================")

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

    # Pull out the optional "hist2d" section; everything else is
    # treated as a 1D histogram definition, exactly as before.
    config_2d = branch_ranges.pop("hist2d", {})

    # -----------------------------------
    # Output file
    # -----------------------------------
    outputFileName = os.path.basename(filename).replace(".root", "_hist.root")
    outputFile = ROOT.TFile(outputFileName, "RECREATE")

    # -----------------------------------
    # Create 1D histograms (lazy)
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
    # Create 2D histograms (lazy)
    # e.g. leading jet eta (x) vs phi (y)
    # -----------------------------------
    hist2d = create_2d_histograms(df, columns, config_2d)

    # -----------------------------------
    # Trigger event loop (single pass)
    # Both 1D and 2D histograms are filled
    # together since they share the same df graph.
    # -----------------------------------
    for hist in histograms.values():
        hist.Write()

    for hist in hist2d.values():
        hist.Write()

    outputFile.Close()

    end_time = time.time()

    print("\n=========== SUMMARY ===========")
    print("Output file:", outputFileName)
    print("1D histograms created:", len(histograms))
    print("2D histograms created:", len(hist2d))
    print("Execution time:", round(end_time - start_time, 2), "seconds")
    print("================================")


if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument("--filename", required=True)
    parser.add_argument("--cross_section", type=float, default=1.0)
    parser.add_argument("--luminosity", type=float, default=1.0)
    parser.add_argument("--tree_name", default="outputTree")
    parser.add_argument("--config", required=True,
                        help="JSON config file for histogram definitions "
                             "(supports 1D branches and an optional "
                             "'hist2d' section for 2D histograms)")
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
