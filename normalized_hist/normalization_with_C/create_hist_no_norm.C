#include <iostream>
#include <map>
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>

// Function to create histograms without normalization
void create_hist_no_norm(const std::string& filename, const std::string& treeName = "outputTree") {
    // Open input ROOT file
    TFile* inputFile = TFile::Open(filename.c_str(), "READ");
    if (!inputFile || inputFile->IsZombie()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return;
    }

    // Get tree
    TTree* tree = static_cast<TTree*>(inputFile->Get(treeName.c_str()));
    if (!tree) {
        std::cerr << "Error: Tree " << treeName << " not found in file." << std::endl;
        inputFile->Close();
        return;
    }

    std::cout << "Tree loaded successfully.\n";
/*
    // Define histogram parameters: {branch_name, (min, max, bins)}
    std::map<std::string, std::tuple<double, double, int>> histParams = {
        {"top_mass", {5, 700,50}},
        {"nJet", {2, 10,8}},
	{"TR_leadingLepton_pt", {0, 250,25}},
	{"TR_subleadingLepton_pt", {0, 150,20}},
	{"TR_trailingLepton_pt", {0, 100,20}},
        {"TR_leadingLepton_eta", {-3, 3,24}},
        {"TR_subleadingLepton_eta", {-3, 3,24}},
        {"TR_trailingLepton_eta", {-3, 3,24}},
        {"nElectron", {0, 8,8}},
        {"nMuon", {0, 6,6}},
        {"Wboson_transversMass", {5, 300, 35}},
        {"OSSF_ZPair_mass", {70, 130, 35}},
        {"mass_of_3lepton", {5, 500, 50}},
        {"baselineElectrons_pt", {0, 600, 50}},
        {"baselineMuons_pt", {0, 600, 50}},
        {"NbaselineElectrons", {0, 5, 5}},
        {"NbaselineMuons", {0, 5, 5}},
        {"NgoodJets", {2, 10, 8}},
        {"goodJets_pt", {0, 1000, 70}},
        {"good_bJetpt", {0, 1000, 70}},
        {"totalLeptonCount", {0, 5, 5}},
        {"combinedLeptonPt", {0, 500, 50}}


    };*/

    // Branch ranges definition
    std::map<std::string, std::tuple<double, double, int>> histParams = {
	// Original variables kept
	{"top_mass", {5, 700, 50}},
	{"TR_leadingLepton_pt", {0, 250, 25}},
	{"TR_subleadingLepton_pt", {0, 150, 20}},
	{"TR_trailingLepton_pt", {0, 100, 20}},
	{"TR_leadingLepton_eta", {-3, 3, 24}},
	{"TR_subleadingLepton_eta", {-3, 3, 24}},
	{"TR_trailingLepton_eta", {-3, 3, 24}},
	{"Wboson_transversMass", {5, 300, 35}},
	{"OSSF_ZPair_mass", {70, 130, 35}},

	// 3-lepton regions
	{"ncleanjetspass_SignalRegion", {-0.5, 7.5, 7}},
	{"ncleanbjetspass_SignalRegion", {-0.5, 5.5, 5}},
	
	{"ncleanjetspass_WZ_Region", {-0.5, 7.5, 7}},
	{"Wboson_transversMass_WZ_Region", {5, 300, 35}},
	
	{"ncleanjetspass_X_gamma_Region", {-0.5, 7.5, 7}},
	{"ncleanbjetspass_X_gamma_Region", {-0.5, 5.5, 5}},
	
	{"ncleanjetspass_NP_2_Region", {-0.5, 7.5, 7}},
	{"ncleanbjetspass_NP_2_Region", {-0.5, 5.5, 5}},
	
	{"ncleanjetspass_NP_1_Region", {-0.5, 7.5, 7}},
	{"ncleanbjetspass_NP_1_Region", {-0.5, 5.5, 5}},

	// 4-lepton regions
	{"ncleanjetspass_ZZ_Region", {-0.5, 7.5, 7}},
	{"mass_of_4L_ZZ_Region", {0, 600, 60}},
	{"Z_mass1_ZZ_Region", {70, 130, 35}},
	{"Z_mass2_ZZ_Region", {70, 130, 35}},
	
	{"ncleanjetspass_ttZ_Region", {-0.5, 7.5, 7}},
	{"ncleanbjetspass_ttZ_Region", {-0.5, 5.5, 5}},


       // tzq region variables
        {"nJet_tzq", {0, 6, 7}},
        {"nBJets_tzq", {0, 5, 6}},
        {"mWT_tzq", {0, 250, 25}},
        {"mTop_tzq", {0, 400, 20}},
        {"mZ_tzq", {70, 110, 20}},
        {"dphi_ll_z_tzq", {0, 3, 12}},
        {"cosThetaPol_tzq", {-1, 1, 20}},
        {"sumHadPt_tzq", {0, 900, 30}},
        {"sumLepMetPt_tzq", {0, 700, 25}},
        {"min_dR_bl_tzq", {0, 5, 20}},
        {"max_dR_bl_tzq", {0, 7, 20}},  // Updated from {0,5,20} to {0,7,20}
        {"max_dphi_jj_tzq", {0, 4, 15}},
        {"max_ptjj_tzq", {0, 500, 20}},
        {"max_mjj_tzq", {0, 1200, 30}},
        {"mass3l_tzq", {50, 500, 20}},
        {"dR_b_recoil_tzq", {0, 8, 16}},
        {"dR_b_l_tzq", {0, 6, 12}},
        {"maxJetAbsEta_tzq", {-5, 5, 20}},  // Updated from {0,5,20} to {-5,5,20}
        {"etaRecoilingJet_tzq", {-5, 5, 20}},  // Updated from {-3,3,20} to {-5,5,20}
        {"lep_asymmetry_tzq", {-3, 3, 20}},  // Updated from {0,1,20} to {-3,3,20}
        {"maxDEEPJET_tzq", {0, 1, 20}},
        {"MET_pt_tzq", {0, 270, 27}},

        // ttz region variables
        {"nJet_ttz", {0, 6, 7}},
        {"nBJets_ttz", {0, 5, 6}},
        {"mWT_ttz", {0, 250, 25}},
        {"mTop_ttz", {0, 400, 20}},
        {"mZ_ttz", {70, 110, 20}},
        {"dphi_ll_z_ttz", {0, 3, 12}},
        {"cosThetaPol_ttz", {-1, 1, 20}},
        {"sumHadPt_ttz", {0, 900, 30}},
        {"sumLepMetPt_ttz", {0, 700, 25}},
        {"min_dR_bl_ttz", {0, 5, 20}},
        {"max_dR_bl_ttz", {0, 7, 20}},  // Updated
        {"max_dphi_jj_ttz", {0, 4, 15}},
        {"max_ptjj_ttz", {0, 500, 20}},
        {"max_mjj_ttz", {0, 1200, 30}},
        {"mass3l_ttz", {50, 500, 20}},
        {"dR_b_recoil_ttz", {0, 8, 16}},
        {"dR_b_l_ttz", {0, 6, 12}},
        {"maxJetAbsEta_ttz", {-5, 5, 20}},  // Updated
        {"etaRecoilingJet_ttz", {-5, 5, 20}},  // Updated
        {"lep_asymmetry_ttz", {-3, 3, 20}},  // Updated
        {"maxDEEPJET_ttz", {0, 1, 20}},
        {"MET_pt_ttz", {0, 270, 27}},

        // trial region variables
        {"nJet_trial", {0, 6, 7}},
        {"nBJets_trial", {0, 5, 6}},
        {"mWT_trial", {0, 250, 25}},
        {"mTop_trial", {0, 400, 20}},
        {"mZ_trial", {70, 110, 20}},
        {"dphi_ll_z_trial", {0, 3, 12}},
        {"cosThetaPol_trial", {-1, 1, 20}},
        {"sumHadPt_trial", {0, 900, 30}},
        {"sumLepMetPt_trial", {0, 700, 25}},
        {"min_dR_bl_trial", {0, 5, 20}},
        {"max_dR_bl_trial", {0, 7, 20}},  // Updated
        {"max_dphi_jj_trial", {0, 4, 15}},
        {"max_ptjj_trial", {0, 500, 20}},
        {"max_mjj_trial", {0, 1200, 30}},
        {"mass3l_trial", {50, 500, 20}},
        {"dR_b_recoil_trial", {0, 8, 16}},
        {"dR_b_l_trial", {0, 6, 12}},
        {"maxJetAbsEta_trial", {-5, 5, 20}},  // Updated
        {"etaRecoilingJet_trial", {-5, 5, 20}},  // Updated
        {"lep_asymmetry_trial", {-3, 3, 20}},  // Updated
        {"maxDEEPJET_trial", {0, 1, 20}},
        {"MET_pt_trial", {0, 270, 27}},

        // signal region variables
        {"nJet_signal", {0, 6, 7}},
        {"nBJets_signal", {0, 5, 6}},
        {"mWT_signal", {0, 250, 25}},
        {"mTop_signal", {0, 400, 20}},
        {"mZ_signal", {70, 110, 20}},
        {"dphi_ll_z_signal", {0, 3, 12}},
        {"cosThetaPol_signal", {-1, 1, 20}},
        {"sumHadPt_signal", {0, 900, 30}},
        {"sumLepMetPt_signal", {0, 700, 25}},
        {"min_dR_bl_signal", {0, 5, 20}},
        {"max_dR_bl_signal", {0, 7, 20}},  // Updated
        {"max_dphi_jj_signal", {0, 4, 15}},
        {"max_ptjj_signal", {0, 500, 20}},
        {"max_mjj_signal", {0, 1200, 30}},
        {"mass3l_signal", {50, 500, 20}},
        {"dR_b_recoil_signal", {0, 8, 16}},
        {"dR_b_l_signal", {0, 6, 12}},
        {"maxJetAbsEta_signal", {-5, 5, 20}},  // Updated
        {"etaRecoilingJet_signal", {-5, 5, 20}},  // Updated
        {"lep_asymmetry_signal", {-3, 3, 20}},  // Updated
        {"maxDEEPJET_signal", {0, 1, 20}},
        {"MET_pt_signal", {0, 270, 27}}


    };


    // Generate output filename
    std::string outputFileName = filename.substr(0, filename.find(".root")) + "_hist_noNorm.root";
    TFile* outputFile = TFile::Open(outputFileName.c_str(), "RECREATE");

    std::cout << "Creating histograms...\n";

    // Loop over branches to create histograms
    std::map<std::string, TH1D*> histograms;
    for (const auto& [branch, params] : histParams) {
        double min = std::get<0>(params);
        double max = std::get<1>(params);
        int bins = std::get<2>(params);

        TH1D* hist = new TH1D(branch.c_str(), branch.c_str(), bins, min, max);
        tree->Draw((branch + ">>" + branch).c_str(), "", "goff"); // Fill histogram
        histograms[branch] = hist;
    }

    // Write histograms to file
    outputFile->cd();
    for (auto& [name, hist] : histograms) {
        hist->Write();
    }

    std::cout << "Histograms saved to: " << outputFileName << "\n";

    // Cleanup
    outputFile->Close();
    inputFile->Close();
}

                              
