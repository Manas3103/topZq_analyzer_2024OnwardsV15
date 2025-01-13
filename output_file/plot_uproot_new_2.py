import os
import uproot
from ROOT import TCanvas, TH1F, gStyle, kBlack, TFile

def plot_histogram(input_file_path, branch_name, sum_genweights, cross_section,
                   dataset_name, output_dir, color=2, bins=25, x_min=0, x_max=500,
                   x_title="Mass (GeV/c^2)", y_title="Events"):

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

    # Draw and update the histogram on the canvas
    canvas.cd()
    hist.Draw("HIST")  # Draw with "HIST" option for filled histogram
    canvas.Update()

    # Ensure output directory exists
    os.makedirs(output_dir, exist_ok=True)

    # Save histogram to ROOT file
    output_root_file_name = os.path.join(output_dir, f"{dataset_name}_{branch_name}_histograms.root")
    output_root_file = TFile(output_root_file_name, "UPDATE")  # Open in "UPDATE" mode to add histograms
    hist.Write(hist_name)  # Write the histogram with its name
    output_root_file.Close()

    return hist, canvas


if __name__ == "__main__":
    # Luminosity (assuming a constant, replace if needed)
    luminosity = 1  # Example value for your analysis

    # Output directory for histograms
    output_dir = "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/rootPlots"  # Replace with your desired path

    # Sample dataset information
    samples = {
        "DYJetsToLL_M-50": {"file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/DYJetsToLL_M-50.root", "gen_weight_sum": 12345},
        "top_tzq_signal": {"file_path": "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/top_tzq_signal.root", "gen_weight_sum": 67890},
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
            color=2  # Example: Set color to red
        )

        if hist and canvas:
            print(f"Histogram created and saved for {sample_name}.")
        else:
            print(f"Failed to process sample {sample_name}.")

