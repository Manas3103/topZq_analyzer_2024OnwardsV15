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
#include "NanoAODAnalyzerrdframe.h"
#include "BaseAnalyser.h"
#include "FakeFactorAnalyser.h"
#include "TChain.h"
using namespace std;
using namespace ROOT;

int main(void) {

	TChain c1("Events");
	// c1.Add("root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/TZQB-Zto2L-4FS_Bin-MLL-30_TuneCP5_13p6TeV_amcatnlo-pythia8/NANOAODSIM/Madgraph_2_6_5_150X_mcRun3_2024_realistic_v2-v2/2810000/43318103-fc71-48c7-8d99-4915164b3b87.root"); 
	c1.Add("root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/QCD_Bin-PT-120to170_Fil-bcToE_TuneCP5_13p6TeV_pythia8/NANOAODSIM/150X_mcRun3_2024_realistic_v2-v2/120000/2bef59fc-fde7-48e6-82ee-61a1c2014cc3.root");



    // FakeFactorAnalyser nanoaodrdf(&c1, "tzq_new.root");
    FakeFactorAnalyser nanoaodrdf(&c1, "QCD_bcToE.root");
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
