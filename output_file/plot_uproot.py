import uproot
from ROOT import TCanvas, TH1F, TLorentzVector, gStyle, TColor, kBlack

def plot_histogram(input_file_path, branch_name, sum_genweights, cross_section,
                  color=2, bins=25, x_min=0, x_max=500,
                  x_title="Mass (GeV/c^2)", y_title="Events",
                  save_formats=["root"]):
    """
    Create and save a histogram from a ROOT file branch with solid color filling.
   
    Parameters:
    -----------
    input_file_path : str
        Path to the input ROOT file
    branch_name : str
        Name of the branch to plot
    sum_genweights : float
        Sum of generator weights
    cross_section : float
        Cross section value
    color : int, optional
        ROOT color code for histogram filling (default: 2)
    bins : int, optional
        Number of bins in histogram (default: 25)
    x_min : float, optional
        Minimum x-axis value (default: 0)
    x_max : float, optional
        Maximum x-axis value (default: 500)
    x_title : str, optional
        X-axis title (default: "Mass (GeV/c^2)")
    y_title : str, optional
        Y-axis title (default: "Events")
    save_formats : list, optional
        List of formats to save the plot in (default: ["root"])
        Supported formats: "root", "cpp", "pdf", "png"
   
    Returns:
    --------
    hist : TH1F
        The created histogram object
    canvas : TCanvas
        The canvas object
    """
   
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
   
    # Save in requested formats
    base_name = f"{branch_name}_distribution"
    for fmt in save_formats:
        if fmt.lower() in ["root", "cpp", "pdf", "png"]:
            canvas.SaveAs(f"{base_name}.{fmt}")
   
    return hist, canvas

def process_samples(samples, branch_name, cross_section=1, color=2, save_formats=["root", "png", "pdf"]):
    """
    Process multiple samples and generate histograms for each.

    Parameters:
    -----------
    samples : dict
        Dictionary containing sample information (file path and gen weights)
    branch_name : str
        Branch to plot
    cross_section : float, optional
        Cross section for all samples (default: 1)
    color : int, optional
        Color code for the histograms (default: 2)
    save_formats : list, optional
        Formats to save the histograms (default: ["root", "png", "pdf"])
    """
    for sample_name, sample_info in samples.items():
        print(f"Processing sample: {sample_name}")
        file_path = sample_info["file_path"]
        genweights_sum = sample_info["gen_weight_sum"]

        hist, canvas = plot_histogram(
            input_file_path=file_path,
            branch_name=branch_name,
            sum_genweights=genweights_sum,
            cross_section=cross_section,
            color=color,
            save_formats=save_formats
        )
        if hist and canvas:
            print(f"Successfully processed {sample_name}.")
        else:
            print(f"Failed to process {sample_name}.")


if __name__ == "__main__":
    # Luminosity (assuming a constant, replace if needed)
    luminosity = 59830  # Example value for your analysis
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
            color=2,  # Example: Set color to red
            save_formats=["root", "png", "pdf"]  # Save in multiple formats
        )

        if hist and canvas:
            print(f"Histogram created and saved for {sample_name}.")
        else:
            print(f"Failed to process sample {sample_name}.")





"""
# Example usage:
if __name__ == "__main__":
    branch = "top_mass"  # Replace with the branch you want to plot
    process_samples(samples, branch_name=branch)




# Example usage:
if __name__ == "__main__":
    file_path = "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/top_tzq_signal.root"
    branch = "top_mass"
    genweights_sum = 276122.412361  # Replace with actual sum of generator weights
    xsec = 1  # Replace with actual cross section
   
    hist, canvas = plot_histogram(
        file_path,
        branch,
        genweights_sum,
        xsec,
color=2,
        save_formats=["root", "png", "pdf"]
    )
   
    # Keep the canvas open until user input
    input("Press Enter to continue...")
"""
