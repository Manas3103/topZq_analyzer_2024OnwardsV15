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
//	c1.Add("/uscms/home/msahoo/nobackup/Project_tzq/Data_DoubleMu_c_2022_skim.root"); //data
//	c1.Add("/uscms/home/msahoo/nobackup/Project_tzq/2022Data_Muon.root"); //data
	c1.Add("/uscms/home/msahoo/nobackup/Project_tzq/2022_postEE_tZq.root"); // MC
	BaseAnalyser nanoaodrdf(&c1, "top_tzq.root");
	nanoaodrdf.setParams(2022, "PreEE", -1);
//        nanoaodrdf.setParams(2022, "PostEE", -1);
	nanoaodrdf.setHLT();

	string goodjsonfname = "data/GoldenJSON/golden_json_latest_2022.json";
	string pileupfname = "data/LUM/2022_Summer22/puWeights.json";
	string pileuptag = "Collisions2022_355100_357900_eraBCD_GoldenJson";
	string btvfname = "data/BTV/2022_Summer22/btagging.json";
	string btvtype = "deepJet_shape";
	string jercfname = "data/JERC/2022_Summer22/jet_jerc.json";
	string jerctag = "Summer22_22Sep2023_RunCD_V2_DATA_L1L2L3Res_AK4PFPuppi";
	string jettagMC = "Summer22_22Sep2023_V2_MC_L1L2L3Res_AK4PFPuppi";
	string jercunctag = "Summer22_22Sep2023_V2_MC_Total_AK4PFPuppi";
	string muon_roch_fname = "data/MUO/2017_UL/RoccoR2017UL.txt";
	string muon_fname = "data/MUO/2022_Summer22/muon_Z.json";
	string muonHLTtype = "NUM_IsoMu24_DEN_CutBasedIdTight_and_PFIsoTight";
	string muonRECOtype = "NUM_TrackerMuons_DEN_genTracks";   // not required for Run3
	string muonIDtype = "NUM_TightID_DEN_TrackerMuons";
	string muonISOtype = "NUM_TightPFIso_DEN_TightID";
	string electron_fname = "data/EGM/2022_Summer22/electron.json";
	string electronHlt_fname = "data/EGM/2022_Summer22/electronHlt.json";
	string electronHlt_type="HLT_SF_Ele30_TightID";
	string electron_reco_type1 = "Reco20to75";
	string electron_reco_type2 =  "RecoAbove75";
	string electron_id_type = "wp90iso";
	string jet_veto_f_name="data/JERC/2022_Summer22/jetvetomaps.json";
        string jet_veto_tag = "Summer22_23Sep2023_RunCD_V1";
	string electron_SSF = "data/EGM/2022_Summer22/electronSS.json";
	string metpt_fname = "data/JERC/2022_Summer22/met_xyCorrections_2022_2022.json";

// The below line is commented out because it causes some problem while loading the DATA file and the code unable to load all the entries need to fix that some how

	nanoaodrdf.setupCorrections(goodjsonfname, pileupfname, pileuptag, btvfname, btvtype, muon_roch_fname, muon_fname, muonHLTtype, muonRECOtype, muonIDtype, muonISOtype, electron_fname,electronHlt_fname,electronHlt_type, electron_reco_type1,electron_reco_type2, electron_id_type, jercfname, jerctag, jettagMC, jercunctag, jet_veto_f_name, jet_veto_tag,electron_SSF, metpt_fname);

	nanoaodrdf.setupObjects();
	nanoaodrdf.setupAnalysis();
	nanoaodrdf.run(false, "outputTree");

	return EXIT_SUCCESS;
}
