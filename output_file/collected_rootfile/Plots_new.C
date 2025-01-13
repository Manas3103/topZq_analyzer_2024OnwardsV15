#include <ROOT/RDataFrame.hxx>
#include <TFile.h>
#include <THStack.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <iostream>

int Plots_new() {
    std::string fileName = "Final_histograms.root";
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
            // Normalize the histogram
            if (hist->Integral() != 0) {
                hist->Scale(1.0 / hist->Integral());
            } else {
                std::cerr << "Warning: Histogram " << histName 
                          << " has zero integral and was not normalized." << std::endl;
            }
            hist->SetFillColorAlpha(i + 2, 0.6);  // Assign colors and transparency
            hist->SetLineColor(i + 2);
            histograms.push_back(hist);
        } else {
            std::cerr << "Histogram " << histName << " not found in the file!" << std::endl;
        }
    }

    TCanvas *canvas = new TCanvas("canvas", "Top Pt Stacked Plot", 800, 600);
    THStack *stack = new THStack("stack", "Normalized Histogram for Top Pt; Top Pt [GeV]; Events");

    for (auto hist : histograms) {
        stack->Add(hist);
    }

    // Draw the stack with the NOSTACK option
    stack->Draw("NOSTACK HIST");

    TLegend *legend = new TLegend(0.7, 0.7, 0.9, 0.9);
    for (size_t i = 0; i < histograms.size(); ++i) {
        legend->AddEntry(histograms[i], samples[i].c_str(), "f");
    }
    legend->Draw();

    canvas->SaveAs("normalized_top_pt_plot.png");

    inputFile->Close();
    delete inputFile;
    delete canvas;

    return 0;
}

