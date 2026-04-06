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
    """branch_ranges = {
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
    }"""
    branch_ranges = {

        "top_mass": (5, 700, 50),
        "leadingLepton_pt": (0, 250, 25),
        "subleadingLepton_pt": (0, 150, 20),
        "trailingLepton_pt": (0, 100, 20),

        "TR_leadingLepton_pt": (0, 250, 25),
        "TR_subleadingLepton_pt": (0, 150, 20),
        "TR_trailingLepton_pt": (0, 100, 20),
        "TR_leadingLepton_eta": (-3, 3, 24),
        "TR_subleadingLepton_eta": (-3, 3, 24),
        "TR_trailingLepton_eta": (-3, 3, 24),

        "Wboson_transversMass": (5, 300, 35),
        "OSSF_ZPair_mass": (70, 130, 35),

        # 3-lepton regions
        "ncleanjetspass_SignalRegion": (-0.5, 7.5, 7),
        "ncleanbjetspass_SignalRegion": (-0.5, 5.5, 5),

        "ncleanjetspass_WZ_Region": (-0.5, 7.5, 7),
        "Wboson_transversMass_WZ_Region": (5, 300, 35),

        "ncleanjetspass_X_gamma_Region": (-0.5, 7.5, 7),
        "ncleanbjetspass_X_gamma_Region": (-0.5, 5.5, 5),

        "ncleanjetspass_NP_2_Region": (-0.5, 7.5, 7),
        "ncleanbjetspass_NP_2_Region": (-0.5, 5.5, 5),

        "ncleanjetspass_NP_1_Region": (-0.5, 7.5, 7),
        "ncleanbjetspass_NP_1_Region": (-0.5, 5.5, 5),

        # 4-lepton regions
        "ncleanjetspass_ZZ_Region": (-0.5, 7.5, 7),
        "mass_of_4L_ZZ_Region": (0, 600, 60),
        "Z_mass1_ZZ_Region": (70, 130, 35),
        "Z_mass2_ZZ_Region": (70, 130, 35),

        "ncleanjetspass_ttZ_Region": (-0.5, 7.5, 7),
        "ncleanbjetspass_ttZ_Region": (-0.5, 5.5, 5),

        # tzq region variables
        "nJet_tzq": (0, 6, 7),
        "nBJets_tzq": (0, 5, 6),
        "mWT_tzq": (0, 250, 25),
        "mTop_tzq": (0, 400, 20),
        "mZ_tzq": (70, 110, 20),
        "dphi_ll_z_tzq": (0, 3, 12),
        "cosThetaPol_tzq": (-1, 1, 20),
        "sumHadPt_tzq": (0, 900, 30),
        "sumLepMetPt_tzq": (0, 700, 25),
        "min_dR_bl_tzq": (0, 5, 20),
        "max_dR_bl_tzq": (0, 7, 20),
        "max_dphi_jj_tzq": (0, 4, 15),
        "max_ptjj_tzq": (0, 500, 20),
        "max_mjj_tzq": (0, 1200, 30),
        "mass3l_tzq": (50, 500, 20),
        "dR_b_recoil_tzq": (0, 8, 16),
        "dR_b_l_tzq": (0, 6, 12),
        "maxJetAbsEta_tzq": (-5, 5, 20),
        "etaRecoilingJet_tzq": (-5, 5, 20),
        "lep_asymmetry_tzq": (-3, 3, 20),
        "maxDEEPJET_tzq": (0, 1, 20),
        "MET_pt_tzq": (0, 270, 27),

        # ttz region variables
        "nJet_ttz": (0, 6, 7),
        "nBJets_ttz": (0, 5, 6),
        "mWT_ttz": (0, 250, 25),
        "mTop_ttz": (0, 400, 20),
        "mZ_ttz": (70, 110, 20),
        "dphi_ll_z_ttz": (0, 3, 12),
        "cosThetaPol_ttz": (-1, 1, 20),
        "sumHadPt_ttz": (0, 900, 30),
        "sumLepMetPt_ttz": (0, 700, 25),
        "min_dR_bl_ttz": (0, 5, 20),
        "max_dR_bl_ttz": (0, 7, 20),
        "max_dphi_jj_ttz": (0, 4, 15),
        "max_ptjj_ttz": (0, 500, 20),
        "max_mjj_ttz": (0, 1200, 30),
        "mass3l_ttz": (50, 500, 20),
        "dR_b_recoil_ttz": (0, 8, 16),
        "dR_b_l_ttz": (0, 6, 12),
        "maxJetAbsEta_ttz": (-5, 5, 20),
        "etaRecoilingJet_ttz": (-5, 5, 20),
        "lep_asymmetry_ttz": (-3, 3, 20),
        "maxDEEPJET_ttz": (0, 1, 20),
        "MET_pt_ttz": (0, 270, 27),

        # trial region variables
        "nJet_trial": (0, 6, 7),
        "nBJets_trial": (0, 5, 6),
        "mWT_trial": (0, 250, 25),
        "mTop_trial": (0, 400, 20),
        "mZ_trial": (70, 110, 20),
        "dphi_ll_z_trial": (0, 3, 12),
        "cosThetaPol_trial": (-1, 1, 20),
        "sumHadPt_trial": (0, 900, 30),
        "sumLepMetPt_trial": (0, 700, 25),
        "min_dR_bl_trial": (0, 5, 20),
        "max_dR_bl_trial": (0, 7, 20),
        "max_dphi_jj_trial": (0, 4, 15),
        "max_ptjj_trial": (0, 500, 20),
        "max_mjj_trial": (0, 1200, 30),
        "mass3l_trial": (50, 500, 20),
        "dR_b_recoil_trial": (0, 8, 16),
        "dR_b_l_trial": (0, 6, 12),
        "maxJetAbsEta_trial": (-5, 5, 20),
        "etaRecoilingJet_trial": (-5, 5, 20),
        "lep_asymmetry_trial": (-3, 3, 20),
        "maxDEEPJET_trial": (0, 1, 20),
        "MET_pt_trial": (0, 270, 27),

        # signal region variables
        "nJet_signal": (0, 6, 7),
        "nBJets_signal": (0, 5, 6),
        "mWT_signal": (0, 250, 25),
        "mTop_signal": (0, 400, 20),
        "mZ_signal": (70, 110, 20),
        "dphi_ll_z_signal": (0, 3, 12),
        "cosThetaPol_signal": (-1, 1, 20),
        "sumHadPt_signal": (0, 900, 30),
        "sumLepMetPt_signal": (0, 700, 25),
        "min_dR_bl_signal": (0, 5, 20),
        "max_dR_bl_signal": (0, 7, 20),
        "max_dphi_jj_signal": (0, 4, 15),
        "max_ptjj_signal": (0, 500, 20),
        "max_mjj_signal": (0, 1200, 30),
        "mass3l_signal": (50, 500, 20),
        "dR_b_recoil_signal": (0, 8, 16),
        "dR_b_l_signal": (0, 6, 12),
        "maxJetAbsEta_signal": (-5, 5, 20),
        "etaRecoilingJet_signal": (-5, 5, 20),
        "lep_asymmetry_signal": (-3, 3, 20),
        "maxDEEPJET_signal": (0, 1, 20),
        "MET_pt_signal": (0, 270, 27),

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
