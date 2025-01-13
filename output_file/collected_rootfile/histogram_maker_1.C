#include <vector> // Include for std::vector
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TString.h>
#include <iostream> // For error messages

void histogram_maker_1() {
    // List of input files (add more files as needed)
    std::vector<TString> file_names = {
        "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/DrellYan.root",
        "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/top_tzq_signal.root",
        "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/diboson.root",
        "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/singletop.root",
        "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/Triboson.root",
        "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/TTGamma_Dilept.root",
        "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/ditop.root"
    };

    // Output file
    TFile *outfile = TFile::Open("Final_trailingElectrons_eta.root", "RECREATE");
    if (!outfile || outfile->IsZombie()) {
        std::cerr << "Error: Could not open output file." << std::endl;
        return;
    }

    // Define histograms
    TH1F *DY_jets = new TH1F("Drellyan", "", 25, -3, 3);
    TH1F *Data = new TH1F("top_tzq_signal", "", 25, -3, 3);
    TH1F *VV = new TH1F("diboson", "", 25, -3, 3);
    TH1F *single_top = new TH1F("singletop", "", 25, -3, 3);
    TH1F *VVV = new TH1F("Triboson", "", 25, -3, 3);
    TH1F *TTGamma = new TH1F("TTGamma_Dilept", "", 25, -3, 3);
    TH1F *TT_bar_background = new TH1F("tt", "", 25, -3, 3);

    // Collect histograms into a vector (for better handling if needed)
    std::vector<TH1F*> histograms = {DY_jets, Data, VV, single_top, VVV, TTGamma, TT_bar_background};

    // Loop over input files
    int kk = 0;
    for (const auto& file_name : file_names) {
        kk++;
        TFile *file = TFile::Open(file_name);
        if (!file || file->IsZombie()) {
            std::cerr << "Error: Could not open file " << file_name << std::endl;
            continue;
        }

        // Get the tree
        TTree *tree = (TTree*)file->Get("outputTree");
        if (!tree) {
            std::cerr << "Error: Tree not found in file " << file_name << std::endl;
            file->Close();
            continue;
        }

        // Declare a pointer to the branch variable
        std::vector<float> *trailingElectronss_eta = nullptr;
        TBranch *b_trailingElectronss_eta = nullptr;
        tree->SetBranchAddress("trailingElectrons_eta", &trailingElectronss_eta, &b_trailingElectronss_eta);

        // Loop over entries in the tree
        Long64_t nEntries = tree->GetEntries();
        for (Long64_t i = 0; i < nEntries; ++i) {
            tree->GetEntry(i);
            double weight = 1.0; // Adjust weight as needed

            // Fill histograms with each value in the vector
            for (const auto& pt : *trailingElectronss_eta) {
                if (kk == 1) {
                    DY_jets->Fill(pt, weight);
                } else if (kk == 2) {
                    Data->Fill(pt, weight);
                } else if (kk == 3) {
                    VV->Fill(pt, weight);
                } else if (kk == 4) {
                    single_top->Fill(pt, weight);
                } else if (kk == 5) {
                    VVV->Fill(pt, weight);
                } else if (kk == 6) {
                    TTGamma->Fill(pt, weight);
                } else if (kk == 7) {
                    TT_bar_background->Fill(pt, weight);
                }
            }
        }

        // Close the input file
        file->Close();
    }

    // Write histograms to the output file
    outfile->Write();

    // Close the output file
    outfile->Close();
    std::cout << "Histograms written to Final_hist_trailingElectronss_eta.root" << std::endl;
}

