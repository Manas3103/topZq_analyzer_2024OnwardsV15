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
"""
def get_sample_group(sample_name):
    
    Extract the base group name from the sample name
    e.g., 'QCD_Pt-80To120_MuEnrichedPt5' -> 'QCD_MuEnriched'
         'QCD_Pt-80to120_EMEnriched' -> 'QCD_EMEnriched'
         'DYJetsToLL_M-50_UL17' -> 'DYJets'
    :wq

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
"""
def get_sample_group(sample_name):
    """
    Categorize the sample into one of the predefined groups:
    DY, VVV, XY, W+Jets, ttbarX, SingleTop, VV, or Other (individual categories for some).
    """
    if 'top_tzq_signal' in sample_name:
        return 'top_signal'
    elif 'WWW' in sample_name or 'WZZ' in sample_name or 'ZZZ' in sample_name:
        return 'VVV'
    elif 'ZGToLLG' in sample_name:
        return 'XY'
    elif 'WJetsToLNu' in sample_name:
        return 'W+Jets'
    elif 'ST_t-channel' in sample_name or 'ST_tW' in sample_name or 'ST_tW_antitop_5f_inclusiveDecays' in sample_name:
        return 'SingleTop'
    elif 'WWTo2L2Nu' in sample_name or 'WZTo2L2Q' in sample_name or 'ZZTo2L2Nu' in sample_name or 'ZZTo2L2Q' in sample_name:
        return 'VV'
    elif 'DY' in sample_name:
        return 'DY'
    elif 'TT' in sample_name:
        return 'ttbarX'  # Merge all TT samples into one category
    else:
        return 'sample_name'


"""
def create_stack_histogram(samples_dict, branch_name="top_mass", n_bins=30, x_low=0, x_high=600):
    ROOT.gROOT.SetBatch(True)
    
    c1 = ROOT.TCanvas("c1", "Stacked Histogram", 800, 600)
    ROOT.gStyle.SetOptStat(0)
    
    hs = ROOT.THStack("hs", f"{branch_name} Distribution")
    
    files = []
    hists = []
    
    # Define colors for different sample groups
    group_colors = {
    'DYJets': ROOT.kGreen,              # Solid Green for DYJets
    'WJets': ROOT.kMagenta,             # Solid Magenta for WJets
    'SingleTop': ROOT.kCyan,            # Solid Cyan for SingleTop
    'QCD_MuEnriched': ROOT.kRed,        # Solid Red for QCD_MuEnriched
    'QCD_EMEnriched': ROOT.kBlue,       # Solid Blue for QCD_EMEnriched
    'VVV': ROOT.kOrange,                # Solid Orange for VVV
    'Xy': ROOT.kPink,                   # Solid Pink for Xy
    'ttbarX': ROOT.kViolet,             # Solid Violet for ttbarX
    'VV': ROOT.kAzure,                  # Solid Azure for VV (WW, WZ, ZZ)
    'Others': ROOT.kGray                # Solid Gray for Others (TTToSemiLeptonic, TTZToLLNuNu, etc.)
    }

    
    # Create legend with more space for many entries
    legend = ROOT.TLegend(0.65, 0.45, 0.89, 0.89)
    #legend.SetBorderSize(0)
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
               # hist.SetLineColor(group_color)
                #hist.SetFillStyle(3004 + samples.index((sample_name, sample_info)) % 4)
                hs.Add(hist)
                legend.AddEntry(hist, sample_name, "f")
    
    if hs.GetNhists() > 0:
        c1.cd()
        hs.Draw("hist")

        hs.GetXaxis().SetTitle(f"{branch_name} [GeV]")
        hs.GetYaxis().SetTitle("Events")

        legend.Draw()
        c1.Modified()
        c1.Update()
        c1.SaveAs(f"{branch_name}_stack.png")
    else:
        print("No histograms with entries to draw!")
    
    for f in files:
        f.Close()
    c1.Close()
"""
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
        'DYJets': ROOT.kGreen,              
        'WJets': ROOT.kMagenta,             
        'SingleTop': ROOT.kCyan,            
        'QCD_MuEnriched': ROOT.kRed,        
        'QCD_EMEnriched': ROOT.kBlue,       
        'VVV': ROOT.kOrange,                
        'Xy': ROOT.kPink,                   
        'ttbarX': ROOT.kViolet,             
        'VV': ROOT.kAzure,                  
        'Others': ROOT.kGray                
    }

    # Create legend with more space for many entries
    legend = ROOT.TLegend(0.65, 0.45, 0.89, 0.89)
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
                # hist.SetLineColor(group_color)
                #hist.SetFillStyle(3004 + samples.index((sample_name, sample_info)) % 4)

                # Now append the hist and its integral as a tuple
                hists.append((hist, hist.Integral()))  # Store hist and its integral for sorting
                legend.AddEntry(hist, sample_name, "f")

    # Sort histograms by integral in descending order (smaller ones come to front)
    hists.sort(key=lambda x: x[1], reverse=True)

    # Add histograms to the stack in sorted order
    for hist, _ in hists:
        hs.Add(hist)

    if hs.GetNhists() > 0:
        c1.cd()
        hs.Draw("hist")

        hs.GetXaxis().SetTitle(f"{branch_name} [GeV]")
        hs.GetYaxis().SetTitle("Events")

        legend.Draw()
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



    # Create histogram with default parameters
    create_stack_histogram(samples)

    # Example with custom parameters
    # create_stack_histogram(samples, branch_name="some_other_branch", n_bins=50, x_low=100, x_high=800)

