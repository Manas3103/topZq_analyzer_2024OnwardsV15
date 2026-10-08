//============================================================================
// Name        : nanoaodrdataframe.cpp
// Author      : Suyong Choi
// Version     :
// Copyright   : suyong@korea.ac.kr, Korea University, Department of Physics
// Description : Hello World in C, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include "NanoAODAnalyzerrdframe.h"
#include "BaseAnalyser.h"
#include "FakeFactorAnalyser.h"
#include "TChain.h"
using namespace std;
using namespace ROOT;

int main(int argc, char **argv) {
    struct Sample {
        const char *name;
        const char *input;
        const char *output;
    };
    const Sample samples[] = {
        {"qcd-bctoe", "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/QCD_Bin-PT-120to170_Fil-bcToE_TuneCP5_13p6TeV_pythia8/NANOAODSIM/150X_mcRun3_2024_realistic_v2-v2/120000/2bef59fc-fde7-48e6-82ee-61a1c2014cc3.root", "QCD_bcToE_FakeFactor.root"},
        {"muoneg-h", "root://cmsxrootd.fnal.gov//store/data/Run2024H/MuonEG/NANOAOD/MINIv6NANOv15-v2/2520000/6fbe0049-3698-4949-981a-e3a6b538b351.root", "MuonEG_Era_H_Run24_FakeFactor.root"},
        {"muon1-h", "root://cmsxrootd.fnal.gov//store/data/Run2024H/Muon1/NANOAOD/MINIv6NANOv15-v2/90000/13443c42-6446-42e3-8b8b-5cdf731dee7f.root", "Muon1_Era_H_Run24_FakeFactor.root"},
        {"wjets", "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/WtoLNu-4Jets_Bin-4J_TuneCP5_13p6TeV_madgraphMLM-pythia8/NANOAODSIM/150X_mcRun3_2024_realistic_v2-v2/2530000/b531ec92-c480-4a84-919d-4d31c8abf7dd.root", "WJets_4J_FakeFactor.root"},
    };
    const char *help = R"HELP(Usage: ./fakefactorframe [--sample NAME | --input PATH_OR_URL] [--output FILE]
       ./fakefactorframe -h | --help

Options:
  --sample NAME        Select an existing sample and its default output.
  --input PATH_OR_URL  Use a custom input; requires --output.
  --output FILE        Override the output filename.
  -h, --help           Show this help without processing events.

Samples:
  qcd-bctoe  QCD bcToE pT120to170 MC -> QCD_bcToE_FakeFactor.root
  muoneg-h  MuonEG Run2024H data -> MuonEG_Era_H_Run24_FakeFactor.root
  muon1-h  Muon1 Run2024H data -> Muon1_Era_H_Run24_FakeFactor.root
  wjets  W+jets MC -> WJets_4J_FakeFactor.root

No arguments: wjets -> QCD_bcToE.root (existing behavior).
--sample and --input are mutually exclusive.
Year is fixed at 2024; MC/data detection remains automatic using genWeight.
Existing analyser, triggers, corrections and selections remain unchanged.

Examples:
  ./fakefactorframe --sample wjets
  ./fakefactorframe --sample muoneg-h --output data_test.root
  ./fakefactorframe --input /path/to/input.root --output custom_test.root
)HELP";
    string sampleName = "wjets";
    string input;
    string output = "QCD_bcToE.root";
    bool haveSample = false, haveInput = false, haveOutput = false;
    bool showHelp = false;
    auto usageError = [&](const string &message) {
        cerr << "Error: " << message << "\nUse --help for usage.\n";
        return EXIT_FAILURE;
    };
    for (int i = 1; i < argc; ++i) {
        string option = argv[i];
        if (option == "-h" || option == "--help") {
            showHelp = true;
            continue;
        }
        if (option != "--sample" && option != "--input" && option != "--output")
            return usageError("unknown option: " + option);
        if (i + 1 >= argc || argv[i + 1][0] == '-' || argv[i + 1][0] == '\0')
            return usageError("missing argument for " + option);
        string value = argv[++i];
        if (option == "--sample") {
            if (haveSample) return usageError("duplicate --sample");
            haveSample = true;
            sampleName = value;
        } else if (option == "--input") {
            if (haveInput) return usageError("duplicate --input");
            haveInput = true;
            input = value;
        } else {
            if (haveOutput) return usageError("duplicate --output");
            haveOutput = true;
            output = value;
        }
    }
    if (haveInput && haveSample)
        return usageError("--input and --sample are mutually exclusive");
    if (haveInput && !haveOutput)
        return usageError("--input requires --output");
    if (!haveInput) {
        const Sample *selected = nullptr;
        for (const auto &sample : samples)
            if (sampleName == sample.name) selected = &sample;
        if (!selected) return usageError("invalid sample: " + sampleName);
        input = selected->input;
        if (haveSample && !haveOutput) output = selected->output;
    }
    if (showHelp) {
        cout << help;
        return EXIT_SUCCESS;
    }
    cout << "Selected sample: " << (haveInput ? "custom" : sampleName)
         << "\nInput: " << input << "\nOutput: " << output
         << "\nAnalysis year: 2024\nMode: automatic (datatype=-1)\n";

    TChain c1("Events");
    c1.Add(input.c_str());
    FakeFactorAnalyser nanoaodrdf(&c1, output);
    nanoaodrdf.setParams(2024, "", -1);
	nanoaodrdf.setHLT();

    // Golden JSON
    string goodjsonfname = "data/GoldenJSON/Cert_Collisions2024_378981_386951_Golden.json";

    // Pileup
    string pileupfname = "data/LUM/2024/puWeights_BCDEFGHI.json";
    string pileuptag   = "Collisions24_BCDEFGHI_goldenJSON";

    // Muons
    string muon_roch_fname = "data/MUO/2024_Summer24/muon_scalesmearing.json";
    string muon_fname      = "data/MUO/2024_Summer24/muon_Z.json";
    string muonHLTtype     = "NUM_IsoMu24_DEN_CutBasedIdTight_and_PFIsoTight"; //not using 

    string muonRECOtype    = "NUM_promptMVA_WP64ID_DEN_MediumID"; // not required for Run3
    string muonIDtype      = "NUM_MediumID_DEN_TrackerMuons"; // not using 
    string muonISOtype     = "NUM_LoosePFIso_DEN_MediumID";

    // Electrons
    string electron_fname      = "data/EGM/2024_Summer24/electron.json";
    string electronHlt_fname   = "data/EGM/2024_Summer24/electronHlt.json";
    string electronHlt_type    = "HLT_SF_Ele30_TightID"; // not using 
    string electron_reco_type1 = "RecoAbove75";
    string electron_reco_type2 = "Reco20to75";
    string electron_reco_type3 = "RecoBelow20"; //not using
    string electron_id_type    = "PromptMVA-Tight";
    string electron_SSF        = "data/EGM/2024_Summer24/electronSS_EtDependent.json";

    // Jet veto
    string jet_veto_f_name = "data/JERC/2024_Summer24/jetvetomaps.json";
    string jet_veto_tag    = "Summer24Prompt24_RunBCDEFGHI_V1";

    // Jet corrections
    string jetidfname          = "data/JERC/2024_Summer24/jetid.json"; // not working in NanoAODv15
    string jetid_workingpoint  = "AK4PUPPI_TightLeptonVeto";
    string jercfname           = "data/JERC/2024_Summer24/jet_jerc.json";
    string jerctag             = "Summer24Prompt24_V3_DATA_L1L2L3Res_AK4PFPuppi";
    string jettagMC            = "Summer24Prompt24_V3_MC_L1L2L3Res_AK4PFPuppi";
    vector<string> jercunctag  = {"Summer24Prompt24_V3_MC_Total_AK4PFPuppi"};
    string metpt_fname         = "data/JERC/2023_Summer23BPix/met_xyCorrections_2023_2023BPix.json";
    string JER_tag             = "Summer24Prompt24_JRV1_MC_ScaleFactor_AK4PFPuppi";
    string JER_tag_res         = "Summer24Prompt24_JRV1_MC_PtResolution_AK4PFPuppi";

    // BTV
    string btvfname  = "data/BTV/2024_Summer24/btagging.json";
    string btvtype   = "UParTAK4_comb";
    string fname_btagEff = "BTag/btag_efficiency_2024_UParT.root";

    string hname_Loose_btagEff_bcflav  = "hist_Loose_btagEff_bcflav";
    string hname_Loose_btagEff_lflav   = "hist_Loose_btagEff_lflav";
    string hname_Medium_btagEff_bcflav = "hist_Medium_btagEff_bcflav";
    string hname_Medium_btagEff_lflav  = "hist_Medium_btagEff_lflav";
    string hname_Tight_btagEff_bcflav  = "hist_Tight_btagEff_bcflav";
    string hname_Tight_btagEff_lflav   = "hist_Tight_btagEff_lflav";

	nanoaodrdf.setupCorrections(goodjsonfname, pileupfname, pileuptag,
            btvfname, btvtype,fname_btagEff,
            hname_Loose_btagEff_bcflav,
            hname_Loose_btagEff_lflav,
            hname_Medium_btagEff_bcflav,
            hname_Medium_btagEff_lflav,
            hname_Tight_btagEff_bcflav,
            hname_Tight_btagEff_lflav,
            muon_roch_fname, muon_fname,
            muonRECOtype , muonIDtype, muonISOtype, electron_fname,
            electronHlt_fname,electronHlt_type, electron_reco_type1,
            electron_reco_type2, electron_id_type,
            jercfname, jerctag, jettagMC, jercunctag, jet_veto_f_name,
            jet_veto_tag,electron_SSF, metpt_fname, jetidfname,
            jetid_workingpoint, JER_tag, JER_tag_res);

	// setupObjects() builds the fakeable/tight electrons & muons, cone-pT,
	// measurement region and FF trigger weight; setupAnalysis() then runs
	// defineCuts() -> defineMoreVars() -> bookHists() -> setupCuts_and_Hists()
	// -> setupTree() internally (see FakeFactorAnalyser::setupAnalysis()).
	nanoaodrdf.setupObjects();
	nanoaodrdf.setupAnalysis();
	nanoaodrdf.run(false, "outputTree");

	return EXIT_SUCCESS;
}
