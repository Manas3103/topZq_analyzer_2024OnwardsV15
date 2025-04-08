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
//	HLT2017Names= {"HLT_IsoMu24","HLT_Ele32_WPTight_Gsf"};
        HLT2017Names= {"HLT_IsoMu24", "HLT_IsoMu24_eta2p1", "HLT_IsoMu27", "HLT_Mu50", "HLT_OldMu100", "HLT_TkMu100",
	               "HLT_Ele32_WPTight_Gsf", "HLT_Ele35_WPTight_Gsf", "HLT_Ele115_CaloIdVT_GsfTrkIdT", "HLT_Photon200",
                       "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL", "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ", "HLT_Mu37_TkMu27", "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8", "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8",
		       "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ", "HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ", "HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ", "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL", "HLT_Mu27_Ele37_CaloIdL_MW", "HLT_Mu37_Ele27_CaloIdL_MW",
		       "HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL", "HLT_DoubleEle33_CaloIdL_MW",
		       "HLT_TripleMu_10_5_5_DZ", "HLT_TripleMu_5_3_3_Mass3p8to60_DZ", "HLT_TripleMu_12_10_5",
		       "HLT_DiMu9_Ele9_CaloIdL_TrackIdL_DZ",
		       "HLT_Mu8_DiEle12_CaloIdL_TrackIdL",
                       "HLT_Ele16_Ele12_Ele8_CaloIdL_TrackIdL"
	
	};

    HLT2016Names= {"Name1","Name2"};
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
//	addCuts("nMuon + nElectron >= 3  && nJet>2 && PV_npvsGood >= 1 && LHE_HT < 70 && LHE_HT > 0", "0");//first change
	addCuts("nMuon + nElectron >= 3  && nJet>2 && PV_npvsGood >= 1 ", "0");//not for drellyan
	addCuts("Flag_goodVertices && Flag_globalSuperTightHalo2016Filter && Flag_HBHENoiseFilter && Flag_HBHENoiseIsoFilter && Flag_EcalDeadCellTriggerPrimitiveFilter && Flag_BadPFMuonFilter && Flag_ecalBadCalibFilter", "0");

	//addCuts("NgoodMuons>=2","00");
        //addCuts("ncleanjetspass>0","00");
	addCuts(setHLT(),"0"); //HLT cut buy checking HLT names in the root file
       // addCuts("LHE_HT < 70", "0");  // Only for DrellYan 50
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

//////////////////////////Electron enery for the new variable ////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

    _rlm = _rlm.Define("Electron_4Vecs", ::generate_4vec, {"Electron_pt", "Electron_eta", "Electron_phi", "Electron_mass"});  
    _rlm = _rlm.Define("Electron_energy", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& electron_4vecs) {
    ROOT::VecOps::RVec<Float_t> energies;
    for (const auto& vec : electron_4vecs) {
        energies.push_back(static_cast<Float_t>(vec.E()));  // E() gives the energy in PtEtaPhiM4D Lorentz vector
    }
    return energies;
}, {"Electron_4Vecs"});

///////////////////////////////////////////////////
///////// NEW VARIABLE FOR THE BDT ////////////////
///////////////////////////////////////////////////

    _rlm = _rlm.Define("electron_oneOverEminusOneOverP","1.0 / Electron_energy - 1.0 / Electron_pt");


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
    // Define trailing electrons with ElectronID(2)
    //-------------------------------------------------------
    _rlm = _rlm.Define("trailingElectronsID", ElectronID(2)); // ID level 2 for trailing electrons
    _rlm = _rlm.Define("trailingElectrons", "trailingElectronsID && Electron_pt > 10 && abs(Electron_eta) < 2.5 && Electron_pfRelIso03_all < 0.40");

    // Define additional variables for trailing electrons
    _rlm = _rlm.Define("trailingElectrons_pt", "Electron_pt[trailingElectrons]")
                .Define("trailingElectrons_eta", "Electron_eta[trailingElectrons]")
                .Define("trailingElectrons_phi", "Electron_phi[trailingElectrons]")
                .Define("trailingElectrons_mass", "Electron_mass[trailingElectrons]")
                .Define("trailingElectrons_charge", "Electron_charge[trailingElectrons]")
                .Define("trailingElectrons_idx", ::good_idx, {"trailingElectrons"})
                .Define("NtrailingElectrons", "int(trailingElectrons_pt.size())");

    //-------------------------------------------------------
    // Generate electron 4-vector from selected good electrons
    //-------------------------------------------------------
    _rlm = _rlm.Define("goodElectron_4Vecs", ::generate_4vec, {"goodElectrons_pt", "goodElectrons_eta", "goodElectrons_phi", "goodElectrons_mass"});



    // =====================================================================
    // Baseline Electron Selection
    // =====================================================================
    _rlm = _rlm.Define("baselineElectrons", 
                       "Electron_pt > 10.0 && abs(Electron_eta) < 2.5 && "
                       "Electron_pfRelIso03_all < 0.40 && abs(Electron_dxy) < 0.05 && "
                       "abs(Electron_dz) < 0.10 && Electron_lostHits <= 1 && "
                       "Electron_hoe < 0.10 && Electron_convVeto &&"
		       "((abs(Electron_eta) < 1.479 && Electron_sieie < 0.011) || " // Barrel cut
                       "(abs(Electron_eta) >= 1.479 && abs(Electron_eta) < 2.5 && Electron_sieie < 0.030)) &&" // Endcap cut
		       "Electron_sip3d < 8 && electron_oneOverEminusOneOverP > -0.04"
		       );

    // Additional variables for baseline electrons
    _rlm = _rlm.Define("baselineElectrons_pt", "Electron_pt[baselineElectrons]")
                .Define("baselineElectrons_eta", "Electron_eta[baselineElectrons]")
                .Define("baselineElectrons_phi", "Electron_phi[baselineElectrons]")
                .Define("baselineElectrons_mass", "Electron_mass[baselineElectrons]")
                .Define("baselineElectrons_charge", "Electron_charge[baselineElectrons]")
                .Define("baselineElectrons_idx", ::good_idx, {"baselineElectrons"})
                .Define("NbaselineElectrons", "int(baselineElectrons_pt.size())");

    // Generate 4-vectors for baseline electrons
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
    
    if (!_isData){
    _rlm = _rlm.Define("baselineElectrons_genPartFlav", "Electron_genPartFlav[baselineElectrons]");
    _rlm = _rlm.Define("baselineElectrons_isPrompt",
    [](const ROOT::VecOps::RVec<unsigned char>& genPartFlav) {
        return ROOT::VecOps::Map(genPartFlav, [](unsigned char flav) {
            return (flav == 1 || flav == 15 || flav == 22) ? 1 : 0;
        });
    },
    {"baselineElectrons_genPartFlav"});
    }
    // Define tight and fakable electrons based on MVA score
    _rlm = _rlm.Define("baselineElectrons_MVAEstimatorRun2Fall17NoIsoV2Values", "Electron_mvaFall17V2noIso[baselineElectrons]");
    _rlm = _rlm.Define("tight_baselineElectrons", "baselineElectrons_MVAEstimatorRun2Fall17NoIsoV2Values > 0.4")
               .Define("fakable_baselineElectrons", "baselineElectrons_MVAEstimatorRun2Fall17NoIsoV2Values <= 0.4");
    

///////////////////////////////////////////////////////////////
    


    // Generate electron 4-vector for trailing electrons
    _rlm = _rlm.Define("trailingElectrons_4Vecs", ::generate_4vec, {"trailingElectrons_pt", "trailingElectrons_eta", "trailingElectrons_phi", "trailingElectrons_mass"});
    _rlm = _rlm.Define("trailingElectrons_TL4Vecs",
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
    {"trailingElectrons_pt", "trailingElectrons_eta", "trailingElectrons_phi", "trailingElectrons_mass"});

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
/////////////VARIABLE FOR BDT USING BASELINE///////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
    _rlm = _rlm.Define("baselineElectrons_JetNDauCharged" , "Electron_jetNDauCharged[baselineElectrons]")
               .Define("baselineElectrons_miniPFRelIso_chg" , "Electron_miniPFRelIso_chg[baselineElectrons]")
	       .Define("baselineElectrons_miniPFRelIso_neutral" , "Electron_miniPFRelIso_all[baselineElectrons]-Electron_miniPFRelIso_chg[baselineElectrons]")
	       .Define("baselineElectrons_jetPtRelv2" , "Electron_jetPtRelv2[baselineElectrons]")
	       .Define("baselineElectrons_pfRelIso03_all" , "Electron_pfRelIso03_all[baselineElectrons]")
	       .Define("baselineElectrons_sip3d" , "Electron_sip3d[baselineElectrons]")
	       .Define("baselineElectrons_dxy" , "Electron_dxy[baselineElectrons]")
	       .Define("baselineElectrons_dz" , "Electron_dz[baselineElectrons]")
	       .Define("baselineElectrons_JetPt_ratio" , "1/(Electron_jetRelIso[baselineElectrons]+1)")
	       .Define("baselineElectrons_closest_jetIdx" , "Electron_jetIdx[baselineElectrons]");

    _rlm = _rlm.Define("baselineElectrons_JetPtRatio", [](const ROOT::VecOps::RVec<float>& electron_jetRelIso, const ROOT::VecOps::RVec<int>& baselineElectrons) {
           std::vector<double> jetPtRatios;
           for (const auto& idx : baselineElectrons) {
                double ratio = 1.0 / (electron_jetRelIso[idx] + 1);
                jetPtRatios.push_back(ratio > 1.5 ? 1.5 : ratio);
           }
           return jetPtRatios;
           }, {"Electron_jetRelIso", "baselineElectrons"});
    _rlm = _rlm.Define("baselineElectron_4Vecs_RVec",
    [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& vec) {
        return ROOT::VecOps::RVec<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>(vec.begin(), vec.end());
    }, {"baselineElectron_4Vecs"});


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
////////////ENERGY OF DIFFERENT CATAGORY //////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
/*

   // Define energy for good electrons
    _rlm = _rlm.Define("goodElectrons_energy", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& electron_4vecs) {
        std::vector<double> energies;
        for (const auto& vec : electron_4vecs) {
            energies.push_back(vec.E());  // E() gives the energy in PtEtaPhiM4D Lorentz vector
        }
        return energies;
    }, {"goodElectron_4Vecs"});

    // Define energy for trailing electrons
    _rlm = _rlm.Define("trailingElectrons_energy", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& electron_4vecs) {
        std::vector<double> energies;
        for (const auto& vec : electron_4vecs) {
            energies.push_back(vec.E());  // E() gives the energy in PtEtaPhiM4D Lorentz vector
        }
        return energies;
    }, {"trailingElectrons_4Vecs"});
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

    //-------------------------------------------------------
    // Define trailing muons with pt > 10
    //-------------------------------------------------------
    _rlm = _rlm.Define("trailingMuonID", MuonID(2)); // loose muons
    _rlm = _rlm.Define("trailingMuons", "trailingMuonID &&  Muon_pt > 10 && abs(Muon_eta) < 2.4 && Muon_miniPFRelIso_all < 0.40");

    // Define additional variables for trailing muons
    _rlm = _rlm.Define("trailingMuons_pt", "Muon_pt[trailingMuons]")
                .Define("trailingMuons_eta", "Muon_eta[trailingMuons]")
                .Define("trailingMuons_phi", "Muon_phi[trailingMuons]")
                .Define("trailingMuons_mass", "Muon_mass[trailingMuons]")
                .Define("trailingMuons_charge", "Muon_charge[trailingMuons]")
                .Define("trailingMuons_idx", ::good_idx, {"trailingMuons"})
                .Define("NtrailingMuons", "int(trailingMuons_pt.size())");


    // ====================================================================
    // Baseline Muon Selection
    // // =====================================================================
    _rlm = _rlm.Define("baselineMuons",
		       "Muon_pt > 10.0 && abs(Muon_eta) < 2.4 && "
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
    
    if (!_isData){
    _rlm = _rlm.Define("baselineMuons_genPartFlav" ,"Muon_genPartFlav[baselineMuons]");
    _rlm = _rlm.Define("baselineMuons_isPrompt",
    [](const ROOT::VecOps::RVec<unsigned char>& genPartFlav) {
        return ROOT::VecOps::Map(genPartFlav, [](unsigned char flav) {
            return (flav == 1 || flav == 15) ? 1 : 0;
        });
    },
    {"baselineMuons_genPartFlav"});
    }
    _rlm = _rlm.Define("tight_baselineMuons", "Muon_segmentComp[baselineMuons] > 0.5")
               .Define("fakable_baselineMuons", "Muon_segmentComp[baselineMuons] <= 0.5");

    

    //-------------------------------------------------------
    // Generate muon 4-vector from selected good muons
    //-------------------------------------------------------
    _rlm = _rlm.Define("goodMuons_4vecs", ::generate_4vec, {"goodMuons_pt", "goodMuons_eta", "goodMuons_phi", "goodMuons_mass"});

    // Generate muon 4-vector for trailing muons
    _rlm = _rlm.Define("trailingMuons_4vecs", ::generate_4vec, {"trailingMuons_pt", "trailingMuons_eta", "trailingMuons_phi", "trailingMuons_mass"});

    _rlm = _rlm.Define("trailingMuons_TL4Vecs",
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
    {"trailingMuons_pt", "trailingMuons_eta", "trailingMuons_phi", "trailingMuons_mass"});



    //////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////
    //////                     BDT TRAINING VARIABLES FOR MVA LEPTON               ///////
    //////////////////////////////////////////////////////////////////////////////////////
    _rlm = _rlm.Define("baselineMuons_JetNDauCharged" , "Muon_jetNDauCharged[baselineMuons]")
            .Define("baselineMuons_miniPFRelIso_chg" , "Muon_miniPFRelIso_chg[baselineMuons]")
            .Define("baselineMuons_miniPFRelIso_neutral" , "Muon_miniPFRelIso_all[baselineMuons]-Muon_miniPFRelIso_chg[baselineMuons]")
            .Define("baselineMuons_jetPtRelv2" , "Muon_jetPtRelv2[baselineMuons]")
            .Define("baselineMuons_pfRelIso04_all" , "Muon_pfRelIso04_all[baselineMuons]")
            .Define("baselineMuons_sip3d" , "Muon_sip3d[baselineMuons]")
            .Define("baselineMuons_dxy" , "Muon_dxy[baselineMuons]")
            .Define("baselineMuons_dz" , "Muon_dz[baselineMuons]")
            .Define("baselineMuons_JetPt_ratio" , "1/(Muon_jetRelIso[baselineMuons]+1)")
            .Define("baselineMuons_segmentCompatibility", "Muon_segmentComp[baselineMuons]");

    _rlm = _rlm.Define("baselineMuons_JetPtRatio", [](const ROOT::VecOps::RVec<float>& muon_jetRelIso, const ROOT::VecOps::RVec<int>& baselineMuons) {
          std::vector<double> jetPtRatios;
          for (const auto& idx : baselineMuons) {
               double ratio = 1.0 / (muon_jetRelIso[idx] + 1);
               jetPtRatios.push_back(ratio > 1.5 ? 1.5 : ratio);
          }
          return jetPtRatios;
          }, {"Muon_jetRelIso", "baselineMuons"});

    _rlm = _rlm.Define("baselineMuon_4Vecs_RVec",
    [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& vec) {
        return ROOT::VecOps::RVec<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>(vec.begin(), vec.end());
    }, {"baselineMuon_4Vecs"});

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
    _rlm = _rlm.Define("withPuId" , "(Jet_puId==7 && Jet_pt < 50.0)||(Jet_pt >50.0)");
    //_rlm = _rlm.Define("goodJets", "goodJetsID && Jet_pt>30.0 && abs(Jet_eta)<2.4 ");
    _rlm = _rlm.Define("goodJets", "goodJetsID && withPuId && ((abs(Jet_eta) > 2.65 && abs(Jet_eta) < 3.139 && Jet_pt > 50.0) || (Jet_pt > 25.0 && abs(Jet_eta) < 5 && abs(Jet_eta)>3.139) || (Jet_pt > 25.0 && abs(Jet_eta) < 2.65 && abs(Jet_eta)>0))");
    _rlm = _rlm.Define("goodJets_pt", "Jet_pt[goodJets]")
                .Define("goodJets_eta", "Jet_eta[goodJets]")
                .Define("goodJets_phi", "Jet_phi[goodJets]")
                .Define("goodJets_mass", "Jet_mass[goodJets]")
                .Define("goodJets_idx", ::good_idx, {"goodJets"});
    if (!_isData){
       _rlm = _rlm.Define("goodJets_hadflav", "Jet_hadronFlavour[goodJets]");
    }
    //goot jets deep-b tag          
	_rlm = _rlm.Define("goodJets_jetdeepbtag", "Jet_btagDeepB[goodJets]")
                .Define("goodJets_deepjetbtag", "Jet_btagDeepFlavB[goodJets]") 
                .Define("NgoodJets", "int(goodJets_pt.size())")
                .Define("goodJets_4vecs", ::generate_4vec, {"goodJets_pt", "goodJets_eta", "goodJets_phi", "goodJets_mass"});

/*	//select b jest within goodjets 
    _rlm = _rlm.Define("btagcuts", "goodJets_deepjetbtag>0.7") //0.2783 -medium, 0.7 - tight 
      .Define("good_bjetpt", "goodJets_pt[btagcuts]")
      .Define("good_bjet_leading_pt", "int(good_bjetpt.size()) > 0 ? good_bjetpt[0] : -999.9")
      .Define("good_bjeteta", "goodJets_eta[btagcuts]")
      .Define("good_bjet_leading_eta", "int(good_bjeteta.size()) > 0 ? good_bjeteta[0] : -999.9")
      .Define("good_bjetphi", "goodJets_phi[btagcuts]")
      .Define("good_bjet_leading_phi", "int(good_bjetphi.size()) > 0 ? good_bjetphi[0] : -999.9")
      .Define("good_bjetmass", "goodJets_mass[btagcuts]")
      .Define("good_bjet_leading_mass", "int(good_bjetmass.size()) > 0 ? good_bjetmass[0] : -999.9")
      .Define("good_bjetdeepjet", "goodJets_deepjetbtag[btagcuts]");
  */

    _rlm = _rlm.Define("btagcuts", "goodJets_deepjetbtag > 0.7") // 0.2783 - medium, 0.7 - tight
               .Define("good_bjetpt", "goodJets_pt[btagcuts]")
//             .Define("good_bjet_leading_pt", "int(good_bjetpt.size()) > 0 ? static_cast<double>(good_bjetpt[0]) : -999.9")
               .Define("good_bjeteta", "goodJets_eta[btagcuts]")
//             .Define("good_bjet_leading_eta", "int(good_bjeteta.size()) > 0 ? static_cast<double>(good_bjeteta[0]) : -999.9")
               .Define("good_bjetphi", "goodJets_phi[btagcuts]")
//             .Define("good_bjet_leading_phi", "int(good_bjetphi.size()) > 0 ? static_cast<double>(good_bjetphi[0]) : -999.9")
               .Define("good_bjetmass", "goodJets_mass[btagcuts]");
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
	//cout << "overlap removal" << endl;
    //==============================Clean Jets==============================================//
    //Use clean jets/bjets for object selections
    //=====================================================================================//
/*
    _rlm = _rlm.Define("muonjetoverlap", checkoverlap, {"goodJets_4vecs","goodMuons_4vecs"});
	_rlm =	_rlm.Define("Selected_jetpt", "goodJets_pt[muonjetoverlap]")
		.Define("Selected_jeteta", "goodJets_eta[muonjetoverlap]")
		.Define("Selected_jetphi", "goodJets_phi[muonjetoverlap]")
		.Define("Selected_jetmass", "goodJets_mass[muonjetoverlap]")
	
		.Define("Selected_jetbtag", "goodJets_deepjetbtag[muonjetoverlap]") //
		.Define("Selected_jethadflav", "goodJets_hadflav[muonjetoverlap]") 
		.Define("ncleanjetspass", "int(Selected_jetpt.size())")
		.Define("cleanjet4vecs", ::generate_4vec, {"Selected_jetpt", "Selected_jeteta", "Selected_jetphi", "Selected_jetmass"})
		.Define("Selected_jetHT", "Sum(Selected_jetpt)");
        
     //==============================Clean b-Jets==============================================// 
	 //--> after remove overlap: use requested btaggedJets for btag-weight SFs && weight_generator. 
	 //=====================================================================================//
	_rlm = _rlm.Define("btagcuts2", "Selected_jetbtag>0.3040") //medium wp -->as an example. 
			.Define("Selected_bjetpt", "Selected_jetpt[btagcuts2]")
			.Define("Selected_bjeteta", "Selected_jeteta[btagcuts2]")
			.Define("Selected_bjetphi", "Selected_jetphi[btagcuts2]")
			.Define("Selected_bjetmass", "Selected_jetmass[btagcuts2]")
			.Define("ncleanbjetspass", "int(Selected_bjetpt.size())")
			.Define("Selected_bjetHT", "Sum(Selected_bjetpt)")
			.Define("Selected_bjethadflav", "Selected_jethadflav[btagcuts2]") 
			.Define("cleanbjet4vecs", ::generate_4vec, {"Selected_bjetpt", "Selected_bjeteta", "Selected_bjetphi", "Selected_bjetmass"});           

*/


//-----------CHECKING THE OVERLAPS WITH THE TRAILING MUON----------#######////

    _rlm = _rlm.Define("muonjetoverlap", checkoverlap, {"goodJets_4vecs","trailingMuons_4vecs"});
        _rlm =  _rlm.Define("Selected_jetpt", "goodJets_pt[muonjetoverlap]")
                .Define("Selected_jeteta", "goodJets_eta[muonjetoverlap]")
                .Define("Selected_jetphi", "goodJets_phi[muonjetoverlap]")
                .Define("Selected_jetmass", "goodJets_mass[muonjetoverlap]")
                .Define("Selected_jetbtag", "goodJets_deepjetbtag[muonjetoverlap]") 
                .Define("ncleanjetspass", "int(Selected_jetpt.size())")
                .Define("cleanjet4vecs", ::generate_4vec, {"Selected_jetpt", "Selected_jeteta", "Selected_jetphi", "Selected_jetmass"})
                .Define("Selected_jetHT", "Sum(Selected_jetpt)");
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
                        .Define("ncleanbjetspass", "int(Selected_bjetpt.size())")
                        .Define("Selected_bjetHT", "Sum(Selected_bjetpt)")
                        .Define("cleanbjet4vecs", ::generate_4vec, {"Selected_bjetpt", "Selected_bjeteta", "Selected_bjetphi", "Selected_bjetmass"});
        if (!_isData){
        _rlm = _rlm.Define("Selected_bjethadflav", "Selected_jethadflav[btagcuts2]");
        }

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

/*
//for the trailing muon need to be removed after check 
    //Scale Factors for Muon HLT, RECO, ID and ISO
  std::vector<std::string> Muon_vars_names = {"trailingMuons_eta", "trailingMuons_pt"};
  std::string output_mu_column_name = "muon_SF_";
  _rlm = calculateMuSF(_rlm, Muon_vars_names, output_mu_column_name);
*/


  //Scale Factors for Electron RECO and ID
  std::vector<std::string> Electron_vars_names = {"goodElectrons_eta", "goodElectrons_pt"};
  std::string output_ele_column_name = "ele_SF_";
  _rlm = calculateEleSF(_rlm, Electron_vars_names, output_ele_column_name);

//   _rlm = _rlm.Define("evWeight_wobtagSF", " pugenWeight * muon_SF_central * ele_SF_central"); 
//  _rlm = _rlm.Define("totbtagSF", "btag_SF_bcflav_central * btag_SF_lflav_central"); 
  //Total event Weight:

  //Prefiring Weight for 2016 and 2017
  _rlm = applyPrefiringWeight(_rlm);
  //Total event Weight:
  //_rlm = _rlm.Define("evWeight", " pugenWeight * prefiring_SF_central * btag_SF_bcflav_central * btag_SF_lflav_central * muon_SF_central * ele_SF_central"); 
//  _rlm = _rlm.Define("evWeight", " pugenWeight * prefiring_SF_central * muon_SF_central * ele_SF_central"); 
  _rlm = _rlm.Define("evWeight", " pugenWeight * prefiring_SF_central "); 
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
    {"tight_baselineElectrons", "tight_baselineMuons"});

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
                       [](const ROOT::VecOps::RVec<int>& electronCharges,
                          const ROOT::VecOps::RVec<int>& muonCharges) {
                           return ROOT::VecOps::Concatenate(electronCharges, muonCharges);
                       },
                       {"baselineElectrons_charge", "baselineMuons_charge"});

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

/*

	// First define the pt-sorted versions of all variables
	_rlm = _rlm.Define("goodLepton_sorted_indices", 
	    [](const ROOT::VecOps::RVec<float>& ptVec, int nLeptons) {
		if (nLeptons < 3) return ROOT::VecOps::RVec<size_t>{};
		
		// Create vector of indices
		ROOT::VecOps::RVec<size_t> indices(ptVec.size());
		std::iota(indices.begin(), indices.end(), 0);
		
		// Sort indices based on pt in descending order
		std::sort(indices.begin(), indices.end(), 
			 [&ptVec](size_t i, size_t j) { return ptVec[i] > ptVec[j]; });
		
		return indices;
	    }, {"combinedLeptonPt", "numCombinedLepton4Vecs"});

	// Now define all goodLepton variables using the sorted indices
	_rlm = _rlm.Define("goodLepton_pt", 
	    [](const ROOT::VecOps::RVec<float>& ptVec, 
	       const ROOT::VecOps::RVec<size_t>& indices, 
	       int nLeptons) {
		return nLeptons >= 3 ? ROOT::VecOps::Take(ptVec, indices) : ROOT::VecOps::RVec<float>{};
	    }, {"combinedLeptonPt", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	_rlm = _rlm.Define("goodLepton_eta", 
	    [](const ROOT::VecOps::RVec<float>& etaVec, 
	       const ROOT::VecOps::RVec<size_t>& indices, 
	       int nLeptons) {
		return nLeptons >= 3 ? ROOT::VecOps::Take(etaVec, indices) : ROOT::VecOps::RVec<float>{};
	    }, {"combinedLeptonEta", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	_rlm = _rlm.Define("goodLepton_phi", 
	    [](const ROOT::VecOps::RVec<float>& phiVec, 
	       const ROOT::VecOps::RVec<size_t>& indices, 
	       int nLeptons) {
		return nLeptons >= 3 ? ROOT::VecOps::Take(phiVec, indices) : ROOT::VecOps::RVec<float>{};
	    }, {"combinedLeptonPhi", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	_rlm = _rlm.Define("goodLepton_isPrompt", 
	    [](const ROOT::VecOps::RVec<int>& promptVec, 
	       const ROOT::VecOps::RVec<size_t>& indices, 
	       int nLeptons) {
		return nLeptons >= 3 ? ROOT::VecOps::Take(promptVec, indices) : ROOT::VecOps::RVec<int>{};
	    }, {"combinedLepton_isPrompt", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	_rlm = _rlm.Define("goodLepton_charge", 
	    [](const ROOT::VecOps::RVec<int>& chargeVec, 
	       const ROOT::VecOps::RVec<size_t>& indices, 
	       int nLeptons) {
		return nLeptons >= 3 ? ROOT::VecOps::Take(chargeVec, indices) : ROOT::VecOps::RVec<int>{};
	    }, {"combinedLeptonCharge", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	_rlm = _rlm.Define("goodLepton_flavor", 
	    [](const std::vector<int>& flavorVec, 
	       const ROOT::VecOps::RVec<size_t>& indices, 
	       int nLeptons) {
		if (nLeptons < 3) return std::vector<int>{};
		std::vector<int> sortedFlavor;
		for (auto i : indices) sortedFlavor.push_back(flavorVec[i]);
		return sortedFlavor;
	    }, {"combinedLeptonFlavor", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	_rlm = _rlm.Define("goodLepton_4Vecs", 
	    [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& vecs,
	       const ROOT::VecOps::RVec<size_t>& indices, 
	       int nLeptons) {
		if (nLeptons < 3) return std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{};
		std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>> sortedVecs;
		for (auto i : indices) sortedVecs.push_back(vecs[i]);
		return sortedVecs;
	    }, {"combinedLepton4Vecs", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});

	_rlm = _rlm.Define("goodLepton_TL4Vecs", 
	    [](const ROOT::VecOps::RVec<TLorentzVector>& vecs,
	       const ROOT::VecOps::RVec<size_t>& indices, 
	       int nLeptons) {
		return nLeptons >= 3 ? ROOT::VecOps::Take(vecs, indices) : ROOT::VecOps::RVec<TLorentzVector>{};
	    }, {"combinedLeptonTLorentzVecs", "goodLepton_sorted_indices", "numCombinedLepton4Vecs"});
*/
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


        /////////////////////////////////////////////
////////currently used but shifting to the upper one//////////////
       /////////////////////////////////////////////

void BaseAnalyser::mergeTrailingLeptons() {
    cout << "merge trailing electrons and muons" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    //-------------------------------------------------------
    // Define number of trailing leptons (sum of electrons and muons)
    //-------------------------------------------------------
    _rlm = _rlm.Define("N_trailingLeptons", [](int NtrailingElectrons, int NtrailingMuons) {
        return NtrailingElectrons + NtrailingMuons;
    }, {"NtrailingElectrons", "NtrailingMuons"});

    // Define variables only for events with exactly 3 trailing leptons
    _rlm = _rlm.Define("NtrailingLeptons", [](int N_trailingLeptons) {
    	return (N_trailingLeptons == 3) ? N_trailingLeptons : -1; // Mark invalid events with -1
    }, {"N_trailingLeptons"});

    // Optionally, define other columns conditioned on having 3 trailing leptons
    _rlm = _rlm.Define("ValidEvent", [](int N_trailingLeptons) {
	return (N_trailingLeptons == 3); // Boolean column for valid events
    }, {"N_trailingLeptons"});

    //-------------------------------------------------------
    // Combine trailing muons and electrons 4-vectors
    //-------------------------------------------------------
    _rlm = _rlm.Define("trailingLeptons_4vecs", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& muons_4vecs,
                                                   const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& electrons_4vecs) {
        std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>> combined_4vecs;
        combined_4vecs.insert(combined_4vecs.end(), muons_4vecs.begin(), muons_4vecs.end());
        combined_4vecs.insert(combined_4vecs.end(), electrons_4vecs.begin(), electrons_4vecs.end());
        return combined_4vecs;
    }, {"trailingMuons_4vecs", "trailingElectrons_4Vecs"});

    //-------------------------------------------------------
    // Define number of trailing 4-vectors in each event
    //-------------------------------------------------------
    _rlm = _rlm.Define("NtrailingLepton4vecs", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& trailingLeptons_4vecs) {
        return int(trailingLeptons_4vecs.size());
    }, {"trailingLeptons_4vecs"});

    //-------------------------------------------------------
    // Combine trailing leptons charge using ROOT::VecOps::Take and ROOT::VecOps::Concatenate
    //-------------------------------------------------------
    _rlm = _rlm.Define("trailingLeptons_charge",
                   [](const ROOT::VecOps::RVec<int>& trailingElectron_charge,
                      const ROOT::VecOps::RVec<int>& trailingMuon_charge) {
                       return ROOT::VecOps::Concatenate(trailingElectron_charge, trailingMuon_charge);
                   },
                   {"trailingElectrons_charge", "trailingMuons_charge"});

    //-------------------------------------------------------
    // Assign flavor to trailing leptons: 1 for muon, 0 for electron
    //-------------------------------------------------------
    _rlm = _rlm.Define("trailingLeptons_flavor", [](const ROOT::VecOps::RVec<int>& muon_charge,
                                                    const ROOT::VecOps::RVec<int>& electron_charge) {
        std::vector<int> flavor;
        flavor.insert(flavor.end(), muon_charge.size(), 1);  // 1 for muons
        flavor.insert(flavor.end(), electron_charge.size(), 0); // 0 for electrons
        return flavor;
    }, {"trailingMuons_charge", "trailingElectrons_charge"});

    //-------------------------------------------------------
    // Merge trailing muons and electrons into a ROOT::VecOps::RVec<TLorentzVector>
    //-------------------------------------------------------
    _rlm = _rlm.Define("trailingLeptons_TLorentzVecs",
                       [](const ROOT::VecOps::RVec<TLorentzVector>& muons_4vecs,
                          const ROOT::VecOps::RVec<TLorentzVector>& electrons_4vecs) {
                           return ROOT::VecOps::Concatenate(muons_4vecs, electrons_4vecs);
                       },
                       {"trailingMuons_TL4Vecs", "trailingElectrons_TL4Vecs"});

		       
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ======================================================================================================
// ***********************       THE OSSF PAIR         **************************************************
// ====================================================================================================== 

void BaseAnalyser::search_for_OSSFPairs() {
    cout << "Search OSSF Pairs" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    //-------------------------------------------------------
    // Define OSSF pair count calculation
    //-------------------------------------------------------
   _rlm = _rlm.Define("OSSF_pair_count", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons_4vecs,
                                         const ROOT::VecOps::RVec<int>& leptons_charge,
                                         const std::vector<int>& leptons_flavor) {
    int ossf_count = 0;

    // Separate leptons by charge
    std::vector<size_t> positive_indices, negative_indices;

    for (size_t i = 0; i < leptons_4vecs.size(); ++i) {
        if (leptons_charge[i] > 0) {
            positive_indices.push_back(i);
        } else {
            negative_indices.push_back(i);
        }
    }

    // Count valid OSSF pairs
    for (int flavor = 0; flavor <= 1; ++flavor) {  // 0: electrons, 1: muons
        std::vector<size_t> pos_list, neg_list;

        for (size_t idx : positive_indices) {
            if (leptons_flavor[idx] == flavor) pos_list.push_back(idx);
        }

        for (size_t idx : negative_indices) {
            if (leptons_flavor[idx] == flavor) neg_list.push_back(idx);
        }

        // Count all possible OSSF pairs
        ossf_count += pos_list.size() * neg_list.size();
    }

    return ossf_count;
    }, {"combinedLepton4Vecs", "combinedLeptonCharge", "combinedLeptonFlavor"});


         /////////////ALL OSSFPAIR MASS //////////////////// 
	 //                                               //
	 ///////////////////////////////////////////////////


    _rlm = _rlm.Define("OSSF_all_pairs_masses", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons_4vecs,
                                               const ROOT::VecOps::RVec<int>& leptons_charge,
                                               const std::vector<int>& leptons_flavor) {
          ROOT::VecOps::RVec<double> ossf_masses;  // Vector to store OSSF pair masses

    for (size_t i = 0; i < leptons_4vecs.size(); ++i) {
        for (size_t j = i + 1; j < leptons_4vecs.size(); ++j) {  // Ensure unique pairs
            if (leptons_flavor[i] == leptons_flavor[j] && leptons_charge[i] != leptons_charge[j]) {
                auto combined_4vec = leptons_4vecs[i] + leptons_4vecs[j];
                ossf_masses.push_back(combined_4vec.M());
            }
        }
    }

         return ossf_masses;
    }, {"combinedLepton4Vecs", "combinedLeptonCharge", "combinedLeptonFlavor"});
 









/////////this branch will helpul for |mz-m(l,l,l)|>window calculation////////

   _rlm = _rlm.Define("mZ_compatible_3l", [](double m3l) {
    constexpr double mZ = 91.1876; // Z boson mass in GeV
    constexpr double window = 15.0; // ±15 GeV window

    if (m3l == -1.0) {
        return -1; // Exclude events where mass_of_3lepton is invalid
    }
    return (std::abs(m3l - mZ) <= window) ? 1 : 0;    ////is 1 if compatible else 0
    }, {"mass_of_3lepton"});
    
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//-----------------------OSSF PAIT WITH TOP LEPTON---------------------------//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
    _rlm = _rlm.Define("OSSF_selection_with_topLep", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons_4vecs,
                                       const ROOT::VecOps::RVec<int>& leptons_charge,
                                       const std::vector<int>& leptons_flavor) {
    const double Z_mass = 91.1876; // Z boson mass
    const double mass_window = 15.0;
    const double lower_mass_threshold = 35.0;

    std::pair<int, int> best_z_pair = {-1, -1};
    std::pair<int, int> lower_mass_pair = {-1, -1};
    int top_lepton_idx = -1;

    double best_z_mass = -1.0;
    double best_lower_mass = -1.0;
    double best_z_mass_diff = std::numeric_limits<double>::max();
    double best_lower_mass_diff = std::numeric_limits<double>::max();

    // Iterate through all unique lepton pairs
    for (size_t i = 0; i < leptons_4vecs.size(); ++i) {
        for (size_t j = i + 1; j < leptons_4vecs.size(); ++j) {
            if (leptons_flavor[i] == leptons_flavor[j] && leptons_charge[i] != leptons_charge[j]) {
                auto combined_4vec = leptons_4vecs[i] + leptons_4vecs[j];
                double mass = combined_4vec.M();
                double mass_diff = std::abs(mass - Z_mass);

                // Z-compatible case: |M_Z - M_OSSF| <= 15 GeV
                if (mass_diff <= mass_window && mass_diff < best_z_mass_diff) {
                    best_z_mass_diff = mass_diff;
                    best_z_pair = {int(i), int(j)};
                    best_z_mass = mass;
                }

                // Lower-mass case: 35 < M < M_Z && |M_Z - M| > 15
                if ((mass > lower_mass_threshold && mass < Z_mass && mass_diff > mass_window && mass_diff < best_lower_mass_diff) ||
		    (mass > Z_mass + mass_window && mass_diff < best_lower_mass_diff))	{
                    best_lower_mass_diff = mass_diff;
                    lower_mass_pair = {int(i), int(j)};
                    best_lower_mass = mass;
                }
            }
        }
    }

    // Find top lepton if we have exactly 3 leptons
    if (leptons_4vecs.size() == 3 && best_z_pair.first != -1) {
        for (int i = 0; i < 3; ++i) {
            if (i != best_z_pair.first && i != best_z_pair.second) {
                top_lepton_idx = i;
                break;
            }
        }
    }

    return std::make_tuple(best_z_pair, best_z_mass, lower_mass_pair, best_lower_mass, top_lepton_idx);
    }, {"combinedLepton4Vecs", "combinedLeptonCharge", "combinedLeptonFlavor"});

	// Extract OSSF Z-compatible pair 4-vectors
	_rlm = _rlm.Define("OSSF_ZPair_4vecs", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons_4vecs,
						 const std::tuple<std::pair<int, int>, double, std::pair<int, int>, double, int>& ossf_info) {
	    auto [best_z_pair, _, __, ___, ____] = ossf_info;
	    if (best_z_pair.first == -1 || best_z_pair.second == -1) {
		return std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{};
	    }
	    return std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{
	    leptons_4vecs[best_z_pair.first], leptons_4vecs[best_z_pair.second]};
	    }, {"combinedLepton4Vecs", "OSSF_selection_with_topLep"});
        


	// Extract OSSF lower-mass pair 4-vectors
	_rlm = _rlm.Define("OSSF_LowerMassPair_4vecs", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons_4vecs,
							 const std::tuple<std::pair<int, int>, double, std::pair<int, int>, double, int>& ossf_info) {
	    auto [__, ___, lower_mass_pair, _, ____] = ossf_info;
	    if (lower_mass_pair.first == -1 || lower_mass_pair.second == -1) {
		return std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{};
	    }
	    return std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{
		leptons_4vecs[lower_mass_pair.first], leptons_4vecs[lower_mass_pair.second]};
	}, {"combinedLepton4Vecs", "OSSF_selection_with_topLep"});



	// Extract OSSF pair masses
	_rlm = _rlm.Define("OSSF_ZPair_mass", [](const std::tuple<std::pair<int, int>, double, std::pair<int, int>, double, int>& ossf_info) {
	    auto [_, best_z_mass, __, ___, ____] = ossf_info;
	    return best_z_mass > 0 ? best_z_mass : -1.0;
	}, {"OSSF_selection_with_topLep"});
  


	_rlm = _rlm.Define("OSSF_LowerMassPair_mass", [](const std::tuple<std::pair<int, int>, double, std::pair<int, int>, double, int>& ossf_info) {
	    auto [__, ___, _, best_lower_mass, ____] = ossf_info;
	    return best_lower_mass > 0 ? best_lower_mass : -1.0;
	}, {"OSSF_selection_with_topLep"});



        // Extract top lepton 4-vector
	_rlm = _rlm.Define("TopLepton_4vec", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons_4vecs,
					       const std::tuple<std::pair<int, int>, double, std::pair<int, int>, double, int>& ossf_info) {
	    auto [_, __, ___, ____, top_lepton_idx] = ossf_info;
	    if (top_lepton_idx == -1) {
		return ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>{};
	    }
	    return leptons_4vecs[top_lepton_idx];
	}, {"combinedLepton4Vecs", "OSSF_selection_with_topLep"});
        

       _rlm = _rlm.Define("TopLepton_TL4vec", [](const ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>& lepton_4vec) -> TLorentzVector {
              TLorentzVector tl4vec;
        tl4vec.SetPtEtaPhiM(lepton_4vec.Pt(), lepton_4vec.Eta(), lepton_4vec.Phi(), lepton_4vec.M());
        return tl4vec;
    }, {"TopLepton_4vec"});

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
    _rlm = _rlm.Define("OSSF_info", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons_4vecs,
                                       const ROOT::VecOps::RVec<int>& leptons_charge,
                                       const std::vector<int>& leptons_flavor) {
        const double Z_mass = 91.1876; // Z boson mass
        const double mass_window = 15.0; // ±15 GeV window
        std::pair<int, int> ossf_indices = {-1, -1};
        double closest_mass_diff = std::numeric_limits<double>::max();
        double ossf_mass = -1.0;

        // Iterate through all unique lepton pairs
        for (size_t i = 0; i < leptons_4vecs.size(); ++i) {
            for (size_t j = i + 1; j < leptons_4vecs.size(); ++j) {
                // Check if same flavor and opposite charge
                if (leptons_flavor[i] == leptons_flavor[j] && leptons_charge[i] != leptons_charge[j]) {
                    // Calculate invariant mass
                    auto combined_4vec = leptons_4vecs[i] + leptons_4vecs[j];
                    double mass = combined_4vec.M();

                    // Check if within mass window and closer to Z boson mass
                    double mass_diff = std::abs(mass - Z_mass);
                    if (mass_diff < mass_window && mass_diff < closest_mass_diff) {
                        closest_mass_diff = mass_diff;
                        ossf_indices = {int(i), int(j)};
                        ossf_mass = mass;
                    }
                }
            }
        }

        return std::make_tuple(ossf_indices, ossf_mass);
    }, {"trailingLeptons_4vecs", "trailingLeptons_charge", "trailingLeptons_flavor"});

    //-------------------------------------------------------
    // Store OSSF pair and trailing lepton information
    //-------------------------------------------------------
    _rlm = _rlm.Define("OSSF_4vecs", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons_4vecs,
                                        const std::tuple<std::pair<int, int>, double>& ossf_info) {
        auto [ossf_indices, _] = ossf_info;
        if (ossf_indices.first == -1 || ossf_indices.second == -1) {
            // No valid OSSF pair found, discard event
            return std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{};
        }
        // Return the OSSF pair 4-vectors
        return std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{
            leptons_4vecs[ossf_indices.first], leptons_4vecs[ossf_indices.second]};
    }, {"trailingLeptons_4vecs", "OSSF_info"});

    //-------------------------------------------------------
    // Store the top lepton 4-vector (remaining lepton not part of OSSF pair)
    //-------------------------------------------------------
    _rlm = _rlm.Define("topLepton_4vecs", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& leptons_4vecs,
                                            const std::tuple<std::pair<int, int>, double>& ossf_info) {
        auto [ossf_indices, _] = ossf_info;
        std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>> top_leptons;

        if (ossf_indices.first != -1 && ossf_indices.second != -1) {
            // Only add the remaining lepton to topLepton_4vecs
            for (size_t i = 0; i < leptons_4vecs.size(); ++i) {
                if (i != ossf_indices.first && i != ossf_indices.second) {
                    top_leptons.push_back(leptons_4vecs[i]);
                    break;  // Only one lepton remains
                }
            }
        }

        // If no valid OSSF pair, the event is discarded (empty vector for topLepton_4vecs)
        return top_leptons;
    }, {"trailingLeptons_4vecs", "OSSF_info"});
    // Extract pt, eta, and phi for the OSSF pair
_rlm = _rlm
    .Define("OSSF_pt", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& ossf_4vecs) {
        return ossf_4vecs.size() == 2 
            ? std::vector<double>{ossf_4vecs[0].Pt(), ossf_4vecs[1].Pt()} 
            : std::vector<double>{};
    }, {"OSSF_4vecs"})
    .Define("OSSF_eta", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& ossf_4vecs) {
        return ossf_4vecs.size() == 2 
            ? std::vector<double>{ossf_4vecs[0].Eta(), ossf_4vecs[1].Eta()} 
            : std::vector<double>{};
    }, {"OSSF_4vecs"})
    .Define("OSSF_phi", [](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& ossf_4vecs) {
        return ossf_4vecs.size() == 2 
            ? std::vector<double>{ossf_4vecs[0].Phi(), ossf_4vecs[1].Phi()} 
            : std::vector<double>{};
    }, {"OSSF_4vecs"});
    //-------------------------------------------------------
    // Store the invariant mass of the OSSF pair (filtering negative masses)
    //-------------------------------------------------------
    _rlm = _rlm.Define("OSSF_mass_all", [](const std::tuple<std::pair<int, int>, double>& ossf_info) {
        auto [_, mass] = ossf_info;

        // Only keep positive values for OSSF mass
        if (mass > 0) {
            return mass;
        } else {
            return -1.0;  // Mark invalid OSSF mass as -1
        }
    }, {"OSSF_info"});
//    _rlm = _rlm.Filter("OSSF_mass>0","mass within window");
    _rlm =_rlm.Define("OSSF_mass", [](double OSSF_mass_all) {
		    return OSSF_mass_all > 0;
	},{"OSSF_mass_all"});
    //-------------------------------------------------------
    // Store the top lepton TLorentzVector from trailingLeptons_TLorentzVecs (remaining lepton not part of OSSF pair)
    //-------------------------------------------------------
    _rlm = _rlm.Define("topLepton_TLorentzVector", [](const ROOT::VecOps::RVec<TLorentzVector>& trailingLeptons_TLorentzVecs,
                                                     const std::tuple<std::pair<int, int>, double>& ossf_info) {
        auto [ossf_indices, _] = ossf_info;

        // Only add the remaining lepton
        if (ossf_indices.first == -1 || ossf_indices.second == -1) {
            return TLorentzVector{};
        }

        for (size_t i = 0; i < trailingLeptons_TLorentzVecs.size(); ++i) {
            if (i != ossf_indices.first && i != ossf_indices.second) {
                return trailingLeptons_TLorentzVecs[i];
            }
        }

        return TLorentzVector{}; // In case no valid lepton is found
    }, {"trailingLeptons_TLorentzVecs", "OSSF_info"});
   
   _rlm = _rlm.Define("ntopLepton", [](const ROOT::VecOps::RVec<TLorentzVector>& trailingLeptons_TLorentzVecs,
                                    const std::tuple<std::pair<int, int>, double>& ossf_info) {
    auto [ossf_indices, _] = ossf_info;

    // Count remaining leptons
    if (ossf_indices.first == -1 || ossf_indices.second == -1) {
        return 0; // No OSSF pair means no top leptons
    }

    int count = 0;
    for (size_t i = 0; i < trailingLeptons_TLorentzVecs.size(); ++i) {
        if (i != ossf_indices.first && i != ossf_indices.second) {
            count++;
        }
    }

    return count;
}, {"trailingLeptons_TLorentzVecs", "OSSF_info"}); 
}

//MET

void BaseAnalyser::selectMET()
{
    if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

    _rlm = _rlm.Define("goodMET_sumET","MET_sumEt>800")
                .Define("goodMET_pt","MET_pt>20");
                //.Define("goodMET_eta","MET_eta[goodMET]")
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

    _rlm = _rlm.Define("lambda_reco", ::calculateLambda, {"TopLepton_TL4vec", "nu_pt", "nu_phi"});

    _rlm = _rlm.Define("delta_reco", ::calculateDelta, {"TopLepton_TL4vec", "nu_pt", "lambda_reco"})
               .Define("isRealSolution", "delta_reco > 0 ? 1 : -1");

    _rlm = _rlm.Define("nu_pz", ::calculate_nu_z, {"TopLepton_TL4vec", "lambda_reco", "delta_reco", "nu_pt", "nu_phi"});

    _rlm = _rlm.Define("nu_energy", ::calculate_nu_energy, {"nu_pt", "nu_phi", "nu_pz"});

    _rlm = _rlm.Define("nu_TL4vec", ::get_neutrino_TL4vec, {"nu_pt", "nu_phi", "nu_pz", "nu_energy"});

    //--------------------- Reconstruct W boson ---------------------
    _rlm = _rlm.Define("Wboson_4vec", ::reconstructWboson_TL4vec, {"TopLepton_TL4vec", "nu_TL4vec"})
               .Define("w_mass", "Wboson_4vec.M()")
               .Define("w_eta", "Wboson_4vec.Eta()")
               .Define("w_phi", "Wboson_4vec.Phi()")
               .Define("w_pt", "Wboson_4vec.Pt()");

    // Calculate transverse mass of the W boson
    _rlm = _rlm.Define("topLepton_phi", "TopLepton_TL4vec.Phi()")
               .Define("topLepton_eta", "TopLepton_TL4vec.Eta()")
               .Define("topLepton_pt", "TopLepton_TL4vec.Pt()")
               .Define("delta_phi_lep_nu", ::calculate_deltaPhi_scalars, {"topLepton_phi", "nu_phi_double"})
               .Define("Wboson_transversMass", "sqrt(2 * TopLepton_TL4vec.Pt() * nu_pt * (1 - cos(delta_phi_lep_nu)))");
}

/*
void BaseAnalyser::reconstructTop()
{
    if (debug){
    std::cout<<std::endl;
    std::cout<< "================================//=================================" << std::endl;
    std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
    std::cout<< "================================//=================================" << std::endl;
    }

//    _rlm = _rlm.Define("bQuark_forReco", "region == 0.0 ? good_bjet_TL4vec : numbLorentzVector")
  //   _rlm = _rlm.Define("b_mass","top_Bjet_TL4Vecs.M()");

    _rlm = _rlm.Define("topQuark_TL4vec", "Wboson_4vec + top_Bjet_TL4Vecs");

    _rlm = _rlm.Define("top_mass", "topQuark_TL4vec.M()")
	       .Filter("top_mass > 0", "Events with top mass")
               .Define("top_pt", "topQuark_TL4vec.Pt()");

}
*/
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
    _rlm = _rlm.Define("top_mass", "topQuark_TL4vec.M()")
               .Define("top_pt", "topQuark_TL4vec.Pt()")
               .Define("top_phi", "topQuark_TL4vec.Phi()")
               .Define("top_eta", "topQuark_TL4vec.Eta()");
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

    // Define signal region selection

    _rlm = _rlm.Define("passLeptonSelection_1", [](const ROOT::VecOps::RVec<float>& combinedLeptonPt) {
	    int count_gt10 = 0;
	    int count_gt15 = 0;
	    bool has_gt25 = false;
	    
	    for (auto pt : combinedLeptonPt) {
		if (pt > 10) count_gt10++;
		if (pt > 15) count_gt15++;
		if (pt > 25) has_gt25 = true;
	    }
	    
	    // Check all conditions:
	    // 1. All leptons > 10 (count_gt10 == size of the collection)
	    // 2. At least 2 leptons > 15
	    // 3. At least 1 lepton > 25
	    return (count_gt10 == combinedLeptonPt.size()) && 
		   (count_gt15 >= 2) && 
		   has_gt25;
            }, {"combinedLeptonPt"});

    _rlm = _rlm.Define("signalRegion", "totalLeptonCount == 3 && allTightLeptons && passLeptonSelection_1 &&" 
                                    " OSSF_ZPair_mass > 0 && NgoodJets > 2 && Ngood_bjets > 1");
    _rlm = _rlm.Define("signalRegion_top", "signalRegion && top_mass >0")
               .Define("signalRegion_top_mass", "signalRegion ? top_mass : std::numeric_limits<double>::quiet_NaN()");
    // Define additional variables for selected events in signal region
    _rlm = _rlm.Define("signalRegion_all_top", "signalRegion ? top_mass : std::numeric_limits<double>::quiet_NaN()");
/*                .Define("signalRegionLeptonPt", "combinedLeptonPt[signalRegion]")
                .Define("signalRegionOSSF_ZPair_masses", "OSSF_ZPair_masses[signalRegion]")
                .Define("signalRegionNgoodJets", "ngoodJets[signalRegion]")
  		.Define("signalRegionNgoodBjets", "nGOODBjets[signalRegion]");
*/		

    _rlm = _rlm.Define("trialRegion", " ncleanjetspass >= 2 && ncleanbjetspass >= 1 && All_good_tightLeptons")
	       .Define("trialRegion_lead" , "trialRegion && leadingLepton_pt > 0")
	       .Define("trialRegion_sublead" , "trialRegion && subleadingLepton_pt > 0")
	       .Define("trialRegion_trail" , "trialRegion && TrailingLepton_pt > 0")
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
               .Define("TR_trailingLepton_eta",
                   [](bool cond, float eta) { return cond ? eta : -999.f; },
                   {"trialRegion_trail", "TrailingLepton_eta"});       
    
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
   // addVartoStore("LHE_HT");
    //addVartoStore("genWeight");
    //addVartoStore("genEventSumw");

    //electron
    addVartoStore("nElectron");
    addVartoStore("baselineElectrons_idx");
    //addVartoStore("ngoodElectrons");
    addVartoStore("Electron_charge");
    addVartoStore("Electron_pt");
/*    addVartoStore("NtrailingElectrons");
    addVartoStore("trailingElectrons_pt");
    addVartoStore("trailingElectrons_eta");
    addVartoStore("trailingElectrons_phi");*/
    ////BDT VARIABLES FOR ELECTRONS///////////
    addVartoStore("fakable_baselineElectrons");  // based on the the baselineElectrons_MVAEstimatorRun2Fall17NoIsoV2Values greater than 0.4
    addVartoStore("tight_baselineElectrons");
    //addVartoStore("baselineElectrons_isPrompt");  // this is decided using genPartFlav

    addVartoStore("baselineElectrons_pt");
    addVartoStore("baselineElectrons_eta");
    addVartoStore("baselineElectrons_phi");
    addVartoStore("NbaselineElectrons");
    addVartoStore("baselineElectrons_JetNDauCharged");
    addVartoStore("baselineElectrons_miniPFRelIso_chg");
    addVartoStore("baselineElectrons_miniPFRelIso_neutral");
    addVartoStore("baselineElectrons_jetPtRelv2");
    addVartoStore("baselineElectrons_JetPtRatio");
    addVartoStore("baselineElectrons_pfRelIso03_all");
    addVartoStore("baselineElectrons_sip3d");
    addVartoStore("baselineElectrons_dxy");
    addVartoStore("baselineElectrons_dz");
    addVartoStore("baselineElectrons_JetPt_ratio");
    addVartoStore("baselineElectrons_MVAEstimatorRun2Fall17NoIsoV2Values");
  //  addVartoStore("Electron_closestJetIds");
  //  addVartoStore("Electron_all_deltaR");


    //muon
    addVartoStore("nMuon");
    addVartoStore("Muon_charge");
    addVartoStore("Muon_mass");
    addVartoStore("Muon_pt");
   // addVartoStore("goodMuons_pt");
    addVartoStore("NtrailingMuons");
    addVartoStore("trailingMuons_pt");
    addVartoStore("trailingMuons_eta");
    addVartoStore("trailingMuons_phi");
    ////BDT VARIABLES FOR MUONS///////////
    addVartoStore("baselineMuons_isPrompt");
    addVartoStore("NbaselineMuons");
    addVartoStore("baselineMuons_pt");
    addVartoStore("baselineMuons_eta");
    addVartoStore("baselineMuons_phi");
    addVartoStore("baselineMuons_JetNDauCharged");
    addVartoStore("baselineMuons_miniPFRelIso_chg");
    addVartoStore("baselineMuons_miniPFRelIso_neutral");
    addVartoStore("baselineMuons_jetPtRelv2");
    addVartoStore("baselineMuons_pfRelIso04_all");
    addVartoStore("baselineMuons_sip3d");
    addVartoStore("baselineMuons_dxy");
    addVartoStore("baselineMuons_dz");
    //addVartoStore("baselineMuons_JetPt_ratio");
    addVartoStore("baselineMuons_segmentCompatibility");
    addVartoStore("baselineMuons_JetPtRatio");
   // addVartoStore("baselineMuon_4Vecs_RVec");
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

    //trailing leptons
   // addVartoStore("totalLeptonCount");
   // addVartoStore("N_trailingLeptons");
  //  addVartoStore("mass_of_3lepton");
  //  addVartoStore("NtrailingLeptons");
   // addVartoStore("trailingLeptons_4vecs");
   // addVartoStore("trailingLeptons_charge");
   // addVartoStore("trailingLeptons_flavor");
   // addVartoStore("NtrailingLepton4vecs");
  //  addVartoStore("ValidEvent");

//new funciton variable for merged lepton
    addVartoStore("totalLeptonCount");
   // addVartoStore("combinedLepton4Vecs");
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


   

    addVartoStore("TR_leadingLepton_pt");
    addVartoStore("TR_subleadingLepton_pt");
    addVartoStore("TR_trailingLepton_pt");
    addVartoStore("TR_leadingLepton_eta");
    addVartoStore("TR_subleadingLepton_eta");
    addVartoStore("TR_trailingLepton_eta");



   //OSSF info
    addVartoStore("OSSF_pair_count");
    addVartoStore("mZ_compatible_3l");
    addVartoStore("OSSF_selection_with_topLep");
    addVartoStore("OSSF_all_pairs_masses");
    addVartoStore("OSSF_ZPair_mass");
    addVartoStore("OSSF_LowerMassPair_mass");





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
    addVartoStore("signalRegion_top_mass");
    addVartoStore("signalRegion_all_top");
    addVartoStore("top_pt");
    addVartoStore("top_eta");
    addVartoStore("top_phi");
    addVartoStore("b_mass");
   
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

//    addVartoStore("evWeight");   
    
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
	removeOverlaps();
	mergeLeptons();
	mergeTrailingLeptons();
	search_for_OSSFPairs();
	processOSSFPairs();
	reconstructWboson();
	reconstructTop();
	defineSignalRegion();
	//calculateZBosonMass();
	//identifyOSSFElectronPair();
	// defineTwoElectronEvent();
	if(!_isData){
	  this->calculateEvWeight(); // PU, genweight and BTV and Mu and Ele
	}
	selectMET();

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

