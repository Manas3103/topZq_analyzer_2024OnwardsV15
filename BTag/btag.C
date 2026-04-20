#include <TFile.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <iostream>

void calculate_btag_efficiency_from_histograms(const std::string &input_file, const std::string &output_file) {
    // Open the input ROOT file
    TFile *input = TFile::Open(input_file.c_str(), "READ");
    if (!input || input->IsZombie()) {
        std::cerr << "Error: Could not open input file " << input_file << std::endl;
        return;
    }

    // Retrieve the histograms from the input file
    TH2D *h_all_bcflav = dynamic_cast<TH2D*>(input->Get("h_all_bcflav"));
    TH2D *h_looseID_bcflav = dynamic_cast<TH2D*>(input->Get("h_looseID_bcflav"));
    TH2D *h_medID_bcflav = dynamic_cast<TH2D*>(input->Get("h_medID_bcflav"));
    TH2D *h_tightID_bcflav = dynamic_cast<TH2D*>(input->Get("h_tightID_bcflav"));

    TH2D *h_all_lflav = dynamic_cast<TH2D*>(input->Get("h_all_lflav"));
    TH2D *h_looseID_lflav = dynamic_cast<TH2D*>(input->Get("h_looseID_lflav"));
    TH2D *h_medID_lflav = dynamic_cast<TH2D*>(input->Get("h_medID_lflav"));
    TH2D *h_tightID_lflav = dynamic_cast<TH2D*>(input->Get("h_tightID_lflav"));

    if (!h_all_bcflav || !h_looseID_bcflav || !h_medID_bcflav || !h_tightID_bcflav ||
        !h_all_lflav || !h_looseID_lflav || !h_medID_lflav || !h_tightID_lflav) {
        std::cerr << "Error: Could not retrieve histograms from the file " << input_file << std::endl;
        input->Close();
        return;
    }

    // Create efficiency histograms by dividing b-tagged jets by total jets
    TH2D *hist_Loose_btagEff_bcflav = dynamic_cast<TH2D*>(h_looseID_bcflav->Clone("hist_Loose_btagEff_bcflav"));
    hist_Loose_btagEff_bcflav->Divide(h_all_bcflav);
    hist_Loose_btagEff_bcflav->SetTitle("Loose B-Tagging Efficiency for BCFlav Jets");

    TH2D *hist_Medium_btagEff_bcflav = dynamic_cast<TH2D*>(h_medID_bcflav->Clone("hist_Medium_btagEff_bcflav"));
    hist_Medium_btagEff_bcflav->Divide(h_all_bcflav);
    hist_Medium_btagEff_bcflav->SetTitle("Medium B-Tagging Efficiency for BCFlav Jets");

    TH2D *hist_Tight_btagEff_bcflav = dynamic_cast<TH2D*>(h_tightID_bcflav->Clone("hist_Tight_btagEff_bcflav"));
    hist_Tight_btagEff_bcflav->Divide(h_all_bcflav);
    hist_Tight_btagEff_bcflav->SetTitle("Tight B-Tagging Efficiency for BCFlav Jets");

    TH2D *hist_Loose_btagEff_lflav = dynamic_cast<TH2D*>(h_looseID_lflav->Clone("hist_Loose_btagEff_lflav"));
    hist_Loose_btagEff_lflav->Divide(h_all_lflav);
    hist_Loose_btagEff_lflav->SetTitle("Loose B-Tagging Efficiency for LFlav Jets");

    TH2D *hist_Medium_btagEff_lflav = dynamic_cast<TH2D*>(h_medID_lflav->Clone("hist_Medium_btagEff_lflav"));
    hist_Medium_btagEff_lflav->Divide(h_all_lflav);
    hist_Medium_btagEff_lflav->SetTitle("Medium B-Tagging Efficiency for LFlav Jets");

    TH2D *hist_Tight_btagEff_lflav = dynamic_cast<TH2D*>(h_tightID_lflav->Clone("hist_Tight_btagEff_lflav"));
    hist_Tight_btagEff_lflav->Divide(h_all_lflav);
    hist_Tight_btagEff_lflav->SetTitle("Tight B-Tagging Efficiency for LFlav Jets");

    // Open the output ROOT file to save the efficiency histograms
    TFile output(output_file.c_str(), "RECREATE");

    // Write the efficiency histograms to the output file
    hist_Loose_btagEff_bcflav->Write();
    hist_Medium_btagEff_bcflav->Write();
    hist_Tight_btagEff_bcflav->Write();

    hist_Loose_btagEff_lflav->Write();
    hist_Medium_btagEff_lflav->Write();
    hist_Tight_btagEff_lflav->Write();

    // Close the output file
    output.Close();

    // Clean up
    input->Close();
    delete input;

    std::cout << "B-Tagging efficiency histograms have been saved to " << output_file << std::endl;
}

void btag() {

    // std::string input_file = "/uscms/home/snehshuc/nobackup/CMSSW_13_2_10/src/fly/BTag/btag_hists_2023postBPix_PaticleNet.root";  // The ROOT file from the first step
    // std::string output_file = "btag_efficiency_2023BPix_ParticleNet.root";


    std::string input_file = "btag_hists_2024_UParT.root";  // The ROOT file from the first step
    std::string output_file = "btag_efficiency_2024_UParT.root";
    

    calculate_btag_efficiency_from_histograms(input_file, output_file);
}
