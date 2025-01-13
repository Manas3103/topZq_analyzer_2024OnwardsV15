import ROOT

file_path = "./DYJetsToLL_100to200_top_mass_distribution.root"  # Replace with any file path

root_file = ROOT.TFile.Open(file_path, "READ")
if not root_file or root_file.IsZombie():
    print(f"Failed to open {file_path}")
else:
    print(f"Contents of {file_path}:")
    root_file.ls()  # List all objects in the file
root_file.Close()

