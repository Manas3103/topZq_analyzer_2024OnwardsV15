import os
import ROOT

def stack_histograms(directory, hist_name, output_file="stacked_histograms.root"):
    """
    Stack histograms from multiple ROOT files in a directory.
    
    Parameters:
    - directory (str): Path to the directory containing the ROOT files.
    - hist_name (str): Name of the histogram to stack.
    - output_file (str): Name of the output ROOT file to save the stacked histogram.
    
    Returns:
    - ROOT.TH1: The stacked histogram.
    """
    # Create an empty histogram to stack into
    stacked_hist = None

    # Iterate over all ROOT files in the directory
    for filename in os.listdir(directory):
        if filename.endswith(".root"):
            filepath = os.path.join(directory, filename)
            print(f"Processing file: {filepath}")
            
            root_file = ROOT.TFile.Open(filepath, "READ")

            if not root_file or root_file.IsZombie():
                print(f"Failed to open {filepath}. Skipping.")
                continue

            # Get the histogram
            hist = root_file.Get(hist_name)
            if not hist:
                print(f"Histogram '{hist_name}' not found in {filepath}. Skipping.")
                root_file.Close()
                continue

            # Add the histogram to the stack
            if stacked_hist is None:
                stacked_hist = hist.Clone()  # Clone the first histogram
                stacked_hist.SetDirectory(0)  # Detach from the file
            else:
                stacked_hist.Add(hist)  # Add subsequent histograms

            root_file.Close()

    # Save the stacked histogram to a new ROOT file
    if stacked_hist:
        print(f"Saving stacked histogram to {output_file}.")
        output_root_file = ROOT.TFile(output_file, "RECREATE")
        stacked_hist.Write()
        output_root_file.Close()
    else:
        print("No histograms were found to stack.")

    return stacked_hist

# Example usage
stack_histograms(
    directory=".",  # Replace with the actual path
    hist_name="top_mass",  # Replace with the exact histogram name
    output_file="stacked_top_mass.root"
)

