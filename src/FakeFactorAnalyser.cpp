#include "Math/GenVector/VectorUtil.h"
#include "FakeFactorAnalyser.h"
#include "utility.h"
#include <iostream>

FakeFactorAnalyser::FakeFactorAnalyser(TTree *t, std::string outfilename): BaseAnalyser(t, outfilename)
{
     if (debug){
            std::cout << "================================//=================================" << std::endl;
            std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
            std::cout << "================================//=================================" << std::endl;
        }

    std::cout << "FakeFactorAnalyser created." << std::endl;

    // Table 25 paths (measurement-region triggers). These overwrite the
    // 2022EE list inherited from BaseAnalyser's constructor -- setHLT()
    // for this class only ever needs to see these seven.
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

// =====================================================================
// Fakeable / tight electrons  (Table 5)
//
// Baseline and Fakeable share every shower-shape/impact-parameter cut;
// the only differences are the pT threshold (7 vs 10 GeV) and, on top
// of Fakeable, an additional requirement that's skipped entirely when
// the TOP lepton MVA score is already >=0.4 (in which case the lepton
// is tight anyway and trivially satisfies "fakeable"):
//   TOP-MVA >= 0.4   OR   (EGamma MVA2 wpLoose AND DeepJetB(nearby jet) < 0.5
//                          AND jetPtRatio > 0.5)
// Tight = Baseline(pT>10) && TOP-MVA > 0.4.
//
// ASSUMPTIONS ON BRANCH NAMES (verify against your ntuple and fix if
// different):
//   - Electron_mvaTOP           : TOP lepton MVA score (float)
//   - Electron_mvaFall17V2noIso_WPL : EGamma POG MVA2 loose WP (bool),
//                                     standard central NanoAOD branch
//   - Electron_jetIdx           : index into the Jet_* collection of the
//                                  nearby jet used for jetRelIso (int,
//                                  -1 if none) -- reused below for both
//                                  the DeepJetB/jetPtRatio cuts here and
//                                  the Eq. 28 cone-pT closest-jet branch,
//                                  since NanoAOD's own jet-lepton
//                                  matching is already the ~dR<0.4
//                                  nearest jet that Eq. 28 wants.
// =====================================================================
void FakeFactorAnalyser::selectFakeableElectrons()
{
     if (debug){
            std::cout << "================================//=================================" << std::endl;
            std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
            std::cout << "================================//=================================" << std::endl;
        }

    cout << "select fakeable electrons" << endl;

    const std::string sieieCut =
        "((abs(Electron_eta) < 1.479 && Electron_sieie < 0.011) || "
        "(abs(Electron_eta) >= 1.479 && abs(Electron_eta) < 2.5 && Electron_sieie < 0.030))";

    const std::string sharedCuts =
        "abs(Electron_eta) < 2.5 && "
        "abs(Electron_dxy) < 0.05 && abs(Electron_dz) < 0.10 && "
        "Electron_sip3d < 8 && Electron_miniPFRelIso_all < 0.4 && "
        "Electron_lostHits <= 1 && " + sieieCut + " && "
        "Electron_hoe < 0.10 && Electron_eInvMinusPInv > -0.04 && Electron_convVeto";

    _rlm = _rlm.Define("FFbaselineElectrons", ("Electron_pt_corr > 7.0 && " + sharedCuts).c_str());

    _rlm = _rlm.Define("FFfakeablePreMVA_Electrons", ("Electron_pt_corr > 10.0 && " + sharedCuts).c_str());

    _rlm = _rlm.Define("Electron_nearbyJetPtRatio", ::nearbyJetPtRatio,{"Electron_jetRelIso"});

    // The "†" extra cuts, skipped (treated as passed) when TOP-MVA>=0.4.
    _rlm = _rlm.Define("FFfakeableExtraElectron",
        [](const floats &mvaTOP, const bools &wpLoose,
           const floats &nearbyDeepFlavB, const floats &nearbyPtRatio)
        {
            bools out(mvaTOP.size(), false);
            for (size_t i = 0; i < mvaTOP.size(); i++)
            {
                out[i] = (mvaTOP[i] >= 0.9f) ||
                         (wpLoose[i] && nearbyDeepFlavB[i] < 0.5f && nearbyPtRatio[i] > 0.5f);
            }
            return out;
        },
        {"Electron_promptMVA", "Electron_mvaIso_WP90",
         "Electron_jetDF", "Electron_nearbyJetPtRatio"});

    _rlm = _rlm.Define("fakeableElectrons", "FFfakeablePreMVA_Electrons && FFfakeableExtraElectron");

    _rlm = _rlm.Define("fakeableElectrons_pt",     "Electron_pt_corr[fakeableElectrons]")
                .Define("fakeableElectrons_eta",    "Electron_eta[fakeableElectrons]")
                .Define("fakeableElectrons_phi",    "Electron_phi[fakeableElectrons]")
                .Define("fakeableElectrons_mass",   "Electron_mass[fakeableElectrons]")
                .Define("fakeableElectrons_charge", "Electron_charge[fakeableElectrons]")
                .Define("fakeableElectrons_miniIso","Electron_miniPFRelIso_all[fakeableElectrons]")
                .Define("fakeableElectrons_jetIdx", "Electron_jetIdx[fakeableElectrons]")
                .Define("NfakeableElectrons", "int(fakeableElectrons_pt.size())");

    // Tight = fakeable-eligible baseline && TOP-MVA > 0.4 (subset mask,
    // same length/order as the fakeable arrays above).
    _rlm = _rlm.Define("fakeableElectrons_isTight", "Electron_promptMVA[fakeableElectrons] > 0.9f");

    _rlm = _rlm.Define("fakeableElectron_4Vecs", ::generate_4vec,
                {"fakeableElectrons_pt", "fakeableElectrons_eta",
                 "fakeableElectrons_phi", "fakeableElectrons_mass"});
}

// =====================================================================
// Fakeable / tight muons  (Table 6)
//
// Baseline and Fakeable share pT/eta/dxy/dz/SIP3D/iso/MediumID; Fakeable
// pT threshold is 10 GeV (same as baseline here -- Table 6 lists 10 GeV
// for both baseline and fakeable, unlike electrons). On top of that,
// the "†" cuts (skipped when TOP-MVA>=0.4):
//   DeepJetB(nearby jet) < f(a,b; ptcone)   AND   jetPtRatio > 0.45
// where f(a,b;x) is the sliding cut of Eq. (footnote 1 of Table 6):
//   x = min( max(ptcone-25,0)/15 , 1 );  f = (1-x)*a + x*b
// (a,b) = (0.02, 0.015) for 2016/2017, (0.025, 0.015) for 2018.
// No official Run-3 (2024) coefficients are given in the note -- using
// the 2018 pair as a placeholder below; flag if your group has settled
// on different Run-3 values.
// Tight = Baseline && TOP-MVA > 0.4.
//
// ASSUMPTIONS ON BRANCH NAMES (verify against your ntuple):
//   - Muon_mvaTOP    : TOP lepton MVA score (float)
//   - Muon_mediumId  : already used elsewhere in this codebase (MuonID(3))
//   - Muon_jetIdx    : nearby-jet index, same role as Electron_jetIdx
// =====================================================================
void FakeFactorAnalyser::selectFakeableMuons()
{
     if (debug){
            std::cout << "================================//=================================" << std::endl;
            std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
            std::cout << "================================//=================================" << std::endl;
        }

    cout << "select fakeable muons" << endl;

    const float sliding_a = 0.025f; // 2018 placeholder -- confirm Run-3 value
    const float sliding_b = 0.015f;

    const std::string sharedCuts =
        "Muon_pt_corr > 10.0 && abs(Muon_eta) < 2.4 && "
        "abs(Muon_dxy) < 0.05 && abs(Muon_dz) < 0.10 && "
        "Muon_sip3d < 8.0 && Muon_miniPFRelIso_all < 0.4 && Muon_mediumId";

    _rlm = _rlm.Define("FFbaselineMuons", sharedCuts.c_str());
    _rlm = _rlm.Define("FFfakeablePreMVA_Muons", sharedCuts.c_str());

    _rlm = _rlm.Define("Muon_nearbyJetPtRatio", ::nearbyJetPtRatio, {"Muon_jetRelIso"});

    // ptcone needed for the sliding-cut threshold is computed further
    // down in defineConePt(); to avoid a circular dependency here we use
    // the *uncorrected* pt for the x = min(max(pt-25,0)/15,1) argument,
    // which is the standard approximation used since ptcone==pt for any
    // already-tight lepton and the two only differ for genuinely
    // nonprompt candidates near the WP boundary.
    _rlm = _rlm.Define("FFfakeableExtraMuon",
        [sliding_a, sliding_b](const floats &mvaTOP, const floats &pt,
                                const floats &nearbyDeepFlavB, const floats &nearbyPtRatio)
        {
            bools out(mvaTOP.size(), false);
            for (size_t i = 0; i < mvaTOP.size(); i++)
            {
                if (mvaTOP[i] >= 0.64f) { out[i] = true; continue; }
                float x = std::min(std::max(pt[i] - 25.f, 0.f) / 15.f, 1.f);
                float thresh = (1.f - x) * sliding_a + x * sliding_b;
                out[i] = (nearbyDeepFlavB[i] < thresh) && (nearbyPtRatio[i] > 0.45f);
            }
            return out;
        },
        {"Muon_promptMVA", "Muon_pt_corr", "Muon_jetDF", "Muon_nearbyJetPtRatio"});

    _rlm = _rlm.Define("fakeableMuons", "FFfakeablePreMVA_Muons && FFfakeableExtraMuon");

    _rlm = _rlm.Define("fakeableMuons_pt",     "Muon_pt_corr[fakeableMuons]")
                .Define("fakeableMuons_eta",    "Muon_eta[fakeableMuons]")
                .Define("fakeableMuons_phi",    "Muon_phi[fakeableMuons]")
                .Define("fakeableMuons_mass",   "Muon_mass[fakeableMuons]")
                .Define("fakeableMuons_charge", "Muon_charge[fakeableMuons]")
                .Define("fakeableMuons_miniIso","Muon_miniPFRelIso_all[fakeableMuons]")
                .Define("fakeableMuons_jetIdx", "Muon_jetIdx[fakeableMuons]")
                .Define("NfakeableMuons", "int(fakeableMuons_pt.size())");

    _rlm = _rlm.Define("fakeableMuons_isTight", "Muon_promptMVA[fakeableMuons] > 0.64f");

    _rlm = _rlm.Define("fakeableMuon_4Vecs", ::generate_4vec,
                {"fakeableMuons_pt", "fakeableMuons_eta",
                 "fakeableMuons_phi", "fakeableMuons_mass"});
}

// =====================================================================
// Cone-pT correction (Eq. 28): reuses the nearby-jet assignment computed
// above (Electron_jetIdx/Muon_jetIdx) rather than re-searching the full
// jet collection -- same jet, just adds the actual DeltaR to it so the
// "within DeltaR<0.4" gate in coneCorrectedPt() can be applied.
// =====================================================================
void FakeFactorAnalyser::defineConePt()
{
     if (debug){
            std::cout << "================================//=================================" << std::endl;
            std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
            std::cout << "================================//=================================" << std::endl;
        }

    cout << "Running Fake Factor Analyzer" << endl;

    _rlm = _rlm.Define("eleNearbyJet", ::nearbyJetInfo,
                {"fakeableElectrons_pt", "fakeableElectrons_eta", "fakeableElectrons_phi",
                 "fakeableElectrons_jetIdx", "Jet_pt_corr", "Jet_eta", "Jet_phi"})
               .Define("eleClosestJet_dR",    "eleNearbyJet.dR")
               .Define("eleClosestJet_jetPt", "eleNearbyJet.jetPt");

    _rlm = _rlm.Define("muNearbyJet", ::nearbyJetInfo,
                {"fakeableMuons_pt", "fakeableMuons_eta", "fakeableMuons_phi",
                 "fakeableMuons_jetIdx", "Jet_pt_corr", "Jet_eta", "Jet_phi"})
               .Define("muClosestJet_dR",    "muNearbyJet.dR")
               .Define("muClosestJet_jetPt", "muNearbyJet.jetPt");

    _rlm = _rlm.Define("fakeableElectrons_ptcone",
        [](const floats& pt, const ints& tight, const floats& iso,
           const floats& dr, const floats& jetpt) {
            return coneCorrectedPt(pt, tight, iso, dr, jetpt);
        },
        {"fakeableElectrons_pt", "fakeableElectrons_isTight",
         "fakeableElectrons_miniIso", "eleClosestJet_dR", "eleClosestJet_jetPt"});

    _rlm = _rlm.Define("fakeableMuons_ptcone",
        [](const floats& pt, const ints& tight, const floats& iso,
           const floats& dr, const floats& jetpt) {
            return coneCorrectedPt(pt, tight, iso, dr, jetpt);
        },
        {"fakeableMuons_pt", "fakeableMuons_isTight",
         "fakeableMuons_miniIso", "muClosestJet_dR", "muClosestJet_jetPt"});

}

// =====================================================================
// Measurement region:
//   - exactly one fakeable lepton in the event (electron XOR muon)
//   - >=1 jet with pT>25, |eta|<2.5, DeltaR(jet, lepton) > 0.7
// Scalars for "the" lepton (pt, eta, pdgId, ptcone, isTight) are picked
// out here since exactly one exists once the MR cut is applied.
// =====================================================================
void FakeFactorAnalyser::defineMeasurementRegion()
{
     if (debug){
            std::cout << "================================//=================================" << std::endl;
            std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
            std::cout << "================================//=================================" << std::endl;
        }


    cout << "Defining Measurement Region " << endl;

    _rlm = _rlm.Define("isElectronMR", "NfakeableElectrons==1 && NfakeableMuons==0")
               .Define("isMuonMR",     "NfakeableMuons==1 && NfakeableElectrons==0")
               .Define("passOneFakeableLepton", "isElectronMR || isMuonMR");

    // The single lepton's kinematics, valid only when passOneFakeableLepton.
    _rlm = _rlm.Define("MRLepton_pt",
                "isElectronMR ? fakeableElectrons_pt[0] : (isMuonMR ? fakeableMuons_pt[0] : -999.f)")
               .Define("MRLepton_eta",
                "isElectronMR ? fakeableElectrons_eta[0] : (isMuonMR ? fakeableMuons_eta[0] : -999.f)")
               .Define("MRLepton_ptcone",
                "isElectronMR ? fakeableElectrons_ptcone[0] : (isMuonMR ? fakeableMuons_ptcone[0] : -999.f)")
               .Define("MRLepton_isTight",
                "isElectronMR ? fakeableElectrons_isTight[0] : (isMuonMR ? fakeableMuons_isTight[0] : false)")
               .Define("MRLepton_pdgId",
                "isElectronMR ? 11 : (isMuonMR ? 13 : 0)")
               .Define("MRLepton_4Vec",
                "isElectronMR ? fakeableElectron_4Vecs[0] : fakeableMuon_4Vecs[0]");

    // MR jets: pT>25, |eta|<2.5, separated from the single lepton by
    // DeltaR>0.7. goodJets_* already exist from BaseAnalyser::selectJets().
    _rlm = _rlm.Define("MRJetMask",
                [](const FourVectorRVec &jets, const FourVector &lep, bool haveLepton)
                {
                    bools mask(jets.size(), false);
                    if (!haveLepton) return mask;
                    for (size_t i = 0; i < jets.size(); i++)
                    {
                        if (jets[i].Pt() > 25.0 && std::abs(jets[i].Eta()) < 2.5 &&
                            ROOT::Math::VectorUtil::DeltaR(jets[i], lep) > 0.7)
                        {
                            mask[i] = true;
                        }
                    }
                    return mask;
                },
                {"goodJets_4vecs", "MRLepton_4Vec", "passOneFakeableLepton"});

    _rlm = _rlm.Define("MRJets_pt", "goodJets_pt[MRJetMask]")
               .Define("NMRJets", "int(MRJets_pt.size())")
               .Define("MRJet_leadingPt", "MRJets_pt.size() > 0 ? MRJets_pt[0] : -999.f")
               .Define("passMRJet", "NMRJets >= 1");
}

// =====================================================================
// Table 25 / Eq. 29: per-path pT/cone windows and Run-2 (2018) placeholder
// prescales. Replace hltBranch selection + prescale numbers once the
// 2024 brilcalc measurement is available; the structure/logic does not
// need to change, only the table entries below.
// =====================================================================
void FakeFactorAnalyser::defineFFTriggerWeight()
{

     if (debug){
            std::cout << "================================//=================================" << std::endl;
            std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
            std::cout << "================================//=================================" << std::endl;
        }
    _rlm = _rlm.Define("FFTrig",
        [this](bool isElectronMR, bool isMuonMR,
               bool hEle8, bool hEle17, bool hMu3, bool hMu8, bool hMu17, bool hMu20, bool hMu27,
               float lepPt, float jetPt, float lepPtCone)
        {
            std::vector<bool>  fired;
            std::vector<float> lepMin, jetMin, coneLo, coneHi, prescale;

            if (isElectronMR)
            {
                fired    = {hEle8, hEle17};
                lepMin   = {8.f,  17.f};
                jetMin   = {30.f, 30.f};
                coneLo   = {15.f, 25.f};
                coneHi   = {45.f, 100.f};
                prescale = {9318.f, 1537.f};   // 2018 placeholder -- update for 2024
            }
            else if (isMuonMR)
            {
                fired    = {hMu3,  hMu8,  hMu17, hMu20, hMu27};
                lepMin   = {3.f,   8.f,   17.f,  20.f,  27.f};
                jetMin   = {45.f,  30.f,  30.f,  30.f,  30.f};
                coneLo   = {10.f,  15.f,  32.f,  32.f,  45.f};
                coneHi   = {32.f,  100.f, 100.f, 100.f, 100.f};
                prescale = {22160.f, 6990.f, 1305.f, 1081.f, 475.f}; // 2018 placeholder
            }
            else
            {
                return FFTriggerResult{false, 1.0};
            }

            return ::evaluateFFTrigger(fired, lepMin, jetMin, coneLo, coneHi, prescale,
                                        lepPt, jetPt, lepPtCone, !_isData);
        },
        {"isElectronMR", "isMuonMR",
         "HLT_Ele8_CaloIdM_TrackIdM_PFJet30", "HLT_Ele17_CaloIdM_TrackIdM_PFJet30",
         "HLT_Mu3_PFJet40", "HLT_Mu8", "HLT_Mu17", "HLT_Mu20", "HLT_Mu27",
         "MRLepton_pt", "MRJet_leadingPt", "MRLepton_ptcone"});

    _rlm = _rlm.Define("passFFTrigger", "FFTrig.passed")
               .Define("FFTrigWeight",  "FFTrig.weight");
}

void FakeFactorAnalyser::defineCuts()
{
    if (debug) {
        std::cout << "\n=============================================\n"
                  << "Line: " << __LINE__
                  << " | Function: " << __FUNCTION__ << "\n"
                  << "=============================================\n";
    }

    auto nEntries = _rlm.Count();
    std::cout << "---------------------------------------------------\n"
              << "Fake Factor total entries: " << *nEntries << "\n"
              << "---------------------------------------------------\n";

    const std::string metFilters =
        "Flag_goodVertices && "
        "Flag_globalSuperTightHalo2016Filter && "
        "Flag_EcalDeadCellTriggerPrimitiveFilter && "
        "Flag_BadPFMuonFilter && "
        "Flag_BadPFMuonDzFilter && "
        "Flag_hfNoisyHitsFilter && "
        "Flag_eeBadScFilter && "
        "Flag_ecalBadCalibFilter";

    // Measurement-region selection (Section C.1 of the note):
    //  - exactly one fakeable lepton
    //  - >=1 MR jet (pT>25, |eta|<2.5, DeltaR(jet,l)>0.7)
    //  - >=1 applicable HLT path fired with pT/cone requirements satisfied
    const std::string oneFakeableLepton = "passOneFakeableLepton";
    const std::string mrJetSelection    = "passMRJet";
    const std::string ffTriggerCut      = "passFFTrigger";

    addCuts(metFilters,          "0");
    addCuts(oneFakeableLepton,   "00");
    addCuts(mrJetSelection,      "000");
    addCuts(ffTriggerCut,        "0000");
}

// =====================================================================
// FF numerator/denominator histograms: 2D (ptcone, |eta|), split by
// flavor. Binning below is a placeholder (equal-width) -- swap in your
// real binning once decided.
// =====================================================================
void FakeFactorAnalyser::bookHists()
{
    if (debug){
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    // FFWeight_base / FFWeight_num are defined in defineMoreVars(), which
    // runs before this in setupAnalysis(). Denominator = every fakeable
    // event (weight = FFWeight_base); numerator = same node, but the
    // weight is zeroed out for events where the lepton isn't tight, so
    // the histogram only accumulates tight-lepton entries. This avoids
    // needing a second branch of the RNodeTree just for the numerator.
    add2DHist({"hFF_ele_denom", "Electron FF denominator; p_{T}^{cone} [GeV]; |#eta|",
               20, 10, 110, 10, 0.0, 2.5},
              "MRLepton_ptcone", "MRLepton_eta", "FFWeight_base_ele", "");

    add2DHist({"hFF_ele_num", "Electron FF numerator; p_{T}^{cone} [GeV]; |#eta|",
               20, 10, 110, 10, 0.0, 2.5},
              "MRLepton_ptcone", "MRLepton_eta", "FFWeight_num_ele", "");

    add2DHist({"hFF_mu_denom", "Muon FF denominator; p_{T}^{cone} [GeV]; |#eta|",
               20, 10, 110, 10, 0.0, 2.4},
              "MRLepton_ptcone", "MRLepton_eta", "FFWeight_base_mu", "");

    add2DHist({"hFF_mu_num", "Muon FF numerator; p_{T}^{cone} [GeV]; |#eta|",
               20, 10, 110, 10, 0.0, 2.4},
              "MRLepton_ptcone", "MRLepton_eta", "FFWeight_num_mu", "");

    // ================================================================
    // Unweighted histograms
    //
    // pT   : 0 - 300 GeV, bin width = 20 GeV  -> 15 bins
    // eta  : -2.5 - 2.5, bin width = 0.5      -> 10 bins
    // weight = 1
    // ================================================================

    add2DHist({"hFF_ele_denom_unweighted",
               "Electron FF denominator (unweighted); p_{T}^{cone} [GeV]; #eta",
               15, 0, 300, 10, -2.5, 2.5},
              "MRLepton_ptcone",
              "MRLepton_eta",
              "one",
              "");

    add2DHist({"hFF_ele_num_unweighted",
               "Electron FF numerator (unweighted); p_{T}^{cone} [GeV]; #eta",
               15, 0, 300, 10, -2.5, 2.5},
              "MRLepton_ptcone",
              "MRLepton_eta",
              "one",
              "");

    add2DHist({"hFF_mu_denom_unweighted",
               "Muon FF denominator (unweighted); p_{T}^{cone} [GeV]; #eta",
               15, 0, 300, 10, -2.5, 2.5},
              "MRLepton_ptcone",
              "MRLepton_eta",
              "one",
              "");

    add2DHist({"hFF_mu_num_unweighted",
               "Muon FF numerator (unweighted); p_{T}^{cone} [GeV]; #eta",
               15, 0, 300, 10, -2.5, 2.5},
              "MRLepton_ptcone",
              "MRLepton_eta",
              "one",
              "");
}

void FakeFactorAnalyser::defineMoreVars()
{
    if (debug){
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    _rlm = _rlm.Define("FFWeight_base", _isData ? "1.0" : "FFTrigWeight")
               .Define("FFWeight_num",  "MRLepton_isTight ? FFWeight_base : 0.0");

    // Flavor-gated versions: an electron event must contribute zero to the
    // muon histograms and vice versa (MRLepton_* itself carries whichever
    // flavor passed the MR, so without this gate every event would fill
    // both flavors' histograms).
    _rlm = _rlm.Define("FFWeight_base_ele", "isElectronMR ? FFWeight_base : 0.0")
               .Define("FFWeight_num_ele",  "isElectronMR ? FFWeight_num  : 0.0")
               .Define("FFWeight_base_mu",  "isMuonMR     ? FFWeight_base : 0.0")
               .Define("FFWeight_num_mu",   "isMuonMR     ? FFWeight_num  : 0.0");
}

void FakeFactorAnalyser::setupObjects()
{
    std::cout << "FakeFactor setupObjects() called." << std::endl;

    selectJets();                 // inherited from BaseAnalyser, unchanged
    selectFakeableElectrons();
    selectFakeableMuons();
    defineConePt();
    defineMeasurementRegion();
    defineFFTriggerWeight();
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
    defineMoreVars();
    bookHists();
    setupCuts_and_Hists();
    setupTree();
}
