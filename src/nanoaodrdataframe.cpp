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
//	c1.Add("root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/TZQB-Zto2L-4FS_Bin-MLL-30_TuneCP5_13p6TeV_amcatnlo-pythia8/NANOAODSIM/Madgraph_2_6_5_150X_mcRun3_2024_realistic_v2-v2/2810000/43318103-fc71-48c7-8d99-4915164b3b87.root"); 
//	c1.Add("root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/WZto3LNu_TuneCP5_13p6TeV_powheg-pythia8/NANOAODSIM/150X_mcRun3_2024_realistic_v2-v2/2810000/211342c7-9687-458b-93c9-dc3b28eae566.root"); 
//	c1.Add("root://cmseos.fnal.gov//store/user/msahoo/2024/EGamma0_Era_C_Run24/EGamma0_Era_C_Run24_part20.root");
//	c1.Add("/eos/uscms/store/user/msahoo/2024/wz_3lnu/part1/wz_3lnu_part17.root");
	c1.Add("/eos/uscms/store/user/msahoo/2024/ttbar_semilep/part3/ttbar_semilep_part41.root");
	//c1.Add("root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/ZZto4L_TuneCP5_13p6TeV_powheg-pythia8/NANOAODSIM/150X_mcRun3_2024_realistic_v2-v2/110000/9a62ce9c-6b2a-4838-853c-d7bcb2737a2c.root");
	BaseAnalyser nanoaodrdf(&c1, "ttbar_semi.root");
        nanoaodrdf.setParams(2024, "", -1);
	nanoaodrdf.setHLT();

	string goodjsonfname = "data/GoldenJSON/Cert_Collisions2024_378981_386951_Golden.json";
	string pileupfname = "data/LUM/2024/puWeights_BCDEFGHI.json";
	string pileuptag = "Collisions24_BCDEFGHI_goldenJSON";
	string btvfname = "data/BTV/2022_Summer22/btagging.json";
	string btvtype = "deepJet_shape";
	string jercfname = "data/JERC/2024_Summer24/jet_jerc.json";
	string jerctag = "Summer24Prompt24_V2_DATA_L1L2L3Res_AK4PFPuppi";
	string jettagMC = "Summer24Prompt24_V2_MC_L1L2L3Res_AK4PFPuppi";
	vector<string> jercunctag = {"Summer24Prompt24_V2_MC_Total_AK4PFPuppi"};
	string jetidfname = "data/JERC/2024_Summer24/jetid.json";  //this is not working in naodv15 so doing it manualy
	string jetid_workingpoint = "AK4PUPPI_TightLeptonVeto";
	string muon_roch_fname = "data/MUO/2024_Summer24/muon_scalesmearing.json";
	string muon_fname = "data/MUO/2024_Summer24/muon_Z.json";
	string muonHLTtype = "NUM_IsoMu24_DEN_CutBasedIdTight_and_PFIsoTight";
	string muonRECOtype = "NUM_TrackerMuons_DEN_genTracks";   // not required for Run3
	string muonIDtype = "NUM_TightID_DEN_TrackerMuons";
	string muonISOtype = "NUM_TightPFIso_DEN_TightID";
	string electron_fname = "data/EGM/2024_Summer24/electron.json";
	string electronHlt_fname = "data/EGM/2024_Summer24/electronHlt.json";
	string electronHlt_type="HLT_SF_Ele30_TightID";
	string electron_reco_type1 = "RecoAbove75";
	string electron_reco_type2 =  "Reco20to75";
	string electron_reco_type3 =  "RecoBelow20";
	string electron_id_type = "PromptMVA-Tight";
	string jet_veto_f_name="data/JERC/2024_Summer24/jetvetomaps.json";
        string jet_veto_tag = "Summer24Prompt24_RunBCDEFGHI_V1";
	string electron_SSF = "data/EGM/2024_Summer24/electronSS_EtDependent.json";
	string metpt_fname = "data/JERC/2023_Summer23BPix/met_xyCorrections_2023_2023BPix.json";
	string JER_tag = "Summer23BPixPrompt23_RunD_JRV1_MC_ScaleFactor_AK4PFPuppi";
	string JER_tag_res = "Summer23BPixPrompt23_RunD_JRV1_MC_PtResolution_AK4PFPuppi";

// The below line is commented out because it causes some problem while loading the DATA file and the code unable to load all the entries need to fix that some how

	nanoaodrdf.setupCorrections(goodjsonfname, pileupfname, pileuptag,
		       	btvfname, btvtype, muon_roch_fname, muon_fname,
		       	muonHLTtype, muonIDtype, muonISOtype, electron_fname,
			electronHlt_fname,electronHlt_type, electron_reco_type1,
			electron_reco_type2,electron_reco_type3, electron_id_type,
		       	jercfname, jerctag, jettagMC, jercunctag, jet_veto_f_name,
		       	jet_veto_tag,electron_SSF, metpt_fname, jetidfname,
		       	jetid_workingpoint, JER_tag, JER_tag_res);

	nanoaodrdf.setupObjects();
	nanoaodrdf.setupAnalysis();
	nanoaodrdf.run(false, "outputTree");

	return EXIT_SUCCESS;
}
