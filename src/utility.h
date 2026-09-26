/*
 * utility.h
 *
 *  Created on: Dec 4, 2018
 *      Author: suyong
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "Math/Vector4D.h"
#include "correction.h"
#include <string>
#include <TLorentzVector.h>
#include <set>

using ushorts = ROOT::RVec<unsigned short>;
using floats =  ROOT::VecOps::RVec<float>;
using doubles =  ROOT::VecOps::RVec<double>;
using ints =  ROOT::VecOps::RVec<int>;
using bools = ROOT::VecOps::RVec<bool>;
using uchars = ROOT::VecOps::RVec<unsigned char>;
using shorts = ROOT::VecOps::RVec<short>;
using FourVector = ROOT::Math::PtEtaPhiMVector;
using FourVectorVec = std::vector<FourVector>;
using FourVectorRVec = ROOT::VecOps::RVec<FourVector>;

// generates vectors of 4 vectors given vectors of pt, eta, phi, mass
FourVectorRVec generate_4vec(floats &pt, floats &eta, floats &phi, floats &mass);

struct ZPairCounts {
    int nPairs;         // total number of OS-SF pairs in the Z window (can double-count leptons)
    int nDistinctPairs; // max number of non-overlapping OS-SF Z pairs (no shared lepton)
};



ints pTcounter(floats vec);

struct hist1dinfo
{
	ROOT::RDF::TH1DModel hmodel;
	std::string varname;
	std::string weightname;
	std::string mincutstep;
} ;

//for 2D histograms
struct hist2dinfo
{
  ROOT::RDF::TH2DModel hmodel;
  std::string varname1;
  std::string varname2;
  std::string weightname;
  std::string mincutstep;
} ;



struct varinfo
{
	std::string varname;
	std::string vardefinition;
	std::string mincutstep;
};

struct cutinfo
{
	std::string cutdefinition;
	std::string idx;
};


// return a vector size equal to length of x all filled with evWeight value
floats weightv(floats &x, float evWeight);

floats sphericity(FourVectorVec &p);

float calculateTopPolarizationAngle(const TLorentzVector& spectatorQuark,const TLorentzVector& lepton,const TLorentzVector& topQuark);

double foxwolframmoment(int l, FourVectorVec &p, int minj=0, int maxj=-1);

float pucorrection(std::unique_ptr<correction::CorrectionSet> &cset, std::string name, std::string syst, float ntruepileup);

ints good_idx(ints good); 
float calculate_deltaEta( FourVector &p1, FourVector &p2);
float calculate_deltaPhi( FourVector &p1, FourVector &p2);
float calculate_deltaPhi_scalars(double &phi1, double &phi2);
float calculate_deltaR( FourVector &p1, FourVector &p2);
float calculate_invMass( FourVector &p1, FourVector &p2);
FourVector sum_4vec( FourVector &p1, FourVector &p2);
floats sort_discriminant( floats discr, floats obj );
FourVector select_leadingvec( FourVectorVec &v );
void PrintVector(floats myvector);
floats w_reconstruction (FourVectorVec &jets);
floats compute_DR (FourVectorVec &muons, ints goodMuons_charge);
float calculate_MT( FourVectorVec &muons, float met, float metphi);
ints findNearestJetsForLeptons(const FourVectorRVec& jets, const FourVectorRVec& leptons);
FourVectorVec addVecFourVec(FourVector &A, FourVector &B, FourVector &C); // 
FourVector generate_single_4vec(double &pt, double &eta, double &phi, double &mass);
doubles calculateDeltaR_group(FourVectorVec &jets, FourVector &lepton);
//int findMinDeltaRJet(const FourVectorRVec& jets, const FourVector& lepton);
ROOT::VecOps::RVec<double> findDeltaR_4all(const FourVectorRVec &leptons, FourVectorVec &jets, int nlepton);
ints findClosestJetsToLeptons(const FourVectorRVec &leptons, FourVectorVec &jets,const floats &jetsPt,const floats &jetsEta);
TLorentzVector generate_TLorentzVector(double &pt, double &eta, double &phi, double &mass) ;

//floats btvcorrection(std::unique_ptr<correction::CorrectionSet> &cset, std::string name, std::string syst, floats &pts, floats &etas, ints &hadflav, floats &btags);


/*=====================================BJets SF============================================*/
floats btvcorrection(std::unique_ptr<correction::CorrectionSet> &cset, std::string name, std::string syst, ints &hadflav, floats &etas,floats &pts,  floats &btags);

floats btv_case1(std::unique_ptr<correction::CorrectionSet> &cset, std::string type, std::string sys,std::string wp, ints &hadflav, floats &etas, floats &pts );
floats btv_case2(std::unique_ptr<correction::CorrectionSet> &cset, std::string type, std::string sys,std::string wp, ints &hadflav, floats &etas, floats &pts );

floats btv_casetest(std::unique_ptr<correction::CorrectionSet>& cset, std::string type1, std::string sys, std::string wp, ints& hadflav, floats& etas, floats& pts);


/*=====================================Muons SF============================================*/
//floats muoncorrection(std::unique_ptr<correction::CorrectionSet> &cset, std::string type, std::string year, floats &etas, floats &pts, std::string sys);
floats muoncorrection(std::unique_ptr<correction::CorrectionSet> &cset, std::string type, std::string year, std::string runtype, floats &etas, floats &pts, std::string sys);

/* --------------- neutrino Reconstruction for SingleTop t-channel ------------------------*/

float calculateLambda(TLorentzVector &lepton, float met_pt, float met_phi);
float calculateDelta(TLorentzVector &lepton, float met_pt, float lambda);

// Reco Pz of neutrino
float calculate_nu_z(TLorentzVector &lepton, float lambda, float delta, float met_pt, float met_phi);


// Real Solutions
float calculate_nu_z_plus(TLorentzVector &lepton, float lambda, float delta);
float calculate_nu_z_minus(TLorentzVector &lepton, float lambda, float delta);

// Complex Solutions
float calculate_nu_z_complex(TLorentzVector &lepton, float met_pt, float met_phi);


float calculate_nu_energy(float met_pt, float met_phi, float met_pz);
TLorentzVector get_neutrino_TL4vec(float met_pt, float met_phi, float met_pz, float met_energy);

/* -------------- W boson Reconstruction for SingleTop t-channel ------------------------*/

TLorentzVector reconstructWboson_TL4vec(TLorentzVector &lepton, TLorentzVector &neutrino);

ROOT::VecOps::RVec<TLorentzVector>
buildTLorentzVectors(
    const ROOT::VecOps::RVec<float>& pt,
    const ROOT::VecOps::RVec<float>& eta,
    const ROOT::VecOps::RVec<float>& phi,
    const ROOT::VecOps::RVec<float>& mass);

TLorentzVector TL4VecFromFourVec(const FourVector &v); 
bool hasExactlyOneOSSFZPair(const FourVectorRVec& leptons,const ints& pdgId); 

ints GetLeptonOrigin(
    const shorts& Lepton_genPartIdx,
    const ints& GenPart_pdgId,
    const ushorts& GenPart_statusFlags,
    const shorts& GenPart_genPartIdxMother
);
int CountUniqueOrigin3(
    const shorts& genIdx,
    const ints& origin);

ROOT::RVec<int> Muon_FromTopW(
    const ROOT::VecOps::RVec<float>& muPt,
    const ROOT::VecOps::RVec<float>& muEta,
    const ROOT::VecOps::RVec<float>& muPhi,
    const ROOT::VecOps::RVec<int>&   pdg,
    const ROOT::VecOps::RVec<short>& mother,
    const ROOT::VecOps::RVec<float>& genEta,
    const ROOT::VecOps::RVec<float>& genPhi,
    float maxDR);

ROOT::RVec<int> GenPart_MuonFromTopW(
    const ROOT::VecOps::RVec<int>&   pdg,
    const ROOT::VecOps::RVec<short>& mother);

bool Event_HasGenMuonFromTopW(
    const ROOT::VecOps::RVec<int>&   pdg,
    const ROOT::VecOps::RVec<short>& mother);

ints CompareLeptonOriginToGenPartFlag(
    const shorts& Lepton_genPartIdx,
    const ints&   origin,              
    const ints&   GenPart_muFromTopW); 

ZPairCounts countOSSFZPairs(const FourVectorRVec& leptons, const ints& pdgId);

// ---------------------------------------------------------------------
// 3-lepton OSSF categorization (single best Z-candidate pair, plus m3l
// fallback). category: 0 = no OSSF pair, 1 = pair on-Z, 2 = pair off-Z
// but m3l on-Z, 3 = pair off-Z and m3l off-Z. idx1/idx2/mass are -1/-1/-1.0
// when category == 0.
struct OSSF3LInfo {
    int    category;
    int    idx1;
    int    idx2;
    double mass;
};

OSSF3LInfo computeOSSF3LInfo(const FourVectorRVec& leptons, const ints& pdgId);

// -1 unless category == 1 (only a genuine on-Z pair leaves a well-defined
// "third" lepton).
int getTopLeptonIndex3L(const OSSF3LInfo& info);

// Returns FourVector{} if idx < 0.
FourVector selectLeptonByIndex(const FourVectorRVec& leptons, int idx);

// ---------------------------------------------------------------------
// 3-lepton OSSF: accessors pulling individual branches back out of
// OSSF3LInfo, plus m3l and the outside-Z-window mask.
// ---------------------------------------------------------------------

int    getOSSF3LCategory(const OSSF3LInfo& info);
double getZBosonMass(const OSSF3LInfo& info);
double getNonZOSSFMass(const OSSF3LInfo& info);
double getMassOf3GoodLeptons4BG(const OSSF3LInfo& info, const FourVectorRVec& leptons);
double getM3L(const FourVectorRVec& leptons);
bool   isMaskCat1OutsideZ3L(const OSSF3LInfo& info, double m3l);



// ---------------------------------------------------------------------
// 4-lepton OSSF categorization. category: 0 = no valid Z pair,
// 1 = one on-Z pair + two leftover leptons, 2 = two non-overlapping
// on-Z pairs (ZZ candidate). idx1/idx2/mass1 describe the best
// ("Z1") pair; idx3/idx4/mass2 describe the other two leptons --
// mass2 is populated whenever category >= 1, regardless of whether
// that pair is itself OSSF or on-Z (it's the leftover pair's mass,
// meaningful even when they don't form a Z).
// ---------------------------------------------------------------------
struct OSSF4LInfo {
    int    category;
    int    idx1, idx2;
    double mass1;
    int    idx3, idx4;
    double mass2;
};

OSSF4LInfo computeOSSF4LInfo(const FourVectorRVec& leptons, const ints& pdgId);

int    getOSSF4LCategory(const OSSF4LInfo& info);
double getOSSF4LBestZMass(const OSSF4LInfo& info);
double getOSSF4LSecondZMass(const OSSF4LInfo& info);
int    getLeftoverLepton1Index(const OSSF4LInfo& info);
int    getLeftoverLepton2Index(const OSSF4LInfo& info);
double getLeftoverPairMass(const OSSF4LInfo& info);

ROOT::RDF::RNode defineRegionObjectBranches(ROOT::RDF::RNode df,
                                             const std::string& regionCol,
                                             const std::string& outPrefix);

// =====================================================================
// utility_ff_additions.h
//
// Append the contents of this file into utility.h (inside the existing
// header, after the other struct/function declarations, before the
// closing #endif). Kept as a separate file here only so the diff is
// easy to review before you merge it in.
// =====================================================================

// ---------------------------------------------------------------------
// Fake-factor (TOP-group cone-pt) helpers
// ---------------------------------------------------------------------

// Per-lepton closest-jet info, used both for the pT^cone jet-matched
// branch (Eq. 28, DeltaR < 0.4) and for the MR jet-lepton separation
// cut (DeltaR(jet, l) > 0.7). Computed once, reused for both.
struct ClosestJetInfo
{
    floats dR;     // DeltaR to closest jet, per lepton (999 if no jets)
    floats jetPt;  // pT of that closest jet, per lepton (-1 if no jets)
};

ClosestJetInfo nearbyJetInfo(const floats &lepPt, const floats &lepEta, const floats &lepPhi,
                              const shorts &jetIdx, const floats &jetPt,
                              const floats &jetEta, const floats &jetPhi);

// DeepJetB / pT of the nearby jet given per-lepton jetIdx (-1 -> no jet:
// DeepJetB returned as -1 (trivially passes "< threshold" cuts), pT
// ratio returned as 1.0 (trivially passes ">threshold" cuts, i.e. an
// isolated lepton with no nearby jet is treated as fully lepton-like).
floats jetDeepFlavBByIdx(const ints &jetIdx, const floats &jetDeepFlavB);
floats jetPtRatioByIdx(const floats &lepPt, const ints &jetIdx, const floats &jetPt);

// Cone-corrected lepton pT, TOP-group Eq. 28:
//   tight            -> pt_l
//   jet within dRmatch -> x * pt_closestJet
//   else             -> x * pt_l * (1 + pfRelIso)
// x = 0.67 for both electrons and muons.
floats coneCorrectedPt(const floats &lepPt,
                        const ints &isTight,
                        const floats &pfRelIso,
                        const floats &closestJetDR,
                        const floats &closestJetPt,
                        float x = 0.67f,
                        float dRmatch = 0.4f);

// Result of the measurement-region trigger + prescale-weight evaluation
// for the single fakeable lepton in the event (Table 25 / Eq. 29).
struct FFTriggerResult
{
    bool   passed;  // fired >=1 applicable path with all pT/cone requirements met
    double weight;  // MC-only prescale reweighting; 1.0 for data or if !passed
};

// hltFired / lepPtMinTab / jetPtMinTab / coneLoTab / coneHiTab / prescaleTab
// are all parallel arrays: one entry per trigger path in Table 25, already
// filtered down (by the caller) to just the paths relevant to this
// lepton's flavor (electron paths for an electron event, muon paths for a
// muon event). hltFired[i] is that event's decision bit for path i.
FFTriggerResult evaluateFFTrigger(const std::vector<bool>  &hltFired,
                                   const std::vector<float> &lepPtMinTab,
                                   const std::vector<float> &jetPtMinTab,
                                   const std::vector<float> &coneLoTab,
                                   const std::vector<float> &coneHiTab,
                                   const std::vector<float> &prescaleTab,
                                   float lepPt,
                                   float jetPt,
                                   float lepPtCone,
                                   bool  isMC);

floats nearbyJetPtRatio(const floats& relIso);
#endif /* UTILITY_H_ */
