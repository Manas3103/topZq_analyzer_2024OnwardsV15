import ROOT
from array import array
import os
import re

def print_branch_info(tree, branch_name):
    """Print information about values in a branch"""
    n_entries = tree.GetEntries()
    print(f"\nBranch '{branch_name}' information:")
    print(f"Number of entries: {n_entries}")
    
    # Print first few values
    for i in range(min(5, n_entries)):
        tree.GetEntry(i)
        value = getattr(tree, branch_name)
        print(f"Entry {i}: {value}")

def get_sample_group(sample_name):
    """
    Extract the base group name from the sample name
    e.g., 'QCD_Pt-80To120_MuEnrichedPt5' -> 'QCD_MuEnriched'
         'QCD_Pt-80to120_EMEnriched' -> 'QCD_EMEnriched'
         'DYJetsToLL_M-50_UL17' -> 'DYJets'
    """
    if 'QCD' in sample_name:
        if 'MuEnriched' in sample_name:
            return 'QCD_MuEnriched'
        elif 'EMEnriched' in sample_name:
            return 'QCD_EMEnriched'
        return 'QCD'
    elif 'DYJetsToLL' in sample_name:
        return 'DYJets'
    elif 'WJets' in sample_name:
        return 'WJets'
    elif 'Single_Top' in sample_name:
        return 'SingleTop'
    else:
        return sample_name

def create_stack_histogram(samples_dict, branch_name="top_mass", n_bins=30, x_low=0, x_high=600):
    """
    Create a stacked histogram from multiple ROOT files with consistent colors for sample groups
    and normalize based on GEN weights.
    
    Parameters:
    samples_dict (dict): Dictionary with sample names as keys and file paths as values
    branch_name (str): Name of the branch to plot
    n_bins (int): Number of bins in histogram
    x_low (float): Lower edge of histogram
    x_high (float): Upper edge of histogram
    """
    ROOT.gROOT.SetBatch(True)
    
    c1 = ROOT.TCanvas("c1", "Stacked Histogram", 800, 600)
    ROOT.gStyle.SetOptStat(0)
    
    hs = ROOT.THStack("hs", f"{branch_name} Distribution")
    
    files = []
    hists = []
    
    # Define colors for different sample groups
    group_colors = {
        'QCD_MuEnriched': ROOT.kRed,
        'QCD_EMEnriched': ROOT.kBlue,
        'DYJets': ROOT.kGreen+2,
        'WJets': ROOT.kMagenta,
        'SingleTop': ROOT.kCyan+2
    }
    
    # Create legend with more space for many entries
    legend = ROOT.TLegend(0.65, 0.45, 0.89, 0.89)
    legend.SetBorderSize(0)
    legend.SetFillStyle(0)
    
    # Group samples by their base name
    grouped_samples = {}
    for sample_name, sample_info in samples_dict.items():
        group = get_sample_group(sample_name)
        if group not in grouped_samples:
            grouped_samples[group] = []
        grouped_samples[group].append((sample_name, sample_info))
    
    # Process each group
    for group, samples in grouped_samples.items():
        group_color = group_colors.get(group, ROOT.kBlack)  # Default to black if group not in colors

        print(f"\nProcessing group {group}:")
        for sample_name, sample_info in samples:
            file_path = sample_info['file_path']
            gen_weight_sum = sample_info['gen_weight_sum']
            
            if not os.path.exists(file_path):
                print(f"Warning: File {file_path} does not exist. Skipping...")
                continue

            f = ROOT.TFile(file_path)
            files.append(f)

            tree = f.Get("outputTree")
            if not tree:
                print(f"Warning: Could not find 'outputTree' in {file_path}. Skipping...")
                continue

            print_branch_info(tree, branch_name)

            hist = ROOT.TH1F(f"h_{sample_name}", sample_name, n_bins, x_low, x_high)
            hists.append(hist)

            entries_filled = 0
            n_filtered_events = 0  # Count the number of events that pass the cuts
            for entry in range(tree.GetEntries()):
                tree.GetEntry(entry)
                if hasattr(tree, branch_name):
                    value = getattr(tree, branch_name)

                    # Apply cuts
                    if apply_cuts(tree):  # Your cut logic here
                        hist.Fill(value)
                        entries_filled += 1
                        n_filtered_events += 1

            print(f"{sample_name} entries filled: {entries_filled}")
            print(f"{sample_name} integral: {hist.Integral()}")

            if hist.Integral() > 0 and gen_weight_sum > 0 and n_filtered_events > 0:
                normalization_factor = n_filtered_events / gen_weight_sum
                hist.Scale(normalization_factor)  # Normalize by GEN weight
                hist.SetFillColor(group_color)
                hist.SetLineColor(group_color)
                hist.SetFillStyle(3004 + samples.index((sample_name, sample_info)) % 4)
                hs.Add(hist)
                legend.AddEntry(hist, sample_name, "f")
    
    if hs.GetNhists() > 0:
        c1.cd()
        hs.Draw("hist")

        hs.GetXaxis().SetTitle(f"{branch_name} [GeV]")
        hs.GetYaxis().SetTitle("Events")

        legend.Draw()

        cms_text = ROOT.TLatex()
        cms_text.SetNDC()
        cms_text.SetTextSize(0.05)
        cms_text.DrawLatex(0.15, 0.92, "CMS")

        c1.Modified()
        c1.Update()
        c1.SaveAs(f"{branch_name}_stack.png")
    else:
        print("No histograms with entries to draw!")
    
    for f in files:
        f.Close()
    c1.Close()

def apply_cuts(tree):
    """
    Placeholder function to apply cuts to events.
    Returns True if the event passes the cuts, False otherwise.
    """
    # Example: Apply cuts, for instance based on a branch value
    if hasattr(tree, "top_mass") and tree.top_mass > 100:  # Example cut on top_mass
        return True
    return False

if __name__ == "__main__":

    samples = {
        "Single_Top_t-channel": {
            "file_path": "/eos/uscms/store/user/vsinha/results/process_ST_t-channel_top_4f.root",
            "gen_weight_sum": 4267923607.324066  # Replace with actual gen weight sum
        },
        "WJetsToLNu_UL17": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_WJetsToLNu_UL17.root",
            "gen_weight_sum": 1030816976.467947  # Replace with actual gen weight sum
        },
        "DYJetsToLL_M-10to50_UL17": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_DYJetsToLL_M-10to50_UL17.root",
            "gen_weight_sum": 68480179.000000  # Replace with actual gen weight sum
        },
        "DYJetsToLL_M-50_UL17": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_DYJetsToLL_M-50_UL17.root",
            "gen_weight_sum": 102863931.000000  # Replace with actual gen weight sum
        },
        "QCD_Pt-120To170_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-120To170_MuEnrichedPt5.root",
            "gen_weight_sum": 39395152.077200  # Replace with actual gen weight sum
        },
        "QCD_Pt-120to170_EMEnriched": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-120to170_EMEnriched.root",
            "gen_weight_sum": 9021334.990524  # Replace with actual gen weight sum
        },
        "QCD_Pt-15To20_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-15To20_MuEnrichedPt5.root",
            "gen_weight_sum": 9021334.990524  # Replace with actual gen weight sum
        },
        "QCD_Pt-15to20_EMEnriched": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-15to20_EMEnriched.root",
            "gen_weight_sum": 7967858.631735  # Replace with actual gen weight sum
        },
        "QCD_Pt-170To300_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-170To300_MuEnrichedPt5.root",
            "gen_weight_sum": 73071987.000000  # Replace with actual gen weight sum
        },
        "QCD_Pt-20To30_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-20To30_MuEnrichedPt5.root",
            "gen_weight_sum": 64321788.990676  # Replace with actual gen weight sum
        },
        "QCD_Pt-20to30_EMEnriched": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-20to30_EMEnriched.root",
            "gen_weight_sum": 14166154.329150  # Replace with actual gen weight sum
        },
        "QCD_Pt-300To470_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-300To470_MuEnrichedPt5.root",
            "gen_weight_sum": 58692920.280472  # Replace with actual gen weight sum
        },
        "QCD_Pt-30To50_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-30To50_MuEnrichedPt5.root",
            "gen_weight_sum": 54929916.000000  # Replace with actual gen weight sum
        },
        "QCD_Pt-30to50_EMEnriched": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-30to50_EMEnriched.root",
            "gen_weight_sum": 8784542.000000  # Replace with actual gen weight sum
        },
        "QCD_Pt-470To600_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-470To600_MuEnrichedPt5.root",
            "gen_weight_sum": 39491905.054731  # Replace with actual gen weight sum
        },
        "QCD_Pt-50To80_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-50To80_MuEnrichedPt5.root",
            "gen_weight_sum": 38616342.000000  # Replace with actual gen weight sum
        },
        "QCD_Pt-50to80_EMEnriched": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-50to80_EMEnriched.root",
            "gen_weight_sum": 10210400.000000  # Replace with actual gen weight sum
        },
        "QCD_Pt-600To800_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-600To800_MuEnrichedPt5.root",
            "gen_weight_sum": 38668707.131553  # Replace with actual gen weight sum
        },
        "QCD_Pt-70to300_EMEnriched": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-70to300_EMEnriched.root",
            "gen_weight_sum": 3678200.000000  # Replace with actual gen weight sum
        },
        "QCD_Pt-80To120_MuEnrichedPt5": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-80To120_MuEnrichedPt5.root",
            "gen_weight_sum": 44225579.154016  # Replace with actual gen weight sum
        },
        "QCD_Pt-80to120_EMEnriched": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_QCD_Pt-80to120_EMEnriched.root",
            "gen_weight_sum": 9617412.941175  # Replace with actual gen weight sum
        },
        "ST_tW-channel_top_UL17": {
            "file_path": "/eos/uscms/store/user/vsinha/results/PROC_ST_tW-channel_top_UL17.root",
            "gen_weight_sum": 65117896.000000  # Replace with actual gen weight sum
        },
        "Single_Top_t-channel_antitop": {
            "file_path": "/eos/uscms/store/user/vsinha/results/process_ST_t-channel_antitop_4f.root",
            "gen_weight_sum": 4462868882.059891  # Replace with actual gen weight sum
        }
    }

    # Create histogram with default parameters
    create_stack_histogram(samples)

    # Example with custom parameters
    # create_stack_histogram(samples, branch_name="some_other_branch", n_bins=50, x_low=100, x_high=800)

