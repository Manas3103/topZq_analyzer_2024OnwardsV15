import os
import uproot
from ROOT import TCanvas, TH1F, gStyle, kBlack

def plot_histogram(input_file_path, branch_name, sum_genweights, cross_section,
                   dataset_name, output_dir, color=2, bins=25, x_min=0, x_max=500,
                   x_title="Mass (GeV/c^2)", y_title="Events",
                   save_formats=["root"]):

    # Set global style
    gStyle.SetOptStat(0)  # Turn off statistics box
    gStyle.SetOptTitle(0)  # Turn off title

    # Open the root file and get the tree
    try:
        infile = uproot.open(input_file_path)
        tree = infile["outputTree"]
    except Exception as e:
        print(f"Error opening file: {e}")
        return None, None

    # Get branch data
    try:
        branch_data = tree[branch_name].array()
    except Exception as e:
        print(f"Error accessing branch {branch_name}: {e}")
        return None, None

    # Calculate luminosity weight
    lumi = 59830  # 2018 luminosity in pb^-1
    weight = (cross_section * lumi) / sum_genweights

    # Create canvas with white background
    canvas = TCanvas("canvas", "Distribution", 800, 600)
    canvas.SetFillColor(0)
    canvas.SetBorderMode(0)
    canvas.SetFrameFillStyle(0)
    canvas.SetFrameBorderMode(0)
    canvas.SetLeftMargin(0.15)
    canvas.SetRightMargin(0.05)
    canvas.SetTopMargin(0.05)
    canvas.SetBottomMargin(0.13)

    # Create histogram
    hist_name = f"hist_{branch_name}"
    hist_title = f"{branch_name} Distribution"
    hist = TH1F(hist_name, hist_title, bins, x_min, x_max)

    # Set histogram properties
    hist.GetXaxis().SetTitle(x_title)
    hist.GetXaxis().SetTitleSize(0.045)
    hist.GetXaxis().SetTitleOffset(1.2)
    hist.GetXaxis().SetLabelSize(0.04)

    hist.GetYaxis().SetTitle(y_title)
    hist.GetYaxis().SetTitleSize(0.045)
    hist.GetYaxis().SetTitleOffset(1.5)
    hist.GetYaxis().SetLabelSize(0.04)

    # Set fill properties for solid color
    hist.SetFillColor(color)
    hist.SetFillStyle(1001)  # Solid fill
    hist.SetLineColor(kBlack)
    hist.SetLineWidth(1)

    # Fill histogram with weighted values
    for value in branch_data:
        hist.Fill(value, weight)

    # Draw and save histogram
    canvas.cd()
    hist.Draw("HIST")  # Draw with "HIST" option for filled histogram
    canvas.Update()

    # Ensure output directory exists
    os.makedirs(output_dir, exist_ok=True)

    # Save in requested formats
    base_name = os.path.join(output_dir, f"{dataset_name}_{branch_name}_distribution")
    for fmt in save_formats:
        if fmt.lower() in ["root", "cpp", "pdf", "png"]:
            canvas.SaveAs(f"{base_name}.{fmt}")

    return hist, canvas


if __name__ == "__main__":
    # Luminosity (assuming a constant, replace if needed)
    luminosity = 1  # Example value for your analysis

    # Output directory for histograms
    output_dir = "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/rootPlots"  # Replace with your desired path

    # Sample dataset information
    """
    samples = {
        "DYJetsToLL_M-50": {"file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYJetsToLL_M-50.root", "gen_weight_sum": 12345},
        "top_tzq_signal": {"file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/top_tzq_signal.root", "gen_weight_sum": 67890},
    }"""
    samples = {
    "DYJetsToLL_M-50": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYJetsToLL_M-50.root",
        "gen_weight_sum": 7601805.0
    },
    "DYJetsToLL_70to100": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYjet_70to100.root",
        "gen_weight_sum": 3287713.0
    },
    "DYJetsToLL_100to200": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYjet_100to200.root",
        "gen_weight_sum": 3916771.0
    },
    "DYJetsToLL_200to400": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYJetsToLL_M-50_HT-200to400.root",
        "gen_weight_sum": 1777636.0
    },
    "DYJetsToLL_400to600": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYJetsToLL_M-50_HT-400to600.root",
        "gen_weight_sum": 2645497.0
    },
    "DYJetsToLL_600to800": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYJetsToLL_M-50_HT-600to800.root",
        "gen_weight_sum": 1899619.0
    },
    "DYJetsToLL_800to1200": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYJetsToLL_M-50_HT-800to1200.root",
        "gen_weight_sum": 470323.0
    },
    "DYJetsToLL_1200to2500": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYJetsToLL_M-50_HT-1200to2500.root",
        "gen_weight_sum": 2468467.0
    },
    "DYJetsToLL_2500toInf": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYJetsToLL_M-50_HT-2500toInf.root",
        "gen_weight_sum": 230856.0
    },
    "top_tzq_signal": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/top_tzq_signal.root",
        "gen_weight_sum": 276122.412361
    },
    "TTGamma_Dilept": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTGamma_Dilept.root",
        "gen_weight_sum": 1150551.812393
    },
    "TTHH": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTHH.root",
        "gen_weight_sum": 64000.0
    },
    "TTTT": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTTT.root",
        "gen_weight_sum": 12723.781795
    },
    "TTToSemiLeptonic": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTToSemiLeptonic.root",
        "gen_weight_sum": 801122522.587494
    },
    "TTWH": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTWH.root",
        "gen_weight_sum": 282000.0
    },
    "TTWJetsToLNu": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTWJetsToLNu.root",
        "gen_weight_sum": 1374908.617302
    },
    "TTWZ": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTWZ.root",
        "gen_weight_sum": 350000.0
    },
    "TTZH": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTZH.root",
        "gen_weight_sum": 150000.0
    },
    "TTZToLLNuNu": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTZToLLNuNu_M-10.root",
        "gen_weight_sum": 291407.041581
    },
    "TTZZ": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTZZ.root",
        "gen_weight_sum": 249000.0
    },
    "ST_t-channel_antitop": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/ST_t-channel_antitop.root",
        "gen_weight_sum": 4438474.615708
    },
    "ST_t-channel_top": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/ST_t-channel_top.root",
        "gen_weight_sum": 4551182.188389
    },
    "ST_tW_top": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/ST_tW_top.root",
        "gen_weight_sum": 209310186.62239
    },
    "ST_tW_antitop_5f_inclusiveDecays": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/ST_tW_antitop_5f_inclusiveDecays.root",
        "gen_weight_sum": 60434553.54943
    },
    "WGToLNuG": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/WGToLNuG.root",
        "gen_weight_sum": 809686.0
    },
    "WJetsToLNu": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/WJetsToLNu.root",
        "gen_weight_sum": 53315163.018044
    },
    "WWTo2L2Nu": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/WWTo2L2Nu.root",
        "gen_weight_sum": 22155848.698236
    },
    "WWW": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/WWW.root",
        "gen_weight_sum": 7114.274035
    },
    "WZTo2L2Q": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/WZTo2L2Q.root",
        "gen_weight_sum": 44053345.742674
    },
    "WZZ": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/WZZ.root",
        "gen_weight_sum": 17023.650741
    },
    "ZGToLLG": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/ZGToLLG.root",
        "gen_weight_sum": 1717514026.388672
    },
    "ZZTo2L2Nu": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/ZZTo2L2Nu.root",
        "gen_weight_sum": 2795712.293901
    },
    "ZZTo2L2Q": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/ZZTo2L2Q.root",
        "gen_weight_sum": 32220774.386971
    },
    "ZZZ": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/ZZZ.root",
        "gen_weight_sum": 974.913533
    },
    "TTWW": {
        "file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/TTWW.root",
        "gen_weight_sum": 131000.0
    }
    }


    # Iterate over each sample in the dictionary
    for sample_name, details in samples.items():
        # Extract file path and generator weight sum
        file_path = details["file_path"]
        gen_weight_sum = details["gen_weight_sum"]

        # Set cross-section to 1 for all samples
        cross_section = 1

        # Branch name to analyze (example: "top_mass")
        branch_name = "top_mass"  # Replace with the actual branch name

        print(f"Processing sample: {sample_name}")
        hist, canvas = plot_histogram(
            input_file_path=file_path,
            branch_name=branch_name,
            sum_genweights=gen_weight_sum,
            cross_section=cross_section,
            dataset_name=sample_name,
            output_dir=output_dir,
            color=2,  # Example: Set color to red
            save_formats=["root"]  # Save in multiple formats
        )

        if hist and canvas:
            print(f"Histogram created and saved for {sample_name}.")
        else:
            print(f"Failed to process sample {sample_name}.")

