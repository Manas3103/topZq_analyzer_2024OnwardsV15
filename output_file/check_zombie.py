import ROOT
import os

def check_root_files(directory):
    """
    Check all ROOT files in the given directory for validity and identify zombie files.

    Parameters:
    directory (str): Path to the directory containing .root files.

    Returns:
    list: A list of zombie files.
    """
    zombie_files = []

    # List all ROOT files in the directory
    root_files = [f for f in os.listdir(directory) if f.endswith('.root')]

    # Check each ROOT file
    for root_file in root_files:
        file_path = os.path.join(directory, root_file)
        try:
            # Open the ROOT file
            f = ROOT.TFile.Open(file_path)
            if not f or f.IsZombie():
                print(f"Zombie file detected: {root_file}")
                zombie_files.append(root_file)
            else:
                print(f"File is valid: {root_file}")
            # Close the ROOT file
            f.Close()
        except Exception as e:
            print(f"Error while checking file {root_file}: {e}")
            zombie_files.append(root_file)

    return zombie_files


if __name__ == "__main__":
    # Current directory with ROOT files
    directory = "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file"
    zombie_files = check_root_files(directory)
    
    if zombie_files:
        print("\nZombie files detected:")
        for zfile in zombie_files:
            print(zfile)
    else:
        print("\nAll files are valid.")

