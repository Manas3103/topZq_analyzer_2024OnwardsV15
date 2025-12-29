#include <iostream>
#include <string>
#include <map>
#include <tuple>
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TH1F.h>
#include <TLeaf.h>
#include <vector>

void create_normalized_histogram(const std::string& filename,

                       double cross_section,
                      // double luminosity=2.9700 ,
		       //double luminosity=5.0104 ,
		       double luminosity=7.9804 ,

                       const std::string& Tree="outputTree") {
    // ROOT file and tree details
    std::string inputFileName = filename;
    std::string treeName = Tree;

    // Open the input ROOT file
    TFile* inputFile = TFile::Open(inputFileName.c_str(), "READ");
    if (!inputFile || inputFile->IsZombie()) {
        std::cerr << "Error: Unable to open the ROOT file: " << inputFileName << std::endl;
        return;
    }

    // Get the tree
    TTree* tree = static_cast<TTree*>(inputFile->Get(treeName.c_str()));
    if (!tree) {
        std::cerr << "Error: Unable to find the tree: " << treeName << std::endl;
        inputFile->Close();
        return;
    }
    std::cout << "Tree is found" << std::endl;
/*
    // Branch ranges definition
    std::map<std::string, std::tuple<double, double, int>> branch_ranges = {
        {"Bdt_discriminant", {0, 1,10}}
};*/


    // Define histogram parameters: {branch_name, (min, max, bins)}
    std::map<std::string, std::tuple<double, double, int>> branch_ranges = {
        {"top_mass", {5, 700,50}},
        {"nJet", {2, 10,8}},
        {"TR_leadingLepton_pt", {0, 250,25}},
        {"TR_subleadingLepton_pt", {0, 150,20}},
        {"TR_trailingLepton_pt", {0, 100,20}},
        {"TR_leadingLepton_eta", {-3, 3,24}},
        {"TR_subleadingLepton_eta", {-3, 3,24}},
        {"TR_trailingLepton_eta", {-3, 3,24}},
        {"combinedLeptonPt", {0, 500, 50}},
        {"ThreeLSignal_leadingLepton_pt", {0, 300, 10}},
        {"ThreeLSignalRegion_leadingJet_pt", {0, 400, 10}},
        {"ThreeLSignalRegion_Jet_HT", {0, 500, 10}},
        {"ThreeLSignalRegion_nElectron", {0, 6, 6}},
        {"ThreeLSignalRegion_nMuon", {0, 6, 6}},
        {"zboson_mass_3LRegion", {70, 110, 20}}


    };

/*    // Branch ranges definition
    std::map<std::string, std::tuple<double, double, int>> branch_ranges = {
	// Original variables kept
	{"top_mass", {5, 700, 50}},
	{"leadingLepton_pt", {0, 250, 25}},
        {"subleadingLepton_pt", {0, 150, 20}},
        {"trailingLepton_pt", {0, 100, 20}},
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


    };*/

    // Calculate normalization factor
    double normalization_factor = luminosity * cross_section ;
    std::cout << "Normalization Factor: " << normalization_factor << std::endl;

    // Event weight
    Float_t evWeight = 1.0;
    Float_t genWeight = 1.0;
    Double_t genEventSumw =1.0;
    if (tree->GetBranch("evWeight")) {
        tree->SetBranchAddress("evWeight", &evWeight);
    } else {
        std::cerr << "Warning: evWeight branch not found. Using default weight of 1.0" << std::endl;
    }

    // Check for genWeight branch
    if (tree->GetBranch("genWeight")) {
        tree->SetBranchAddress("genWeight", &genWeight);
    } else {
        std::cerr << "Warning: genWeight branch not found. Using default weight of 1.0" << std::endl;
    }

    // Check for sum of genWeight branch
    if (tree->GetBranch("genEventSumw")) {
        tree->SetBranchAddress("genEventSumw", &genEventSumw);
    } else {
        std::cerr << "Warning: genEventSumw branch not found. Using default weight of 1.0" << std::endl;
    }


    // Output filename
//    std::string outputFileName = filename;
    size_t lastSlash = filename.find_last_of("/\\");
    std::string outputFileName = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;

    size_t rootPos = outputFileName.rfind(".root");
    if (rootPos != std::string::npos) {
        outputFileName.replace(rootPos, 5, "_hist_new.root");
    } else {
        std::cerr << "Error: Input file does not have .root extension." << std::endl;
        return;
    }

    // Create output ROOT file
    TFile* outputFile = TFile::Open(outputFileName.c_str(), "RECREATE");
    if (!outputFile || outputFile->IsZombie()) {
        std::cerr << "Error: Unable to create output ROOT file." << std::endl;
        inputFile->Close();
        return;
    }
    std::cout << "Output ROOT file created" << std::endl;

    // Buffers for different data types
    Float_t float_buffer = 0.0;
    Int_t int_buffer = 0;
    Double_t double_buffer = 0.0;
    UInt_t uint_buffer = 0;
    std::vector<float>* vector_float_buffer = nullptr;

    // Loop over the branches and create histograms
    int iterationCount = 0;
    for (const auto& [branchName, range] : branch_ranges) {
        double minRange = std::get<0>(range);
        double maxRange = std::get<1>(range);
        int bins = std::get<2>(range);

        // Create histogram
        TH1D* hist = new TH1D(branchName.c_str(), branchName.c_str(), bins, minRange, maxRange);
        hist->Sumw2(); // Enable proper error calculation

        // Check if the branch exists in the tree
        TBranch* branch = tree->GetBranch(branchName.c_str());
        if (!branch) {
            std::cerr << "Warning: Branch " << branchName << " not found in the tree." << std::endl;
            delete hist;
            continue;
        }

        // Get branch type
        TLeaf* leaf = branch->GetLeaf(branchName.c_str());
        if (!leaf) {
            std::cerr << "Warning: Leaf not found for branch: " << branchName << std::endl;
            delete hist;
            continue;
        }

        const char* typeName = leaf->GetTypeName();
        std::string typeNameStr(typeName);
        bool isVector = typeNameStr.find("vector") != std::string::npos;

        try {
            // Set the appropriate branch address based on type
            if (strcmp(typeName, "Float_t") == 0) {
                tree->SetBranchAddress(branchName.c_str(), &float_buffer);
            } else if (strcmp(typeName, "Int_t") == 0) {
                tree->SetBranchAddress(branchName.c_str(), &int_buffer);
            } else if (strcmp(typeName, "Double_t") == 0) {
                tree->SetBranchAddress(branchName.c_str(), &double_buffer);
            } else if (isVector && typeNameStr.find("float") != std::string::npos) {
                tree->SetBranchAddress(branchName.c_str(), &vector_float_buffer);
	    } else if (strcmp(typeName, "UInt_t") == 0) {
                tree->SetBranchAddress(branchName.c_str(), &uint_buffer); // Define `UInt_t uint_buffer;` somewhere
            } else {
                std::cout << "Info: Skipping branch " << branchName << " with type " << typeName << std::endl;
                delete hist;
                continue;
            }

            // Fill histogram
            Long64_t nEntries = tree->GetEntries();
            for (Long64_t i = 0; i < nEntries; ++i) {
                tree->GetEntry(i);

                // Calculate total weight with normalization
                //double total_weight = evWeight * genWeight * normalization_factor; use this when evWeight is properly defind
                double total_weight = (evWeight/genEventSumw) * normalization_factor;

                if (strcmp(typeName, "Float_t") == 0) {
                    hist->Fill(float_buffer, total_weight);
                } else if (strcmp(typeName, "Int_t") == 0) {
                    hist->Fill(static_cast<float>(int_buffer), total_weight);
                } else if (strcmp(typeName, "Double_t") == 0) {
                    hist->Fill(static_cast<float>(double_buffer), total_weight);
		} else if (strcmp(typeName, "UInt_t") == 0) {
                    hist->Fill(static_cast<float>(uint_buffer), total_weight);
                } else if (isVector && vector_float_buffer != nullptr) {
                    // Fill histogram with each element of the vector
                    for (const auto& value : *vector_float_buffer) {
                        hist->Fill(value, total_weight);
                    }
                }
            }

            std::cout << "Histogram for " << branchName << " is filled" << std::endl;

            // Write histogram to the output file
            outputFile->cd();
            hist->Write();

            iterationCount++;
        }
        catch (const std::exception& e) {
            std::cerr << "Error processing branch " << branchName << ": " << e.what() << std::endl;
        }

        delete hist; // Cleanup
        tree->ResetBranchAddress(branch);

        // Clean up vector buffer if it was used
        if (isVector) {
            vector_float_buffer = nullptr;
        }
    }

    // Write and close files
    outputFile->Write();  // Write all histograms
    outputFile->Close();
    delete outputFile;

    std::cout << "Histograms created and saved to " << outputFileName << std::endl;
}
