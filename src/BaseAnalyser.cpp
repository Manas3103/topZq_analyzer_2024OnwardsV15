/*
 * BaseAnalyser.cpp
 *
 *  Created on: May 6, 2022
 *      Author: suyong
 *      Developper: cdozen
 */

#include "Math/GenVector/VectorUtil.h"
#include "BaseAnalyser.h"
#include "utility.h"
#include <fstream>
#include "correction.h"
using correction::CorrectionSet;

BaseAnalyser::BaseAnalyser(TTree *t, std::string outfilename)
:NanoAODAnalyzerrdframe(t, outfilename)
{
    //initiliaze the HLT names in your analyzer class
    HLT2018Names= {"HLT_PFHT380_SixJet32_DoubleBTagCSV_p075",
                    "HLT_PFHT300PT30_QuadPFJet_75_60_45_40_TriplePFBTagCSV_3p0",
                    "HLT_PFHT330PT30_QuadPFJet_75_60_45_40_TriplePFBTagDeepCSV_4p5",
                    "HLT_PFJet550","HLT_PFHT400_FivePFJet_100_100_60_30_30_DoublePFBTagDeepCSV_4p5",
                    "HLT_PFHT400_FivePFJet_120_120_60_30_30_DoublePFBTagDeepCSV_4p5"};
	
    HLT2017Names = {
	    "HLT_IsoMu24",
	    "HLT_IsoMu24_eta2p1",
	    "HLT_IsoMu27",
	    "HLT_Mu50",
	    "HLT_Ele27_WPTight_Gsf",
	    "HLT_Ele32_WPTight_Gsf",
	    "HLT_Ele32_WPTight_Gsf_L1DoubleEG",
	    "HLT_Ele35_WPTight_Gsf",
	    "HLT_Ele115_CaloIdVT_GsfTrkIdT",
	    "HLT_Photon200",
	    "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL",
	    "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ",
	    "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8",
	    "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8",
	    "HLT_Mu19_TrkIsoVVL_Mu9_TrkIsoVVL_DZ_Mass3p8",
	    "HLT_Mu37_TkMu27",
	    "HLT_TripleMu_12_10_5",
	    "HLT_TripleMu_10_5_5_DZ",
	    "HLT_TripleMu_5_3_3_Mass3p8to60_DZ",
	    "HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL",
	    "HLT_Ele16_Ele12_Ele8_CaloIdL_TrackIdL",
	    "HLT_DoubleEle25_CaloIdL_MW",
	    "HLT_DoubleEle33_CaloIdL_MW",
	    "HLT_DiEle27_WPTightCaloOnly_L1DoubleEG",
	    "HLT_DoublePhoton70",
	    "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL",
	    "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ",
	    "HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ",
	    "HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ",
	    "HLT_Mu27_Ele37_CaloIdL_MW",
	    "HLT_Mu37_Ele27_CaloIdL_MW",
	    "HLT_DiMu9_Ele9_CaloIdL_TrackIdL",
	    "HLT_DiMu9_Ele9_CaloIdL_TrackIdL_DZ",
	    "HLT_Mu8_DiEle12_CaloIdL_TrackIdL"
    };

    HLT2016Names= {"Name1","Name2"};

    HLT2022Names = {
            "HLT_IsoMu24_eta2p1",
	    "HLT_IsoMu24",
            "HLT_IsoMu27",
            "HLT_Mu50",
            "HLT_Ele27_WPTight_Gsf",
            "HLT_Ele32_WPTight_Gsf",
            "HLT_Ele32_WPTight_Gsf_L1DoubleEG",
            "HLT_Ele35_WPTight_Gsf",
            "HLT_Ele115_CaloIdVT_GsfTrkIdT",
            "HLT_Photon200",
            "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL",
            "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ",
            "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8",
            "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8",
            "HLT_Mu19_TrkIsoVVL_Mu9_TrkIsoVVL_DZ_Mass3p8",
            "HLT_Mu37_TkMu27",
            "HLT_TripleMu_12_10_5",
            "HLT_TripleMu_10_5_5_DZ",
            "HLT_TripleMu_5_3_3_Mass3p8to60_DZ",
            "HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL",
            "HLT_Ele16_Ele12_Ele8_CaloIdL_TrackIdL",
            "HLT_DoubleEle25_CaloIdL_MW",
            "HLT_DoubleEle33_CaloIdL_MW",
            "HLT_DiEle27_WPTightCaloOnly_L1DoubleEG",
            "HLT_DoublePhoton70",
            "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL",
            "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ",
            "HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ",
            "HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ",
            "HLT_Mu27_Ele37_CaloIdL_MW",
            "HLT_Mu37_Ele27_CaloIdL_MW",
            "HLT_DiMu9_Ele9_CaloIdL_TrackIdL",
            "HLT_DiMu9_Ele9_CaloIdL_TrackIdL_DZ",
            "HLT_Mu8_DiEle12_CaloIdL_TrackIdL"
    };
}


// Define your cuts here
void BaseAnalyser::defineCuts()
{
	if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }
	auto Nentry = _rlm.Count();
	// This is how you can express a range of the first 100 entries
	//_rlm = _rlm.Range(0, 100000);
        auto Nentry_100 = _rlm.Count();
	std::cout<< "-------------------------------------------------------------------" << std::endl;
        cout << "Usage of ranges:\n"
        << " - All entries: " << *Nentry << endl;
		//<< " - Entries from 0 to 100: " << *Nentr
	std::cout<< "-------------------------------------------------------------------" << std::endl;

	//MinimalSelection to filter events
//	addCuts("nMuon + nElectron >= 3  && nJet>2 && PV_npvsGood >= 1 && LHE_HT < 70", "0");//first change
	addCuts("nMuon + nElectron >= 3  && nJet>0 && PV_npvsGood >= 1 ", "0");//not for drellyan
	addCuts("Flag_goodVertices && Flag_globalSuperTightHalo2016Filter && Flag_HBHENoiseFilter && Flag_HBHENoiseIsoFilter && Flag_EcalDeadCellTriggerPrimitiveFilter && Flag_BadPFMuonFilter && Flag_ecalBadCalibFilter", "00");
	addCuts(setHLT(),"000");


	//addCuts("NgoodMuons>=2","00");
        //addCuts("ncleanjetspass>0","00");
	//addCuts(setHLT(),"000"); //HLT cut buy checking HLT names in the root file
}



// ##=========THIS IS THE NEW FUNCITON WITH THE NEW BRANCH=========##
// ==================================================================
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void BaseAnalyser::selectElectrons()
{
    cout << "select good electrons" << endl;
    if (debug){
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    // Define good electrons based on ID and additional criteria
    _rlm = _rlm.Define("goodElectronsID", ElectronID(2)); // ID level 2
    _rlm = _rlm.Define("goodElectrons", "goodElectronsID && Electron_pt > 25.0 && abs(Electron_eta) < 2.1 && Electron_pfRelIso03_all < 0.15");

    // Define additional variables for good electrons
    _rlm = _rlm.Define("goodElectrons_pt", "Electron_pt[goodElectrons]")
                .Define("goodElectrons_eta", "Electron_eta[goodElectrons]")
                .Define("goodElectrons_phi", "Electron_phi[goodElectrons]")
                .Define("goodElectrons_mass", "Electron_mass[goodElectrons]")
                .Define("goodElectrons_idx", ::good_idx, {"goodElectrons"})
                .Define("NgoodElectrons", "int(goodElectrons_pt.size())");


    //-------------------------------------------------------
    // Generate electron 4-vector from selected good electrons
    //-------------------------------------------------------
    _rlm = _rlm.Define("goodElectron_4Vecs", ::generate_4vec, {"goodElectrons_pt", "goodElectrons_eta", "goodElectrons_phi", "goodElectrons_mass"});



    // =====================================================================
    // baselineElectrons_isPromptBaseline Electron Selection
    // =====================================================================
    _rlm = _rlm.Define("Electron_4Vecs", ::generate_4vec, {"Electron_pt", "Electron_eta", "Electron_phi", "Electron_mass"});
    _rlm = _rlm.Define("Electron_energy", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& electron_4vecs) {
    ROOT::VecOps::RVec<Float_t> energies;
    for (const auto& vec : electron_4vecs) {
        energies.push_back(static_cast<Float_t>(vec.E()));  // E() gives the energy in PtEtaPhiM4D Lorentz vector
    }
    return energies;
    }, {"Electron_4Vecs"});  

    _rlm = _rlm.Define("electron_oneOverEminusOneOverP","1.0 / Electron_energy - 1.0 / Electron_pt");
    _rlm = _rlm.Define("baselineElectrons", 
                       "Electron_pt > 10.0 && abs(Electron_eta) < 2.5 && goodElectronsID &&"
                       "Electron_pfRelIso03_all < 0.40 && abs(Electron_dxy) < 0.05 && "
                       "abs(Electron_dz) < 0.10 && Electron_lostHits <= 1 && "
                       "Electron_hoe < 0.10 && Electron_convVeto &&"
		       "((abs(Electron_eta) < 1.479 && Electron_sieie < 0.011) || " // Barrel cut
                       "(abs(Electron_eta) >= 1.479 && abs(Electron_eta) < 2.5 && Electron_sieie < 0.030)) &&" // Endcap cut
		       "Electron_sip3d < 8 && electron_oneOverEminusOneOverP > -0.04"
		       );

    // Additional variables for baseline electrons
    _rlm = _rlm.Define("A_baselineElectrons_pt", "Electron_pt[baselineElectrons]")
                .Define("A_baselineElectrons_eta", "Electron_eta[baselineElectrons]")
                .Define("A_baselineElectrons_phi", "Electron_phi[baselineElectrons]")
                .Define("A_baselineElectrons_mass", "Electron_mass[baselineElectrons]")
                .Define("A_baselineElectrons_charge", "Electron_charge[baselineElectrons]")
                .Define("A_baselineElectrons_idx", ::good_idx, {"baselineElectrons"})
                .Define("A_NbaselineElectrons", "int(A_baselineElectrons_pt.size())");

    // Generate 4-vectors for baseline electrons
    _rlm = _rlm.Define("A_baselineElectron_4Vecs", ::generate_4vec, {"A_baselineElectrons_pt", "A_baselineElectrons_eta", "A_baselineElectrons_phi", "A_baselineElectrons_mass"});
    

    _rlm = _rlm.Define("A_baselineElectrons_TL4Vecs",
    [](const ROOT::VecOps::RVec<float>& pt,
       const ROOT::VecOps::RVec<float>& eta,
       const ROOT::VecOps::RVec<float>& phi,
       const ROOT::VecOps::RVec<float>& mass) -> ROOT::VecOps::RVec<TLorentzVector> {
        ROOT::VecOps::RVec<TLorentzVector> vecs;
        for (size_t i = 0; i < pt.size(); ++i) {
            TLorentzVector vec;
            vec.SetPtEtaPhiM(pt[i], eta[i], phi[i], mass[i]);
            vecs.emplace_back(vec);
        }
        return vecs;
    },
    {"A_baselineElectrons_pt", "A_baselineElectrons_eta", "A_baselineElectrons_phi", "A_baselineElectrons_mass"});
    
    // Define tight and fakable electrons based on MVA score
    _rlm = _rlm.Define("A_baselineElectrons_mvaTTH", "Electron_mvaTTH[baselineElectrons]");
    _rlm = _rlm.Define("A_tight_baselineElectrons", "A_baselineElectrons_mvaTTH > 0.90")
               .Define("A_fakable_baselineElectrons", "A_baselineElectrons_mvaTTH <= 0.90");
    
/*    _rlm = _rlm.Define("baselineElectron_4Vecs_RVec",
    [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& vec) {
        return ROOT::VecOps::RVec<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>(vec.begin(), vec.end());
    }, {"baselineElectron_4Vecs"});

*/

}



// ##=========THIS IS THE NEW FUNCITON WITH THE NEW BRANCH=========##
// ==================================================================
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void BaseAnalyser::selectMuons()
{
    cout << "select good muons" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }



    // Define good muons based on ID and additional criteria
    _rlm = _rlm.Define("goodMuonsID", MuonID(2)); // loose muons
    _rlm = _rlm.Define("goodMuons", "goodMuonsID && Muon_pt > 30 && abs(Muon_eta) < 2.4 && Muon_miniPFRelIso_all < 0.40");

    // Define additional variables for good muons
    _rlm = _rlm.Define("goodMuons_pt", "Muon_pt[goodMuons]")
                .Define("goodMuons_eta", "Muon_eta[goodMuons]")
                .Define("goodMuons_phi", "Muon_phi[goodMuons]")
                .Define("goodMuons_mass", "Muon_mass[goodMuons]")
                .Define("goodMuons_charge", "Muon_charge[goodMuons]")
                .Define("goodMuons_idx", ::good_idx, {"goodMuons"})
                .Define("NgoodMuons", "int(goodMuons_pt.size())");

    // ====================================================================
    // Baseline Muon Selection
    // // =====================================================================
    _rlm = _rlm.Define("baselineMuons",
		       "Muon_pt > 10.0 && abs(Muon_eta) < 2.4 && goodMuonsID && "
	       	       "Muon_pfRelIso04_all < 0.4 && abs(Muon_dxy) < 0.05 && "
		       "abs(Muon_dz) < 0.10 && Muon_sip3d < 8.0 && "
		       "Muon_mediumId");

	// Additional variables for baseline muons
    _rlm = _rlm.Define("baselineMuons_pt", "Muon_pt[baselineMuons]")
	       .Define("baselineMuons_eta", "Muon_eta[baselineMuons]")
	       .Define("baselineMuons_phi", "Muon_phi[baselineMuons]")
	       .Define("baselineMuons_mass", "Muon_mass[baselineMuons]")
	       .Define("baselineMuons_charge", "Muon_charge[baselineMuons]")
               .Define("baselineMuons_idx", ::good_idx, {"baselineMuons"})
	       .Define("NbaselineMuons", "int(baselineMuons_pt.size())");
         
	    // Generate 4-vectors for baseline muons
    _rlm = _rlm.Define("baselineMuon_4Vecs", ::generate_4vec, {"baselineMuons_pt", "baselineMuons_eta", "baselineMuons_phi", "baselineMuons_mass"});
    
    _rlm = _rlm.Define("baselineMuons_TL4Vecs",
    [](const ROOT::VecOps::RVec<float>& pts,
       const ROOT::VecOps::RVec<float>& etas,
       const ROOT::VecOps::RVec<float>& phis,
       const ROOT::VecOps::RVec<float>& masses) -> ROOT::VecOps::RVec<TLorentzVector> {
        ROOT::VecOps::RVec<TLorentzVector> vecs;
        for (size_t i = 0; i < pts.size(); ++i) {
            TLorentzVector vec;
            vec.SetPtEtaPhiM(pts[i], etas[i], phis[i], masses[i]);
            vecs.emplace_back(vec);
        }
        return vecs;
    },
    {"baselineMuons_pt", "baselineMuons_eta", "baselineMuons_phi", "baselineMuons_mass"});
    
    _rlm = _rlm.Define("tight_baselineMuons", "Muon_mvaTTH[baselineMuons] > 0.64")
               .Define("fakable_baselineMuons", "Muon_mvaTTH[baselineMuons] <= 0.64");

    

    //-------------------------------------------------------
    // Generate muon 4-vector from selected good muons
    //-------------------------------------------------------
    _rlm = _rlm.Define("goodMuons_4vecs", ::generate_4vec, {"goodMuons_pt", "goodMuons_eta", "goodMuons_phi", "goodMuons_mass"});
}

//=================================Select Jets=================================================//
//check the twiki page :    https://twiki.cern.ch/twiki/bin/view/CMS/JetID
//to find jetId working points for the purpose of  your analysis.
    //jetId==2 means: pass tight ID, fail tightLepVeto
    //jetId==6 means: pass tight ID and tightLepVeto ID.
//=============================================================================================//
void BaseAnalyser::selectJets()
{

    cout << "select good jets" << endl;
    if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

    _rlm = _rlm.Define("goodJetsID", JetID(6)); //without pt-eta cuts
    _rlm = _rlm.Define("goodJets", "goodJetsID && ((abs(Jet_eta) > 2.65 && abs(Jet_eta) < 3.139 && Jet_pt > 50.0) || (Jet_pt > 25.0 && abs(Jet_eta) < 5 && abs(Jet_eta)>3.139) || (Jet_pt > 25.0 && abs(Jet_eta) < 2.65 && abs(Jet_eta)>0))");
    _rlm = _rlm.Define("goodJets_pt", "Jet_pt[goodJets]")
               .Define("goodJets_eta", "Jet_eta[goodJets]")
               .Define("goodJets_phi", "Jet_phi[goodJets]")
               .Define("goodJets_mass", "Jet_mass[goodJets]")
               .Define("goodJets_idx", ::good_idx, {"goodJets"});

    if (!_isData){
       _rlm = _rlm.Define("goodJets_hadflav", "Jet_hadronFlavour[goodJets]");
    }
    //goot jets deep-b tag          
//	_rlm = _rlm.Define("goodJets_jetdeepbtag", "Jet_btagDeepB[goodJets]")
        _rlm = _rlm.Define("goodJets_deepjetbtag", "Jet_btagDeepFlavB[goodJets]") 
                   .Define("NgoodJets", "int(goodJets_pt.size())")
                   .Define("goodJets_4vecs", ::generate_4vec, {"goodJets_pt", "goodJets_eta", "goodJets_phi", "goodJets_mass"});


    _rlm = _rlm.Define("btagcuts", "goodJets_deepjetbtag > 0.7") // 0.2783 - medium, 0.7 - tight
               .Define("good_bjetpt", "goodJets_pt[btagcuts]")
	       .Define("good_bjeteta", "goodJets_eta[btagcuts]")
	       .Define("good_bjetphi", "goodJets_phi[btagcuts]")
	       .Define("good_bjetmass", "goodJets_mass[btagcuts]");

//             .Define("good_bjet_leading_pt", "int(good_bjetpt.size()) > 0 ? static_cast<double>(good_bjetpt[0]) : -999.9")
//             .Define("good_bjet_leading_eta", "int(good_bjeteta.size()) > 0 ? static_cast<double>(good_bjeteta[0]) : -999.9")
//             .Define("good_bjet_leading_phi", "int(good_bjetphi.size()) > 0 ? static_cast<double>(good_bjetphi[0]) : -999.9")
//             .Define("good_bjet_leading_mass", "int(good_bjetmass.size()) > 0 ? static_cast<double>(good_bjetmass[0]) : -999.9");

 
    if (!_isData){  
    _rlm = _rlm.Define("good_bjethadflav", "goodJets_hadflav[btagcuts]");
    }
    _rlm = _rlm.Define("Ngood_bjets", "int(good_bjetpt.size())")   //when remove the comment from the next linw remove the ; of this line 
               .Define("good_bjet4vecs", ::generate_4vec, {"good_bjetpt", "good_bjeteta", "good_bjetphi", "good_bjetmass"})
               .Define("top_Bjet_TL4Vecs", [](const ROOT::VecOps::RVec<float>& pts,
                               const ROOT::VecOps::RVec<float>& etas,
                               const ROOT::VecOps::RVec<float>& phis,
                               const ROOT::VecOps::RVec<float>& masses) -> ROOT::VecOps::RVec<TLorentzVector> {
    ROOT::VecOps::RVec<TLorentzVector> vecs;
    for (size_t i = 0; i < pts.size(); ++i) {
        TLorentzVector vec;
        vec.SetPtEtaPhiM(pts[i], etas[i], phis[i], masses[i]);
        vecs.emplace_back(vec);
    }
    return vecs;
    }, {"good_bjetpt", "good_bjeteta", "good_bjetphi", "good_bjetmass"})
  
      .Define("is_top_Bjets_event", "NgoodJets >= 1 && Ngood_bjets >= 1");


    if(!_isData){
      //For Btagging Efficiency    
      _rlm = _rlm.Define("btagpass_bcflav_goodJets", "goodJets_deepjetbtag>0.2783 && goodJets_hadflav!=0") //0.2783 -medium, 0.7 - tight 
		 .Define("goodJets_btagpass_bcflav_pt", "goodJets_pt[btagpass_bcflav_goodJets]")
		 .Define("goodJets_btagpass_bcflav_eta", "goodJets_eta[btagpass_bcflav_goodJets]");
      
      _rlm = _rlm.Define("all_bcflav_goodJets", "goodJets_hadflav!=0") //0.2783 -medium, 0.7 - tight 
		 .Define("goodJets_all_bcflav_pt", "goodJets_pt[all_bcflav_goodJets]")
		 .Define("goodJets_all_bcflav_eta", "goodJets_eta[all_bcflav_goodJets]");
      
      
      _rlm = _rlm.Define("btagpass_lflav_goodJets", "goodJets_deepjetbtag>0.2783 && goodJets_hadflav==0") //0.2783 -medium, 0.7 - tight 
		 .Define("goodJets_btagpass_lflav_pt", "goodJets_pt[btagpass_lflav_goodJets]")
		 .Define("goodJets_btagpass_lflav_eta", "goodJets_eta[btagpass_lflav_goodJets]");
      
      _rlm = _rlm.Define("all_lflav_goodJets", "goodJets_hadflav==0") //0.2783 -medium, 0.7 - tight 
		 .Define("goodJets_all_lflav_pt", "goodJets_pt[all_lflav_goodJets]")
		 .Define("goodJets_all_lflav_eta", "goodJets_eta[all_lflav_goodJets]");
    }
}
//=================================Overlap function=================================================//
void BaseAnalyser::removeOverlaps()
{
    cout << "checking overlapss between jets and muons" << endl;
	// lambda function
	// for checking overlapped jets with leptons
    auto checkoverlap = [](FourVectorVec &goodjets, FourVectorVec &goodlep)
		{
			doubles mindrlepton;
			for (auto ajet: goodjets)
			{
                auto mindr = 6.0;
				for (auto alepton: goodlep)
				{
					auto dr = ROOT::Math::VectorUtil::DeltaR(ajet, alepton);
                    if (dr < mindr) mindr = dr;
                }
                int out = mindr > 0.4 ? 1 : 0;
                mindrlepton.emplace_back(out);

			}
            return mindrlepton;
	    };

    auto checkoverlapeandmu = [](FourVectorVec &goodjets, FourVectorVec &goodlep)
                {
                        doubles mindrlepton;
                        for (auto ajet: goodjets)
                        {
                auto mindr = 6.0;
                                for (auto alepton: goodlep)
                                {
                                        auto dr = ROOT::Math::VectorUtil::DeltaR(ajet, alepton);
                    if (dr < mindr) mindr = dr;
                }
                int out = mindr > 0.05 ? 1 : 0;
                mindrlepton.emplace_back(out);

                        }
            return mindrlepton;
            };

	//cout << "overlap removal" << endl;

    _rlm = _rlm.Define("Electron_Muonoverlap", checkoverlapeandmu, {"A_baselineElectron_4Vecs","baselineMuon_4Vecs"})
               .Define("baselineElectrons_pt", "A_baselineElectrons_pt[Electron_Muonoverlap]")
	       .Define("baselineElectrons_eta", "A_baselineElectrons_eta[Electron_Muonoverlap]")
               .Define("baselineElectrons_phi", "A_baselineElectrons_phi[Electron_Muonoverlap]")
               .Define("baselineElectrons_mass", "A_baselineElectrons_mass[Electron_Muonoverlap]")
               .Define("baselineElectrons_charge", "A_baselineElectrons_charge[Electron_Muonoverlap]")
               .Define("baselineElectrons_mvaTTH", "A_baselineElectrons_mvaTTH[Electron_Muonoverlap]")
               .Define("tight_baselineElectrons", "A_tight_baselineElectrons[Electron_Muonoverlap]")
               .Define("fakable_baselineElectrons", "A_fakable_baselineElectrons[Electron_Muonoverlap]")
	       .Define("baselineElectrons_idx", ::good_idx, {"baselineElectrons"})
               .Define("NbaselineElectrons", "int(baselineElectrons_pt.size())");

    _rlm = _rlm.Define("baselineElectron_4Vecs", ::generate_4vec, {"baselineElectrons_pt", "baselineElectrons_eta", "baselineElectrons_phi", "baselineElectrons_mass"});


    _rlm = _rlm.Define("baselineElectrons_TL4Vecs",
    [](const ROOT::VecOps::RVec<float>& pt,
       const ROOT::VecOps::RVec<float>& eta,
       const ROOT::VecOps::RVec<float>& phi,
       const ROOT::VecOps::RVec<float>& mass) -> ROOT::VecOps::RVec<TLorentzVector> {
        ROOT::VecOps::RVec<TLorentzVector> vecs;
        for (size_t i = 0; i < pt.size(); ++i) {
            TLorentzVector vec;
            vec.SetPtEtaPhiM(pt[i], eta[i], phi[i], mass[i]);
            vecs.emplace_back(vec);
        }
        return vecs;
    },
    {"baselineElectrons_pt", "baselineElectrons_eta", "baselineElectrons_phi", "baselineElectrons_mass"});
    //==============================Clean Jets==============================================//
    //Use clean jets/bjets for object selections
    //=====================================================================================//

//-----------CHECKING THE OVERLAPS WITH THE TRAILING MUON----------#######////

    _rlm = _rlm.Define("Muonoverlap", checkoverlap, {"goodJets_4vecs","baselineMuon_4Vecs"});
    _rlm = _rlm.Define("Electronoverlap", checkoverlap, {"goodJets_4vecs","baselineElectron_4Vecs"});
    _rlm = _rlm.Define("muonjetoverlap", "Muonoverlap && Electronoverlap");
        _rlm =  _rlm.Define("Selected_jetpt", "goodJets_pt[muonjetoverlap]")
                .Define("Selected_jeteta", "goodJets_eta[muonjetoverlap]")
                .Define("Selected_jetphi", "goodJets_phi[muonjetoverlap]")
                .Define("Selected_jetmass", "goodJets_mass[muonjetoverlap]")
                .Define("Selected_jetbtag", "goodJets_deepjetbtag[muonjetoverlap]") 
                .Define("ncleanjetspass", "int(Selected_jetpt.size())")
                .Define("cleanjet4vecs", ::generate_4vec, {"Selected_jetpt", "Selected_jeteta", "Selected_jetphi", "Selected_jetmass"})
		.Define("centraljetpass", "abs(Selected_jeteta)<2.4")
		.Define("Central_jetpt", "Selected_jetpt[centraljetpass]")
		.Define("nCentral_jet","int(Central_jetpt.size())")
                .Define("Selected_jetHT", "Sum(Selected_jetpt)");
	_rlm = applyJetVetoMap(_rlm,"Selected_jeteta","Selected_jetphi").Filter("!vetoed_jets");
	if (!_isData){
        _rlm = _rlm.Define("Selected_jethadflav", "goodJets_hadflav[muonjetoverlap]");
	}
     //==============================Clean b-Jets==============================================// 
         //--> after remove overlap: use requested btaggedJets for btag-weight SFs && weight_generator. 
         //=====================================================================================//
        _rlm = _rlm.Define("btagcuts2", "Selected_jetbtag>0.3040") //medium wp -->as an example. 
                        .Define("Selected_bjetpt", "Selected_jetpt[btagcuts2]")
                        .Define("Selected_bjeteta", "Selected_jeteta[btagcuts2]")
                        .Define("Selected_bjetphi", "Selected_jetphi[btagcuts2]")
                        .Define("Selected_bjetmass", "Selected_jetmass[btagcuts2]")
			.Define("Selected_bjet_score", "Selected_jetbtag[btagcuts2]")
                        .Define("ncleanbjetspass", "int(Selected_bjetpt.size())")
                        .Define("Selected_bjetHT", "Sum(Selected_bjetpt)")
                        .Define("cleanbjet4vecs", ::generate_4vec, {"Selected_bjetpt", "Selected_bjeteta", "Selected_bjetphi", "Selected_bjetmass"});
        if (!_isData){
        _rlm = _rlm.Define("Selected_bjethadflav", "Selected_jethadflav[btagcuts2]");
        }
	_rlm = _rlm.Define("Topquark_Bjet_TL4Vecs", [](const ROOT::VecOps::RVec<float>& pts,
                               const ROOT::VecOps::RVec<float>& etas,
                               const ROOT::VecOps::RVec<float>& phis,
                               const ROOT::VecOps::RVec<float>& masses) -> ROOT::VecOps::RVec<TLorentzVector> {
	    ROOT::VecOps::RVec<TLorentzVector> vecs;
	    for (size_t i = 0; i < pts.size(); ++i) {
		TLorentzVector vec;
		vec.SetPtEtaPhiM(pts[i], etas[i], phis[i], masses[i]);
		vecs.emplace_back(vec);
	    }
	    return vecs;
	    }, {"Selected_bjetpt", "Selected_bjeteta", "Selected_bjetphi", "Selected_bjetmass"});

	_rlm = _rlm.Define("cleanjet_TL4Vecs", [](const ROOT::VecOps::RVec<float>& pts,
                               const ROOT::VecOps::RVec<float>& etas,
                               const ROOT::VecOps::RVec<float>& phis,
                               const ROOT::VecOps::RVec<float>& masses) -> ROOT::VecOps::RVec<TLorentzVector> {
            ROOT::VecOps::RVec<TLorentzVector> vecs;
            for (size_t i = 0; i < pts.size(); ++i) {
                TLorentzVector vec;
                vec.SetPtEtaPhiM(pts[i], etas[i], phis[i], masses[i]);
                vecs.emplace_back(vec);
            }
            return vecs;
            }, {"Selected_jetpt", "Selected_jeteta", "Selected_jetphi", "Selected_jetmass"});


}

void BaseAnalyser::calculateEvWeight(){

  //Scale Factors for BTag ID	
  int _case = 1;
  std::vector<std::string> Jets_vars_names = {"Selected_jethadflav", "Selected_jeteta",  "Selected_jetpt", "Selected_jetbtag"};  
  if(_case !=1){
    Jets_vars_names.emplace_back("Selected_jetbtag");
  }
  std::string output_btag_column_name = "btag_SF_";
//  _rlm = calculateBTagSF(_rlm, Jets_vars_names, _case, 0.2783, "M", output_btag_column_name);

  _rlm = calculateBTagSF(_rlm, Jets_vars_names, 0.3040,output_btag_column_name);

// #####------------ THIS IS THE CORRECTION THAT NEED TO IMPLEMENTED LATER--------######## 
  
  //Scale Factors for Muon HLT, RECO, ID and ISO
  std::vector<std::string> Muon_vars_names = {"goodMuons_eta", "goodMuons_pt"};
  std::string output_mu_column_name = "muon_SF_";
  _rlm = calculateMuSF(_rlm, Muon_vars_names, output_mu_column_name);


  //Scale Factors for Electron RECO and ID
  std::vector<std::string> Electron_vars_names = {"goodElectrons_eta", "goodElectrons_pt"};
  std::string output_ele_column_name = "ele_SF_";
  _rlm = calculateEleSF(_rlm, Electron_vars_names, output_ele_column_name);

//   _rlm = _rlm.Define("evWeight_wobtagSF", " pugenWeight * muon_SF_central * ele_SF_central"); 
//  _rlm = _rlm.Define("totbtagSF", "btag_SF_bcflav_central * btag_SF_lflav_central"); 
  //Total event Weight:

  //Prefiring Weight for 2016 and 2017
//  _rlm = applyPrefiringWeight(_rlm);
  //Total event Weight:
  //_rlm = _rlm.Define("evWeight", " pugenWeight * prefiring_SF_central * btag_SF_bcflav_central * btag_SF_lflav_central * muon_SF_central * ele_SF_central"); 
//  _rlm = _rlm.Define("evWeight", " pugenWeight * prefiring_SF_central * muon_SF_central * ele_SF_central"); 
  _rlm = _rlm.Define("evWeight", " pugenWeight ");  //prefiring_SF_central has been removed 
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ================================================================================
// @@@@@@@@@@@@@@@@@@@@ The leptton branch @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@



void BaseAnalyser::mergeLeptons() {
    cout << "merge electrons and muons" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    //-------------------------------------------------------
    // Define total number of leptons (sum of electrons and muons)
    //-------------------------------------------------------
    _rlm = _rlm.Define("totalLeptonCount", [](int electronCount, int muonCount) {
        return electronCount + muonCount;
    }, {"NbaselineElectrons", "NbaselineMuons"});

    //-------------------------------------------------------
    // Combine muon and electron 4-vectors
    //-------------------------------------------------------
    _rlm = _rlm.Define("combinedLepton4Vecs", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& muonVectors,
                                                 const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& electronVectors) {
        std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>> mergedVectors;
        mergedVectors.insert(mergedVectors.end(), muonVectors.begin(), muonVectors.end());
        mergedVectors.insert(mergedVectors.end(), electronVectors.begin(), electronVectors.end());
        return mergedVectors;
    }, {"baselineMuon_4Vecs", "baselineElectron_4Vecs"});

    //-------------------------------------------------------
    // Compute mass of 3-lepton system
    //-------------------------------------------------------
    _rlm = _rlm.Define("mass_of_3lepton", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& combined4Vecs) {
        if (combined4Vecs.size() == 3) {
            auto total4Vec = combined4Vecs[0] + combined4Vecs[1] + combined4Vecs[2];
            return total4Vec.M();
        }
        return -1.0; // Default value for events without exactly 3 leptons
    }, {"combinedLepton4Vecs"});


    //-------------------------------------------------------
    // Combine lepton pt, eta, phi properties
    //-------------------------------------------------------
    _rlm = _rlm.Define("combinedLeptonPt", [](const ROOT::VecOps::RVec<float>& muonPt,
                                              const ROOT::VecOps::RVec<float>& electronPt) {
        return ROOT::VecOps::Concatenate(muonPt, electronPt);
    }, {"baselineMuons_pt", "baselineElectrons_pt"});

    _rlm = _rlm.Define("combinedLeptonEta", [](const ROOT::VecOps::RVec<float>& muonEta,
                                               const ROOT::VecOps::RVec<float>& electronEta) {
        return ROOT::VecOps::Concatenate(muonEta, electronEta);
    }, {"baselineMuons_eta", "baselineElectrons_eta"});

    _rlm = _rlm.Define("combinedLeptonPhi", [](const ROOT::VecOps::RVec<float>& muonPhi,
                                               const ROOT::VecOps::RVec<float>& electronPhi) {
        return ROOT::VecOps::Concatenate(muonPhi, electronPhi);
    }, {"baselineMuons_phi", "baselineElectrons_phi"});
    
    

    _rlm = _rlm.Define("combinedLepton_isPrompt", [](const ROOT::VecOps::RVec<int>& muonIsPrompt,
                                                     const ROOT::VecOps::RVec<int>& electronIsPrompt) {
        return ROOT::VecOps::Concatenate(muonIsPrompt, electronIsPrompt);
    },
    {"tight_baselineMuons" , "tight_baselineElectrons"});

    _rlm = _rlm.Define("allTightLeptons",
    [](const ROOT::VecOps::RVec<int>& combinedLepton_isPrompt) {
        // Return true if ALL elements are 1 (prompt)
        return ROOT::VecOps::All(combinedLepton_isPrompt == 1);
    },
    {"combinedLepton_isPrompt"}
    );
    
    //-------------------------------------------------------
    // Define the number of combined 4-vectors in each event
    //-------------------------------------------------------
    _rlm = _rlm.Define("numCombinedLepton4Vecs", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& combined4Vecs) {
        return int(combined4Vecs.size());
    }, {"combinedLepton4Vecs"});

    //-------------------------------------------------------
    // Combine lepton charges
    //-------------------------------------------------------
    _rlm = _rlm.Define("combinedLeptonCharge",
                       [](const ROOT::VecOps::RVec<int>& muonCharges,
                          const ROOT::VecOps::RVec<int>& electronCharges) {
                           return ROOT::VecOps::Concatenate(muonCharges, electronCharges);
                       },
                       {"baselineMuons_charge" , "baselineElectrons_charge"});

    //-------------------------------------------------------
    // Assign flavor to leptons: 1 for muon, 0 for electron
    //-------------------------------------------------------
    _rlm = _rlm.Define("combinedLeptonFlavor", [](const ROOT::VecOps::RVec<int>& muonCharges,
                                                  const ROOT::VecOps::RVec<int>& electronCharges) {
        std::vector<int> flavor;
        flavor.insert(flavor.end(), muonCharges.size(), 1);  // 1 for muons
        flavor.insert(flavor.end(), electronCharges.size(), 0); // 0 for electrons
        return flavor;
    }, {"baselineMuons_charge", "baselineElectrons_charge"});

    //-------------------------------------------------------
    // Merge muons and electrons into a ROOT::VecOps::RVec<TLorentzVector>
    //-------------------------------------------------------
    _rlm = _rlm.Define("combinedLeptonTLorentzVecs",
                       [](const ROOT::VecOps::RVec<TLorentzVector>& muonTLVectors,
                          const ROOT::VecOps::RVec<TLorentzVector>& electronTLVectors) {
                           return ROOT::VecOps::Concatenate(muonTLVectors, electronTLVectors);
                       },
                       {"baselineMuons_TL4Vecs", "baselineElectrons_TL4Vecs"});

	// ==============================================
	// Define sorted indices with pT cuts
	// ==============================================
	_rlm = _rlm.Define("goodLepton_sorted_indices",
	    [](const ROOT::VecOps::RVec<float>& ptVec, int nLeptons) {
		if (nLeptons < 3) return ROOT::VecOps::RVec<size_t>{};

		// Create vector of indices [0, 1, 2, ...]
		ROOT::VecOps::RVec<size_t> indices(ptVec.size());
		std::iota(indices.begin(), indices.end(), 0);

		// Sort indices based on pt (descending)
		std::sort(indices.begin(), indices.end(),
			[&ptVec](size_t i, size_t j) { return ptVec[i] > ptVec[j]; });

		// Apply pT cuts: pt[0]>25, pt[1]>15, pt[2]>10
		if (ptVec[indices[0]] <= 25.f || ptVec[indices[1]] <= 15.f || ptVec[indices[2]] <= 10.f) {
		    return ROOT::VecOps::RVec<size_t>{}; // Reject event if cuts fail
		}

		return indices;
	    }, {"combinedLeptonPt", "numCombinedLepton4Vecs"});

	// ==============================================
	// Define all goodLepton properties with pT cuts
	// ==============================================

	// 1. Transverse momentum (pt)
	_rlm = _rlm.Define("goodLepton_pt",
	    [](const ROOT::VecOps::RVec<float>& ptVec,
	       const ROOT::VecOps::RVec<size_t>& indices,
	       int nLeptons) {
		return (nLeptons >= 3 && indices.size() >= 3) 
		       ? ROOT::VecOps::Take(ptVec, indices) 
		       : ROOT::VecOps::RVec<float>{};
	    }, {"combinedLeptonPt", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});
        _rlm = _rlm.Define("NgoodLepton", "int(goodLepton_pt.size())");
	// 2. Pseudorapidity (eta)
	_rlm = _rlm.Define("goodLepton_eta",
	    [](const ROOT::VecOps::RVec<float>& etaVec,
	       const ROOT::VecOps::RVec<size_t>& indices,
	       int nLeptons) {
		return (nLeptons >= 3 && indices.size() >= 3) 
		       ? ROOT::VecOps::Take(etaVec, indices) 
		       : ROOT::VecOps::RVec<float>{};
	    }, {"combinedLeptonEta", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	// 3. Azimuthal angle (phi)
	_rlm = _rlm.Define("goodLepton_phi",
	    [](const ROOT::VecOps::RVec<float>& phiVec,
	       const ROOT::VecOps::RVec<size_t>& indices,
	       int nLeptons) {
		return (nLeptons >= 3 && indices.size() >= 3) 
		       ? ROOT::VecOps::Take(phiVec, indices) 
		       : ROOT::VecOps::RVec<float>{};
	    }, {"combinedLeptonPhi", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	// 4. Prompt status (isPrompt)
	_rlm = _rlm.Define("goodLepton_isPrompt",
	    [](const ROOT::VecOps::RVec<int>& promptVec,
	       const ROOT::VecOps::RVec<size_t>& indices,
	       int nLeptons) {
		return (nLeptons >= 3 && indices.size() >= 3) 
		       ? ROOT::VecOps::Take(promptVec, indices) 
		       : ROOT::VecOps::RVec<int>{};
	    }, {"combinedLepton_isPrompt", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	// 5. Electric charge
	_rlm = _rlm.Define("goodLepton_charge",
	    [](const ROOT::VecOps::RVec<int>& chargeVec,
	       const ROOT::VecOps::RVec<size_t>& indices,
	       int nLeptons) {
		return (nLeptons >= 3 && indices.size() >= 3) 
		       ? ROOT::VecOps::Take(chargeVec, indices) 
		       : ROOT::VecOps::RVec<int>{};
	    }, {"combinedLeptonCharge", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	// 6. Flavor (e.g., 11 for electron, 13 for muon)
	_rlm = _rlm.Define("goodLepton_flavor",
	    [](const std::vector<int>& flavorVec,
	       const ROOT::VecOps::RVec<size_t>& indices,
	       int nLeptons) {
		if (nLeptons < 3 || indices.size() < 3) return std::vector<int>{};
		
		std::vector<int> sortedFlavor;
		sortedFlavor.reserve(indices.size());
		for (auto i : indices) sortedFlavor.push_back(flavorVec[i]);
		return sortedFlavor;
	    }, {"combinedLeptonFlavor", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	// 7. LorentzVectors (ROOT::Math::PtEtaPhiM4D)
	_rlm = _rlm.Define("goodLepton_4Vecs",
	    [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& vecs,
	       const ROOT::VecOps::RVec<size_t>& indices,
	       int nLeptons) {
		if (nLeptons < 3 || indices.size() < 3) 
		    return std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{};
		
		std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>> sortedVecs;
		sortedVecs.reserve(indices.size());
		for (auto i : indices) sortedVecs.push_back(vecs[i]);
		return sortedVecs;
	    }, {"combinedLepton4Vecs", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	// 8. TLorentzVectors
	_rlm = _rlm.Define("goodLepton_TL4Vecs",
	    [](const ROOT::VecOps::RVec<TLorentzVector>& vecs,
	       const ROOT::VecOps::RVec<size_t>& indices,
	       int nLeptons) {
		return (nLeptons >= 3 && indices.size() >= 3) 
		       ? ROOT::VecOps::Take(vecs, indices) 
		       : ROOT::VecOps::RVec<TLorentzVector>{};
	    }, {"combinedLeptonTLorentzVecs", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	_rlm = _rlm.Define("All_good_tightLeptons",
	    [](const ROOT::VecOps::RVec<int>& goodLepton_isPrompt) {
		// Only check if there are exactly 3 good leptons
		if (goodLepton_isPrompt.size() != 3) return false;
		
		// Return true if ALL 3 leptons are prompt (isPrompt == 1)
		return ROOT::VecOps::All(goodLepton_isPrompt == 1);
	    },
	    {"goodLepton_isPrompt"}  // Uses the *already-selected* good leptons
	);

	_rlm = _rlm.Define("leadingLepton_pt", 
	    [](const ROOT::VecOps::RVec<float>& ptVec) {
		return ptVec.size() == 3 ? ptVec[0] : -999.f;  // -999 as default if no leptons
	    }, {"goodLepton_pt"});

	_rlm = _rlm.Define("leadingLepton_eta", 
	    [](const ROOT::VecOps::RVec<float>& etaVec) {
		return etaVec.size() == 3 ? etaVec[0] : -999.f;
	    }, {"goodLepton_eta"});


        _rlm = _rlm.Define("subleadingLepton_pt",
            [](const ROOT::VecOps::RVec<float>& ptVec) {
                return ptVec.size() == 3 ? ptVec[1] : -999.f;  // -999 as default if no leptons
            }, {"goodLepton_pt"});

        _rlm = _rlm.Define("subleadingLepton_eta",
            [](const ROOT::VecOps::RVec<float>& etaVec) {
                return etaVec.size() == 3 ? etaVec[1] : -999.f;
            }, {"goodLepton_eta"});


        _rlm = _rlm.Define("TrailingLepton_pt",
            [](const ROOT::VecOps::RVec<float>& ptVec) {
                return ptVec.size() == 3 ? ptVec[2] : -999.f;  // -999 as default if no leptons
            }, {"goodLepton_pt"});

        _rlm = _rlm.Define("TrailingLepton_eta",
            [](const ROOT::VecOps::RVec<float>& etaVec) {
                return etaVec.size() == 3 ? etaVec[2] : -999.f;
            }, {"goodLepton_eta"});



}


void BaseAnalyser::DefineGoodLeptonGroups()
{
    // ============================
    // Good Leptons: 3-lepton case
    // ============================
    _rlm = _rlm.Define("is3LeptonEvent", "NgoodLepton == 3");

    _rlm = _rlm.Define("goodLepton3_pt", "is3LeptonEvent ? goodLepton_pt : ROOT::VecOps::RVec<float>{}")
               .Define("goodLepton3_eta", "is3LeptonEvent ? goodLepton_eta : ROOT::VecOps::RVec<float>{}")
               .Define("goodLepton3_phi", "is3LeptonEvent ? goodLepton_phi : ROOT::VecOps::RVec<float>{}")
               .Define("goodLepton3_charge", "is3LeptonEvent ? goodLepton_charge : ROOT::VecOps::RVec<int>{}")
               .Define("goodLepton3_flavor", "is3LeptonEvent ? goodLepton_flavor : std::vector<int>{}")
               .Define("goodLepton3_isPrompt", "is3LeptonEvent ? goodLepton_isPrompt : ROOT::VecOps::RVec<int>{}")
               .Define("goodLepton3_4Vecs", "is3LeptonEvent ? goodLepton_4Vecs : std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{}")
               .Define("goodLepton3_TL4Vecs", "is3LeptonEvent ? goodLepton_TL4Vecs : ROOT::VecOps::RVec<TLorentzVector>{}");

    // ============================
    // Good Leptons: 4-lepton case
    // ============================
    _rlm = _rlm.Define("is4LeptonEvent", "NgoodLepton == 4");

    _rlm = _rlm.Define("goodLepton4_pt", "is4LeptonEvent ? goodLepton_pt : ROOT::VecOps::RVec<float>{}")
               .Define("goodLepton4_eta", "is4LeptonEvent ? goodLepton_eta : ROOT::VecOps::RVec<float>{}")
               .Define("goodLepton4_phi", "is4LeptonEvent ? goodLepton_phi : ROOT::VecOps::RVec<float>{}")
               .Define("goodLepton4_charge", "is4LeptonEvent ? goodLepton_charge : ROOT::VecOps::RVec<int>{}")
               .Define("goodLepton4_flavor", "is4LeptonEvent ? goodLepton_flavor : std::vector<int>{}")
               .Define("goodLepton4_isPrompt", "is4LeptonEvent ? goodLepton_isPrompt : ROOT::VecOps::RVec<int>{}")
               .Define("goodLepton4_4Vecs", "is4LeptonEvent ? goodLepton_4Vecs : std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{}")
               .Define("goodLepton4_TL4Vecs", "is4LeptonEvent ? goodLepton_TL4Vecs : ROOT::VecOps::RVec<TLorentzVector>{}");
}





void BaseAnalyser::processOSSFPairs() {
    cout << "Process OSSF Pairs" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    //-------------------------------------------------------
    // Define OSSF pair selection and invariant mass calculation
    //-------------------------------------------------------
    _rlm = _rlm.Define("OSSF_info", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& lep4vecs,
                                       const ROOT::VecOps::RVec<int>& charges,
                                       const std::vector<int>& flavors) {
        const double Z_mass = 91.1876;
        const double window = 15.0;

        // Require exactly 3 leptons
       // if (lep4vecs.size() != 3 || charges.size() != 3 || flavors.size() != 3) {
        if (lep4vecs.size() != 3 && charges.size() != 3 && flavors.size() != 3) {
            return std::make_tuple(0, std::make_pair(-1, -1), -1.0);
        }

        std::vector<std::pair<int, int>> ossf_pairs;
        std::vector<double> ossf_masses;

        for (int i = 0; i < 3; ++i) {
            for (int j = i + 1; j < 3; ++j) {
                if (flavors[i] == flavors[j] && charges[i] != charges[j]) {
                    ossf_pairs.emplace_back(i, j);
                    ossf_masses.push_back((lep4vecs[i] + lep4vecs[j]).M());
                }
            }
        }

        if (ossf_pairs.empty()) {
            return std::make_tuple(0, std::make_pair(-1, -1), -1.0);
        }

        if (ossf_pairs.size() == 1) {
            double m = ossf_masses[0];
            if (std::abs(m - Z_mass) < window)
                return std::make_tuple(1, ossf_pairs[0], m);
            double m3l = (lep4vecs[0] + lep4vecs[1] + lep4vecs[2]).M();
            return std::make_tuple(std::abs(m3l - Z_mass) < window ? 2 : 3, ossf_pairs[0], m);
        }

        double closest_diff = 1e9;
        int closest_index = -1;
        for (size_t i = 0; i < ossf_masses.size(); ++i) {
            double diff = std::abs(ossf_masses[i] - Z_mass);
            if (diff < closest_diff) {
                closest_diff = diff;
                closest_index = i;
            }
        }

        if (closest_index != -1 && closest_diff < window)
            return std::make_tuple(1, ossf_pairs[closest_index], ossf_masses[closest_index]);

        double m3l = (lep4vecs[0] + lep4vecs[1] + lep4vecs[2]).M();
        return std::make_tuple(std::abs(m3l - Z_mass) < window ? 2 : 3,
                               closest_index != -1 ? ossf_pairs[closest_index] : std::make_pair(-1, -1),
                               closest_index != -1 ? ossf_masses[closest_index] : -1.0);
    }, {"goodLepton3_4Vecs", "goodLepton3_charge", "goodLepton3_flavor"});

    // Extract OSSF category
    _rlm = _rlm.Define("OSSF_category", [](const std::tuple<int, std::pair<int, int>, double>& info) {
        return std::get<0>(info);
    }, {"OSSF_info"});

    // Z boson mass for category == 1
    _rlm = _rlm.Define("zboson_mass", [](const std::tuple<int, std::pair<int, int>, double>& info) {
        return std::get<0>(info) == 1 ? std::get<2>(info) : -1.0;
    }, {"OSSF_info"});

    // OSSF mass for categories 2 and 3
    _rlm = _rlm.Define("nonZ_OSSF_mass", [](const std::tuple<int, std::pair<int, int>, double>& info) {
        int cat = std::get<0>(info);
        return (cat == 2 || cat == 3) ? std::get<2>(info) : -1.0;
    }, {"OSSF_info"});

    //mass of the 3 lepton where the ossf does not compatible with the z mass
    _rlm = _rlm.Define("mass_of_3_goodLepton_4BG", [](int category,
                                        const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& lep4vecs) {
    if ((category == 2 || category == 3) && lep4vecs.size() == 3) {
        return (lep4vecs[0] + lep4vecs[1] + lep4vecs[2]).M();
    }
    return -1.0;
    }, {"OSSF_category", "goodLepton3_4Vecs"});

    //Information about topLepton
    _rlm = _rlm.Define("topLepton_index", [](const std::tuple<int, std::pair<int, int>, double>& info) {
	    int category = std::get<0>(info);
	    std::pair<int, int> ossf_idx = std::get<1>(info);

	    if (category != 1) return -1;
	    for (int i = 0; i < 3; ++i) {
		if (i != ossf_idx.first && i != ossf_idx.second) return i;
	    }
	    return -1;
    }, {"OSSF_info"});

    // pt
    _rlm = _rlm.Define("topLepton_pt_new", [](int idx, const ROOT::VecOps::RVec<float>& pts) {
	return idx >= 0 ? pts[idx] : -1.0f;
    }, {"topLepton_index", "goodLepton3_pt"});

    // eta
    _rlm = _rlm.Define("topLepton_eta_new", [](int idx, const ROOT::VecOps::RVec<float>& etas) {
	return idx >= 0 ? etas[idx] : -99.0;
    }, {"topLepton_index", "goodLepton3_eta"});

    // phi
    _rlm = _rlm.Define("topLepton_phi_new", [](int idx, const ROOT::VecOps::RVec<float>& phis) {
	return idx >= 0 ? phis[idx] : -99.0;
    }, {"topLepton_index", "goodLepton3_phi"});

    // 4-vector
    _rlm = _rlm.Define("topLepton_4Vec_new", [](int idx, const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& vecs) {
	return idx >= 0 ? vecs[idx] : ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>{};
    }, {"topLepton_index", "goodLepton3_4Vecs"});

    // TLorentzVector
    _rlm = _rlm.Define("topLepton_TL4Vec_new", [](int idx, const ROOT::VecOps::RVec<TLorentzVector>& tlv) {
	return idx >= 0 ? tlv[idx] : TLorentzVector{};
    }, {"topLepton_index", "goodLepton3_TL4Vecs"});

    // charge
    _rlm = _rlm.Define("topLepton_charge_new", [](int idx, const ROOT::VecOps::RVec<int>& charges) {
	return idx >= 0 ? charges[idx] : 0;
    }, {"topLepton_index", "goodLepton3_charge"});

    // flavor
    _rlm = _rlm.Define("topLepton_flavor_new", [](int idx, const std::vector<int>& flavors) {
	return idx >= 0 ? flavors[idx] : -1;
    }, {"topLepton_index", "goodLepton3_flavor"});





    /////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    /////////////////// OSSF for 4 Lepton channel ///////////////////////
    /////////////////////////////////////////////////////////////////////

    _rlm = _rlm.Define("OSSF4L_info", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& lep4vecs,
                                     const ROOT::VecOps::RVec<int>& charges,
                                     const std::vector<int>& flavors) {
       const double Z_mass = 91.1876;
       const double window = 15.0;

       // Require exactly 4 leptons
       if (lep4vecs.size() != 4 || charges.size() != 4 || flavors.size() != 4) {
	   return std::make_tuple(0, std::make_pair(-1, -1), std::make_pair(-1, -1), -1.0, -1.0);
       }

       // Check if all charges are the same
       bool same_charge = std::all_of(charges.begin() + 1, charges.end(),
				      [&](int c) { return c == charges[0]; });
       if (same_charge) {
	   return std::make_tuple(0, std::make_pair(-1, -1), std::make_pair(-1, -1), -1.0, -1.0);
       }

       std::vector<std::pair<int, int>> ossf_pairs;
       std::vector<double> ossf_masses;

       for (int i = 0; i < 4; ++i) {
	   for (int j = i + 1; j < 4; ++j) {
	       if (flavors[i] == flavors[j] && charges[i] != charges[j]) {
		   ossf_pairs.emplace_back(i, j);
		   ossf_masses.push_back((lep4vecs[i] + lep4vecs[j]).M());
	       }
	   }
       }

       if (ossf_pairs.empty()) {
	   return std::make_tuple(0, std::make_pair(-1, -1), std::make_pair(-1, -1), -1.0, -1.0);
       }

       // Find the best (closest to Z) pair
       int best_idx = -1;
       double closest_diff = 1e9;
       for (size_t i = 0; i < ossf_masses.size(); ++i) {
	   double diff = std::abs(ossf_masses[i] - Z_mass);
	   if (diff < closest_diff) {
	       closest_diff = diff;
	       best_idx = i;
	   }
       }

       if (best_idx == -1 || closest_diff > window) {
	   return std::make_tuple(0, std::make_pair(-1, -1), std::make_pair(-1, -1), -1.0, -1.0);
       }

       auto best_pair = ossf_pairs[best_idx];
       double best_mass = ossf_masses[best_idx];

       // Check for second non-overlapping pair
       for (size_t i = 0; i < ossf_pairs.size(); ++i) {
	   if ((int)i == best_idx) continue;

	   auto& p = ossf_pairs[i];
	   double m = ossf_masses[i];
	   if (std::abs(m - Z_mass) < window) {
	       // Ensure no index overlap
	       if (p.first != best_pair.first && p.first != best_pair.second &&
		   p.second != best_pair.first && p.second != best_pair.second) {
		   return std::make_tuple(2, best_pair, p, best_mass, m);  // Category 2
	       }
	   }
       }

       return std::make_tuple(1, best_pair, std::make_pair(-1, -1), best_mass, -1.0);  // Category 1

    }, {"goodLepton4_4Vecs", "goodLepton4_charge", "goodLepton4_flavor"});


    _rlm = _rlm.Define("OSSF4L_category", [](const std::tuple<int, std::pair<int, int>, std::pair<int, int>, double, double>& info) {
       return std::get<0>(info);
    }, {"OSSF4L_info"});

    // Best Z candidate mass (category 1 or 2)
    _rlm = _rlm.Define("OSSF4L_bestZ_mass", [](const std::tuple<int, std::pair<int, int>, std::pair<int, int>, double, double>& info) {
	int cat = std::get<0>(info);
	double best_mass = std::get<3>(info);
	return (cat == 1 || cat == 2) ? best_mass : -1.0;
    }, {"OSSF4L_info"});

    // Second Z candidate mass (only for category 2)
    _rlm = _rlm.Define("OSSF4L_secondZ_mass", [](const std::tuple<int, std::pair<int, int>, std::pair<int, int>, double, double>& info) {
	int cat = std::get<0>(info);
	double second_mass = std::get<4>(info);
	return (cat == 2) ? second_mass : -1.0;
    }, {"OSSF4L_info"});

    _rlm = _rlm.Define("mass_of_4L", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& lep4vecs) {
	if (lep4vecs.size() != 4) return -1.0;
	auto total = lep4vecs[0] + lep4vecs[1] + lep4vecs[2] + lep4vecs[3];
	return total.M();
    }, {"goodLepton4_4Vecs"});





}




//MET

void BaseAnalyser::selectMET()
{
    if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

    _rlm = _rlm.Define("goodMET","MET_pt>20");
              // .Define("goodMET_pt","MET_pt[goodMET]")
	      // .Define("goodMET_phi", "ROOT::VecOps::RVec<float>{MET_phi}[goodMET]");
               //.Define("goodMET_phi","MET_phi[goodMET]");
                //.Define("goodMET_phi","MET_phi[goodMET]")
                //.Define("NgoodMET","int(goodMET_pt.size())");
    //_rlm = _rlm.Define("goodMet", "MET_sumEt>600 && MET_pt>5");
    //_rlm = _rlm.Define("goodMet_pt", "MET_pt[goodMet]");

    
}
/*
void BaseAnalyser::reconstructWboson()
{
    if (debug){
    std::cout<<std::endl;
    std::cout<< "================================//=================================" << std::endl;
    std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
    std::cout<< "================================//=================================" << std::endl;
    }
    //-------------------- Reconstruct neutrino ---------------------

    std::cout<<"Reconstructing neutrino from MET"<<std::endl;
    _rlm = _rlm.Define("nu_pt", "MET_pt")
               .Define("nu_phi", "MET_phi")
               .Define("nu_phi_double", "static_cast<double>(nu_phi)")

               .Define("nu_px", "nu_pt*cos(nu_phi)")
               .Define("nu_py", "nu_pt*sin(nu_phi)");
   
    _rlm = _rlm.Define("lambda_reco", ::calculateLambda, {"topLepton_TLorentzVector", "nu_pt", "nu_phi"});

    _rlm = _rlm.Define("delta_reco", ::calculateDelta, {"topLepton_TLorentzVector", "nu_pt", "lambda_reco"})
               .Define("isRealSolution", "delta_reco > 0 ? 1 : -1");

    _rlm = _rlm.Define("nu_pz", ::calculate_nu_z, {"topLepton_TLorentzVector", "lambda_reco", "delta_reco", "nu_pt", "nu_phi"});

    _rlm = _rlm.Define("nu_energy", ::calculate_nu_energy, {"nu_pt", "nu_phi", "nu_pz"});

    _rlm = _rlm.Define("nu_TL4vec", ::get_neutrino_TL4vec, {"nu_pt", "nu_phi", "nu_pz", "nu_energy"});


    // for (const auto &delta_reco: _rlm.Take<float>("delta_reco"))
    // {
    //     std::cout << "delta_reco: " << delta_reco << "\n";
    // }

    //--------------------- Reconstruct W boson ---------------------

    _rlm = _rlm.Define("Wboson_4vec", ::reconstructWboson_TL4vec, {"topLepton_TLorentzVector", "nu_TL4vec"})
               .Define("w_mass","Wboson_4vec.M()")
	       .Define("w_eta","Wboson_4vec.Eta()")
	       .Define("w_phi","Wboson_4vec.Phi()")

               .Define("w_pt","Wboson_4vec.Pt()");

    // _rlm = _rlm.Define("Wboson_transversMass", "Wboson_4vec.Mt()");
    _rlm = _rlm.Define("topLepton_phi", "topLepton_TLorentzVector.Phi()");
    _rlm = _rlm.Define("topLepton_eta", "topLepton_TLorentzVector.Eta()");
    _rlm = _rlm.Define("topLepton_pt", "topLepton_TLorentzVector.Pt()");

    // _rlm = _rlm.Define("Wboson_transversMass", "sqrt(pow(topLepton_4vecs.Pt()+nu_pt,2)-pow(nu_pt*cos(nu_phi)+topLepton_4vecs.Px(),2) - pow(nu_pt*sin(nu_phi)+topLepton_4vecs.Py(),2))");
    _rlm = _rlm.Define("delta_phi_lep_nu", ::calculate_deltaPhi_scalars, {"topLepton_phi", "nu_phi_double"})
               .Define("Wboson_transversMass", "sqrt(2*topLepton_TLorentzVector.Pt()*nu_pt*(1-cos(delta_phi_lep_nu)))");
   // _rlm = _rlm.Filter("Wboson_transversMass > 0", "Events with invariant mass close to Z boson mass");   

}
*/

void BaseAnalyser::reconstructWboson()
{
    if (debug) {
        std::cout << std::endl;
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    //-------------------- Reconstruct neutrino ---------------------
    std::cout << "Reconstructing neutrino from MET" << std::endl;
    _rlm = _rlm.Define("nu_pt", "MET_pt")
               .Define("nu_phi", "MET_phi")
               .Define("nu_phi_double", "static_cast<double>(nu_phi)")
               .Define("nu_px", "nu_pt * cos(nu_phi)")
               .Define("nu_py", "nu_pt * sin(nu_phi)");

    _rlm = _rlm.Define("lambda_reco", ::calculateLambda, {"topLepton_TL4Vec_new", "nu_pt", "nu_phi"});

    _rlm = _rlm.Define("delta_reco", ::calculateDelta, {"topLepton_TL4Vec_new", "nu_pt", "lambda_reco"})
               .Define("isRealSolution", "delta_reco > 0 ? 1 : -1");

    _rlm = _rlm.Define("nu_pz", ::calculate_nu_z, {"topLepton_TL4Vec_new", "lambda_reco", "delta_reco", "nu_pt", "nu_phi"});

    _rlm = _rlm.Define("nu_energy", ::calculate_nu_energy, {"nu_pt", "nu_phi", "nu_pz"});

    _rlm = _rlm.Define("nu_TL4vec", ::get_neutrino_TL4vec, {"nu_pt", "nu_phi", "nu_pz", "nu_energy"});

    //--------------------- Reconstruct W boson ---------------------
    _rlm = _rlm.Define("Wboson_4vec", ::reconstructWboson_TL4vec, {"topLepton_TL4Vec_new", "nu_TL4vec"})
               .Define("w_mass", "Wboson_4vec.M()")
               .Define("w_eta", "Wboson_4vec.Eta()")
               .Define("w_phi", "Wboson_4vec.Phi()")
               .Define("w_pt", "Wboson_4vec.Pt()");

    // Calculate transverse mass of the W boson
    _rlm = _rlm.Define("topLepton_phi", "topLepton_TL4Vec_new.Phi()")
               .Define("topLepton_eta", "topLepton_TL4Vec_new.Eta()")
               .Define("topLepton_pt", "topLepton_TL4Vec_new.Pt()")
               .Define("delta_phi_lep_nu", ::calculate_deltaPhi_scalars, {"topLepton_phi", "nu_phi_double"})
               .Define("Wboson_transversMass", "sqrt(2 * topLepton_TL4Vec_new.Pt() * nu_pt * (1 - cos(delta_phi_lep_nu)))");
}

/*
void BaseAnalyser::reconstructTop()
{
    if (debug){
        std::cout << std::endl;
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    //-------------------------------------------------------
    // Reconstruct the top quark by combining W boson and b-jet 4-vectors
    //-------------------------------------------------------
    _rlm = _rlm.Define("topQuark_TL4vec",
        [](const ROOT::VecOps::RVec<TLorentzVector>& bjet_vecs, const TLorentzVector& w_boson_4vec) -> TLorentzVector {
            TLorentzVector best_top;
            double min_mass_diff = std::numeric_limits<double>::max(); // Set an initial large value for min mass difference

            const double top_mass = 172.76; // Mass of top quark in GeV (can be adjusted as needed)

            for (const auto& bjet : bjet_vecs) {
                TLorentzVector candidate_top = w_boson_4vec + bjet;
                double mass_diff = std::abs(candidate_top.M() - top_mass); // Calculate mass difference from the top quark mass

                // Update the best_top if this candidate has a smaller mass difference
                if (mass_diff < min_mass_diff) {
                    best_top = candidate_top;
                    min_mass_diff = mass_diff;
                }
            }

            return best_top; // Return the 4-vector of the best top candidate
        }, {"top_Bjet_TL4Vecs", "Wboson_4vec"});


    //-------------------------------------------------------
    // Calculate the top mass and filter the events based on it
    //-------------------------------------------------------
    _rlm = _rlm.Define("top_mass_all", "topQuark_TL4vec.M()");
              // .Filter("top_mass > 0", "Events with top mass")
    _rlm = _rlm.Define("top_mass", [](double top_mass_all) {
			       return top_mass_all >0;
			     }, {"top_mass_all"});
    _rlm = _rlm.Define("top_pt", "topQuark_TL4vec.Pt()")
	       .Define("top_phi", "topQuark_TL4vec.Phi()")
               .Define("top_eta", "topQuark_TL4vec.Eta()");

}
*/
void BaseAnalyser::reconstructTop()
{
    if (debug) {
        std::cout << std::endl;
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    //-------------------------------------------------------
    // Reconstruct the top quark by combining W boson and b-jet 4-vectors
    //-------------------------------------------------------
/*    _rlm = _rlm.Define("topQuark_TL4vec",
        [](const ROOT::VecOps::RVec<TLorentzVector>& bjet_vecs, const TLorentzVector& w_boson_4vec) -> TLorentzVector {
            TLorentzVector best_top;
            double min_mass_diff = std::numeric_limits<double>::max(); // Set an initial large value for min mass difference

            const double top_mass = 172.76; // Mass of top quark in GeV (can be adjusted as needed)

            for (const auto& bjet : bjet_vecs) {
                TLorentzVector candidate_top = w_boson_4vec + bjet;
                double mass_diff = std::abs(candidate_top.M() - top_mass); // Calculate mass difference from the top quark mass

                // Update the best_top if this candidate has a smaller mass difference
                if (mass_diff < min_mass_diff) {
                    best_top = candidate_top;
                    min_mass_diff = mass_diff;
                }
            }

            return best_top; // Return the 4-vector of the best top candidate
        }, {"Topquark_Bjet_TL4Vecs", "Wboson_4vec"});*/
    _rlm = _rlm.Define("topQuark_info",
	    [](const ROOT::VecOps::RVec<TLorentzVector>& bjet_vecs, const TLorentzVector& w_boson_4vec) {
		TLorentzVector best_top;
		TLorentzVector best_bjet;
		double min_mass_diff = std::numeric_limits<double>::max();
		const double top_mass = 172.76; // GeV

		for (const auto& bjet : bjet_vecs) {
		    TLorentzVector candidate_top = w_boson_4vec + bjet;
		    double mass_diff = std::abs(candidate_top.M() - top_mass);

		    if (mass_diff < min_mass_diff) {
			best_top = candidate_top;
			best_bjet = bjet;
			min_mass_diff = mass_diff;
		    }
		}

		// Return a pair: (top_4vec, bjet_4vec)
		return std::make_pair(best_top, best_bjet);
	    }, {"Topquark_Bjet_TL4Vecs", "Wboson_4vec"});

    // Now split the pair into separate columns
    _rlm = _rlm.Define("topQuark_TL4vec", "topQuark_info.first")
	      .Define("topQuark_bjet_TL4vec", "topQuark_info.second");

    //-------------------------------------------------------
    // Calculate the top mass and filter the events based on it
    //-------------------------------------------------------
    _rlm = _rlm.Define("top_mass", "topQuark_TL4vec.M()")
               .Define("top_pt", "topQuark_TL4vec.Pt()")
               .Define("top_phi", "topQuark_TL4vec.Phi()")
               .Define("top_eta", "topQuark_TL4vec.Eta()");
    _rlm = _rlm.Define("top_bjet_mass", "topQuark_bjet_TL4vec.M()")
	       .Define("top_bjet_pt", "topQuark_bjet_TL4vec.Pt()")
	       .Define("top_bjet_phi", "topQuark_bjet_TL4vec.Phi()")
	       .Define("top_bjet_eta", "topQuark_bjet_TL4vec.Eta()");
}


void BaseAnalyser::BDT_variables()
{
    if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

    _rlm = _rlm.Define("sum_selectedJet_pt", [](const ROOT::VecOps::RVec<float>& jet_pts) {
                   float sum = 0.0;
                   for (auto pt : jet_pts) sum += pt;
                   return sum;
               }, {"Selected_jetpt"})

               .Define("sum_lepton_MET_pt", [](const ROOT::VecOps::RVec<float>& lepton_pts, float met_pt) {
                   float sum = met_pt;
                   for (auto pt : lepton_pts) sum += pt;
                   return sum;
               }, {"goodLepton3_pt", "MET_pt"});

    _rlm = _rlm.Define("Selected_jeteta_maxAbs", [](const ROOT::VecOps::RVec<float>& etas) {
	    if (etas.empty()) return -999.0f;
	    return *std::max_element(etas.begin(), etas.end(), [](float a, float b) {
		return std::abs(a) < std::abs(b);
	    });
	}, {"Selected_jeteta"});


    _rlm = _rlm.Define("RecoilingJet_index", [](const ROOT::RVec<float>& jetpt, const ROOT::RVec<int>& bmask) {
        int idx = -1;
        float maxpt = -1;
        for (size_t i = 0; i < jetpt.size(); ++i) {
            if (!bmask[i] && jetpt[i] > maxpt) {
                maxpt = jetpt[i];
                idx = i;
            }
        }
        return idx;
        }, {"Selected_jetpt", "btagcuts2"})

             .Define("RecoilingJet_pt", "RecoilingJet_index >= 0 ? Selected_jetpt[RecoilingJet_index] : -1")
	     .Define("RecoilingJet_phi", "RecoilingJet_index >= 0 ? Selected_jetphi[RecoilingJet_index] : -99")
             .Define("RecoilingJet_eta", "RecoilingJet_index >= 0 ? Selected_jeteta[RecoilingJet_index] : -99");

    _rlm = _rlm.Define("RecoilingJet_TL4Vec",
	[](int recoil_idx, const ROOT::VecOps::RVec<TLorentzVector>& clean_jets) {
	    // Return empty {} initialized TLorentzVector if invalid index
	    return (recoil_idx >= 0) ? clean_jets[recoil_idx] : TLorentzVector{};
	},
	{"RecoilingJet_index", "cleanjet_TL4Vecs"});
	
    _rlm = _rlm.Define("dphi_lepZ_OSSF", [](const std::tuple<int, std::pair<int, int>, double>& ossf_info,
                                        const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& lep4vecs) {
	    int category = std::get<0>(ossf_info);
	    auto indices = std::get<1>(ossf_info);

	    if (category != 1 || indices.first == -1 || indices.second == -1) {
		return -999.0;
	    }

	    const auto& lep1 = lep4vecs[indices.first];
	    const auto& lep2 = lep4vecs[indices.second];

	    return std::abs(ROOT::Math::VectorUtil::DeltaPhi(lep1, lep2));
	}, {"OSSF_info", "goodLepton3_4Vecs"});

    _rlm = _rlm.Define("dR_bjet_lepton_info", 
	[](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& bjets,
	   const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons) 
	-> std::pair<float, float> {  // Returns (min_dR, max_dR)
	    
	    // Initialize with extreme values
	    float min_dR = 999.f;
	    float max_dR = -1.f;

	    // Only calculate if we have both bjets and leptons
	    if (!bjets.empty() && !leptons.empty()) {
		for (const auto& bjet : bjets) {
		    for (const auto& lepton : leptons) {
			float dR = ROOT::Math::VectorUtil::DeltaR(bjet, lepton);
			
			// Update min and max
			if (dR < min_dR) min_dR = dR;
			if (dR > max_dR) max_dR = dR;
		    }
		}
	    } else {
		// Return invalid values if no bjets or leptons
		min_dR = -1.f;
		max_dR = -1.f;
	    }

	    return {min_dR, max_dR};
	}, 
	{"cleanbjet4vecs", "goodLepton3_4Vecs"})

    // Split into separate branches
    .Define("min_dR_bjet_lepton", "dR_bjet_lepton_info.first")   // Smallest ΔR(b,l)
    .Define("max_dR_bjet_lepton", "dR_bjet_lepton_info.second"); // Largest ΔR(b,l)

_rlm = _rlm.Define("jet_correlations",
    [](const ROOT::VecOps::RVec<TLorentzVector>& jets) {
        float max_dphi = -1.0f;
        float max_ptjj = -1.0f;
        float max_mjj = -1.0f;
        const int njets = jets.size();
        
        if (njets >= 2) {
            for (int i = 0; i < njets; ++i) {
                for (int j = i+1; j < njets; ++j) {
                    TLorentzVector dijet = jets[i] + jets[j];
                    // Explicitly cast to float to avoid type mismatch
                    float dphi = static_cast<float>(jets[i].DeltaPhi(jets[j]));
                    float ptjj = static_cast<float>(dijet.Pt());
                    float mjj = static_cast<float>(dijet.M());
                    
                    max_dphi = std::max(max_dphi, dphi);
                    max_ptjj = std::max(max_ptjj, ptjj);
                    max_mjj = std::max(max_mjj, mjj);
                }
            }
        }
        return std::make_tuple(max_dphi, max_ptjj, max_mjj);
    }, {"cleanjet_TL4Vecs"});


    // Then define the individual variables
    _rlm = _rlm.Define("max_dphi_jj", "std::get<0>(jet_correlations)")
	      .Define("max_ptjj", "std::get<1>(jet_correlations)")
	      .Define("max_mjj", "std::get<2>(jet_correlations)");
    _rlm = _rlm.Define("max_dphi_jj_confirm", 
	    [](const ROOT::VecOps::RVec<TLorentzVector>& jets) {
		float max_dphi = -1.0;
		const int njets = jets.size();
		if (njets < 2) return max_dphi;
		
		for (int i = 0; i < njets; ++i) {
		    for (int j = i+1; j < njets; ++j) {
			float dphi = jets[i].DeltaPhi(jets[j]);
			if (dphi > max_dphi) max_dphi = dphi;
		    }
		}
		return max_dphi;
	    }, {"cleanjet_TL4Vecs"});
   //lepton assymetry 
    _rlm = _rlm.Define("topLepton_absEta_times_charge", 
	[](int idx, const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<int>& charges) {
	    if (idx < 0) return 99.f;  // Default value when no lepton is found
	    return std::abs(etas[idx]) * charges[idx];
	}, 
	{"topLepton_index", "goodLepton3_eta", "goodLepton3_charge"});

	_rlm = _rlm.Define("mass_3lepton", 
	[](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons) {
	    if (leptons.size() != 3) return -1.0;
	    auto total = leptons[0] + leptons[1] + leptons[2];
	    return total.M();
	}, {"goodLepton3_4Vecs"});

    _rlm = _rlm.Define("dR_b_recoilJet", 
	    [](const TLorentzVector& bjet, int recoil_idx, 
	       const ROOT::VecOps::RVec<float>& jet_pt,
	       const ROOT::VecOps::RVec<float>& jet_eta,
	       const ROOT::VecOps::RVec<float>& jet_phi,
	       const ROOT::VecOps::RVec<float>& jet_mass) {
		
		if (recoil_idx < 0) return -1.f; // No recoiling jet
		
		TLorentzVector recoilJet;
		recoilJet.SetPtEtaPhiM(
		    jet_pt[recoil_idx], 
		    jet_eta[recoil_idx], 
		    jet_phi[recoil_idx], 
		    jet_mass[recoil_idx]
		);
		
		return static_cast<float>(bjet.DeltaR(recoilJet));
	    }, 
	    {"topQuark_bjet_TL4vec", "RecoilingJet_index", 
	     "Selected_jetpt", "Selected_jeteta", 
	     "Selected_jetphi", "Selected_jetmass"})

	// ΔR(b, lepton)
	.Define("dR_b_lepton", 
	    [](const TLorentzVector& bjet, const TLorentzVector& lepton) {
		return static_cast<float>(bjet.DeltaR(lepton));
	    }, 
	    {"topQuark_bjet_TL4vec", "topLepton_TL4Vec_new"});


/*    _rlm = _rlm.Define("cosTheta_Polarization_angle", 
	[](const TLorentzVector& spectator, 
	   const TLorentzVector& lepton,
	   const TLorentzVector& top) {
	    return calculateTopPolarizationAngle(spectator, lepton, top);
	},
	{"RecoilingJet_TL4Vec", "topLepton_TL4Vec_new", "topQuark_TL4vec"});*/

    _rlm = _rlm.Define("cosTheta_Polarization_angle",
	[](const TLorentzVector& spectator,
	   const TLorentzVector& lepton,
	   const TLorentzVector& top) {
	    if (top.M() != 0){
		float result = calculateTopPolarizationAngle(spectator, lepton, top);
		return (result == 1.0f) ? -999.0f : result;
	    } else {
		return -999.0f;
	    }
	},
	{"RecoilingJet_TL4Vec", "topLepton_TL4Vec_new", "topQuark_TL4vec"});

}



///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////// SIGNAL REGION AND CONTROL REGION ////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

void BaseAnalyser::defineSignalRegion()
{
    cout << "Defining Signal Region" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }


    _rlm = _rlm.Define("trialRegion", " NgoodLepton==3 && ncleanjetspass >= 2 && ncleanbjetspass >= 1 && All_good_tightLeptons && MET_pt>20")
	       .Define("trialRegion_lead" , "trialRegion && leadingLepton_pt > 0")
	       .Define("trialRegion_sublead" , "trialRegion && subleadingLepton_pt > 0")
	       .Define("trialRegion_trail" , "trialRegion && TrailingLepton_pt > 0")
	       .Define("trialRegion_top_lepton" , "trialRegion && topLepton_pt_new > 0") 
               .Define("TR_leadingLepton_pt", 
                   [](bool cond, float pt) { return cond ? pt : -999.f; },
                   {"trialRegion_lead", "leadingLepton_pt"})
               .Define("TR_subleadingLepton_pt",
                   [](bool cond, float pt) { return cond ? pt : -999.f; },
                   {"trialRegion_sublead", "subleadingLepton_pt"})
               .Define("TR_trailingLepton_pt",
                   [](bool cond, float pt) { return cond ? pt : -999.f; },
                   {"trialRegion_trail", "TrailingLepton_pt"})
               .Define("TR_leadingLepton_eta",
                   [](bool cond, float eta) { return cond ? eta : -999.f; },
                   {"trialRegion_lead", "leadingLepton_eta"})
               .Define("TR_subleadingLepton_eta",
                   [](bool cond, float eta) { return cond ? eta : -999.f; },
                   {"trialRegion_sublead", "subleadingLepton_eta"})
	       .Define("TR_topLepton_pt",
                   [](bool cond, float eta) { return cond ? eta : -999.f; },
                   {"trialRegion_top_lepton", "topLepton_pt_new"})
               .Define("TR_trailingLepton_eta",
                   [](bool cond, float eta) { return cond ? eta : -999.f; },
                   {"trialRegion_trail", "TrailingLepton_eta"});       
    
    _rlm = _rlm.Define("baseRegion", " NgoodLepton==3 && All_good_tightLeptons && MET_pt>20")
	       .Define("SignalRegion", "baseRegion && ncleanjetspass >= 2 && ncleanbjetspass >= 1 && OSSF_category ==1")
	       .Define("SignalRegion_tzq", "SignalRegion && nCentral_jet < 4")
	       .Define("SignalRegion_ttz", "SignalRegion && nCentral_jet >= 4")
	       .Define("WZ_Region", "baseRegion && ncleanbjetspass == 0 && OSSF_category ==1 && MET_pt > 50")
	       .Define("X_gamma_Region", "baseRegion && OSSF_category ==2 && nonZ_OSSF_mass > 35 && nonZ_OSSF_mass < 76")
	       .Define("NP_2_Region", "baseRegion && OSSF_category ==3 && nonZ_OSSF_mass > 35 && ncleanjetspass >= 2 && ncleanjetspass <= 3 && ncleanbjetspass == 1")
	       .Define("NP_1_Region", "baseRegion && OSSF_category ==0 && ncleanjetspass >= 2 && ncleanjetspass <= 3 && ncleanbjetspass == 1");

    _rlm = _rlm.Define("baseRegion_4L", "NgoodLepton==4 && OSSF4L_category != 0")
	       .Define("ZZ_Region", "baseRegion_4L && OSSF4L_category==2")
	       .Define("ttZ_Region", "baseRegion_4L && OSSF4L_category==1 && nCentral_jet >= 2"); 
/////////////////////////////////////////////
/////////////////////////////////////////////
//Variable for different region//////////////
////////////////////////////////////////////
    // 3-lepton regions
    _rlm = _rlm.Define("ncleanjetspass_SignalRegion", "SignalRegion ? ncleanjetspass : -1")
	    .Define("ncleanbjetspass_SignalRegion", "SignalRegion ? ncleanbjetspass : -1")

	    .Define("ncleanjetspass_WZ_Region", "WZ_Region ? ncleanjetspass : -1")
	    .Define("Wboson_transversMass_WZ_Region", "WZ_Region ? Wboson_transversMass : -1")

	    .Define("ncleanjetspass_X_gamma_Region", "X_gamma_Region ? ncleanjetspass : -1")
	    .Define("ncleanbjetspass_X_gamma_Region", "X_gamma_Region ? ncleanbjetspass : -1")

	    .Define("ncleanjetspass_NP_2_Region", "NP_2_Region ? ncleanjetspass : -1")
	    .Define("ncleanbjetspass_NP_2_Region", "NP_2_Region ? ncleanbjetspass : -1")

	    .Define("ncleanjetspass_NP_1_Region", "NP_1_Region ? ncleanjetspass : -1")
	    .Define("ncleanbjetspass_NP_1_Region", "NP_1_Region ? ncleanbjetspass : -1");

    // 4-lepton regions
    _rlm = _rlm.Define("ncleanjetspass_ZZ_Region", "ZZ_Region ? ncleanjetspass : -1")
	    .Define("mass_of_4L_ZZ_Region", "ZZ_Region ? mass_of_4L : -1")
	    .Define("Z_mass1_ZZ_Region", "ZZ_Region ? OSSF4L_bestZ_mass : -1")
            .Define("Z_mass2_ZZ_Region", "ZZ_Region ? OSSF4L_secondZ_mass : -1")

	    .Define("ncleanjetspass_ttZ_Region", "ttZ_Region ? ncleanjetspass : -1")
	    .Define("ncleanbjetspass_ttZ_Region", "ttZ_Region ? ncleanbjetspass : -1");

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////BDT VARIABLE IN DIFFERENT REGION/////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
    _rlm = _rlm.Define("nJet_tzq", "SignalRegion_tzq ? ncleanjetspass : -1")
	       .Define("nBJets_tzq", "SignalRegion_tzq ? ncleanbjetspass : -1")
	       .Define("mWT_tzq", "SignalRegion_tzq ? Wboson_transversMass : -1")
	       .Define("mTop_tzq", "SignalRegion_tzq ? top_mass : -1")
	       .Define("mZ_tzq", "SignalRegion_tzq ? zboson_mass : -1")
	       .Define("dphi_ll_z_tzq", "SignalRegion_tzq ? dphi_lepZ_OSSF : -10")
	       .Define("cosThetaPol_tzq", "SignalRegion_tzq ? cosTheta_Polarization_angle : -9")
	       .Define("sumHadPt_tzq", "SignalRegion_tzq ? sum_selectedJet_pt : -1")
	       .Define("sumLepMetPt_tzq", "SignalRegion_tzq ? Selected_jeteta_maxAbs : -1")
	       .Define("min_dR_bl_tzq", "SignalRegion_tzq ? min_dR_bjet_lepton : -10")
	       .Define("max_dR_bl_tzq", "SignalRegion_tzq ? max_dR_bjet_lepton : -10")
	       .Define("max_dphi_jj_tzq", "SignalRegion_tzq ? max_dphi_jj : -10")
	       .Define("max_ptjj_tzq", "SignalRegion_tzq ? max_ptjj : -10")
	       .Define("max_mjj_tzq", "SignalRegion_tzq ? max_mjj : -10")
	       .Define("mass3l_tzq", "SignalRegion_tzq ? mass_3lepton : -10")
	       .Define("dR_b_recoil_tzq", "SignalRegion_tzq ? dR_b_recoilJet : -10")
	       .Define("dR_b_l_tzq", "SignalRegion_tzq ? dR_b_lepton : -10")
	       .Define("maxJetAbsEta_tzq", "SignalRegion_tzq ? Selected_jeteta_maxAbs : -10")
	       .Define("etaRecoilingJet_tzq", "SignalRegion_tzq ? RecoilingJet_eta : -99.0")
	       .Define("lep_asymmetry_tzq", "SignalRegion_tzq ? topLepton_absEta_times_charge : -9.0")
	       .Define("maxDEEPJET_tzq", "SignalRegion_tzq ? Selected_bjet_score : ROOT::VecOps::RVec<float>{}")
	       .Define("MET_pt_tzq", "SignalRegion_tzq ? MET_pt : -10");


    _rlm = _rlm.Define("nJet_ttz", "SignalRegion_ttz ? ncleanjetspass : -1")
	       .Define("nBJets_ttz", "SignalRegion_ttz ? ncleanbjetspass : -1")
	       .Define("mWT_ttz", "SignalRegion_ttz ? Wboson_transversMass : -1")
	       .Define("mTop_ttz", "SignalRegion_ttz ? top_mass : -1")
	       .Define("mZ_ttz", "SignalRegion_ttz ? zboson_mass : -1")
	       .Define("dphi_ll_z_ttz", "SignalRegion_ttz ? dphi_lepZ_OSSF : -10")
	       .Define("cosThetaPol_ttz", "SignalRegion_ttz ? cosTheta_Polarization_angle : -9")
	       .Define("sumHadPt_ttz", "SignalRegion_ttz ? sum_selectedJet_pt : -1")
	       .Define("sumLepMetPt_ttz", "SignalRegion_ttz ? Selected_jeteta_maxAbs : -1")
	       .Define("min_dR_bl_ttz", "SignalRegion_ttz ? min_dR_bjet_lepton : -10")
	       .Define("max_dR_bl_ttz", "SignalRegion_ttz ? max_dR_bjet_lepton : -10")
	       .Define("max_dphi_jj_ttz", "SignalRegion_ttz ? max_dphi_jj : -10")
	       .Define("max_ptjj_ttz", "SignalRegion_ttz ? max_ptjj : -10")
	       .Define("max_mjj_ttz", "SignalRegion_ttz ? max_mjj : -10")
	       .Define("mass3l_ttz", "SignalRegion_ttz ? mass_3lepton : -10")
	       .Define("dR_b_recoil_ttz", "SignalRegion_ttz ? dR_b_recoilJet : -10")
	       .Define("dR_b_l_ttz", "SignalRegion_ttz ? dR_b_lepton : -10")
	       .Define("maxJetAbsEta_ttz", "SignalRegion_ttz ? Selected_jeteta_maxAbs : -10")
	       .Define("etaRecoilingJet_ttz", "SignalRegion_ttz ? RecoilingJet_eta : -99.0")
	       .Define("lep_asymmetry_ttz", "SignalRegion_ttz ? topLepton_absEta_times_charge : -9.0")
	       .Define("maxDEEPJET_ttz", "SignalRegion_ttz ? Selected_bjet_score : ROOT::VecOps::RVec<float>{}")
	       .Define("MET_pt_ttz", "SignalRegion_ttz ? MET_pt : -10");

    _rlm = _rlm.Define("nJet_trial", "trialRegion ? ncleanjetspass : -1")
	       .Define("nBJets_trial", "trialRegion ? ncleanbjetspass : -1")
	       .Define("mWT_trial", "trialRegion ? Wboson_transversMass : -1")
	       .Define("mTop_trial", "trialRegion ? top_mass : -1")
	       .Define("mZ_trial", "trialRegion ? zboson_mass : -1")
	       .Define("dphi_ll_z_trial", "trialRegion ? dphi_lepZ_OSSF : -10")
	       .Define("cosThetaPol_trial", "trialRegion ? cosTheta_Polarization_angle : -9")
	       .Define("sumHadPt_trial", "trialRegion ? sum_selectedJet_pt : -1")
	       .Define("sumLepMetPt_trial", "trialRegion ? Selected_jeteta_maxAbs : -1")
	       .Define("min_dR_bl_trial", "trialRegion ? min_dR_bjet_lepton : -10")
	       .Define("max_dR_bl_trial", "trialRegion ? max_dR_bjet_lepton : -10")
	       .Define("max_dphi_jj_trial", "trialRegion ? max_dphi_jj : -10")
	       .Define("max_ptjj_trial", "trialRegion ? max_ptjj : -10")
	       .Define("max_mjj_trial", "trialRegion ? max_mjj : -10")
	       .Define("mass3l_trial", "trialRegion ? mass_3lepton : -10")
	       .Define("dR_b_recoil_trial", "trialRegion ? dR_b_recoilJet : -10")
	       .Define("dR_b_l_trial", "trialRegion ? dR_b_lepton : -10")
	       .Define("maxJetAbsEta_trial", "trialRegion ? Selected_jeteta_maxAbs : -10")
	       .Define("etaRecoilingJet_trial", "trialRegion ? RecoilingJet_eta : -99.0")
	       .Define("lep_asymmetry_trial", "trialRegion ? topLepton_absEta_times_charge : -9.0")
	       .Define("maxDEEPJET_trial", "trialRegion ? Selected_bjet_score : ROOT::VecOps::RVec<float>{}")
	       .Define("MET_pt_trial", "trialRegion ? MET_pt : -10");

    _rlm = _rlm.Define("nJet_signal", "SignalRegion ? ncleanjetspass : -1")
	       .Define("nBJets_signal", "SignalRegion ? ncleanbjetspass : -1")
	       .Define("mWT_signal", "SignalRegion ? Wboson_transversMass : -1")
	       .Define("mTop_signal", "SignalRegion ? top_mass : -1")
	       .Define("mZ_signal", "SignalRegion ? zboson_mass : -1")
	       .Define("dphi_ll_z_signal", "SignalRegion ? dphi_lepZ_OSSF : -10")
	       .Define("cosThetaPol_signal", "SignalRegion ? cosTheta_Polarization_angle : -9")
	       .Define("sumHadPt_signal", "SignalRegion ? sum_selectedJet_pt : -1")
	       .Define("sumLepMetPt_signal", "SignalRegion ? Selected_jeteta_maxAbs : -1")
	       .Define("min_dR_bl_signal", "SignalRegion ? min_dR_bjet_lepton : -10")
	       .Define("max_dR_bl_signal", "SignalRegion ? max_dR_bjet_lepton : -10")
	       .Define("max_dphi_jj_signal", "SignalRegion ? max_dphi_jj : -10")
	       .Define("max_ptjj_signal", "SignalRegion ? max_ptjj : -10")
	       .Define("max_mjj_signal", "SignalRegion ? max_mjj : -10")
	       .Define("mass3l_signal", "SignalRegion ? mass_3lepton : -10")
	       .Define("dR_b_recoil_signal", "SignalRegion ? dR_b_recoilJet : -10")
	       .Define("dR_b_l_signal", "SignalRegion ? dR_b_lepton : -10")
	       .Define("maxJetAbsEta_signal", "SignalRegion ? Selected_jeteta_maxAbs : -10")
	       .Define("etaRecoilingJet_signal", "SignalRegion ? RecoilingJet_eta : -99.0")
	       .Define("lep_asymmetry_signal", "SignalRegion ? topLepton_absEta_times_charge : -9.0")
	       .Define("maxDEEPJET_signal", "SignalRegion ? Selected_bjet_score : ROOT::VecOps::RVec<float>{}")
	       .Define("MET_pt_signal", "SignalRegion ? MET_pt : -10");



}




//=============================define variables==================================================//
void BaseAnalyser::defineMoreVars()
{
    if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

    //addVar({"good_muon1pt", "goodMuons_pt[0]", ""});

    //selected jet candidates
   // addVar({"good_jet1pt", "(goodJets_pt.size()>0) ? goodJets_pt[0] : -1", ""});
    //addVar({"Selected_jet1pt", "(Selected_jetpt.size()>0) ? Selected_jetpt[0] : -1", ""});
    //addVar({"good_jet1eta", "goodJets_eta[0]", ""});
    //addVar({"good_jet1mass", "goodJets_mass[0]", ""});

    //================================Store variables in tree=======================================//
    // define variables that you want to store
    //==============================================================================================//
    addVartoStore("run");
    addVartoStore("luminosityBlock");
    addVartoStore("event");
    addVartoStore("evWeight");
    //electron
    addVartoStore("nElectron");
    addVartoStore("baselineElectrons_idx");
    //addVartoStore("ngoodElectrons");
    addVartoStore("Electron_charge");
    addVartoStore("Electron_pt");
   ////BDT VARIABLES FOR ELECTRONS///////////
//    addVartoStore("fakable_baselineElectrons");  // based on the the baselineElectrons_MVAEstimatorRun2Fall17NoIsoV2Values greater than 0.4
    addVartoStore("tight_baselineElectrons");
    //addVartoStore("baselineElectrons_isPrompt");  // this is decided using genPartFlav

    addVartoStore("baselineElectrons_pt");
    addVartoStore("baselineElectrons_eta");
    addVartoStore("baselineElectrons_phi");
    addVartoStore("NbaselineElectrons");
    addVartoStore("baselineElectrons_charge");


    //muon
    addVartoStore("nMuon");
    addVartoStore("Muon_charge");
    addVartoStore("Muon_mass");
    addVartoStore("Muon_pt");
    ////BDT VARIABLES FOR MUONS///////////
    addVartoStore("baselineMuons_isPrompt");
    addVartoStore("NbaselineMuons");
    addVartoStore("baselineMuons_pt");
    addVartoStore("baselineMuons_eta");
    addVartoStore("baselineMuons_phi");
    addVartoStore("baselineMuons_charge");
    addVartoStore("fakable_baselineMuons");
    addVartoStore("tight_baselineMuons");

 
    

    //jet
    addVartoStore("nJet");
    addVartoStore("Jet_pt");
    addVartoStore("NgoodJets");
    addVartoStore("goodJets_pt");
    addVartoStore("Selected_jetpt");
    addVartoStore("Selected_jeteta");
    addVartoStore("Selected_jetbtag");
    addVartoStore("Ngood_bjets");
    addVartoStore("good_bjetpt");
    addVartoStore("good_bjeteta"); 
    addVartoStore("good_bjetphi");
    addVartoStore("good_bjetmass");
    addVartoStore("good_bjetdeepjet");

    addVartoStore("top_bjetpt");
    addVartoStore("top_bjeteta");
    addVartoStore("top_bjetphi");
//    addVartoStore("top_bjetmass");

    
    //jetmet corr
    addVartoStore("Jet_pt_corr");
    addVartoStore("Jet_pt_corr_up");
    addVartoStore("Jet_pt_corr_down");
    addVartoStore("Jet_pt_relerror");
    addVartoStore("MET_pt_corr");
    addVartoStore("MET_pt");


//new funciton variable for merged lepton
    addVartoStore("totalLeptonCount");
    addVartoStore("mass_of_3lepton");
    addVartoStore("combinedLeptonPt");
    addVartoStore("combinedLeptonEta");
    addVartoStore("combinedLeptonPhi");
    addVartoStore("combinedLepton_isPrompt");
    addVartoStore("allTightLeptons");
    addVartoStore("numCombinedLepton4Vecs");
    addVartoStore("combinedLeptonCharge");
    addVartoStore("combinedLeptonFlavor");
   // addVartoStore("combinedLeptonTLorentzVecs");
    
    addVartoStore("goodLepton_pt");
    addVartoStore("All_good_tightLeptons");
    addVartoStore("NgoodLepton");
    addVartoStore("goodLepton_eta");
    addVartoStore("goodLepton_phi");
    addVartoStore("goodLepton_isPrompt");
    addVartoStore("goodLepton_charge");
    addVartoStore("goodLepton_flavor");
    addVartoStore("leadingLepton_pt");
    addVartoStore("leadingLepton_eta");
    addVartoStore("subleadingLepton_pt");
    addVartoStore("subleadingLepton_eta");
    addVartoStore("TrailingLepton_pt");
    addVartoStore("TrailingLepton_eta");

    // 3-lepton case
    addVartoStore("goodLepton3_pt");
    addVartoStore("goodLepton3_eta");
    addVartoStore("goodLepton3_phi");
    addVartoStore("goodLepton3_charge");
    addVartoStore("goodLepton3_flavor");
    addVartoStore("goodLepton3_isPrompt");


    // 4-lepton case
    addVartoStore("goodLepton4_pt");
    addVartoStore("goodLepton4_eta");
    addVartoStore("goodLepton4_phi");
    addVartoStore("goodLepton4_charge");
    addVartoStore("goodLepton4_flavor");
    addVartoStore("goodLepton4_isPrompt");
   /////////////////////////////////
   //////////BDT variable///////////
    addVartoStore("sum_selectedJet_pt");
    addVartoStore("sum_lepton_MET_pt");
    addVartoStore("Selected_jeteta_maxAbs");
    addVartoStore("RecoilingJet_pt");
    addVartoStore("RecoilingJet_eta");

    addVartoStore("TR_leadingLepton_pt");
    addVartoStore("TR_subleadingLepton_pt");
    addVartoStore("TR_trailingLepton_pt");
    addVartoStore("TR_leadingLepton_eta");
    addVartoStore("TR_subleadingLepton_eta");
    addVartoStore("TR_topLepton_pt");
    addVartoStore("TR_trailingLepton_eta");

    // 3-lepton region variables
    addVartoStore("ncleanjetspass_SignalRegion");
    addVartoStore("ncleanbjetspass_SignalRegion");

    addVartoStore("ncleanjetspass_WZ_Region");
    addVartoStore("Wboson_transversMass_WZ_Region");

    addVartoStore("ncleanjetspass_X_gamma_Region");
    addVartoStore("ncleanbjetspass_X_gamma_Region");

    addVartoStore("ncleanjetspass_NP_2_Region");
    addVartoStore("ncleanbjetspass_NP_2_Region");

    addVartoStore("ncleanjetspass_NP_1_Region");
    addVartoStore("ncleanbjetspass_NP_1_Region");

    // 4-lepton region variables
    addVartoStore("ncleanjetspass_ZZ_Region");
    addVartoStore("mass_of_4L_ZZ_Region");
    addVartoStore("Z_mass1_ZZ_Region");
    addVartoStore("Z_mass2_ZZ_Region");


    addVartoStore("ncleanjetspass_ttZ_Region");
    addVartoStore("ncleanbjetspass_ttZ_Region");




   //OSSF info
    addVartoStore("OSSF_pair_count");
    addVartoStore("mZ_compatible_3l");
    addVartoStore("OSSF_selection_with_topLep");
    addVartoStore("OSSF_all_pairs_masses");
    addVartoStore("OSSF_ZPair_mass");
    addVartoStore("OSSF_LowerMassPair_mass");

    addVartoStore("OSSF_category");
    addVartoStore("zboson_mass");
    addVartoStore("nonZ_OSSF_mass");
    addVartoStore("mass_of_3_goodLepton_4BG");
    addVartoStore("topLepton_index");
    addVartoStore("topLepton_pt_new");
    addVartoStore("topLepton_eta_new");
    addVartoStore("topLepton_phi_new");
    addVartoStore("topLepton_charge");
    addVartoStore("topLepton_flavor");
    addVartoStore("OSSF4L_category");
    addVartoStore("OSSF4L_info");               // Tuple: (category, best_pair_idx, second_pair_idx, best_mass, second_mass)
    addVartoStore("OSSF4L_bestZ_mass");         // First Z candidate mass (cat 1 or 2)
    addVartoStore("OSSF4L_secondZ_mass");       // Second Z candidate mass (only for cat 2)
    addVartoStore("mass_of_4L");





    addVartoStore("OSSF_info");
    addVartoStore("OSSF_mass");
    addVartoStore("OSSF_pt");
    addVartoStore("OSSF_eta");
    addVartoStore("OSSF_phi");
    addVartoStore("ntopLepton");
    addVartoStore("topLepton_pt");
    addVartoStore("topLepton_eta");
    addVartoStore("topLepton_phi");
    addVartoStore("Wboson_transversMass");
    addVartoStore("w_pt");
    addVartoStore("w_eta");
    addVartoStore("w_phi");
    addVartoStore("top_mass");
   // addVartoStore("signalRegion_top_mass");
   // addVartoStore("signalRegion_all_top");
    addVartoStore("top_pt");
    addVartoStore("top_eta");
    addVartoStore("top_phi");
    addVartoStore("b_mass");


    addVartoStore("dphi_lepZ_OSSF");
    addVartoStore("min_dR_bjet_lepton");
    addVartoStore("max_dR_bjet_lepton");
    addVartoStore("max_dphi_jj");
    addVartoStore("max_dphi_jj_confirm");
    addVartoStore("max_ptjj");
    addVartoStore("max_mjj");
    addVartoStore("topLepton_absEta_times_charge");
    addVartoStore("mass_3lepton");
    addVartoStore("dR_b_recoilJet");
    addVartoStore("dR_b_lepton");
    addVartoStore("cosTheta_Polarization_angle");


    addVartoStore("trialRegion");
    addVartoStore("SignalRegion");
    addVartoStore("SignalRegion_tzq");
    addVartoStore("SignalRegion_ttz");
    //////bdt variable fo the signal background discrimination /////
    addVartoStore("nJet_tzq");
    addVartoStore("nBJets_tzq");
    addVartoStore("mWT_tzq");
    addVartoStore("mTop_tzq");
    addVartoStore("mZ_tzq");
    addVartoStore("dphi_ll_z_tzq");
    addVartoStore("cosThetaPol_tzq");
    addVartoStore("sumHadPt_tzq");
    addVartoStore("sumLepMetPt_tzq");
    addVartoStore("min_dR_bl_tzq");
    addVartoStore("max_dR_bl_tzq");
    addVartoStore("max_dphi_jj_tzq");
    addVartoStore("max_ptjj_tzq");
    addVartoStore("max_mjj_tzq");
    addVartoStore("mass3l_tzq");
    addVartoStore("dR_b_recoil_tzq");
    addVartoStore("dR_b_l_tzq");
    addVartoStore("maxJetAbsEta_tzq");
    addVartoStore("etaRecoilingJet_tzq");
    addVartoStore("lep_asymmetry_tzq");
    addVartoStore("maxDEEPJET_tzq");
    addVartoStore("MET_pt_tzq");


    addVartoStore("nJet_ttz");
    addVartoStore("nBJets_ttz");
    addVartoStore("mWT_ttz");
    addVartoStore("mTop_ttz");
    addVartoStore("mZ_ttz");
    addVartoStore("dphi_ll_z_ttz");
    addVartoStore("cosThetaPol_ttz");
    addVartoStore("sumHadPt_ttz");
    addVartoStore("sumLepMetPt_ttz");
    addVartoStore("min_dR_bl_ttz");
    addVartoStore("max_dR_bl_ttz");
    addVartoStore("max_dphi_jj_ttz");
    addVartoStore("max_ptjj_ttz");
    addVartoStore("max_mjj_ttz");
    addVartoStore("mass3l_ttz");
    addVartoStore("dR_b_recoil_ttz");
    addVartoStore("dR_b_l_ttz");
    addVartoStore("maxJetAbsEta_ttz");
    addVartoStore("etaRecoilingJet_ttz");
    addVartoStore("lep_asymmetry_ttz");
    addVartoStore("maxDEEPJET_ttz");
    addVartoStore("MET_pt_ttz");

    addVartoStore("nJet_trial");
    addVartoStore("nBJets_trial");
    addVartoStore("mWT_trial");
    addVartoStore("mTop_trial");
    addVartoStore("mZ_trial");
    addVartoStore("dphi_ll_z_trial");
    addVartoStore("cosThetaPol_trial");
    addVartoStore("sumHadPt_trial");
    addVartoStore("sumLepMetPt_trial");
    addVartoStore("min_dR_bl_trial");
    addVartoStore("max_dR_bl_trial");
    addVartoStore("max_dphi_jj_trial");
    addVartoStore("max_ptjj_trial");
    addVartoStore("max_mjj_trial");
    addVartoStore("mass3l_trial");
    addVartoStore("dR_b_recoil_trial");
    addVartoStore("dR_b_l_trial");
    addVartoStore("maxJetAbsEta_trial");
    addVartoStore("etaRecoilingJet_trial");
    addVartoStore("lep_asymmetry_trial");
    addVartoStore("maxDEEPJET_trial");
    addVartoStore("MET_pt_trial");

    addVartoStore("nJet_signal");
    addVartoStore("nBJets_signal");
    addVartoStore("mWT_signal");
    addVartoStore("mTop_signal");
    addVartoStore("mZ_signal");
    addVartoStore("dphi_ll_z_signal");
    addVartoStore("cosThetaPol_signal");
    addVartoStore("sumHadPt_signal");
    addVartoStore("sumLepMetPt_signal");
    addVartoStore("min_dR_bl_signal");
    addVartoStore("max_dR_bl_signal");
    addVartoStore("max_dphi_jj_signal");
    addVartoStore("max_ptjj_signal");
    addVartoStore("max_mjj_signal");
    addVartoStore("mass3l_signal");
    addVartoStore("dR_b_recoil_signal");
    addVartoStore("dR_b_l_signal");
    addVartoStore("maxJetAbsEta_signal");
    addVartoStore("etaRecoilingJet_signal");
    addVartoStore("lep_asymmetry_signal");
    addVartoStore("maxDEEPJET_signal");
    addVartoStore("MET_pt_signal");


   
   if(!_isData){
      //case1 btag correction- fixed wp	
      //case3 shape correction
      //addVartoStore("btagWeight_case3");
    addVartoStore("LHE_HT");
    addVartoStore("Selected_jethadflav");
    addVartoStore("Selected_bjethadflav");//after overlap and btag cut

    addVartoStore("good_bjethadflav");
    addVartoStore("goodJets_hadflav");
      
    //For Btagging Efficiency
    addVartoStore("goodJets_btagpass_bcflav_pt");
    addVartoStore("goodJets_btagpass_bcflav_eta");
    addVartoStore("goodJets_all_bcflav_pt");
    addVartoStore("goodJets_all_bcflav_eta");

    addVartoStore("goodJets_btagpass_lflav_pt");
    addVartoStore("goodJets_btagpass_lflav_eta");
    addVartoStore("goodJets_all_lflav_pt");
    addVartoStore("goodJets_all_lflav_eta");

    addVartoStore("genWeight");
    addVartoStore("genEventSumw");      
    //"evWeight", " pugenWeight * btag_SF_bcflav_central * btag_SF_lflav_central * muon_SF_central * ele_SF_central"
    //case1 btag correction- fixed wp	
//    addVartoStore("btag_SF_bcflav_central");
//    addVartoStore("btag_SF_lflav_central");
    addVartoStore("totbtagSF");
    //addVartoStore("btag_SF_down");
    addVartoStore("evWeight_wobtagSF");
    
    //case3 shape correction
    //addVartoStore("btagWeight_case3");
    
    
    //MUONID - ISO SF & WEIGHT	
    addVartoStore("muon_SF_central");
    //addVartoStore("muon_id_weight");
    addVartoStore("muon_SF_id_sf");
    addVartoStore("muon_SF_id_syst");
    addVartoStore("muon_SF_id_systup");
    addVartoStore("muon_SF_id_systdown");
    //addVartoStore("muonISO_SF");
    addVartoStore("muon_SF_iso_sf");
    }

    
}
void BaseAnalyser::bookHists()
{
    //=================================structure of histograms==============================================//
    //add1DHist( {"hnevents", "hist_title; x_axis title; y_axis title", 2, -0.5, 1.5}, "one", "evWeight", "");
    //add1DHist( {"hgoodelectron1_pt", "good electron1_pt; #electron p_{T}; Entries / after ", 18, -2.7, 2.7}, "good_electron1pt", "evWeight", "0");
    //======================================================================================================//
    
    if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }
    
    //================================gen/LHE weights======================================================//
   // if(!_isData && !isDefined("genWeight")){
   //     add1DHist({"hgenWeight", "genWeight", 1001, -100, 100}, "genWeight", "one", "");
    
    /*if(isDefined("LHEWeight_originalXWGTUP")){
        add1DHist({"hLHEweight", "LHEweight", 1001, -100, 100}, "LHEWeight_originalXWGTUP", "one", "");
    }*/
   // add1DHist({"hgenEventSumw","Sum of gen Weights",1001,-8e+09,8e+09},"one","genEventSumw","");
    //====================================================================================================//
    //}
	
    // add1DHist( {"hnevents", "Number of Events", 2, -0.5, 1.5}, "one", "evWeight", "");
    // add1DHist( {"hnevents_no_weight", "Number of Events w/o", 2, -0.5, 1.5}, "one", "one", "");
    
  //  add1DHist( {"hNgoodElectrons", "NumberofGoodElectrons", 5, 0.0, 5.0}, "NgoodElectrons", "evWeight", "");
    
   // add1DHist( {"hNgoodMuons", "# of good Muons ", 5, 0.0, 5.0}, "NgoodMuons", "evWeight", "");//i made the change 
   // add1DHist( {"zboson_m", "Mass of z boson", 50, 70.0, 110.0}, "zboson_m", "one", "");
    // add1DHist( {"hgood_jetpt_with weight", "Good Jet pt with weight " , 100, 0, 1000} , "goodJets_pt", "evWeight", "");
    // add1DHist( {"hgood_jetpt_NOWeight", "Good Jet pt no weihght " , 100, 0, 1000} , "goodJets_pt", "one", "");
    
    // add1DHist( {"hgood_jet1pt", "Good Jet_1 pt with weight " , 100, 0, 2500} , "good_jet1pt", "evWeight", "");
    // add1DHist( {"hselected_jet1pt", "SelectedJet_1 pt no weight" , 100, 0, 1000} , "Selected_jet1pt", "evWeight", "");
    // add1DHist( {"hselected_jetptWithweight", "clean-Jets with weight" , 100, 0, 2500} , "Selected_jetpt", "evWeight", "");
    // add1DHist( {"hselected_jetptNoweight", "clean-Jets w/o weight" , 100, 0, 2500} , "Selected_jetpt", "one", "");
/*    if(!_isData){
      add1DHist( {"hbtag_SF_bcflav_central", "btag SF bcflav central" , 100, 0, 2} , "btag_SF_bcflav_central", "one", "");
      add1DHist( {"hbtag_SF_lflav_central", "btag SF lflav central" , 100, 0, 2} , "btag_SF_lflav_central", "one", "");
    }
  */  
    //add2DHist( {"btagscalef", "btvcent_sf vs seljet_pt" , 100, 0, 500, 100, 0, 500} ,  "Selected_jetpt","btag_SF_case1", "one","");

    
}
void BaseAnalyser::setTree(TTree *t, std::string outfilename)
{
	if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

	_rd = ROOT::RDataFrame(*t);
	_rlm = RNode(_rd);
	_outfilename = outfilename;
	_hist1dinfovector.clear();
	_th1dhistos.clear();
	_hist2dinfovector.clear();
	_th2dhistos.clear();
	_varstostore.clear();
	_selections.clear();

	this->setupAnalysis();
}
//================================Selected Object Definitions====================================//

void BaseAnalyser::setupObjects()
{
	// Object selection will be defined in sequence.
	// Selected objects will be stored in new vectors.
	selectElectrons();
	selectMuons();
	selectJets();
	selectMET();
	removeOverlaps();
	mergeLeptons();
	DefineGoodLeptonGroups();
//	mergeTrailingLeptons();
//	search_for_OSSFPairs();
	processOSSFPairs();
	reconstructWboson();
	reconstructTop();
	BDT_variables();
	defineSignalRegion();

	/*calculateZBosonMass();
	//identifyOSSFElectronPair();
	// defineTwoElectronEvent();*/

	//This code is also off due to the problem arising entries of Data files can continue when add the correction files
//	if(!_isData){
//	  this->calculateEvWeight(); // PU, genweight and BTV and Mu and Ele
//	}
	//selectMET();

}

void BaseAnalyser::setupAnalysis()
{
	if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }
	
 	cout<<"year===="<< _year<< "==runtype=== " <<  _runtype <<endl;

    //==========================================event/gen/ weights==========================================//
    // Event weight for data it's always one. For MC, it depends on the sign
    //=====================================================================================================//
   
	_rlm = _rlm.Define("one", "1.0");
	
	if (_isData && !isDefined("evWeight"))
	{
		_rlm = _rlm.Define("evWeight", [](){
				return 1.0;
			}, {} );
	}
	if(!_isData ) // Only use genWeight
	  {
	    //_rlm = _rlm.Define("evWeight", "genWeight");
	    
	    //std::cout<<"Using evWeight = genWeight"<<std::endl;

	    auto sumgenweight = _rd.Sum("genWeight");
	    string sumofgenweight = Form("%f",*sumgenweight);
	    _rlm = _rlm.Define("genEventSumw",sumofgenweight.c_str());
	    std::cout<<"Sum of genWeights = "<<sumofgenweight.c_str()<<std::endl;
	  }
	
	defineCuts();
	defineMoreVars();
	bookHists();
	setupCuts_and_Hists();
	setupTree();
}

