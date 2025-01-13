#include <ROOT/RDataFrame.hxx>
#include <TFile.h>
#include <THStack.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <iostream>

int Plots() {
    std::string fileName = "Final_hist_topquark_eta.root";
    TFile *inputFile = TFile::Open(fileName.c_str());
    if (!inputFile || inputFile->IsZombie()) {
        std::cerr << "Error: Cannot open file " << fileName << std::endl;
        return 1;
    }


    std::vector<std::string> samples = {
        "Drellyan", "top_tzq_signal", "diboson", "singletop",
    "Triboson", "TTGamma_Dilept", "ditop"
    };


    std::vector<TH1*> histograms;


    for (size_t i = 0; i < samples.size(); ++i) {
        std::string histName = samples[i];
        TH1 *hist = dynamic_cast<TH1*>(inputFile->Get(histName.c_str()));
        if (hist) {
            hist->SetFillColorAlpha(i + 3, 0.6);  // Assign colors and transparency
            hist->SetLineColor(i + 3);
            histograms.push_back(hist);
        } else {
            std::cerr << "Histogram " << histName << " not found in the file!" << std::endl;
        }
    }

    TCanvas *canvas = new TCanvas("canvas", "Top Pt Stacked Plot", 800, 600);
    THStack *stack = new THStack("stack", "Stacked Histogram for Top eta; eta; Events");

    for (auto hist : histograms) {
        stack->Add(hist);
    }

    stack->Draw("HIST");

    TLegend *legend = new TLegend(0.7, 0.7, 0.9, 0.9);
    for (size_t i = 0; i < histograms.size(); ++i) {
        legend->AddEntry(histograms[i], samples[i].c_str(), "f");
    }
    legend->Draw();

    canvas->SaveAs("Select_top_eta.png");

    inputFile->Close();
    delete inputFile;
    delete canvas;

    return 0;
}
