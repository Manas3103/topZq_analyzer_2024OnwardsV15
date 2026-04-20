#include <ROOT/RDataFrame.hxx>
#include <TH2D.h>
#include <TFile.h>
#include <TCanvas.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

struct BTagWP {
    float loose, medium, tight;
};

// DeepJet Working Points

// BTagWP get_btag_working_points(const std::string &tag) {
//     if (tag == "2022") return {0.0583, 0.3086, 0.7183};
//     else if (tag == "2022EE") return {0.0614, 0.3196, 0.73};
//     else if (tag == "2023") return {0.0479, 0.2431, 0.6553};
//     else if (tag == "2023BPix") return {0.048, 0.2435, 0.6563};
//     else {
//         std::cerr << "Unknown btag year tag. Defaulting to 2023 values." << std::endl;
//         return {0.0479, 0.2431, 0.6553};
//     }
// }

// ParticleNet Working Points

BTagWP get_btag_working_points(const std::string &tag) {
    if (tag == "2022") return {0.047, 0.245, 0.6734};
    else if (tag == "2022EE") return {0.0499, 0.2605, 0.6915};
    else if (tag == "2023") return {0.0358, 0.1917, 0.6172};
    else if (tag == "2023BPix") return {0.0359, 0.1919, 0.6133};
    else if (tag == "2024") return {0.0246, 0.1272, 0.4648};
    // else if (tag == "2024") return {L,M,T};

    else {
        std::cerr << "Unknown btag year tag. Defaulting to 2023 values." << std::endl;
        return {0.0479, 0.2431, 0.6553};
    }
}


void calculate_btag_efficiencies(std::vector<std::string> input_files, const std::string &output_file, const std::string &year_tag) {
    // Define the pt and eta binning
    std::vector<double> pt_bins = {0, 30, 50, 70, 100, 140, 200, 300, 600, 1000};
    std::vector<double> eta_bins = {0., 0.6, 1.2, 2.4};
    BTagWP wp = get_btag_working_points(year_tag);

    // Initialize histograms for total and b-tagged jets
    TH2D h_all_bcflav("h_all_bcflav", "Total BCFlav Jets", static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data());
    TH2D h_looseID_bcflav("h_looseID_bcflav", "Loose BTagged BCFlav Jets", static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data());
    TH2D h_medID_bcflav("h_medID_bcflav", "Medium BTagged BCFlav Jets", static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data());
    TH2D h_tightID_bcflav("h_tightID_bcflav", "Tight BTagged BCFlav Jets", static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data());

    TH2D h_all_lflav("h_all_lflav", "Total LFlav Jets", static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data());
    TH2D h_looseID_lflav("h_looseID_lflav", "Loose BTagged LFlav Jets", static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data());
    TH2D h_medID_lflav("h_medID_lflav", "Medium BTagged LFlav Jets", static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data());
    TH2D h_tightID_lflav("h_tightID_lflav", "Tight BTagged LFlav Jets", static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data());

    // define tagger Jet_btagPNetB for before 2024 and Jet_btagUParTAK4B for 2024
    std::string tagger = (year_tag == "2024") ? "Jet_btagUParTAK4B" : "Jet_btagPNetB";

     // Loop over each file and process it
    for (const auto &file_path : input_files) {
        std::cout << "Processing file: " << file_path << std::endl;
        
        // Create a DataFrame for each file
        ROOT::RDataFrame df("Events", file_path);

        // Define new columns with lambda functions to extract and process each jet
        auto df_bcflav = df.Define("is_bcflav", "Jet_hadronFlavour == 5 || Jet_hadronFlavour == 4")
                    .Define("bcflav_pts", [&wp](const ROOT::VecOps::RVec<float> &Jet_pt, const ROOT::VecOps::RVec<int> &is_bcflav) {
                        ROOT::VecOps::RVec<float> pts;
                        for (size_t i = 0; i < Jet_pt.size(); ++i) {
                            if (is_bcflav[i]) pts.push_back(Jet_pt[i]);
                        }
                        return pts;
                    }, {"Jet_pt", "is_bcflav"})
                    .Define("bcflav_etas", [&wp](const ROOT::VecOps::RVec<float> &Jet_eta, const ROOT::VecOps::RVec<int> &is_bcflav) {
                        ROOT::VecOps::RVec<float> etas;
                        for (size_t i = 0; i < Jet_eta.size(); ++i) {
                            if (is_bcflav[i]) etas.push_back(std::abs(Jet_eta[i]));
                        }
                        return etas;
                    }, {"Jet_eta", "is_bcflav"})
                    .Define("bcflav_loose_btagged_pts", [&wp](const ROOT::VecOps::RVec<float> &Jet_pt, const ROOT::VecOps::RVec<int> &is_bcflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_pts;
                        for (size_t i = 0; i < Jet_pt.size(); ++i) {
                            if (is_bcflav[i] && tagger[i] > wp.loose) btagged_pts.push_back(Jet_pt[i]);
                        }
                        return btagged_pts;
                    }, {"Jet_pt", "is_bcflav", tagger})
                    .Define("bcflav_loose_btagged_etas", [&wp](const ROOT::VecOps::RVec<float> &Jet_eta, const ROOT::VecOps::RVec<int> &is_bcflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_etas;
                        for (size_t i = 0; i < Jet_eta.size(); ++i) {
                            if (is_bcflav[i] && tagger[i] > wp.loose) btagged_etas.push_back(std::abs(Jet_eta[i]));
                        }
                        return btagged_etas;
                    }, {"Jet_eta", "is_bcflav", tagger})
                    .Define("bcflav_med_btagged_pts", [&wp](const ROOT::VecOps::RVec<float> &Jet_pt, const ROOT::VecOps::RVec<int> &is_bcflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_pts;
                        for (size_t i = 0; i < Jet_pt.size(); ++i) {
                            if (is_bcflav[i] && tagger[i] > wp.medium) btagged_pts.push_back(Jet_pt[i]);
                        } //0.3196	- 2022EE, 0.3086 - 2022preEE
                        return btagged_pts;
                    }, {"Jet_pt", "is_bcflav", tagger})
                    .Define("bcflav_med_btagged_etas", [&wp](const ROOT::VecOps::RVec<float> &Jet_eta, const ROOT::VecOps::RVec<int> &is_bcflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_etas;
                        for (size_t i = 0; i < Jet_eta.size(); ++i) {
                            if (is_bcflav[i] && tagger[i] > wp.medium) btagged_etas.push_back(std::abs(Jet_eta[i]));
                        }
                        return btagged_etas;
                    }, {"Jet_eta", "is_bcflav", tagger})
                    .Define("bcflav_tight_btagged_pts", [&wp](const ROOT::VecOps::RVec<float> &Jet_pt, const ROOT::VecOps::RVec<int> &is_bcflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_pts;
                        for (size_t i = 0; i < Jet_pt.size(); ++i) {
                            if (is_bcflav[i] && tagger[i] > wp.tight) btagged_pts.push_back(Jet_pt[i]);
                        }
                        return btagged_pts;
                    }, {"Jet_pt", "is_bcflav", tagger})
                    .Define("bcflav_tight_btagged_etas", [&wp](const ROOT::VecOps::RVec<float> &Jet_eta, const ROOT::VecOps::RVec<int> &is_bcflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_etas;
                        for (size_t i = 0; i < Jet_eta.size(); ++i) {
                            if (is_bcflav[i] && tagger[i] > wp.tight) btagged_etas.push_back(std::abs(Jet_eta[i]));
                        }
                        return btagged_etas;
                    }, {"Jet_eta", "is_bcflav", tagger});

        auto df_lflav = df.Define("is_lflav", "Jet_hadronFlavour == 0")
                    .Define("lflav_pts", [&wp](const ROOT::VecOps::RVec<float> &Jet_pt, const ROOT::VecOps::RVec<int> &is_lflav) {
                        ROOT::VecOps::RVec<float> pts;
                        for (size_t i = 0; i < Jet_pt.size(); ++i) {
                            if (is_lflav[i]) pts.push_back(Jet_pt[i]);
                        }
                        return pts;
                    }, {"Jet_pt", "is_lflav"})
                    .Define("lflav_etas", [&wp](const ROOT::VecOps::RVec<float> &Jet_eta, const ROOT::VecOps::RVec<int> &is_lflav) {
                        ROOT::VecOps::RVec<float> etas;
                        for (size_t i = 0; i < Jet_eta.size(); ++i) {
                            if (is_lflav[i]) etas.push_back(std::abs(Jet_eta[i]));
                        }
                        return etas;
                    }, {"Jet_eta", "is_lflav"})
                    .Define("lflav_loose_btagged_pts", [&wp](const ROOT::VecOps::RVec<float> &Jet_pt, const ROOT::VecOps::RVec<int> &is_lflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_pts;
                        for (size_t i = 0; i < Jet_pt.size(); ++i) {
                            if (is_lflav[i] && tagger[i] >  wp.loose) btagged_pts.push_back(Jet_pt[i]);
                        } 
                        return btagged_pts;
                    }, {"Jet_pt", "is_lflav", tagger})
                    .Define("lflav_loose_btagged_etas", [&wp](const ROOT::VecOps::RVec<float> &Jet_eta, const ROOT::VecOps::RVec<int> &is_lflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_etas;
                        for (size_t i = 0; i < Jet_eta.size(); ++i) {
                            if (is_lflav[i] && tagger[i] >  wp.loose) btagged_etas.push_back(std::abs(Jet_eta[i]));
                        }
                        return btagged_etas;
                    }, {"Jet_eta", "is_lflav", tagger})
                    .Define("lflav_med_btagged_pts", [&wp](const ROOT::VecOps::RVec<float> &Jet_pt, const ROOT::VecOps::RVec<int> &is_lflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_pts;
                        for (size_t i = 0; i < Jet_pt.size(); ++i) {
                            if (is_lflav[i] && tagger[i] > wp.medium) btagged_pts.push_back(Jet_pt[i]);
                        }
                        return btagged_pts;
                    }, {"Jet_pt", "is_lflav", tagger})
                    .Define("lflav_med_btagged_etas", [&wp](const ROOT::VecOps::RVec<float> &Jet_eta, const ROOT::VecOps::RVec<int> &is_lflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_etas;
                        for (size_t i = 0; i < Jet_eta.size(); ++i) {
                            if (is_lflav[i] && tagger[i] > wp.medium) btagged_etas.push_back(std::abs(Jet_eta[i]));
                        }
                        return btagged_etas;
                    }, {"Jet_eta", "is_lflav", tagger})
                    .Define("lflav_tight_btagged_pts", [&wp](const ROOT::VecOps::RVec<float> &Jet_pt, const ROOT::VecOps::RVec<int> &is_lflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_pts;
                        for (size_t i = 0; i < Jet_pt.size(); ++i) {
                            if (is_lflav[i] && tagger[i] > wp.tight) btagged_pts.push_back(Jet_pt[i]);
                        } // 0.7183 - 2022preEE, 0.73 - 2022EE
                        return btagged_pts;
                    }, {"Jet_pt", "is_lflav", tagger})
                    .Define("lflav_tight_btagged_etas", [&wp](const ROOT::VecOps::RVec<float> &Jet_eta, const ROOT::VecOps::RVec<int> &is_lflav, const ROOT::VecOps::RVec<float> &tagger) {
                        ROOT::VecOps::RVec<float> btagged_etas;
                        for (size_t i = 0; i < Jet_eta.size(); ++i) {
                            if (is_lflav[i] && tagger[i] > wp.tight) btagged_etas.push_back(std::abs(Jet_eta[i]));
                        }
                        return btagged_etas;
                    }, {"Jet_eta", "is_lflav", tagger});

        // Fill histograms for this file
        auto h_all_bcflav_tmp = df_bcflav.Histo2D({"h_all_bcflav_tmp", ("Total BCFlav Jets from " + file_path).c_str(), static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data()}, "bcflav_pts", "bcflav_etas");
        auto h_looseID_bcflav_tmp = df_bcflav.Histo2D({"h_looseID_bcflav_tmp", ("Loose BTagged BCFlav Jets from " + file_path).c_str(), static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data()}, "bcflav_loose_btagged_pts", "bcflav_loose_btagged_etas");
        auto h_medID_bcflav_tmp = df_bcflav.Histo2D({"h_medID_bcflav_tmp", ("Medium BTagged BCFlav Jets from " + file_path).c_str(), static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data()}, "bcflav_med_btagged_pts", "bcflav_med_btagged_etas");
        auto h_tightID_bcflav_tmp = df_bcflav.Histo2D({"h_tightID_bcflav_tmp", ("Tight BTagged BCFlav Jets from " + file_path).c_str(), static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data()}, "bcflav_tight_btagged_pts", "bcflav_tight_btagged_etas");

        auto h_all_lflav_tmp = df_lflav.Histo2D({"h_all_lflav_tmp", ("Total LFlav Jets from " + file_path).c_str(), static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data()}, "lflav_pts", "lflav_etas");
        auto h_looseID_lflav_tmp = df_lflav.Histo2D({"h_looseID_lflav_tmp", ("Loose BTagged LFlav Jets from " + file_path).c_str(), static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data()}, "lflav_loose_btagged_pts", "lflav_loose_btagged_etas");
        auto h_medID_lflav_tmp = df_lflav.Histo2D({"h_medID_lflav_tmp", ("Medium BTagged LFlav Jets from " + file_path).c_str(), static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data()}, "lflav_med_btagged_pts", "lflav_med_btagged_etas");
        auto h_tightID_lflav_tmp = df_lflav.Histo2D({"h_tightID_lflav_tmp", ("Tight BTagged LFlav Jets from " + file_path).c_str(), static_cast<int>(pt_bins.size()-1), pt_bins.data(), static_cast<int>(eta_bins.size()-1), eta_bins.data()}, "lflav_tight_btagged_pts", "lflav_tight_btagged_etas");

         // Add this file's histograms to the cumulative histograms
        h_all_bcflav.Add(h_all_bcflav_tmp.GetPtr());
        h_looseID_bcflav.Add(h_looseID_bcflav_tmp.GetPtr());
        h_medID_bcflav.Add(h_medID_bcflav_tmp.GetPtr());
        h_tightID_bcflav.Add(h_tightID_bcflav_tmp.GetPtr());

        h_all_lflav.Add(h_all_lflav_tmp.GetPtr());
        h_looseID_lflav.Add(h_looseID_lflav_tmp.GetPtr());
        h_medID_lflav.Add(h_medID_lflav_tmp.GetPtr());
        h_tightID_lflav.Add(h_tightID_lflav_tmp.GetPtr());
    }

   // Open the output file to save the combined histograms
    TFile output(output_file.c_str(), "RECREATE");

    // Write the combined histograms to the file
    h_all_bcflav.Write();
    h_looseID_bcflav.Write();
    h_medID_bcflav.Write();
    h_tightID_bcflav.Write();

    h_all_lflav.Write();
    h_looseID_lflav.Write();
    h_medID_lflav.Write();
    h_tightID_lflav.Write();

    // Close the output file
    output.Close();
}

// Function to read input files from input.txt
std::vector<std::string> read_input_files(const std::string &input_file_list) {
    std::vector<std::string> input_files;
    std::ifstream infile(input_file_list);
    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty()) {
            input_files.push_back(line);
        }
    }
    return input_files;
}
    
void btag_efficiency() {
//     std::string input_file_list = "input_files_2024.txt";
//     std::string output_file = "btag_hists_2024_UParTAK4.root";
//     std::string year_tag = "2024";

    std::string input_file_list = "2024_MC_file_list.txt";
    std::string output_file = "btag_hists_2024_UParT.root";
    std::string year_tag = "2024";

    std::vector<std::string> input_files = read_input_files(input_file_list);
    calculate_btag_efficiencies(input_files, output_file, year_tag);
}

