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
#include "GenParticleHelper.h"
#include <fstream>
#include "correction.h"
using correction::CorrectionSet;
using RVecF   = ROOT::VecOps::RVec<float>;
using RVecI   = ROOT::VecOps::RVec<int>;
using RVec4Vec = ROOT::VecOps::RVec<ROOT::Math::PtEtaPhiMVector>;


BaseAnalyser::BaseAnalyser(TTree *t, std::string outfilename)
:NanoAODAnalyzerrdframe(t, outfilename)
{
    //initiliaze the HLT names in your analyzer class
        HLT2022Names = {
            "HLT_Ele32_WPTight_Gsf",
            "HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL",
            "HLT_Ele16_Ele12_Ele8_CaloIdL_TrackIdL",
            "HLT_IsoMu24",
            "HLT_IsoMu27",
            "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8",
            "HLT_TripleMu_12_10_5",
            "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ",
            "HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ",
            "HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ",
            "HLT_Mu8_DiEle12_CaloIdL_TrackIdL",
            "HLT_DiMu9_Ele9_CaloIdL_TrackIdL_DZ"
            };


	HLT2022EENames = {
            "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL",
            "HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ",
            "HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ",
            "HLT_Mu37_Ele27_CaloIdL_MW",
            "HLT_Mu27_Ele37_CaloIdL_MW",
            "HLT_DiMu9_Ele9_CaloIdL_TrackIdL_DZ",
            "HLT_Mu8_DiEle12_CaloIdL_TrackIdL",
            "HLT_Mu8_DiEle12_CaloIdL_TrackIdL_DZ",
            "HLT_Ele30_WPTight_Gsf",
            "HLT_Ele32_WPTight_Gsf",
            "HLT_Ele35_WPTight_Gsf",
            "HLT_Ele38_WPTight_Gsf",
            "HLT_Ele40_WPTight_Gsf",
            "HLT_Ele115_CaloIdVT_GsfTrkIdT",
            "HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL",
            "HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ",
            "HLT_DoubleEle33_CaloIdL_MW",
            "HLT_DoubleEle25_CaloIdL_MW",
            "HLT_DoubleEle27_CaloIdL_MW",
            "HLT_Ele16_Ele12_Ele8_CaloIdL_TrackIdL",
            "HLT_IsoMu24",
            "HLT_IsoMu24_eta2p1",
            "HLT_IsoMu27",
            "HLT_Mu50",
            "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8",
            "HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8",
            "HLT_Mu19_TrkIsoVVL_Mu9_TrkIsoVVL_DZ_Mass8",
            "HLT_Mu19_TrkIsoVVL_Mu9_TrkIsoVVL_DZ_Mass3p8",
            "HLT_TripleMu_10_5_5_DZ",
            "HLT_TripleMu_12_10_5"
        };

}

void BaseAnalyser::defineCuts()
{
    if (debug) {
        std::cout << "\n=============================================\n"
                  << "Line: " << __LINE__ 
                  << " | Function: " << __FUNCTION__ << "\n"
                  << "=============================================\n";
    }

    // -----------------------------------------------------
    // Event statistics
    // -----------------------------------------------------
    auto nEntries = _rlm.Count();
    std::cout << "---------------------------------------------------\n"
              << "Total entries: " << *nEntries << "\n"
              << "---------------------------------------------------\n";

    // -----------------------------------------------------
    // Basic Event Quality
    // -----------------------------------------------------
    const std::string primarySelection =
        "(NbaselineElectrons+NbaselineMuons) >= 3";       
        
    const std::string minimalSelection =
        "PV_npvsGood >= 1 && "
        "NgoodLepton >= 3 ";
 
    const std::string metFilters =
        "Flag_goodVertices && "
        "Flag_globalSuperTightHalo2016Filter && "
        "Flag_EcalDeadCellTriggerPrimitiveFilter && "
        "Flag_BadPFMuonFilter && "
        "Flag_BadPFMuonDzFilter && "
        "Flag_hfNoisyHitsFilter && "
        "Flag_eeBadScFilter && "
        "Flag_ecalBadCalibFilter";
    
    const std::string jetVetoCut = "!vetoed_jets";
    // -----------------------------------------------------
    // Apply Cuts (ordered logically)
    // -----------------------------------------------------
    addCuts(setHLT(), "0");
    addCuts(metFilters, "00");
    addCuts(primarySelection, "000");
    addCuts(minimalSelection, "0000");
    addCuts(jetVetoCut, "00000");

    std::string cut0 = primarySelection;
    std::string cut1 = "(" + cut0 + ") && (" + setHLT() + ")";
    std::string cut2 = "(" + cut1 + ") && (" + metFilters + ")";
    std::string cut3 = "(" + cut2 + ") && (" + minimalSelection + ")";
    std::string cut4 = "(" + cut3 + ") && (" + jetVetoCut + ")";

    // Cutflow order
    // addCuts(cut0, "0");  // >=3 reconstructed electrons+muons
    // addCuts(cut1, "1");  // HLT
    // addCuts(cut2, "2");  // MET filters
    // addCuts(cut3, "3");  // PV + good leptons
    // addCuts(cut4, "4");  // Jet veto

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



    // =====================================================================
    // baselineElectrons_isPromptBaseline Electron Selection
    // =====================================================================
    _rlm = _rlm.Define("baselineElectrons", 
            "Electron_pt_corr> 20.0 && abs(Electron_eta) < 2.5 && "
                "Electron_cutBased >=4 &&" 
                "!(abs(Electron_eta) > 1.442 && abs(Electron_eta) < 1.566) && " 
                "abs(Electron_dxy) < 0.05 && " 
                "abs(Electron_dz) < 0.10 && Electron_lostHits <= 1 && " 
                "Electron_hoe < 0.10 && Electron_convVeto &&" 
                "((abs(Electron_eta) < 1.479 && Electron_sieie < 0.011) || " // Barrel cut 
                "(abs(Electron_eta) >= 1.479 && abs(Electron_eta) < 2.5 && Electron_sieie < 0.030)) &&" // Endcap cut 
                "Electron_sip3d < 8 && Electron_eInvMinusPInv > -0.04 "
                // "Electron_miniPFRelIso_all < 0.40 &&"
                // "&& Electron_promptMVA > 0.90"
                ); 

//     _rlm = _rlm.Define("baselineElectrons", 
//             "Electron_pt_corr > 20.0 && abs(Electron_eta) < 2.5 && "
//             "!(abs(Electron_eta) > 1.442 && abs(Electron_eta) < 1.566) &&"
//             "abs(Electron_dxy) < 0.05 && "
//             "abs(Electron_dz) < 0.10 && "
//             "Electron_sip3d < 8 &&"
//             // "Electron_promptMVA > 0.90");  
//             "Electron_cutBased >=4");  //Tight ID



    _rlm = _rlm.Define("baselineElectrons_pt", "Electron_pt_corr[baselineElectrons]")
                .Define("baselineElectrons_eta", "Electron_eta[baselineElectrons]")
                .Define("baselineElectrons_phi", "Electron_phi[baselineElectrons]")
                .Define("baselineElectrons_mass", "Electron_mass[baselineElectrons]")
                .Define("baselineElectrons_charge", "Electron_charge[baselineElectrons]")
                .Define("baselineElectrons_idx", ::good_idx, {"baselineElectrons"})
                .Define("baselineElectrons_pdgId", "Electron_pdgId[baselineElectrons]")
                .Define("NbaselineElectrons", "int(baselineElectrons_pt.size())");

    if (!_isData) {
    _rlm = _rlm.Define("Gen_baselineElectrons_genPartFlav", "Electron_genPartFlav[baselineElectrons]")
               .Define("Gen_baselineElectrons_genPartIdx", "Electron_genPartIdx[baselineElectrons]");
    }

    if (_year == 2024)
    {
        // Define tight and fakable electrons based on MVA score
        _rlm = _rlm.Define("TightElectrons", "baselineElectrons && Electron_promptMVA > 0.90")
                    .Define("baselineElectrons_mvaTTH", "Electron_promptMVA[baselineElectrons]")
                    .Define("tight_baselineElectrons", "Electron_promptMVA[baselineElectrons] > 0.90");
    }else
    {
        // Define tight and fakable electrons based on MVA score
        _rlm = _rlm.Define("TightElectrons", "baselineElectrons && Electron_mvaTTH > 0.90")
                    .Define("baselineElectrons_mvaTTH", "Electron_mvaTTH[baselineElectrons]")
                    .Define("tight_baselineElectrons", "Electron_mvaTTH[baselineElectrons] > 0.90");

    }

    // Generate 4-vectors for baseline electrons
    _rlm = _rlm.Define("baselineElectron_4Vecs", ::generate_4vec, {"baselineElectrons_pt", "baselineElectrons_eta", "baselineElectrons_phi", "baselineElectrons_mass"});
    _rlm = _rlm.Define("baselineElectron_TL4Vecs", ::buildTLorentzVectors, {"baselineElectrons_pt", "baselineElectrons_eta", "baselineElectrons_phi", "baselineElectrons_mass"});
    
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
    _rlm = _rlm.Define("goodMuonsID", MuonID(2)); // tight muons
    // ====================================================================
    // Baseline Muon Selection
    // =====================================================================
     _rlm = _rlm.Define("baselineMuons", 
		        "Muon_pt_corr> 20.0 && abs(Muon_eta) < 2.4 && " 
                "abs(Muon_dxy) < 0.05 && " 
		        "abs(Muon_dz) < 0.10 && Muon_sip3d < 8.0 && " 
                "Muon_tightId &&"
                "Muon_isPFcand" 
		        "&& (Muon_isGlobal || Muon_isTracker)"
                // "&& Muon_promptMVA > 0.64"
                // "Muon_mediumId &&"
                // "Muon_miniPFRelIso_all < 0.4 &&"
               ); 

//     _rlm = _rlm.Define("baselineMuons", "Muon_pt_corr > 20 && "
//             "abs(Muon_eta) < 2.4 && "
//             "Muon_tightId &&"
//             // "Muon_mediumId &&"
//             // "Muon_promptMVA > 0.64 &&"
//             "abs(Muon_dxy) < 0.05 && abs(Muon_dz) < 0.10 && Muon_sip3d < 8.0");



    _rlm = _rlm.Define("baselineMuons_pt", "Muon_pt_corr[baselineMuons]")
	       .Define("baselineMuons_eta", "Muon_eta[baselineMuons]")
	       .Define("baselineMuons_phi", "Muon_phi[baselineMuons]")
	       .Define("baselineMuons_mass", "Muon_mass[baselineMuons]")
	       .Define("baselineMuons_charge", "Muon_charge[baselineMuons]")
           .Define("baselineMuons_idx", ::good_idx, {"baselineMuons"})
	       .Define("baselineMuons_pdgId", "Muon_pdgId[baselineMuons]")
	       .Define("NbaselineMuons", "int(baselineMuons_pt.size())");
         
	_rlm = _rlm
        .Define("leadingMuon_pt",    "baselineMuons_pt.size() == 3 ? baselineMuons_pt[0] : -999.f")
        .Define("subleadingMuon_pt", "baselineMuons_pt.size() == 3 ? baselineMuons_pt[1] : -999.f")
        .Define("trailingMuon_pt",   "baselineMuons_pt.size() == 3 ? baselineMuons_pt[2] : -999.f")

        .Define("leadingMuon_eta",    "baselineMuons_eta.size() == 3 ? baselineMuons_eta[0] : -999.f")
        .Define("subleadingMuon_eta", "baselineMuons_eta.size() == 3 ? baselineMuons_eta[1] : -999.f")
        .Define("trailingMuon_eta",   "baselineMuons_eta.size() == 3 ? baselineMuons_eta[2] : -999.f");
    
    if (!_isData) {
    _rlm = _rlm
        .Define("truth_baselineMuons_genPart", "Muon_genPartIdx[baselineMuons]")
        .Define("maxDR_muon", "0.3f")
        .Define("baselineMuons_fromTopW", Muon_FromTopW,
               {"baselineMuons_pt", "baselineMuons_eta", "baselineMuons_phi",
                "GenPart_pdgId", "GenPart_genPartIdxMother",
                "GenPart_eta", "GenPart_phi", "maxDR_muon"})
        .Define("truth_baselineMuons_origin",::GetLeptonOrigin, {"truth_baselineMuons_genPart","GenPart_pdgId","GenPart_statusFlags", "GenPart_genPartIdxMother"});
    _rlm = _rlm.Define("Ntruth_baselineMuons_origin3","Sum(truth_baselineMuons_origin == 4)");
    _rlm = _rlm.Define("GenPromptLeptonIdx", GenParticleHelper::GetAllPromptGenLeptonIndices, {"GenPart_pdgId", "GenPart_genPartIdxMother", "GenPart_statusFlags"});
    _rlm = _rlm.Define("GenPromptLeptonPdgId", GenParticleHelper::GetPromptGenLeptonPdgId, {"GenPromptLeptonIdx","GenPart_pdgId"});
    _rlm = _rlm.Define("Nunique_origin3",::CountUniqueOrigin3,{"truth_baselineMuons_genPart","truth_baselineMuons_origin"});

    // Per-GenPart-entry flag array
    _rlm = _rlm.Define("GenPart_muFromTopW", GenPart_MuonFromTopW,
                        {"GenPart_pdgId", "GenPart_genPartIdxMother"});

    // Event-level boolean flag
    _rlm = _rlm.Define("hasGenMuFromTopW", Event_HasGenMuonFromTopW,
                        {"GenPart_pdgId", "GenPart_genPartIdxMother"});
    _rlm = _rlm.Define("Muon_originCheck", CompareLeptonOriginToGenPartFlag,
                    {"truth_baselineMuons_genPart", "truth_baselineMuons_origin", "GenPart_muFromTopW"});

    }
    if (!_isData) {
    _rlm = _rlm.Define("Gen_baselineMuons_genPartFlav", "Muon_genPartFlav[baselineMuons]")
               .Define("Gen_baselineMuons_genPartIdx", "Muon_genPartIdx[baselineMuons]");
    }

    //-------------------------------------------------------
    // Generate muon 4-vector from selected good muons
    //-------------------------------------------------------
    _rlm = _rlm.Define("baselineMuon_4Vecs", ::generate_4vec, {"baselineMuons_pt", "baselineMuons_eta", "baselineMuons_phi", "baselineMuons_mass"});
    _rlm = _rlm.Define("baselineMuon_TL4Vecs",::buildTLorentzVectors, {"baselineMuons_pt", "baselineMuons_eta", "baselineMuons_phi", "baselineMuons_mass"});
    if (_year == 2024)
    {
    _rlm = _rlm.Define("tight_Muons", "Muon_promptMVA[baselineMuons] > 0.64");
    }else
    {
    _rlm = _rlm.Define("tight_Muons", "Muon_mvaTTH[baselineMuons] > 0.64");
    }
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


    if (_year == 2024)
    {
    // Tight + TightLeptonVeto cut
    _rlm = _rlm.Define("goodJetsID",
	[](const ROOT::VecOps::RVec<UChar_t>& neMult,
	   const ROOT::VecOps::RVec<UChar_t>& chMult,
	   const ROOT::VecOps::RVec<float>& pt,
	   const ROOT::VecOps::RVec<float>& neEmEF,
	   const ROOT::VecOps::RVec<float>& chEmEF,
	   const ROOT::VecOps::RVec<float>& chHEF,
	   const ROOT::VecOps::RVec<float>& neHEF,
	   const ROOT::VecOps::RVec<float>& muEF,
	   const ROOT::VecOps::RVec<float>& eta)
	{
	    ROOT::VecOps::RVec<char> mask(pt.size(), false);

	    for (size_t i = 0; i < pt.size(); i++) {

		if (pt[i] <= 25.0 || std::abs(eta[i]) >= 5.0) continue;

		bool is_tight = false;
		float aeta = std::abs(eta[i]);

		// |eta| <= 2.6
		if (aeta <= 2.6) {
		    is_tight =
			neHEF[i] < 0.99 &&
			neEmEF[i] < 0.90 &&
			(chMult[i] + neMult[i]) > 1 &&
			chHEF[i] > 0.01 &&
			chMult[i] > 0;
		}

		// 2.6 < |eta| <= 2.7
		else if (aeta <= 2.7) {
		    is_tight =
			neHEF[i] < 0.90 &&
			neEmEF[i] < 0.99;
		}

		// 2.7 < |eta| <= 3.0
		else if (aeta <= 3.0) {
		    is_tight = (neHEF[i] < 0.99);
		}

		// |eta| > 3.0
		else {
		    is_tight =
			neMult[i] >= 2 &&
			neEmEF[i] < 0.40;
		}

		// TightLeptonVeto extension
		bool pass_lepton_veto = (aeta > 2.7) || (muEF[i] < 0.8 && chEmEF[i] < 0.8);

		mask[i] = is_tight && pass_lepton_veto;
	    }

	    return mask;
	},
	{"Jet_neMultiplicity", "Jet_chMultiplicity", "Jet_pt_corr",
	 "Jet_neEmEF", "Jet_chEmEF", "Jet_chHEF", "Jet_neHEF",
	 "Jet_muEF", "Jet_eta"});
    }
    else
    {
    _rlm = _rlm.Define("goodJetsID", JetID(6)); //without pt-eta cuts here i have to add other cuts since its NanoAODv12
    }
   
    
	// =====================================================
	// 1. GOOD JET SELECTION
	// =====================================================
	//Here the Tight LepVeto id is used in all the jet but a combination of loose and tight is required as per the AN 
	//It will be updated ASAP the Jet id will work as json
	//Also PU JetID is also not applied since it's not present in the NanoAOD v15

    _rlm = _rlm.Define(
        "for_jetVeto",
        "goodJetsID && Jet_pt_corr > 15.0 && (Jet_chEmEF + Jet_neEmEF) < 0.9"
    );
    _rlm = _rlm.Define(
        "jet_eta_veto",
        "Jet_eta[for_jetVeto]"
    );

    _rlm = _rlm.Define(
        "jet_phi_veto",
        "Jet_phi[for_jetVeto]"
    );

    _rlm = applyJetVetoMap(
        _rlm,
        "jet_eta_veto",
        "jet_phi_veto",
        "vetoed_jets"
    );


	_rlm = _rlm.Define(
	    "goodJets",
	    "for_jetVeto && vetoed_jets == 0 && ("
		"("
		    "abs(Jet_eta) > 2.5 && abs(Jet_eta) < 3.0 && Jet_pt_corr > 50.0"
		") || ("
		    "abs(Jet_eta) > 3.0 && abs(Jet_eta) < 5.0 && Jet_pt_corr > 25.0"
		") || ("
		    "abs(Jet_eta) > 0.0 && abs(Jet_eta) < 2.5 && Jet_pt_corr > 25.0"
		")"
	    ") && abs(Jet_eta) < 5.0"
	);


    // =====================================================
    // 2. EXTRACT GOOD JET VARIABLES
    // =====================================================
    _rlm = _rlm.Define("goodJets_pt",   "Jet_pt_corr[goodJets]")
            .Define("goodJets_eta",  "Jet_eta[goodJets]")
            .Define("goodJets_phi",  "Jet_phi[goodJets]")
            .Define("goodJets_mass", "Jet_mass[goodJets]");

    // -----------------------------------------------------
    // 2b. SORT GOOD JETS BY CORRECTED PT (DESCENDING)
    // -----------------------------------------------------
    _rlm = _rlm.Define("goodJetPtSortIdx", "ROOT::VecOps::Argsort(-goodJets_pt)");

    _rlm = _rlm.Redefine("goodJets_pt",   "ROOT::VecOps::Take(goodJets_pt,   goodJetPtSortIdx)")
           .Redefine("goodJets_eta",  "ROOT::VecOps::Take(goodJets_eta,  goodJetPtSortIdx)")
           .Redefine("goodJets_phi",  "ROOT::VecOps::Take(goodJets_phi,  goodJetPtSortIdx)")
           .Redefine("goodJets_mass", "ROOT::VecOps::Take(goodJets_mass, goodJetPtSortIdx)");

    // -----------------------------------------------------
    // 2c. INDEX / 4-VECTORS (must be built AFTER sorting so they match)
    // -----------------------------------------------------
    _rlm = _rlm.Define("goodJets_idx", ::good_idx, {"goodJets"})
           .Redefine("goodJets_idx", "ROOT::VecOps::Take(goodJets_idx, goodJetPtSortIdx)")
           .Define("NgoodJets", "int(goodJets_pt.size())")
           .Define("goodJets_4vecs", ::generate_4vec, {"goodJets_pt", "goodJets_eta", "goodJets_phi", "goodJets_mass"});

_rlm = _rlm.Define("good_leadingJet_pt",      "goodJets_pt.size()  > 0 ? goodJets_pt[0]  : -999.f")
           .Define("good_subleadingJet_pt",   "goodJets_pt.size()  > 1 ? goodJets_pt[1]  : -999.f")
           .Define("good_leadingJet_eta",     "goodJets_eta.size()  > 0 ? goodJets_eta[0]  : -999.f")
           .Define("good_subleadingJet_eta",  "goodJets_eta.size()  > 1 ? goodJets_eta[1]  : -999.f");



    // =====================================================
    // 3. MC-ONLY INFORMATION
    // =====================================================
    if (!_isData) {
       _rlm = _rlm.Define("goodJets_hadflav", "Jet_hadronFlavour[goodJets]")
                  .Redefine("goodJets_hadflav", "ROOT::VecOps::Take(goodJets_hadflav, goodJetPtSortIdx)");
    }

    // =====================================================
    // 4. BTAGGING VARIABLES
    // =====================================================
    _rlm = _rlm.Define("goodJets_deepjetbtag", "Jet_btagDeepFlavB[goodJets]")                                                                                                                                                                
           .Redefine("goodJets_deepjetbtag", "ROOT::VecOps::Take(goodJets_deepjetbtag, goodJetPtSortIdx)")
           .Define("goodJets_UparTjetbtag", "Jet_btagUParTAK4B[goodJets]")
           .Redefine("goodJets_UparTjetbtag", "ROOT::VecOps::Take(goodJets_UparTjetbtag, goodJetPtSortIdx)");

}

void BaseAnalyser::removeOverlaps()
{
    cout << "Checking overlaps between jets and leptons" << endl;

    // ================================================================
    // buildDeltaRMask
    //
    // Creates a cleaning mask for collection1 using collection2
    // based on a minimum delta R separation.
    //
    // For each object in collection1:
    //   - Compute delat R to all objects in collection2
    //   - Find the minimum delta R
    //   - Return 1 (keep) if min delta R > threshold
    //   - Return 0 (remove) if min delta R <= threshold
    //
    // IMPORTANT:
    //   - This function removes objects from collection1 only.
    //   - collection2 is used only as a reference.
    //   - Mask size == collection1.size()
    //
    // Typical usage:
    //   - Clean jets with respect to leptons  (delta R > 0.4)
    //   - Clean electrons with respect to muons (delta R > 0.05)
    // ================================================================
    

    auto buildDeltaRMask = [](const FourVectorRVec &collection1,
			      const FourVectorRVec &collection2,
			      double minDeltaR)
    {
	ROOT::VecOps::RVec<int> mask;

	for (const auto &obj1 : collection1)
	{
	    double mindr = 999.0;

	    for (const auto &obj2 : collection2)
	    {
		double dr = ROOT::Math::VectorUtil::DeltaR(obj1, obj2);
		if (dr < mindr)
		    mindr = dr;
	    }

	    mask.emplace_back(mindr > minDeltaR ? 1 : 0);
	}

	return mask;
    };

    // =====================================================
    // 1 ELECTRO AND MUON OVERLAP CLEANING (dR > 0.05)
    // =====================================================

    _rlm = _rlm.Define("ElectronMuonCleanMask",
                       [buildDeltaRMask](const FourVectorRVec &ele,
                                         const FourVectorRVec &mu)
                       { return buildDeltaRMask(ele, mu, 0.05); },
                       {"baselineElectron_4Vecs", "baselineMuon_4Vecs"})

               .Redefine("baselineElectrons_pt",   "baselineElectrons_pt[ElectronMuonCleanMask]")
               .Redefine("baselineElectrons_eta",  "baselineElectrons_eta[ElectronMuonCleanMask]")
               .Redefine("baselineElectrons_phi",  "baselineElectrons_phi[ElectronMuonCleanMask]")
               .Redefine("baselineElectrons_mass", "baselineElectrons_mass[ElectronMuonCleanMask]")
               .Redefine("baselineElectrons_charge", "baselineElectrons_charge[ElectronMuonCleanMask]")
               .Redefine("baselineElectrons_pdgId", "baselineElectrons_pdgId[ElectronMuonCleanMask]")
               .Redefine("baselineElectrons_mvaTTH", "baselineElectrons_mvaTTH[ElectronMuonCleanMask]")
               .Redefine("tight_baselineElectrons", "tight_baselineElectrons[ElectronMuonCleanMask]")
               .Redefine("NbaselineElectrons", "int(baselineElectrons_pt.size())");


    if (!_isData) {
    _rlm = _rlm.Redefine("Gen_baselineElectrons_genPartFlav", "Electron_genPartFlav[baselineElectrons]")
               .Redefine("Gen_baselineElectrons_genPartIdx", "Electron_genPartIdx[baselineElectrons]");
    }
    // =====================================================
    // 2 JET MUON OVERLAP CLEANING (dR > 0.4)
    // =====================================================

    _rlm = _rlm.Define("JetMuonCleanMask",
                       [buildDeltaRMask](const FourVectorRVec &jets,
                                         const FourVectorRVec &mu)
                       { return buildDeltaRMask(jets, mu, 0.4); },
                       {"goodJets_4vecs", "baselineMuon_4Vecs"})

               .Define("JetElectronCleanMask",
                       [buildDeltaRMask](const FourVectorRVec &jets,
                                         const FourVectorRVec &ele)
                       { return buildDeltaRMask(jets, ele, 0.4); },
                       {"goodJets_4vecs", "baselineElectron_4Vecs"})

               .Define("CleanJetMask",
                       "JetMuonCleanMask && JetElectronCleanMask");


    // =====================================================
    // 3  CLEAN JETS
    // =====================================================

    _rlm = _rlm.Define("Selected_jetpt",   "goodJets_pt[CleanJetMask]")
               .Define("Selected_jeteta",  "goodJets_eta[CleanJetMask]")
               .Define("Selected_jetphi",  "goodJets_phi[CleanJetMask]")
               .Define("Selected_jetmass", "goodJets_mass[CleanJetMask]")
               .Define("Selected_jetbtag", "goodJets_UparTjetbtag[CleanJetMask]")
               .Define("ncleanjetspass", "int(Selected_jetpt.size())")
               .Define("Selected_jetHT", "Sum(Selected_jetpt)")
               .Define("Leading_SelectedJet_pt",
                       "Selected_jetpt.size() > 0 ? Selected_jetpt[0] : -999.f");

    if (!_isData)
        _rlm = _rlm.Define("Selected_jethadflav",
                           "goodJets_hadflav[CleanJetMask]");


    // =====================================================
    // 4  CENTRAL JETS AND LEADING JETS
    // =====================================================

    _rlm = _rlm.Define("centraljetpass",
                       "abs(Selected_jeteta) < 2.4")
               .Define("Central_jetpt",
                       "Selected_jetpt[centraljetpass]")
               .Define("nCentral_jet",
                       "int(Central_jetpt.size())");
    // =====================================================
    // 5  CLEAN BJETS
    // =====================================================

    _rlm = _rlm.Define("btagcuts2",
                       "Selected_jetbtag > 0.1272")

               .Define("Selected_bjetpt",
                       "Selected_jetpt[btagcuts2]")
               .Define("Selected_bjeteta",
                       "Selected_jeteta[btagcuts2]")
               .Define("Selected_bjetphi",
                       "Selected_jetphi[btagcuts2]")
               .Define("Selected_bjetmass",
                       "Selected_jetmass[btagcuts2]")
               .Define("Selected_bjet_score",
                       "Selected_jetbtag[btagcuts2]")
               .Define("ncleanbjetspass",
                       "int(Selected_bjetpt.size())")
               .Define("Selected_bjetHT",
                       "Sum(Selected_bjetpt)")
               .Define("cleanbjet4vecs",
                       ::generate_4vec,
                       {"Selected_bjetpt",
                        "Selected_bjeteta",
                        "Selected_bjetphi",
                        "Selected_bjetmass"});

    if (!_isData)
        _rlm = _rlm.Define("Selected_bjethadflav",
                           "Selected_jethadflav[btagcuts2]");


    _rlm = _rlm.Define("leadingJet_pt",      "Selected_jetpt.size()  > 0 ? Selected_jetpt[0]  : -999.f")
             .Define("subleadingJet_pt",   "Selected_jetpt.size()  > 1 ? Selected_jetpt[1]  : -999.f")
             .Define("leadingbJet_pt",     "Selected_bjetpt.size() > 0 ? Selected_bjetpt[0] : -999.f")
             .Define("subleadingbJet_pt",  "Selected_bjetpt.size() > 1 ? Selected_bjetpt[1] : -999.f")
             .Define("leadingJet_eta",     "Selected_jeteta.size()  > 0 ? Selected_jeteta[0]  : -999.f")
             .Define("subleadingJet_eta",  "Selected_jeteta.size()  > 1 ? Selected_jeteta[1]  : -999.f")
             .Define("leadingbJet_eta",    "Selected_bjeteta.size() > 0 ? Selected_bjeteta[0] : -999.f")
             .Define("subleadingbJet_eta", "Selected_bjeteta.size() > 1 ? Selected_bjeteta[1] : -999.f")
             .Define("leadingJet_phi",     "Selected_jetphi.size()  > 0 ? Selected_jetphi[0]  : -999.f")
             .Define("subleadingJet_phi",  "Selected_jetphi.size()  > 1 ? Selected_jetphi[1]  : -999.f")
             .Define("leadingbJet_phi",    "Selected_bjetphi.size() > 0 ? Selected_bjetphi[0] : -999.f")
             .Define("subleadingbJet_phi", "Selected_bjetphi.size() > 1 ? Selected_bjetphi[1] : -999.f");



    // =====================================================
    // TLorentzVector Collections (ALL IN ONE PLACE)
    // =====================================================

    _rlm = _rlm

	// ---- Baseline Electrons ----
	.Redefine("baselineElectron_TL4Vecs",
		::buildTLorentzVectors,
		{"baselineElectrons_pt",
		 "baselineElectrons_eta",
		 "baselineElectrons_phi",
		 "baselineElectrons_mass"})

	// ---- Clean Jets ----
	.Define("cleanjet_TL4Vecs",
		::buildTLorentzVectors,
		{"Selected_jetpt",
		 "Selected_jeteta",
		 "Selected_jetphi",
		 "Selected_jetmass"})

	// ---- Clean b-Jets ----
	.Define("cleanbjet_TL4Vecs",
		::buildTLorentzVectors,
		{"Selected_bjetpt",
		 "Selected_bjeteta",
		 "Selected_bjetphi",
		 "Selected_bjetmass"});

    _rlm = _rlm
	  .Define("Topquark_Bjet_TL4Vecs",
	   ::buildTLorentzVectors,
	   {"Selected_bjetpt",
	    "Selected_bjeteta",
	    "Selected_bjetphi",
	    "Selected_bjetmass"});

}


//MET

void BaseAnalyser::selectMET()
{
    if (debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

//    _rlm = _rlm.Define("goodMET_pt","PuppiMET_pt_corr>20 ? PuppiMET_pt_corr : std::numeric_limits<float>::quiet_NaN()")
//	       .Define("goodMET_phi","PuppiMET_pt_corr > 20 ? PuppiMET_phi_corr : std::numeric_limits<float>::quiet_NaN()");

    _rlm =  _rlm.Define("goodMET_pt",  "PuppiMET_pt_corr")
                .Define("goodMET_phi", "PuppiMET_phi_corr");

    std::cout<< "================================//=================================" << std::endl;
    std::cout<< "==================CORRECT MET HAS BEEN SELECTED====================" << std::endl;
    std::cout<< "================================//=================================" << std::endl;
}

void BaseAnalyser::calculateEvWeight(){

    if (debug){        
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

    int _case = 1;
    int _redefine = 0; 
    //std::vector<std::string> Jets_vars_names = {"goodJets_hadFlav", "goodJets_eta", "goodJets_pt", "goodJets_btag"};
    std::vector<std::string> Jets_vars_names = {"Selected_jethadflav", "Selected_jeteta",  "Selected_jetpt", "Selected_jetbtag"};  
    if (_case != 1)
    {
        Jets_vars_names.emplace_back("Selected_jetbtag");
    }

    std::string output_btag_medium_column_name = "btag_SF_";

    float btag_cut;

    if (_year == 2024)
    {
        btag_cut = 0.1272;  // UParTAK4 Medium WP for 2024
        std::cout << "Btag Cut for year " << _year << ", Era: " << " -> " << btag_cut << " applied" << std::endl;
    }
    else
    {
        std::cerr << "[ERROR] Unsupported year or era combination: " << _year << ", " << std::endl;
        return;
    }

    
    // _rlm = calculateBTagSF(_rlm, Jets_vars_names, _case, output_btag_column_name);
    _rlm = calculateBTagSF(_rlm, Jets_vars_names, _case, btag_cut, "M", output_btag_medium_column_name, _redefine);

    _rlm = _rlm.Define("btag_SF_bcflav_vector", [](float central, float up, float down
        // float up_corr, float down_corr,float up_uncorr, float down_uncorr
        ) {
        return std::vector<float>{
        central,        // 0
        up,             // 1
        down,           // 2
        // up_corr,        // 3
        // down_corr,      // 4
        // up_uncorr,      // 5
        // down_uncorr,    // 6
        
        };
        }, {
        "btag_SF_bcflav_central",
        "btag_SF_bcflav_up", "btag_SF_bcflav_down",
        //"btag_SF_bcflav_up_correlated", "btag_SF_bcflav_down_correlated",
        //"btag_SF_bcflav_up_uncorrelated", "btag_SF_bcflav_down_uncorrelated"
        
        });


    _rlm = _rlm.Define("btag_SF_lflav_vector", [](float central, float up, float down,
        float up_corr, float down_corr,float up_uncorr, float down_uncorr
        ) {
        return std::vector<float>{
        central,        // 0
        up,             // 1
        down,           // 2
        up_corr,        // 3
        down_corr,      // 4
        up_uncorr,      // 5
        down_uncorr,    // 6
        
        };
        }, {
        "btag_SF_lflav_central",
        "btag_SF_lflav_up", "btag_SF_lflav_down",
        "btag_SF_lflav_up_correlated", "btag_SF_lflav_down_correlated",
        "btag_SF_lflav_up_uncorrelated", "btag_SF_lflav_down_uncorrelated"
        });



  //Scale Factors for Muon HLT, RECO, ID and ISO
  std::vector<std::string> Muon_vars_names = {"baselineMuons_eta", "baselineMuons_pt"};
  std::string output_mu_column_name = "muon_SF_";
  _rlm = calculateMuSF(_rlm, Muon_vars_names, output_mu_column_name);

  //Scale Factors for Electron RECO and ID
  std::vector<std::string> Electron_vars_names = {"baselineElectrons_eta", "baselineElectrons_pt", "baselineElectrons_phi"};
  std::string output_ele_column_name = "ele_SF_";
  _rlm = calculateEleSF(_rlm, Electron_vars_names, output_ele_column_name);

  ////Total event Weight:
  //  double norm_factor = _X_section * _SumOfGenWeight_Computed ;
  //  std::cout << "Normalization factor = "
  //        << norm_factor
  //        << std::endl;


  //  _rlm = _rlm.Define("norm_factor", [norm_factor]() {
  //      return norm_factor;
  //  });
    _rlm = _rlm.Define("evWeight", " pugenWeight * btag_SF_bcflav_central * btag_SF_lflav_central * muon_SF_central * ele_SF_central"); 
    _rlm = _rlm.Define("evWeight_with_em_SF_Only", " pugenWeight * muon_SF_central * ele_SF_central"); 
//  _rlm = _rlm.Define("evWeight", " pugenWeight * muon_SF_central "); 
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
    // Combine lepton pt, eta, phi properties
    //-------------------------------------------------------
	_rlm = _rlm.Define("Lepton_pt","ROOT::VecOps::Concatenate(baselineMuons_pt, baselineElectrons_pt)")
		   .Define("Lepton_eta", "ROOT::VecOps::Concatenate(baselineMuons_eta, baselineElectrons_eta)")
		   .Define("Lepton_phi", "ROOT::VecOps::Concatenate(baselineMuons_phi, baselineElectrons_phi)")
		   .Define("Lepton_mass", "ROOT::VecOps::Concatenate(baselineMuons_mass, baselineElectrons_mass)")
		   .Define("Lepton_pdgId", "ROOT::VecOps::Concatenate(baselineMuons_pdgId, baselineElectrons_pdgId)")
		   .Define("Lepton_isPrompt", "ROOT::VecOps::Concatenate(tight_Muons, tight_baselineElectrons)")
		   .Define("NLepton","int(Lepton_pt.size())")
		   .Define("Lepton_charge", "ROOT::VecOps::Concatenate(baselineMuons_charge, baselineElectrons_charge)");

    if (!_isData) {
	_rlm = _rlm.Define("Gen_Lepton_genPartFlav", "ROOT::VecOps::Concatenate(Gen_baselineMuons_genPartFlav, Gen_baselineElectrons_genPartFlav)")
		       .Define("Gen_Lepton_genPartIdx", "ROOT::VecOps::Concatenate(Gen_baselineMuons_genPartIdx, Gen_baselineElectrons_genPartIdx)");
        }

	_rlm = _rlm.Define(
	    "Lepton_flavor",
	    "ROOT::VecOps::Concatenate("
	    "ROOT::VecOps::RVec<int>(baselineMuons_charge.size(), 1), "
	    "ROOT::VecOps::RVec<int>(baselineElectrons_charge.size(), 0))"
	);

	// 1. All prompt leptons
	_rlm = _rlm.Define("allTightLeptons", "ROOT::VecOps::All(Lepton_isPrompt == 1)");

	// 3. Merge TLorentzVectors (muons + electrons)
	_rlm = _rlm.Define("Lepton4Vecs", ::generate_4vec, {"Lepton_pt", "Lepton_eta", "Lepton_phi", "Lepton_mass"}) 
               .Define("LeptonTLorentzVecs", "ROOT::VecOps::Concatenate(baselineMuon_TL4Vecs, baselineElectron_TL4Vecs)");

    _rlm = _rlm.Define("mass_of_3lepton","Lepton4Vecs.size() == 3 ? (Lepton4Vecs[0] + Lepton4Vecs[1] + Lepton4Vecs[2]).M() : -1.0");

    // 2. Single index vector: sort + cut
    // _rlm = _rlm.Define("goodLepton_idx",
    //     [](const RVecF& pt) -> ROOT::VecOps::RVec<size_t> {
    //         if (pt.size() < 3) return {};
    //         ROOT::VecOps::RVec<size_t> idx(pt.size());
    //         std::iota(idx.begin(), idx.end(), 0);
    //         std::sort(idx.begin(), idx.end(), [&](size_t i, size_t j){ return pt[i] > pt[j]; });
    //         if (pt[idx[0]] <= 25.f || pt[idx[1]] <= 15.f || pt[idx[2]] <= 15.f) return {};
    //         return idx;
    //     }, {"Lepton_pt"});


    // 2. Single index vector: sort only, no pt threshold cut
    _rlm = _rlm.Define("goodLepton_idx",
        [](const RVecF& pt) -> ROOT::VecOps::RVec<size_t> {
            if (pt.size() < 1) return {};
            ROOT::VecOps::RVec<size_t> idx(pt.size());
            std::iota(idx.begin(), idx.end(), 0);
            std::sort(idx.begin(), idx.end(), [&](size_t i, size_t j){ return pt[i] > pt[j]; });
            return idx;
        }, {"Lepton_pt"});

    auto takeF = [](const ROOT::VecOps::RVec<float>& vec, const ROOT::VecOps::RVec<size_t>& idx) {
        return ROOT::VecOps::Take(vec, idx);
    };
    auto takeI = [](const ROOT::VecOps::RVec<int>& vec, const ROOT::VecOps::RVec<size_t>& idx) {
        return ROOT::VecOps::Take(vec, idx);
    };
    auto takeUC = [](const ROOT::VecOps::RVec<unsigned char>& vec,
                     const ROOT::VecOps::RVec<size_t>& idx) {
        return ROOT::VecOps::Take(vec, idx);
    };
    auto takeS = [](const ROOT::VecOps::RVec<short>& vec,
                    const ROOT::VecOps::RVec<size_t>& idx) {
        return ROOT::VecOps::Take(vec, idx);
    };
    _rlm = _rlm
        .Define("goodLepton_pt",       takeF, {"Lepton_pt",         "goodLepton_idx"})
        .Define("goodLepton_eta",      takeF, {"Lepton_eta",        "goodLepton_idx"})
        .Define("goodLepton_phi",      takeF, {"Lepton_phi",        "goodLepton_idx"})
        .Define("goodLepton_mass",     takeF, {"Lepton_mass",       "goodLepton_idx"})
        .Define("goodLepton_pdgId",    takeI, {"Lepton_pdgId",     "goodLepton_idx"})
        .Define("goodLepton_charge",   takeI, {"Lepton_charge",     "goodLepton_idx"})
        .Define("goodLepton_flavor",   takeI, {"Lepton_flavor",     "goodLepton_idx"})
        .Define("goodLepton_isPrompt", takeI, {"Lepton_isPrompt",  "goodLepton_idx"})
        .Define("NgoodLepton",         "int(goodLepton_pt.size())")
        .Define("goodLepton4Vecs",     ::generate_4vec, {"goodLepton_pt", "goodLepton_eta", "goodLepton_phi", "goodLepton_mass"})
        // .Define("goodLepton_TL4Vecs",::buildTLorentzVectors, {"goodLepton_pt", "goodLepton_eta", "goodLepton_phi", "goodLepton_mass"})
        .Define("channel_code",
                "Lepton_pdgId.size() >= 3 ? "
                "(abs(Lepton_pdgId[0]) + abs(Lepton_pdgId[1]) + abs(Lepton_pdgId[2]) - 33)/2 "
                ": -1")
        .Define("allTightgoodLeptons",     "ROOT::VecOps::All(goodLepton_isPrompt == 1)")
        .Define("goodlepton_m3",
            [](const RVec4Vec& vecs) {
                return vecs.size() == 3
                    ? (vecs[0] + vecs[1] + vecs[2]).M()
                    : -1.0;
            }, {"Lepton4Vecs"});

	_rlm = _rlm.Define("sum_goodLepton_flavor","ROOT::VecOps::Sum(goodLepton_flavor)");	


    if (!_isData) {
        _rlm = _rlm
            .Define("Gen_goodLepton_genPartFlav", takeUC,
                    {"Gen_Lepton_genPartFlav", "goodLepton_idx"})
            .Define("Gen_goodLepton_genPartIdx", takeS,
                    {"Gen_Lepton_genPartIdx", "goodLepton_idx"})
            .Define("Gen_goodLepton_isRecoMatch",GenParticleHelper::MatchLeptonToPromptGen,{"Gen_goodLepton_genPartIdx","GenPromptLeptonIdx"})
            .Define("Gen_goodLepton_Num_isRecoMatch",GenParticleHelper::CountPromptMatchedLeptons,{"Gen_goodLepton_isRecoMatch"});
    }

    // 3. Boolean mask: pt threshold cuts on sorted leptons
    _rlm = _rlm.Define("goodLepton_ptCut",
        [](const RVecF& pt, const ROOT::VecOps::RVec<size_t>& idx) -> bool {
            if (idx.size() < 3) return false;
            return pt[idx[0]] > 25.f && pt[idx[1]] > 15.f && pt[idx[2]] > 15.f;
        }, {"goodLepton_pt", "goodLepton_idx"});



    auto getLepton = [](const RVecF& vec, int idx) -> float {
        return vec.size() >= 3 ? vec[idx] : -999.f;
    };

    _rlm = _rlm
        .Define("leadingLepton_pt",     [getLepton](const RVecF& v){ return getLepton(v, 0); }, {"goodLepton_pt"})
        .Define("leadingLepton_eta",    [getLepton](const RVecF& v){ return getLepton(v, 0); }, {"goodLepton_eta"})
        .Define("subleadingLepton_pt",  [getLepton](const RVecF& v){ return getLepton(v, 1); }, {"goodLepton_pt"})
        .Define("subleadingLepton_eta", [getLepton](const RVecF& v){ return getLepton(v, 1); }, {"goodLepton_eta"})
        .Define("trailingLepton_pt",    [getLepton](const RVecF& v){ return getLepton(v, 2); }, {"goodLepton_pt"})
        .Define("trailingLepton_eta",   [getLepton](const RVecF& v){ return getLepton(v, 2); }, {"goodLepton_eta"});
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
               .Define("goodLepton3_flavor", "is3LeptonEvent ? goodLepton_flavor : ROOT::VecOps::RVec<int>{}")
               .Define("goodLepton3_isPrompt", "is3LeptonEvent ? goodLepton_isPrompt : ROOT::VecOps::RVec<int>{}")
               .Define("goodLepton3_4Vecs", "is3LeptonEvent ? goodLepton4Vecs : std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{}")
               .Define("goodLepton3_TL4Vecs", "is3LeptonEvent ? goodLepton_TL4Vecs : ROOT::VecOps::RVec<TLorentzVector>{}");

    // ============================
    // Good Leptons: 4-lepton case
    // ============================
    _rlm = _rlm.Define("is4LeptonEvent", "NgoodLepton == 4");

    _rlm = _rlm.Define("goodLepton4_pt", "is4LeptonEvent ? goodLepton_pt : ROOT::VecOps::RVec<float>{}")
               .Define("goodLepton4_eta", "is4LeptonEvent ? goodLepton_eta : ROOT::VecOps::RVec<float>{}")
               .Define("goodLepton4_phi", "is4LeptonEvent ? goodLepton_phi : ROOT::VecOps::RVec<float>{}")
               .Define("goodLepton4_charge", "is4LeptonEvent ? goodLepton_charge : ROOT::VecOps::RVec<int>{}")
               .Define("goodLepton4_flavor", "is4LeptonEvent ? goodLepton_flavor : ROOT::VecOps::RVec<int>{}")
               .Define("goodLepton4_isPrompt", "is4LeptonEvent ? goodLepton_isPrompt : ROOT::VecOps::RVec<int>{}")
               .Define("goodLepton4_4Vecs", "is4LeptonEvent ? goodLepton4Vecs : std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>{}")
               .Define("goodLepton4_TL4Vecs", "is4LeptonEvent ? goodLepton_TL4Vecs : ROOT::VecOps::RVec<TLorentzVector>{}");
}


void BaseAnalyser::defineInZPeak() {
    cout << "Define inzpeak" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    _rlm = _rlm.Define(
        "inzpeak",
        hasExactlyOneOSSFZPair,
        {"goodLepton4Vecs", "goodLepton_pdgId"}
    );
    _rlm = _rlm.Define(
        "zPairCounts",
        countOSSFZPairs,
        {"goodLepton4Vecs", "goodLepton_pdgId"}
    )
    .Define("nZPairs", "zPairCounts.nPairs")
    .Define("nDistinctZPairs", "zPairCounts.nDistinctPairs");


    _rlm = _rlm.Define("OSSF3L_info", ::computeOSSF3LInfo, {"goodLepton4Vecs", "goodLepton_pdgId"})
               .Define("OSSF_category", ::getOSSF3LCategory, {"OSSF3L_info"})
               .Define("zboson_mass", ::getZBosonMass, {"OSSF3L_info"})
               .Define("nonZ_OSSF_mass", ::getNonZOSSFMass, {"OSSF3L_info"})
               .Define("mass_of_3_goodLepton_4BG", ::getMassOf3GoodLeptons4BG, {"OSSF3L_info", "goodLepton4Vecs"})
               .Define("m3l", ::getM3L, {"goodLepton4Vecs"})
               .Define("mask_cat1_outsideZ3l", ::isMaskCat1OutsideZ3L, {"OSSF3L_info", "m3l"})
               .Define("topLepton_index", ::getTopLeptonIndex3L, {"OSSF3L_info"})
               .Define("topLepton_4Vec_new", ::selectLeptonByIndex, {"goodLepton4Vecs", "topLepton_index"})
               .Define("topLepton_TL4Vec_new", ::TL4VecFromFourVec, {"topLepton_4Vec_new"});



    _rlm = _rlm.Define("OSSF4L_info", ::computeOSSF4LInfo, {"goodLepton4Vecs", "goodLepton_pdgId"})
               .Define("OSSF4L_category", ::getOSSF4LCategory, {"OSSF4L_info"})
               .Define("OSSF4L_bestZ_mass", ::getOSSF4LBestZMass, {"OSSF4L_info"})
               .Define("OSSF4L_secondZ_mass", ::getOSSF4LSecondZMass, {"OSSF4L_info"})
               .Define("leftoverLepton1_index", ::getLeftoverLepton1Index, {"OSSF4L_info"})
               .Define("leftoverLepton2_index", ::getLeftoverLepton2Index, {"OSSF4L_info"})
               .Define("leftoverPair_mass", ::getLeftoverPairMass, {"OSSF4L_info"});
    
}


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
    _rlm = _rlm.Define("nu_pt", "goodMET_pt")
               .Define("nu_phi", "goodMET_phi")
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


void BaseAnalyser::reconstructTop()
{
    if (debug) {
        std::cout << std::endl;
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }
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
               }, {"goodLepton3_pt", "goodMET_pt"});

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


   _rlm = _rlm
        .Define("EEE_Region", "channel_code == 0 && NgoodLepton == 3 ")
        .Define("EEU_Region", "channel_code == 1 && NgoodLepton == 3 ")
        .Define("UUE_Region", "channel_code == 2 && NgoodLepton == 3 ")
        .Define("UUU_Region", "channel_code == 3 && NgoodLepton == 3 ")
        .Define("THREELRegion", "channel_code >= 0 && NgoodLepton == 3 ");

   _rlm = _rlm
        .Define("eee_Region", "channel_code == 0 && NgoodLepton == 3 && inzpeak && goodLepton_ptCut")
        .Define("eeu_Region", "channel_code == 1 && NgoodLepton == 3 && inzpeak && goodLepton_ptCut")
        .Define("uue_Region", "channel_code == 2 && NgoodLepton == 3 && inzpeak && goodLepton_ptCut")
        .Define("uuu_Region", "channel_code == 3 && NgoodLepton == 3 && inzpeak && goodLepton_ptCut")
        .Define("threeLRegion", "channel_code >= 0 && NgoodLepton == 3 && inzpeak && goodLepton_ptCut")
        .Define("BaseRegion", "threeLRegion && ncleanjetspass >= 2 && ncleanbjetspass >= 1");

    _rlm = _rlm.Define("WZ_Region", "threeLRegion && ncleanbjetspass == 0 && goodMET_pt > 50 ");


    // tzq / ttz split, straight off BaseRegion.
    _rlm = _rlm
        .Define("SignalRegion_tzq", "BaseRegion && nCentral_jet < 4")
        .Define("SignalRegion_ttz", "BaseRegion && nCentral_jet >= 4");

    // Stricter WZ variant with the m3l cut, layered on top of WZ_Region.
    _rlm = _rlm.Define("WZ_Region_m3l", "WZ_Region && m3l > 100");

    // Z-peak-agnostic base, needed because threeLRegion already bakes in
    // inzpeak (== category 1), so it can't be the parent for the off-Z
    // categories below.
    _rlm = _rlm.Define("threeLBaseRegion_anyZ", "channel_code >= 0 && NgoodLepton == 3 && goodLepton_ptCut");

    _rlm = _rlm
        .Define("X_gamma_Region", "threeLBaseRegion_anyZ && OSSF_category==2 && nonZ_OSSF_mass>35 && nonZ_OSSF_mass<76")
        .Define("NP_2_Region", "threeLBaseRegion_anyZ && OSSF_category==3 && nonZ_OSSF_mass>35 && ncleanjetspass>=2 && ncleanjetspass<=3 && ncleanbjetspass==1")
        .Define("NP_1_Region", "threeLBaseRegion_anyZ && OSSF_category==0 && ncleanjetspass>=2 && ncleanjetspass<=3 && ncleanbjetspass==1");

    // ============================================================
    // 4-lepton regions (untouched from before)
    // ============================================================

    _rlm = _rlm
        .Define("baseRegion_4L", "NgoodLepton==4 && OSSF4L_category != 0")
        .Define("ZZ_Region", "baseRegion_4L && OSSF4L_category==2")
        .Define("ttZ_Region", "baseRegion_4L && OSSF4L_category==1 && nCentral_jet >= 4");

    _rlm = ::defineRegionObjectBranches(_rlm, "uuu_Region",       "uuu_ThreeLRegion");
    _rlm = ::defineRegionObjectBranches(_rlm, "uue_Region",       "uue_ThreeLRegion");
    _rlm = ::defineRegionObjectBranches(_rlm, "eeu_Region",       "eeu_ThreeLRegion");
    _rlm = ::defineRegionObjectBranches(_rlm, "eee_Region",       "eee_ThreeLRegion");


    _rlm = ::defineRegionObjectBranches(_rlm, "BaseRegion",       "BaseRegion");
    _rlm = ::defineRegionObjectBranches(_rlm, "threeLRegion",     "ThreeLRegion");
    _rlm = ::defineRegionObjectBranches(_rlm, "WZ_Region",        "WZ_Region");
    _rlm = ::defineRegionObjectBranches(_rlm, "WZ_Region_m3l",    "WZ_Region_m3l");
    _rlm = ::defineRegionObjectBranches(_rlm, "SignalRegion_tzq", "SignalRegion_tzq");
    _rlm = ::defineRegionObjectBranches(_rlm, "SignalRegion_ttz", "SignalRegion_ttz");
    _rlm = ::defineRegionObjectBranches(_rlm, "X_gamma_Region",   "X_gamma_Region");
    _rlm = ::defineRegionObjectBranches(_rlm, "NP_1_Region",      "NP_1_Region");
    _rlm = ::defineRegionObjectBranches(_rlm, "NP_2_Region",      "NP_2_Region");

    _rlm = ::defineRegionObjectBranches(_rlm, "baseRegion_4L", "baseRegion_4L");
    _rlm = ::defineRegionObjectBranches(_rlm, "ZZ_Region",     "ZZ_Region");
    _rlm = ::defineRegionObjectBranches(_rlm, "ttZ_Region",    "ttZ_Region");

    _rlm = _rlm.Define("SignalRegion_tzq_zMass", "SignalRegion_tzq ? zboson_mass : -1.0")
               .Define("SignalRegion_ttz_zMass", "SignalRegion_ttz ? zboson_mass : -1.0");

    _rlm = _rlm.Define("ThreeLRegion_zMass",   "threeLRegion   ? zboson_mass    : -1.0")
               .Define("WZ_Region_zMass",      "WZ_Region      ? zboson_mass    : -1.0")
               .Define("WZ_Region_zMass_3l",   "WZ_Region_m3l  ? zboson_mass    : -1.0")
               .Define("X_gamma_Region_mass",  "X_gamma_Region ? nonZ_OSSF_mass : -1.0")
               .Define("NP_2_Region_mass",     "NP_2_Region    ? nonZ_OSSF_mass : -1.0")
               .Define("NP_1_Region_m3l",      "NP_1_Region    ? m3l            : -1.0");

    _rlm = _rlm.Define("baseRegion_4L_bestZmass", "baseRegion_4L  ? OSSF4L_bestZ_mass   : -1.0")
               .Define("ttZ_Region_bestZmass",    "ttZ_Region     ? OSSF4L_bestZ_mass   : -1.0")
               .Define("ttZ_Region_leftoverMass", "ttZ_Region     ? leftoverPair_mass   : -1.0")
               .Define("ZZ_Region_Z1mass",        "ZZ_Region      ? OSSF4L_bestZ_mass   : -1.0")
               .Define("ZZ_Region_Z2mass",        "ZZ_Region      ? OSSF4L_secondZ_mass : -1.0");

    _rlm = _rlm
            .Define("ThreeLRegion_WbosonMT",     "threeLRegion     ? Wboson_transversMass : -1.0")
            .Define("BaseRegion_WbosonMT",       "BaseRegion       ? Wboson_transversMass : -1.0")
            .Define("WZ_Region_WbosonMT",        "WZ_Region        ? Wboson_transversMass : -1.0")
            .Define("WZ_Region_m3l_WbosonMT",    "WZ_Region_m3l    ? Wboson_transversMass : -1.0")
            .Define("SignalRegion_tzq_WbosonMT", "SignalRegion_tzq ? Wboson_transversMass : -1.0")
            .Define("SignalRegion_ttz_WbosonMT", "SignalRegion_ttz ? Wboson_transversMass : -1.0");

    _rlm = _rlm
            .Define("ThreeLRegion_topMass",     "threeLRegion     ? top_mass : -1.0")
            .Define("BaseRegion_topMass",       "BaseRegion       ? top_mass : -1.0")
            .Define("eee_Region_topMass",       "eee_Region       ? top_mass : -1.0")
            .Define("eeu_Region_topMass",       "eeu_Region       ? top_mass : -1.0")
            .Define("uue_Region_topMass",       "uue_Region       ? top_mass : -1.0")
            .Define("uuu_Region_topMass",       "uuu_Region       ? top_mass : -1.0")
            .Define("WZ_Region_topMass",        "WZ_Region        ? top_mass : -1.0")
            .Define("WZ_Region_m3l_topMass",    "WZ_Region_m3l    ? top_mass : -1.0")
            .Define("SignalRegion_tzq_topMass", "SignalRegion_tzq ? top_mass : -1.0")
            .Define("SignalRegion_ttz_topMass", "SignalRegion_ttz ? top_mass : -1.0");


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////BDT VARIABLE IN DIFFERENT REGION/////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/*    _rlm = _rlm.Define("nJet_tzq", "SignalRegion_tzq ? ncleanjetspass : -1")
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
	       .Define("MET_pt_tzq", "SignalRegion_tzq ? goodMET_pt : -10");


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
	       .Define("MET_pt_ttz", "SignalRegion_ttz ? goodMET_pt : -10");

    _rlm = _rlm.Define("nJet_trial", "threeLRegion ? ncleanjetspass : -1")
	       .Define("nBJets_trial", "threeLRegion ? ncleanbjetspass : -1")
	       .Define("mWT_trial", "threeLRegion ? Wboson_transversMass : -1")
	       .Define("mTop_trial", "threeLRegion ? top_mass : -1")
	       .Define("mZ_trial", "threeLRegion ? zboson_mass : -1")
	       .Define("dphi_ll_z_trial", "threeLRegion ? dphi_lepZ_OSSF : -10")
	       .Define("cosThetaPol_trial", "threeLRegion ? cosTheta_Polarization_angle : -9")
	       .Define("sumHadPt_trial", "threeLRegion ? sum_selectedJet_pt : -1")
	       .Define("sumLepMetPt_trial", "threeLRegion ? Selected_jeteta_maxAbs : -1")
	       .Define("min_dR_bl_trial", "threeLRegion ? min_dR_bjet_lepton : -10")
	       .Define("max_dR_bl_trial", "threeLRegion ? max_dR_bjet_lepton : -10")
	       .Define("max_dphi_jj_trial", "threeLRegion ? max_dphi_jj : -10")
	       .Define("max_ptjj_trial", "threeLRegion ? max_ptjj : -10")
	       .Define("max_mjj_trial", "threeLRegion ? max_mjj : -10")
	       .Define("mass3l_trial", "threeLRegion ? mass_3lepton : -10")
	       .Define("dR_b_recoil_trial", "threeLRegion ? dR_b_recoilJet : -10")
	       .Define("dR_b_l_trial", "threeLRegion ? dR_b_lepton : -10")
	       .Define("maxJetAbsEta_trial", "threeLRegion ? Selected_jeteta_maxAbs : -10")
	       .Define("etaRecoilingJet_trial", "threeLRegion ? RecoilingJet_eta : -99.0")
	       .Define("lep_asymmetry_trial", "threeLRegion ? topLepton_absEta_times_charge : -9.0")
	       .Define("maxDEEPJET_trial", "threeLRegion ? Selected_bjet_score : ROOT::VecOps::RVec<float>{}")
	       .Define("MET_pt_trial", "threeLRegion ? goodMET_pt : -10");

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
	       .Define("MET_pt_signal", "SignalRegion ? goodMET_pt : -10");
*/


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
    addVartoStore("nElectron");
    addVartoStore("nMuon");
    addVartoStore("norm_factor");
    addVartoStore("GenPart_.*");
    addVartoStore("GenPromptLepton_.*");
    

    // The Trigger list starts from here  
    addVartoStore("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL");
    addVartoStore("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ");
    addVartoStore("HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ");
    addVartoStore("HLT_Mu37_Ele27_CaloIdL_MW");
    addVartoStore("HLT_Mu27_Ele37_CaloIdL_MW");
    addVartoStore("HLT_DiMu9_Ele9_CaloIdL_TrackIdL_DZ");
    addVartoStore("HLT_Mu8_DiEle12_CaloIdL_TrackIdL");
    addVartoStore("HLT_Mu8_DiEle12_CaloIdL_TrackIdL_DZ");

    addVartoStore("HLT_Ele30_WPTight_Gsf");
    addVartoStore("HLT_Ele32_WPTight_Gsf");
    addVartoStore("HLT_Ele35_WPTight_Gsf");
    addVartoStore("HLT_Ele38_WPTight_Gsf");
    addVartoStore("HLT_Ele40_WPTight_Gsf");
    addVartoStore("HLT_Ele115_CaloIdVT_GsfTrkIdT");

    addVartoStore("HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL");
    addVartoStore("HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ");
    addVartoStore("HLT_DoubleEle33_CaloIdL_MW");
    addVartoStore("HLT_DoubleEle25_CaloIdL_MW");
    addVartoStore("HLT_DoubleEle27_CaloIdL_MW");
    addVartoStore("HLT_Ele16_Ele12_Ele8_CaloIdL_TrackIdL");

    addVartoStore("HLT_IsoMu24");
    addVartoStore("HLT_IsoMu24_eta2p1");
    addVartoStore("HLT_IsoMu27");
    addVartoStore("HLT_Mu50");

    addVartoStore("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8");
    addVartoStore("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8");
    addVartoStore("HLT_Mu19_TrkIsoVVL_Mu9_TrkIsoVVL_DZ_Mass8");
    addVartoStore("HLT_Mu19_TrkIsoVVL_Mu9_TrkIsoVVL_DZ_Mass3p8");

    addVartoStore("HLT_TripleMu_10_5_5_DZ");
    addVartoStore("HLT_TripleMu_12_10_5");
    //The Trigger list ended here
    
    //The MET Filters
    addVartoStore("Flag_goodVertices");
    addVartoStore("Flag_globalSuperTightHalo2016Filter");
    addVartoStore("Flag_EcalDeadCellTriggerPrimitiveFilter");
    addVartoStore("Flag_BadPFMuonFilter");
    addVartoStore("Flag_BadPFMuonDzFilter");
    addVartoStore("Flag_hfNoisyHitsFilter");
    addVartoStore("Flag_eeBadScFilter");
    addVartoStore("Flag_ecalBadCalibFilter");

    addVartoStore("baselineElectrons_.*");
    addVartoStore("NbaselineElectrons");
    // addVartoStore("A_tight_baselineElectrons");
   


    addVartoStore("baselineMuons_.*");
    addVartoStore("NbaselineMuons");
    if (!_isData){
    addVartoStore("truth_baselineMuons_.*");
    addVartoStore("Gen_baseline.*");
    addVartoStore("Ntruth_baselineMuons_origin3");
    addVartoStore("Nunique_origin3");
    addVartoStore("GenPart_muFromTopW");
    addVartoStore("hasGenMuFromTopW");
    addVartoStore("Muon_originCheck");
    addVartoStore("Gen_goodLepton_.*");

    }


    //jetmet corr
    addVartoStore("Jet_pt_corr.*");
    addVartoStore("Jet_pt");
    addVartoStore("Selected_.*");
    addVartoStore("ncleanjetspass");
    addVartoStore("good_.*");


    addVartoStore("PuppiMET_pt.*");
    addVartoStore("PuppiMET_phi.*");
    addVartoStore("PuppiMET_pt_corr.*");
    addVartoStore("PuppiMET_phi_corr.*");
    addVartoStore("goodMET_pt");
    addVartoStore("goodMET_phi");

    addVartoStore("NLepton");
    addVartoStore("Lepton_.*");
    addVartoStore("NgoodLepton");
    addVartoStore("goodLepton_.*");

    addVartoStore("ThreeLSignal_leadingLepton_pt");
    addVartoStore("ThreeLSignalRegion_nElectron");
    addVartoStore("ThreeLSignalRegion_nMuon");
    addVartoStore("ThreeLSignalRegion_leadingJet_pt");
    addVartoStore("ThreeLSignalRegion_Jet_HT");
    addVartoStore("zboson_mass_3LRegion");
    addVartoStore("nElectron_T3E_TR");

    // 3-lepton case
    addVartoStore("goodLepton3_.*");
    // 4-lepton case
    addVartoStore("goodLepton4_.*");

    addVartoStore("OSSF_category");
    addVartoStore("zboson_mass");
    addVartoStore("Wboson_transversMass");
    addVartoStore("topLepton_index");
    addVartoStore("OSSF4L_.*");
    addVartoStore("leftover.*");




   /////////////////////////////////
   //////////BDT variable///////////
    addVartoStore("sum_selectedJet_pt");
    addVartoStore("sum_lepton_MET_pt");
    addVartoStore("Selected_jeteta_maxAbs");
    addVartoStore("RecoilingJet_pt");
    addVartoStore("RecoilingJet_eta");


    addVartoStore("eee_Region");
    addVartoStore("eeu_Region");
    addVartoStore("uue_Region");
    addVartoStore("uuu_Region");
    addVartoStore("threeLRegion");

    addVartoStore("EEE_Region");
    addVartoStore("EEU_Region");
    addVartoStore("UUE_Region");
    addVartoStore("UUU_Region");
    addVartoStore("THREELRegion");
    addVartoStore("inzpeak");
    addVartoStore("nDistinctZPairs");
    addVartoStore("nZPairs");
    addVartoStore("goodLepton_ptCut");
    addVartoStore("channel_code");

    addVartoStore("ThreeLRegion.*");
    addVartoStore("uuu_ThreeLRegion.*");
    addVartoStore("uue_ThreeLRegion.*");
    addVartoStore("eeu_ThreeLRegion.*");
    addVartoStore("eee_ThreeLRegion.*");

    addVartoStore("WZ_Region.*");
    addVartoStore("WZ_Region_m3l.*");
    addVartoStore("BaseRegion.*");


    addVartoStore("SignalRegion.*");

    // off-Z 3L regions
    addVartoStore("X_gamma_Region.*");
    addVartoStore("NP_1_Region.*");
    addVartoStore("NP_2_Region.*");

    addVartoStore("zboson_mass_3LRegion.*");

    // 4-lepton regions
    addVartoStore("baseRegion_4L.*");
    addVartoStore("ZZ_Region.*");
    addVartoStore("ttZ_Region.*");


   //these branches are for the skimmer files 
	addVartoStore("totalWeight");
	addVartoStore("globalScale");
	addVartoStore("sumGenWeight");
	addVartoStore("crossSection");
   
   if(!_isData){
      //case1 btag correction- fixed wp	
      //case3 shape correction
    addVartoStore("LHE_HT");
    addVartoStore("Selected_jethadflav");
    addVartoStore("Selected_bjethadflav");//after overlap and btag cut

/*    addVartoStore("good_bjethadflav");
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
*/
    addVartoStore("genWeight");
    addVartoStore("pugenWeight");
    addVartoStore("genEventSumw");      
    //case1 btag correction- fixed wp	
    addVartoStore("btag_SF_bcflav_vector");
    addVartoStore("btag_SF_lflav_vector");
    addVartoStore("totbtagSF");
    addVartoStore("evWeight_wobtagSF");
    
    
    
    //MUONID - ISO SF & WEIGHT	
    addVartoStore("muon_SF_.*");
    addVartoStore("ele_SF_.*");
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
	
   if(!_isData){
    add1DHist( {"hnevents", "Number of Events", 2, -0.5, 1.5}, "one", "genWeight", "");
    add1DHist( {"hnevents_pugenweight", "Number of Events with pugenWeight", 2, -0.5, 1.5}, "one", "pugenWeight", "");
    add1DHist( {"hselected_jetpt_pugenweight", "clean-Jets w/o weight" , 100, 0, 2500} , "Selected_jetpt", "pugenWeight", "");
    add1DHist( {"hNum_selectedjets_puweight", "Number of clean-Jets w/o weight" , 11, -0.5, 10.5} , "ncleanjetspass", "pugenWeight", "");
    add1DHist( {"hNum_goodjets_puweight", "Number of clean-Jets w/o weight" , 11, -0.5, 10.5} , "NgoodJets", "pugenWeight", "");
   }
    add1DHist( {"hnevents_no_weight", "Number of Events w/o", 2, -0.5, 1.5}, "one", "one", "");
    add1DHist( {"hselected_jetptNoweight", "clean-Jets w/o weight" , 100, 0, 2500} , "Selected_jetpt", "one", "");
    add1DHist( {"hNum_selectedjets_Noweight", "Number of clean-Jets w/o weight" , 11, -0.5, 10.5} , "ncleanjetspass", "one", "");
    add1DHist( {"hNum_goodjets_Noweight", "Number of clean-Jets w/o weight" , 11, -0.5, 10.5} , "NgoodJets", "one", "");
    
  //  add1DHist( {"hNgoodElectrons", "NumberofGoodElectrons", 5, 0.0, 5.0}, "NgoodElectrons", "evWeight", "");
    
   // add1DHist( {"hNgoodMuons", "# of good Muons ", 5, 0.0, 5.0}, "NgoodMuons", "evWeight", "");//i made the change 
   // add1DHist( {"zboson_m", "Mass of z boson", 50, 70.0, 110.0}, "zboson_m", "one", "");
    // add1DHist( {"hgood_jetpt_with weight", "Good Jet pt with weight " , 100, 0, 1000} , "goodJets_pt", "evWeight", "");
    // add1DHist( {"hgood_jetpt_NOWeight", "Good Jet pt no weihght " , 100, 0, 1000} , "goodJets_pt", "one", "");
    
    // add1DHist( {"hgood_jet1pt", "Good Jet_1 pt with weight " , 100, 0, 2500} , "good_jet1pt", "evWeight", "");
    // add1DHist( {"hselected_jet1pt", "SelectedJet_1 pt no weight" , 100, 0, 1000} , "Selected_jet1pt", "evWeight", "");
    // add1DHist( {"hselected_jetptWithweight", "clean-Jets with weight" , 100, 0, 2500} , "Selected_jetpt", "evWeight", "");
    add1DHist( {"hselected_jetptNoweight", "clean-Jets w/o weight" , 100, 0, 2500} , "Selected_jetpt", "one", "");
    add1DHist( {"hNum_selectedjets_Noweight", "Number of clean-Jets w/o weight" , 11, -0.5, 10.5} , "ncleanjetspass", "one", "");
    add1DHist( {"hNum_goodjets_Noweight", "Number of clean-Jets w/o weight" , 11, -0.5, 10.5} , "NgoodJets", "one", "");
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
    defineInZPeak();
	// DefineGoodLeptonGroups(); 
	reconstructWboson(); 
	reconstructTop(); 
	// BDT_variables();
	defineSignalRegion();


	//This code is also off due to the problem arising entries of Data files can continue when add the correction files
	if(!_isData){
	  this->calculateEvWeight(); // PU, genweight and BTV and Mu and Ele
	}

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
	// bookHists();
	setupCuts_and_Hists();
	setupTree();
}

