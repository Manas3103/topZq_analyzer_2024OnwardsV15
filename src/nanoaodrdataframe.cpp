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
//#include "SkimEvents.h"
#include "TChain.h"

using namespace std;
using namespace ROOT;

int main(void) {

	TChain c1("Events");
	c1.Add("/uscms/home/msahoo/nobackup/Project_tzq/060de505-8c73-4ec2-840a-027846b86e67.root"); //data
//	c1.Add("/uscms/home/msahoo/nobackup/Project_tzq/2022_postEE_tZq.root"); // MC
	BaseAnalyser nanoaodrdf(&c1, "top_tzq.root");
	nanoaodrdf.setParams(2022, "PreEE", -1);
	nanoaodrdf.setHLT();

	string goodjsonfname = "data/Cert_Collisions2022_355100_362760_Golden.json";
	string pileupfname = "data/LUM/2022_Summer22/puWeights.json";
	string pileuptag = "Collisions2022_355100_357900_eraBCD_GoldenJson";
	string btvfname = "data/BTV/2022_Summer22/btagging.json";
	string btvtype = "deepJet_shape";
//	string fname_btagEff = "data/BTV/2017_UL/BtaggingEfficiency.root";
//	string hname_btagEff_bcflav = "h_btagEff_bcflav";
//	string hname_btagEff_lflav = "h_btagEff_lflav";
	//string electron_fname = "data/ELECTRON/2018_UL/electron_Z.json";
	//string electrontype = "UL-Electron-ID-SF";
	string jercfname = "data/JERC/2022_Summer22/jet_jerc.json";
	string jerctag = "Summer22_22Sep2023_RunCD_V2_DATA_L1L2L3Res_AK4PFPuppi";
	string jettagMC = "Summer22_22Sep2023_V2_MC_L1L2L3Res_AK4PFPuppi";
	string jercunctag = "Summer22_22Sep2023_V2_MC_Total_AK4PFPuppi";
	string muon_roch_fname = "data/MUO/2017_UL/RoccoR2017UL.txt";
	string muon_fname = "data/MUO/2017_UL/muon_Z.json.gz";
	string muonHLTtype = "NUM_IsoMu27_DEN_CutBasedIdTight_and_PFIsoTight";
	string muonRECOtype = "NUM_TrackerMuons_DEN_genTracks";   // not required for Run3
	string muonIDtype = "NUM_MediumID_DEN_TrackerMuons";
	string muonISOtype = "NUM_TightRelIso_DEN_MediumID";
	string electron_fname = "data/EGM/2017_UL/electron.json.gz";
	string electron_reco_type = "RecoAbove20";
	string electron_id_type = "Tight";
	string jet_veto_f_name="data/JERC/2022_Summer22/jetvetomaps.json";
        string jet_veto_tag = "Summer22_23Sep2023_RunCD_V1";

// The below line is commented out because it causes some problem while loading the DATA file and the code unable to load all the entries need to fix that some how

	nanoaodrdf.setupCorrections(goodjsonfname, pileupfname, pileuptag, btvfname, btvtype, muon_roch_fname, muon_fname, muonHLTtype, muonRECOtype, muonIDtype, muonISOtype, electron_fname, electron_reco_type, electron_id_type, jercfname, jerctag, jettagMC, jercunctag, jet_veto_f_name, jet_veto_tag);

	nanoaodrdf.setupObjects();
	nanoaodrdf.setupAnalysis();
	nanoaodrdf.run(false, "outputTree");

	return EXIT_SUCCESS;
}
