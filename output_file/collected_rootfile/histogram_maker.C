#include <vector>
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TString.h>

void histogram_maker() {
    // List of input files (add more files as needed)
    std::vector<TString> file_names = {"/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/DrellYan.root",
	    "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/top_tzq_signal.root",
	    "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/diboson.root",
	    "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/singletop.root",
            "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/Triboson.root"	,    
	    "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/TTGamma_Dilept.root",
	    "/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file/collected_rootfile/ditop.root"};
   
    // Output file
    TFile *outfile = TFile::Open("Final_hist_topquark_eta.root", "RECREATE");
    if (!outfile || outfile->IsZombie()) {
        std::cerr << "Error: Could not open output file." << std::endl;
        return;
    }

    // Define histograms
    TH1F *DY_jets = new TH1F("Drellyan", "", 25, -6, 6);
    TH1F *Data = new TH1F("top_tzq_signal", "", 25,-6 , 6);
    TH1F *VV = new TH1F("diboson", "", 25, -6, 6);
    TH1F *single_top = new TH1F("singletop", "", 25, -6, 6);
    TH1F *VVV = new TH1F("Triboson", "", 25, -6, 6);
    TH1F *TTGamma = new TH1F("TTGamma_Dilept", "", 25, -6, 6);
    TH1F *TT_bar_background = new TH1F("ditop", "", 25, -6, 6);
   
    // Create a map for histogram types if needed later
    std::vector<TH1F*> histograms = {DY_jets,Data,VV,single_top,VVV,
                                     TTGamma, TT_bar_background};
   
    // Loop over input files
     int kk=0;
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
            continue;
        }

Double_t top_eta;
        TBranch *b_top_eta;
        tree->SetBranchAddress("top_eta", &top_eta, &b_top_eta);

        // Loop over entries in the tree
        Long64_t nEntries = tree->GetEntries();
        for (Long64_t i = 0; i < nEntries; ++i) {
            tree->GetEntry(i);
            double weight = 1.0;  // Adjust weight as needed

            // Fill histograms
             if (kk ==1) {
            DY_jets->Fill(top_eta, weight);}
if (kk ==2) {
            Data->Fill(top_eta, weight);}  
if (kk ==3) {
            VV->Fill(top_eta, weight);}  
if (kk ==4) {
            single_top->Fill(top_eta, weight);}  
if (kk ==5) {
            VVV->Fill(top_eta, weight);}  
if (kk ==6) {
            TTGamma->Fill(top_eta, weight);}  
if (kk ==7) {
            TT_bar_background->Fill(top_eta, weight);}  
        }

file->Close();
    }

    // Write histograms to the output file
    outfile->Write();

    // Close the output file
    outfile->Close();
    std::cout << "Histograms written to Final_histograms.root" << std::endl;
}
