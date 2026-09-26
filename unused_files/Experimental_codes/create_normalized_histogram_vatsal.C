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
                       double sum_gen_weight,
                       double luminosity=41.480 ,

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

    // Branch ranges definition
    std::map<std::string, std::tuple<double, double, int>> branch_ranges = {
        {"top_mass", {5, 700,50}},
	{"nJet", {2, 10,8}},
        {"TR_leadingLepton_pt", {0, 250,25}},
        {"TR_subleadingLepton_pt", {0, 250,25}},
        {"TR_trailingLepton_pt", {0, 250,25}},
        {"TR_leadingLepton_eta", {-3, 3,24}},
        {"TR_subleadingLepton_eta", {-3, 3,24}},
        {"TR_trailingLepton_eta", {-3, 3,24}},
	{"nElectron", {0, 8, 8}},
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

};

    // Calculate normalization factor
    double normalization_factor = luminosity * cross_section / sum_gen_weight;
    std::cout << "Normalization Factor: " << normalization_factor << std::endl;

    // Event weight
    Float_t evWeight = 1.0;
    Float_t genWeight = 1.0;
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

    // Output filename
//    std::string outputFileName = filename;
    size_t lastSlash = filename.find_last_of("/\\");
    std::string outputFileName = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;

    size_t rootPos = outputFileName.rfind(".root");
    if (rootPos != std::string::npos) {
        outputFileName.replace(rootPos, 5, "_hist.root");
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
                double total_weight = evWeight * normalization_factor;

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
