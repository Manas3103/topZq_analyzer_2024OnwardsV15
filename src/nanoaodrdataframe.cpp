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
        {"tzqb", "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/TZQB-Zto2L-4FS_Bin-MLL-30_TuneCP5_13p6TeV_amcatnlo-pythia8/NANOAODSIM/Madgraph_2_6_5_150X_mcRun3_2024_realistic_v2-v2/2810000/43318103-fc71-48c7-8d99-4915164b3b87.root", "TZQB_TZQAnalysis.root"},
        {"muoneg-c", "root://cmsxrootd.fnal.gov//store/data/Run2024C/MuonEG/NANOAOD/MINIv6NANOv15-v1/2530000/5ec3440b-ed82-41a9-9740-a4b5829ff450.root", "MuonEG_Era_C_Run24_TZQAnalysis.root"},
        {"zz4l", "root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/ZZto4L_TuneCP5_13p6TeV_powheg-pythia8/NANOAODSIM/150X_mcRun3_2024_realistic_v2-v2/110000/7b6611e3-13d6-419c-b1b2-93274d009935.root", "ZZ4L_TZQAnalysis.root"},
    };
    const char *help = R"HELP(Usage: ./nanoaodrdataframe [--sample NAME | --input PATH_OR_URL] [--output FILE]
       ./nanoaodrdataframe -h | --help

Options:
  --sample NAME        Select an existing sample and its default output.
  --input PATH_OR_URL  Use a custom input; requires --output.
  --output FILE        Override the output filename.
  -h, --help           Show this help without processing events.

Samples:
  tzqb  TZQB MC -> TZQB_TZQAnalysis.root
  muoneg-c  MuonEG Run2024C data -> MuonEG_Era_C_Run24_TZQAnalysis.root
  zz4l  ZZto4L MC -> ZZ4L_TZQAnalysis.root

No arguments: tzqb -> tzq_new.root (existing behavior).
--sample and --input are mutually exclusive.
Year is fixed at 2024; MC/data detection remains automatic using genWeight.
Existing analyser, triggers, corrections and selections remain unchanged.

Examples:
  ./nanoaodrdataframe --sample tzqb
  ./nanoaodrdataframe --sample muoneg-c --output data_test.root
  ./nanoaodrdataframe --input /path/to/input.root --output custom_test.root
)HELP";
    string sampleName = "tzqb";
    string input;
    string output = "tzq_new.root";
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
    BaseAnalyser nanoaodrdf(&c1, output);
    nanoaodrdf.setParams(2024, "", -1);
    // nanoaodrdf.setParams(2024, "", -1, 1.0, 1.0);
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

    string muonRECOtype    = "NUM_promptMVA_WP64ID_DEN_TightID"; // not required for Run3
    string muonIDtype      = "NUM_TightID_DEN_TrackerMuons"; // not using 
    // string muonRECOtype    = "NUM_promptMVA_WP64ID_DEN_MediumID"; // not required for Run3
    // string muonIDtype      = "NUM_MediumID_DEN_TrackerMuons"; // not using 
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
            // muonHLTtype,
            muonRECOtype , muonIDtype, muonISOtype, electron_fname,
            electronHlt_fname,electronHlt_type, electron_reco_type1,
            electron_reco_type2, electron_id_type,
            jercfname, jerctag, jettagMC, jercunctag, jet_veto_f_name,
            jet_veto_tag,electron_SSF, metpt_fname, jetidfname,
            jetid_workingpoint, JER_tag, JER_tag_res);

	nanoaodrdf.setupObjects();
	nanoaodrdf.setupAnalysis();
	nanoaodrdf.run(false, "outputTree");

	return EXIT_SUCCESS;
}
