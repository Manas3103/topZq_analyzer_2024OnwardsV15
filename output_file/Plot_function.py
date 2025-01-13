import ROOT

def create_and_save_histogram(input_file, branch_name, output_file, hist_name="hist", n_bins=100, x_min=None, x_max=None):
    # Open the ROOT file
    file = ROOT.TFile(input_file, "UPDATE")  # Open in update mode to modify it
    
    # Create an RDataFrame from the file
    df = ROOT.RDataFrame("outputTree", input_file)  # "tree" is the name of the tree in the ROOT file

    # If x_min or x_max are not provided, use the min and max of the branch values
    if x_min is None:
        x_min = df.Min(branch_name).GetValue()
    if x_max is None:
        x_max = df.Max(branch_name).GetValue()

    # Create a histogram from the specified branch
    hist = df.Histo1D((hist_name, f"Histogram of {branch_name}", n_bins, x_min, x_max), branch_name)

    # Save the histogram to the output file
    hist.Write(hist_name)

    # Close the file
    file.Close()

# Example usage:
create_and_save_histogram("top_tzq_signal.root", "top_mass", "top_mass.root", n_bins=50, x_min=0, x_max=500)

