#include "FakeFactorAnalyser.h"
#include <iostream>

FakeFactorAnalyser::FakeFactorAnalyser(TTree *t, std::string outfilename): BaseAnalyser(t, outfilename)
{
    std::cout << "FakeFactorAnalyser created." << std::endl;
    HLT2022EENames = {
        "HLT_Ele8_CaloIdM_TrackIdM_PFJet30",
        "HLT_Ele17_CaloIdM_TrackIdM_PFJet30",
        "HLT_Mu3_PFJet40",
        "HLT_Mu8",
        "HLT_Mu17",
        "HLT_Mu20",
        "HLT_Mu27",
        };

}

void FakeFactorAnalyser::defineCuts()
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
              << "Fake Factor total entries: " << *nEntries << "\n"
              << "---------------------------------------------------\n";


    // -----------------------------------------------------
    // MET filters
    // -----------------------------------------------------
    const std::string metFilters =
        "Flag_goodVertices && "
        "Flag_globalSuperTightHalo2016Filter && "
        "Flag_EcalDeadCellTriggerPrimitiveFilter && "
        "Flag_BadPFMuonFilter && "
        "Flag_BadPFMuonDzFilter && "
        "Flag_hfNoisyHitsFilter && "
        "Flag_eeBadScFilter && "
        "Flag_ecalBadCalibFilter";


    // -----------------------------------------------------
    // Fake Factor primary selection
    //
    // Exactly 1 electron AND exactly 1 muon
    // -----------------------------------------------------
    const std::string primarySelection =
        "nElectron == 1 || "
        "nMuon == 1";


    // -----------------------------------------------------
    // Apply cuts
    // -----------------------------------------------------

    addCuts(setHLT(), "0");
    addCuts(metFilters, "00");
    addCuts(primarySelection, "000");
}

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
                "Electron_miniPFRelIso_all < 0.40 && abs(Electron_dxy) < 0.05 && " 
                "abs(Electron_dz) < 0.10 && Electron_lostHits <= 1 && " 
                "Electron_hoe < 0.10 && Electron_convVeto &&" 
                "((abs(Electron_eta) < 1.479 && Electron_sieie < 0.011) || " // Barrel cut 
                "(abs(Electron_eta) >= 1.479 && abs(Electron_eta) < 2.5 && Electron_sieie < 0.030)) &&" // Endcap cut 
                "Electron_sip3d < 8 && Electron_eInvMinusPInv > -0.04 "
                ); 


    // _rlm = _rlm.Define("baselineElectrons_pt", "Electron_pt_corr[baselineElectrons]")
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

    // Generate 4-vectors for baseline electrons
    _rlm = _rlm.Define("baselineElectron_4Vecs", ::generate_4vec, {"baselineElectrons_pt", "baselineElectrons_eta", "baselineElectrons_phi", "baselineElectrons_mass"});
    _rlm = _rlm.Define("baselineElectron_TL4Vecs", ::buildTLorentzVectors, {"baselineElectrons_pt", "baselineElectrons_eta", "baselineElectrons_phi", "baselineElectrons_mass"});
}

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
                "Muon_miniPFRelIso_all < 0.4 && abs(Muon_dxy) < 0.05 && " 
		        "abs(Muon_dz) < 0.10 && Muon_sip3d < 8.0 && " 
		        "Muon_tightId && Muon_isPFcand" 
		        "&& (Muon_isGlobal || Muon_isTracker)"
                ); 


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
}

void FakeFactorAnalyser::setupObjects()
{
    std::cout << "FakeFactor setupObjects() called." << std::endl;

}

void FakeFactorAnalyser::setupAnalysis()
{
    std::cout << "FakeFactor setupAnalysis() called." << std::endl;

    _rlm = _rlm.Define("one", "1.0");

    if (_isData && !isDefined("evWeight"))
    {
        _rlm = _rlm.Define("evWeight", [](){ return 1.0; }, {});
    }

    if (!_isData)
    {
        auto sumgenweight = _rd.Sum("genWeight");
        std::string sumofgenweight = Form("%f", *sumgenweight);

        _rlm = _rlm.Define("genEventSumw", sumofgenweight.c_str());

        std::cout << "Sum of genWeights = "
                  << sumofgenweight << std::endl;
    }

    defineCuts();

}


