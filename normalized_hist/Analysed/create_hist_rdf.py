import ROOT
import os

def create_normalized_histogram_rdf(filename,
                                    cross_section,
                                    luminosity=109,
                                    tree_name="outputTree"):

    ROOT.EnableImplicitMT()  # 🔥 Enable multi-threading

    # -----------------------------
    # Open DataFrame
    # -----------------------------
    df = ROOT.RDataFrame(tree_name, filename)

    normalization_factor = luminosity * cross_section

    print("Normalization Factor:", normalization_factor)

    # -----------------------------
    # Define total weight column
    # -----------------------------
    # Handles missing branches safely using Define
    columns = df.GetColumnNames()

    has_pu   = "pugenWeight" in columns
    has_sumw = "sumGenWeight" in columns or "genEventSumw" in columns

    if has_pu and has_sumw:
        sumw_branch = "sumGenWeight" if "sumGenWeight" in columns else "genEventSumw"

        df = df.Define(
            "total_weight",
            f"(pugenWeight/{sumw_branch}) * {normalization_factor}"
        )
    else:
        print("Warning: weight branches missing, using unity weight")
        df = df.Define("total_weight", f"{normalization_factor}")

    # -----------------------------
    # Branch ranges
    # -----------------------------
    branch_ranges = {
        "top_mass": (5, 400, 20),
        "TR_leadingLepton_pt": (0, 250, 18),
        "TR_subleadingLepton_pt": (0, 150, 15),
        "TR_trailingLepton_pt": (0, 100, 15),
        "TR_leadingLepton_eta": (-3, 3, 14),
        "TR_subleadingLepton_eta": (-3, 3, 14),
        "TR_trailingLepton_eta": (-3, 3, 14),
        "ncleanjetspass_SignalRegion": (-0.5, 7.5, 7),
        "ncleanbjetspass_SignalRegion": (-0.5, 5.5, 5),
        "mass_of_4L_ZZ_Region": (0, 600, 60),
        "Z_mass1_ZZ_Region": (70, 130, 35),
        "Z_mass2_ZZ_Region": (70, 130, 35),
        "combinedLeptonPt": (0, 500, 50),
        "zboson_mass_3LRegion": (70, 110, 20),
    }

    # -----------------------------
    # Output file name
    # -----------------------------
    outputFileName = os.path.basename(filename).replace(".root", "_hist_new.root")

    outputFile = ROOT.TFile(outputFileName, "RECREATE")

    # -----------------------------
    # Create histograms (lazy)
    # -----------------------------
    histograms = {}

    for branch, (xmin, xmax, bins) in branch_ranges.items():

        if branch not in columns:
            print(f"Warning: Branch {branch} not found, skipping.")
            continue

        histograms[branch] = df.Histo1D(
            (branch, branch, bins, xmin, xmax),
            branch,
            "total_weight"
        )

    # -----------------------------
    # Trigger event loop (ONE PASS)
    # -----------------------------
    for hist in histograms.values():
        hist.Write()

    outputFile.Close()

    print("Histograms saved to:", outputFileName)

if __name__ == "__main__":

    filename = "../../top_tzq.root"
    cross_section = 80   # <-- put your real cross section

    create_normalized_histogram_rdf(
        filename=filename,
        cross_section=cross_section,
        luminosity=109,
        tree_name="outputTree"
    )
