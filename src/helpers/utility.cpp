/*
 * utility.cpp
 *
 *  Created on: Dec 4, 2018
 *      Author: suyong
 */
#include "utility.h"
#include "TMatrixDSym.h"
#include "TVectorT.h"
#include "Math/SpecFuncMathMore.h"
#include "correction.h"
#include "Math/GenVector/VectorUtil.h"
#include "Math/GenVector/Rotation3D.h"
#include "Math/Math.h"
#include<cmath>
#include <vector>      // For std::vector
#include <algorithm>   // For std::min_element and std::distance
#include <TLorentzVector.h>
#include <TVector3.h>

// Utility function to generate fourvector objects for thigs that pass selections

using namespace std;

// FourVectorVec generate_4vec(floats &pt, floats &eta, floats &phi, floats &mass)  //so this function type has been changed i want to see what else is being changed
FourVectorRVec generate_4vec(floats &pt, floats &eta, floats &phi, floats &mass)
{
	const int nsize = pt.size();
	FourVectorVec fourvecs;
	fourvecs.reserve(nsize);
	for (auto i=0; i<nsize; i++)
	{
		fourvecs.emplace_back(pt[i], eta[i], phi[i], fabs(mass[i]));
	}

	return fourvecs;
}

float calculateTopPolarizationAngle(const TLorentzVector& spectatorQuark,
                                   const TLorentzVector& lepton,
                                   const TLorentzVector& topQuark) {
    
    // Boost all vectors to top quark rest frame
    TLorentzVector spectator_topRF = spectatorQuark;
    TLorentzVector lepton_topRF = lepton;
    
    TVector3 topBoost = topQuark.BoostVector();
    spectator_topRF.Boost(-topBoost);
    lepton_topRF.Boost(-topBoost);

    // Get 3-momenta in top rest frame
    TVector3 p_spec = spectator_topRF.Vect();
    TVector3 p_lep = lepton_topRF.Vect();

    // Calculate cos(theta*)
    float cosThetaPol = p_spec.Dot(p_lep) / (p_spec.Mag() * p_lep.Mag());

    // Clamp to [-1, 1] to avoid numerical artifacts
    cosThetaPol = std::max(-1.0f, std::min(1.0f, cosThetaPol));

    return cosThetaPol;
}



floats weightv(floats &x, float evWeight)
{
	const int nsize = x.size();
	floats weightvector(nsize, evWeight);
	return weightvector;
}

floats sphericity(FourVectorVec &p)
{
	TMatrixDSym NormMomTensor(3);

	NormMomTensor = 0.0;
	double p2sum = 0.0;
	for (auto x: p)
	{
		p2sum += x.P2();
		double mom[3] = {x.Px(), x.Py(), x.Pz()};
		for (int irow=0; irow<3; irow++)
		{
			for (int icol=irow; icol<3; icol++)
			{
				NormMomTensor(irow, icol) += mom[irow] * mom[icol];
			}
		}
	}
	NormMomTensor *= (1.0/p2sum);
	TVectorT<double> Qrev;
	NormMomTensor.EigenVectors(Qrev);
	floats Q(3);
	for (auto i=0; i<3; i++) Q[i] = Qrev[2-i];

	return Q;
}


double foxwolframmoment(int l, FourVectorVec &p, int minj, int maxj)
{   // PRD 87, 073014 (2013)
	double answer = 0.0;

	double ptsum=0.0;

	if (maxj==-1) // process everything
	{
		maxj = p.size();
	}
	//for (auto x: p)
	for (auto i=minj; i<maxj; i++)
	{
		auto x = p[i];
		ptsum += x.Pt();
		//for (auto y: p)
		for (auto j=minj; j<maxj; j++)
		{
			auto y = p[j];
			double wij = x.Pt() * y.Pt();
			double cosdOmega = x.Vect().Dot(y.Vect()) / (x.P() * y.P());
			if (cosdOmega>1.0) cosdOmega=1.0;
			if (cosdOmega<-1.0) cosdOmega=-1.0;
			answer += wij * ROOT::Math::legendre(l, cosdOmega);
		}
	}
	answer /= ptsum*ptsum;
	if (fabs(answer)>1.0) std::cout << "FW>1 " << answer << std::endl;
	return answer;
}


// case 1: Evaluate central SFs 
//fixedWP correction with mujets (here medium WP)
// evaluate('systematic', 'working_point', 'flavor', 'abseta', 'pt')
floats btv_case1(std::unique_ptr<correction::CorrectionSet>& cset, std::string type, std::string sys, std::string wp, ints& hadflav, floats& etas, floats& pts)
{
  floats scalefactors_case1;
  const auto nvecs = pts.size();
  //cout << "NVECS======== " << nvecs << endl;
  scalefactors_case1.reserve(nvecs);
  const auto abs_etas = [etas]() {
    floats res;
    res.reserve(etas.size());
    std::transform(etas.begin(), etas.end(), std::back_inserter(res), [](const auto& e) { return std::fabs(e); });
    return res;
  }();
  const auto cast_pts = [pts]() {
    floats res;
    res.reserve(pts.size());
    std::transform(pts.begin(), pts.end(), std::back_inserter(res), [](const auto& p) { return static_cast<float>(p); });
    return res;
  }();
  float bc_jets = 1.0;
  
  for (auto i = 0; i < int(nvecs); i++) {
    //std::cout << "sys: " << sys << ", wp: " << wp << ", hadflav: " << hadflav[i] << ", etas: " << abs_etas[i] << ", pts: " << cast_pts[i] << '\n';
    if(abs_etas[i]<=2.4){
      if (hadflav[i] != 0) {
	bc_jets *= cset->at("deepJet_mujets")->evaluate({sys, wp, hadflav[i], abs_etas[i], cast_pts[i]});
	//scalefactors_case1.emplace_back(bc_jets);
	//std::cout << "\njet SFs from deepJe_mujets at medium WP\n";
	//std::cout << "SF b/c jets : " << bc_jets << '\n';
      } else{ 
	//const auto bc_jets = cset->at("deepJet_incl")->evaluate({sys, wp, hadflav[i], abs_etas[i], cast_pts[i]});
	bc_jets *= cset->at("deepJet_incl")->evaluate({sys, wp, hadflav[i], abs_etas[i], cast_pts[i]});
      }
    }
    scalefactors_case1.emplace_back(bc_jets);
    //std::cout << "\njet SFs from deepJet_incl at medium WP\n";
    //std::cout << "SF light jets : " << bc_jets << '\n';
    //}
    //
    
  }

  return scalefactors_case1;
}

// case 2: Evaluate varied SFs 
//fixedWP correction uncertainty (here tight WP and comb SF)
// evaluate('systematic', 'working_point', 'flavor', 'abseta', 'pt')
floats btv_case2(std::unique_ptr<correction::CorrectionSet>& cset, std::string type, std::string sys, std::string wp, ints& hadflav, floats& etas, floats& pts)
{
  floats scalefactors_case2;
  const auto nvecs = pts.size();
  //cout << "NVECS======== " << nvecs << endl;
  scalefactors_case2.reserve(nvecs);
  const auto abs_etas = [etas]() {
    floats res;
    res.reserve(etas.size());
    std::transform(etas.begin(), etas.end(), std::back_inserter(res), [](const auto& e) { return std::fabs(e); });
    return res;
  }();
  const auto cast_pts = [pts]() {
    floats res;
    res.reserve(pts.size());
    std::transform(pts.begin(), pts.end(), std::back_inserter(res), [](const auto& p) { return static_cast<float>(p); });
    return res;
  }();
  for (auto i = 0; i < int(nvecs); i++) {

    float bweight = 1.0;
     
    if (hadflav[i] != 0) {
      //std::string type = "deepJet_comb" ;
      const auto bc_jets = cset->at("deepJet_comb")->evaluate({sys, wp, hadflav[i], abs_etas[i], cast_pts[i]});
      bweight = bc_jets;
      //std::cout << "\njet SFs up_correlated for comb at tight WP\n";
      //std::cout << "SF b/c : " << bc_jets << '\n';
    } else{ 
      //std::string type = "depJet_incl" ;
      const auto bc_jets = cset->at("deepJet_incl")->evaluate({sys, wp, hadflav[i], abs_etas[i], cast_pts[i]});
      bweight = bc_jets;
      
      //std::cout << "\njet up_correlated for comb at tight  WP\n";
      //std::cout << "SF light jets : " << bc_jets << '\n';
    }
    scalefactors_case2.emplace_back(bweight);
  }

  return scalefactors_case2;

}


floats btvcorrection(std::unique_ptr<correction::CorrectionSet> &cset, std::string type, std::string sys, ints &hadflav,floats &etas,floats &pts,   floats &btags)
{
  floats scalefactors;
  auto nvecs = pts.size();
  scalefactors.reserve(nvecs);
  for (auto i=0; i<int(nvecs); i++)
    {
      //std::cout << "sys: " << sys << ", hadflav: " << hadflav[i] << ", etas: " << fabs(float(etas[i])) << ", pts: " << float(pts[i]) <<" btag discriminator : " << float(btags[i]) << '\n';
      //for 2018UL working points l: 0.0494, m: 0.2770, t: 0.7264
      //if (btags[i]>0.7264){
      float sfi = cset->at(type)->evaluate({sys, int(hadflav[i]), fabs(float(etas[i])), float(pts[i]), float(btags[i])});
      scalefactors.emplace_back(sfi);
      cout<<" jets central scale factors == "<< sfi << endl;
      //}
    }
  return scalefactors;
}


float pucorrection(std::unique_ptr<correction::CorrectionSet> &cset, std::string name, std::string syst, float ntruepileup)
{
	return float(cset->at(name)->evaluate({ntruepileup, syst.c_str()}));
}

ints good_idx(ints good)
{
	ints out;
	for(int i = 0; i < int(good.size()); i++){
		if( good[i] ){
			out.emplace_back(i);
		}
	}
	return out;

}

float calculate_deltaEta( FourVector &p1, FourVector &p2){
	return p1.Eta() - p2.Eta();
}
float calculate_deltaPhi( FourVector &p1, FourVector &p2){
	return ROOT::Math::VectorUtil::DeltaPhi(p1, p2);
}

float calculate_deltaPhi_scalars(double &phi1, double &phi2){
	return ROOT::VecOps::DeltaPhi(phi1, phi2);
}

float calculate_deltaR( FourVector &p1, FourVector &p2){
	return ROOT::Math::VectorUtil::DeltaR(p1, p2);
}
float calculate_invMass( FourVector &p1, FourVector &p2){
	return ROOT::Math::VectorUtil::InvariantMass(p1, p2);
}
FourVector sum_4vec( FourVector &p1, FourVector &p2){
	return p1+p2;
}
//Get indices that sort the object vectors in descending order
floats sort_discriminant( floats discr, floats obj ){
	auto sorted_discr = Reverse(Argsort(discr));
	floats out;
	for (auto idx : sorted_discr){
		out.emplace_back(obj[idx]);
	}
	return out;
}
FourVector select_leadingvec( FourVectorVec &v ){
	FourVector vout;
	if(v.size() > 0) return v[0];
	else return vout;
}

void PrintVector(floats myvector){
	for (size_t i = 0; i < myvector.size(); i++){
		std::cout<<myvector[i]<<"\n";
	}

}


//======================Muons SF===============================================================================//
//muonID_SF
floats muoncorrection(std::unique_ptr<correction::CorrectionSet> &cset, std::string type, std::string year, std::string runtype,floats &etas, floats &pts, std::string sys)
{
  string Data_Type = year + "_" + runtype;
    floats sf_muon;
    auto nvecs = pts.size();
    //cout<<"NVECS====== == "<< nvecs << endl;
    sf_muon.reserve(nvecs);
    
    if (etas.size() != nvecs) {
        throw std::invalid_argument("etas and pts vectors must have the same size!");
    }
    
    for (auto i=0; i<int(nvecs); i++)
    {
        //std::cout << "year: " << year  << ", etas: " << fabs(float(etas[i])) << ", pts: " << float(pts[i]) << " sys : " << sys << '\n';
        
        if (pts[i] < 0 ) {
            throw std::invalid_argument("Invalid value of pT detected!");
        }
        
        if (fabs(float(etas[i])) > 2.5) {
            throw std::invalid_argument("Invalid value of eta detected!");
        }
		//for muons pt<15 
		/*if (pts[i] < 15 || fabs(float(etas[i])) > 2.4) {
        sf_muon.emplace_back(-1);
        continue;
    	}*/
	
        float sfm = cset->at(type)->evaluate({Data_Type, fabs(float(etas[i])), float(pts[i]), sys});
        sf_muon.emplace_back(sfm);

    }
    return sf_muon;
}


//======================Function to check the number of objects with pT greather than x========================//
ints pTcounter(floats vec)
{
	ints output;
	int counter = 0;
	for(auto pt: vec)
	{
		if (pt>10.0) counter++;
	}
	output.emplace_back(counter);
	return output;
}




//=====================build pair example=============================================//
//W reconstruction : 2 jets
//====================================================================================//
floats w_reconstruction (FourVectorVec &jets){
floats out;
float dijetMass;
float dijetMass_out;
float dijetDR;
const float Mass_W = 80.9; //W mass 
const float Width_W = 10.8; //W width
float X_Min = 99999;
float X_Recwidth_W; 

//Loop on all selected jets
	for(unsigned int j1 = 0; j1<jets.size()-1; j1++){
		for(unsigned int j2 = j1+1; j2<jets.size(); j2++){
			 //select 2 jets
			dijetMass = (jets[j1]+jets[j2]).M();
			  //select a best W candidate in min width in a event
			X_Recwidth_W = std::pow((Mass_W-dijetMass)/Width_W,2);
			if (X_Recwidth_W<X_Min){
				dijetDR = ROOT::Math::VectorUtil::DeltaR(jets[j1],jets[j2]);
				X_Min = X_Recwidth_W;
				dijetMass_out = dijetMass;
			}

		}
	}
	out.push_back(dijetMass_out);   //0:w_mass
	out.push_back(dijetDR);  
	return out;
}


floats compute_DR (FourVectorVec &muons, ints goodMuons_charge){
	floats out;
	float mu_ss_DR;
	float mu_os_DR;
	//std::cout<<"Muonsize: " << muons.size()<<std::endl;
	if(muons.size()>0)
	//Loop on all selected muons
	for(unsigned int mu1 = 0; mu1<muons.size()-1; mu1++){
		for(unsigned int mu2 = mu1+1; mu2<muons.size(); mu2++){
			 //select 2 muons with same sign
			 if (goodMuons_charge[mu1]!=goodMuons_charge[mu2]) continue; //check charge of muons
			mu_ss_DR = ROOT::Math::VectorUtil::DeltaR(muons[mu1],muons[mu2]);
			 //select 2 muons with same sign
			if (goodMuons_charge[mu1]==goodMuons_charge[mu2]) continue;
			mu_os_DR = ROOT::Math::VectorUtil::DeltaR(muons[mu1],muons[mu2]);

		}
	}
	out.push_back(mu_ss_DR);		//0: same sign dimuon DR
	out.push_back(mu_os_DR);        //1: opposite sign dimuon dR
	return out;

}



FourVectorVec addVecFourVec(FourVector &A, FourVector &B, FourVector &C) // 
{
	FourVectorVec all;
	all.push_back(A);
	all.push_back(B);
	all.push_back(C);
	return all;
}



FourVector generate_single_4vec(double &pt, double &eta, double &phi, double &mass)
{
	FourVector fourvecs;

	fourvecs.SetPt(pt);
	fourvecs.SetEta(eta);
	fourvecs.SetPhi(phi);
	fourvecs.SetM(fabs(mass));

	return fourvecs;
}


doubles calculateDeltaR_group(FourVectorVec &jets, FourVector &lepton)
{
	std::vector<double> deltaR(jets.size());
	for(auto ajet: jets)
	{
		deltaR.emplace_back(calculate_deltaR(ajet,lepton));
	}

	return deltaR;
}
///////////////////////////////////////////////////////////////////
////////////edited by manas for the nearest jet for lepton/////////
///////////////////////////////////////////////////////////////////
ROOT::VecOps::RVec<double> findDeltaR_4all(const FourVectorRVec &leptons, FourVectorVec &jets, int nlepton)
{
    // Check if both nlepton and njet are greater than 1 before proceeding
    if (nlepton > 1) {
        // Declare deltaR to store the result
        ROOT::VecOps::RVec<double> deltaR;

        for (const auto &lepton : leptons) {
            // Calculate deltaR for each lepton, which returns a ROOT::VecOps::RVec<double>
            ROOT::VecOps::RVec<double> deltaR_single_vec = calculateDeltaR_group(jets, const_cast<FourVector&>(lepton));

            // Push all deltaR values from the ROOT::VecOps::RVec to the result
            for (const double& delta : deltaR_single_vec) {
                deltaR.push_back(delta);
            }
        }

        return deltaR;  // Return the deltaR vector
    } else {
        // Return an empty vector or handle the case when the condition is not satisfied
        return ROOT::VecOps::RVec<double>();  // Empty vector
    }
}



ints findClosestJetsToLeptons(const FourVectorRVec &leptons, 
                               FourVectorVec &jets, 
                               const floats &jetsPt,
                               const floats &jetsEta)
{
    // RVec to store the jet IDs corresponding to each lepton
    ints closestJetIds;

    for (const auto &lepton : leptons) {  // Keep lepton as a const reference
        // Cast away const qualifier for passing to calculateDeltaR_group
        doubles deltaR = calculateDeltaR_group(jets, const_cast<FourVector&>(lepton));

        // Initialize variables to track the minimum delta R and corresponding jet ID
        double minDeltaR = 0.4;  // A large value to begin with
        int minJetId = -1;

        // Iterate over the jets to find the closest one with valid conditions
        for (size_t i = 0; i < jets.size(); ++i) {
            // Check if the jet satisfies the Pt and eta conditions
            if (jetsPt[i] > 5.0 && jetsEta[i] > -5.0 && jetsEta[i] < 5.0 && deltaR[i] < 0.4) {
                if (deltaR[i] < minDeltaR) {
                    minDeltaR = deltaR[i];
                    minJetId = i;  // Store the ID of the jet with the minimum delta R
                }
            }
        }

        // If a valid jet was found, store its ID
        if (minJetId != -1) {
            closestJetIds.push_back(minJetId);
        }
    }

    return closestJetIds;
}


//////////////////////////////////////////
/////////ends here////////////////////////
//////////////////////////////////////////

TLorentzVector generate_TLorentzVector(double &pt, double &eta, double &phi, double &mass) 
{
   TLorentzVector TLorentzVec;
   TLorentzVec.SetPtEtaPhiM(pt, eta, phi, mass);
   return TLorentzVec;
};


/* -------------- W boson Reconstruction for SingleTop t-channel ------------------------*/


float calculateLambda(TLorentzVector &lepton, float met_pt, float met_phi)
{
   float met_px = met_pt*TMath::Cos(met_phi);
   float met_py = met_pt*TMath::Sin(met_phi);
   float mW = 80.4;

   float lambda = (pow(mW,2))/2 + met_px*lepton.Px() + met_py*lepton.Py();
   return lambda;

}

float calculateDelta(TLorentzVector &lepton, float met_pt, float lambda)
{
    // float delta = 4*lambda*lambda*lepton.Pz()*lepton.Pz() - 4*lepton.Pt()*lepton.Pt()*(lepton.E()*lepton.E()*met_pt*met_pt - lambda*lambda);

	float delta1 = pow(lambda*lepton.Pz(),2);
	float delta2 = pow(lepton.Pt(),2)*(pow(lepton.E(),2)*pow(met_pt,2)-pow(lambda,2));

	float delta = 4*(delta1-delta2);

    return delta;
}


float calculate_nu_z(TLorentzVector &lepton, float lambda, float delta, float met_pt, float met_phi)
{
  float p_nu_z = 0.0;
  if(delta>=0)
    {
      float p_nu_z_plus = calculate_nu_z_plus(lepton, lambda, delta);
      float p_nu_z_minus = calculate_nu_z_minus(lepton, lambda, delta);
      
      p_nu_z = p_nu_z_plus < p_nu_z_minus ? p_nu_z_plus : p_nu_z_minus;
      
    }
  
  if(delta<0)
    {
      p_nu_z = calculate_nu_z_complex(lepton, met_pt, met_phi);
      
    }
  return p_nu_z;
}



float calculate_nu_z_plus(TLorentzVector &lepton, float lambda, float delta)
{
   double p_nu_z_plus = (lambda*lepton.Pz())/(pow(lepton.Pt(),2)) + (sqrt(delta))/(2*pow(lepton.Pt(),2));
   return p_nu_z_plus;
}

float calculate_nu_z_minus(TLorentzVector &lepton, float lambda, float delta)
{
   double p_nu_z_minus = (lambda*lepton.Pz())/(pow(lepton.Pt(),2)) - (sqrt(delta))/(2*pow(lepton.Pt(),2));
   return p_nu_z_minus;
}



float calculate_nu_z_complex(TLorentzVector &lepton, float met_pt, float met_phi)
{
  //float met_px; 
  //float met_py;
  float mW = 80.4;
  
  float reco_nu_pt_up = sqrt(2)*abs(mW + lepton.Pt()/sqrt(2));
  float reco_nu_pt_down = sqrt(2)*abs(mW - lepton.Pt()/sqrt(2));
  
  float reco_nu_pt;
  float reco_nu_phi;
  
  if (reco_nu_pt_down < 0)
    {
      reco_nu_pt = reco_nu_pt_up;
    }
  else if (abs(met_pt - reco_nu_pt_down) < abs(met_pt - reco_nu_pt_up))
    {
      reco_nu_pt = reco_nu_pt_down;
    }
  else if (abs(met_pt - reco_nu_pt_down) > abs(met_pt - reco_nu_pt_up))
    {
      reco_nu_pt = reco_nu_pt_up;
    }
  
  float cosThetaLepNu = (mW*mW - reco_nu_pt*reco_nu_pt - lepton.Pt()*lepton.Pt())/(2*lepton.Pt()*reco_nu_pt);
  
  
  if (abs(cosThetaLepNu)>1 && abs(met_pt - reco_nu_pt_down) < abs(met_pt - reco_nu_pt_up) && reco_nu_pt_up > 0 )
    {
      reco_nu_pt = reco_nu_pt_up;
    }
  if (abs(cosThetaLepNu)>1 && abs(met_pt - reco_nu_pt_down) > abs(met_pt - reco_nu_pt_up) && reco_nu_pt_down > 0)
    {
      reco_nu_pt = reco_nu_pt_down;
    }
  
  cosThetaLepNu = (mW*mW - reco_nu_pt*reco_nu_pt - lepton.Pt()*lepton.Pt())/(2*lepton.Pt()*reco_nu_pt);
  
  if(cosThetaLepNu>=0) reco_nu_phi = lepton.Phi() + acos(cosThetaLepNu);
  if(cosThetaLepNu<0) reco_nu_phi = lepton.Phi() + (2*M_PI - acos(cosThetaLepNu));
  if (isnan(reco_nu_phi)) return -999.9;
  
  //met_px = reco_nu_pt*TMath::Cos(reco_nu_phi); 
  //met_py = reco_nu_pt*TMath::Sin(reco_nu_phi);
  
  float lambda = mW*mW/2 + reco_nu_pt*lepton.Pt()*cosThetaLepNu;
  
  float met_pz = lambda*lepton.Pz()/pow(lepton.Pt(),2);
  
  return met_pz;
}


float calculate_nu_energy(float met_pt, float met_phi, float met_pz)
{
   float met_px = met_pt*TMath::Cos(met_phi); 
   float met_py = met_pt*TMath::Sin(met_phi);

   float nu_energy = TMath::Sqrt(met_px*met_px+met_py*met_py+met_pz*met_pz);

   return nu_energy;
}

TLorentzVector get_neutrino_TL4vec(float met_pt, float met_phi, float met_pz, float met_energy)
{
    float met_px = met_pt*TMath::Cos(met_phi); 
    float met_py = met_pt*TMath::Sin(met_phi);

	TLorentzVector neutrino_TL4vec;

	neutrino_TL4vec.SetPxPyPzE(met_px, met_py, met_pz, met_energy);

	return neutrino_TL4vec;
}


TLorentzVector reconstructWboson_TL4vec(TLorentzVector &lepton, TLorentzVector &neutrino)
{
	TLorentzVector Wboson_TL4vec = lepton + neutrino;
	return Wboson_TL4vec;
}



ROOT::VecOps::RVec<TLorentzVector>
buildTLorentzVectors(
    const ROOT::VecOps::RVec<float>& pt,
    const ROOT::VecOps::RVec<float>& eta,
    const ROOT::VecOps::RVec<float>& phi,
    const ROOT::VecOps::RVec<float>& mass)
{
    ROOT::VecOps::RVec<TLorentzVector> vecs;
    vecs.reserve(pt.size());

    for (size_t i = 0; i < pt.size(); ++i)
    {
        TLorentzVector v;
        v.SetPtEtaPhiM(pt[i], eta[i], phi[i], mass[i]);
        vecs.emplace_back(v);
    }

    return vecs;
}



bool hasExactlyOneOSSFZPair(const FourVectorRVec& leptons,
                            const ints& pdgId)
{
    int nZPairs = 0;
    const size_t n = leptons.size();

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            const bool os = (pdgId[i] * pdgId[j] < 0);
            const bool sf = (std::abs(pdgId[i]) == std::abs(pdgId[j]));

            if (!(os && sf)) continue;

            const float mll = (leptons[i] + leptons[j]).M();

            if (mll >= 76.f && mll <= 106.f) {
                ++nZPairs;
            }
        }
    }

    // return nZPairs == 1;
    return nZPairs >= 1;
}

ZPairCounts countOSSFZPairs(const FourVectorRVec& leptons, const ints& pdgId)
{
    const size_t n = leptons.size();

    // Step 1: collect all qualifying (i, j) pairs
    std::vector<std::pair<size_t, size_t>> pairs;
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            const bool os = (pdgId[i] * pdgId[j] < 0);
            const bool sf = (std::abs(pdgId[i]) == std::abs(pdgId[j]));
            if (!(os && sf)) continue;

            const float mll = (leptons[i] + leptons[j]).M();
            if (mll >= 76.f && mll <= 106.f) {
                pairs.emplace_back(i, j);
            }
        }
    }

    const int nPairs = static_cast<int>(pairs.size());

    // Step 2: find the maximum number of non-overlapping pairs
    // (simple greedy/brute-force works fine since lepton counts are small, e.g. <= 4-6)
    int nDistinctPairs = 0;
    const size_t nCand = pairs.size();
    // brute force over subsets is fine for small nCand; otherwise use a matching algorithm
    std::function<void(size_t, std::vector<bool>&, int)> recurse =
        [&](size_t idx, std::vector<bool>& used, int count) {
            nDistinctPairs = std::max(nDistinctPairs, count);
            for (size_t k = idx; k < nCand; ++k) {
                auto [a, b] = pairs[k];
                if (used[a] || used[b]) continue;
                used[a] = used[b] = true;
                recurse(k + 1, used, count + 1);
                used[a] = used[b] = false;
            }
        };
    std::vector<bool> used(n, false);
    recurse(0, used, 0);

    return {nPairs, nDistinctPairs};
}




namespace {
constexpr double kZMassLo = 76.0;
constexpr double kZMassHi = 106.0;
constexpr double kZPole   = 91.1876;  // tie-break only, not a cut

inline bool isOSSF(int pdgA, int pdgB) {
    return pdgA + pdgB == 0;
}
}  // namespace

OSSF3LInfo computeOSSF3LInfo(const FourVectorRVec& leptons, const ints& pdgId) {
    OSSF3LInfo result{0, -1, -1, -1.0};

    if (leptons.size() != 3 || pdgId.size() != 3) return result;

    struct Candidate { int i; int j; double mass; };
    Candidate candidates[2];
    int nFound = 0;

    auto tryPair = [&](int i, int j) {
        if (!isOSSF(pdgId[i], pdgId[j])) return;
        candidates[nFound++] = {i, j, (leptons[i] + leptons[j]).M()};
    };

    tryPair(0, 1);
    tryPair(0, 2);
    tryPair(1, 2);

    if (nFound == 0) return result;

    int best = 0;
    if (nFound == 2) {
        double d0 = std::abs(candidates[0].mass - kZPole);
        double d1 = std::abs(candidates[1].mass - kZPole);
        best = (d1 < d0) ? 1 : 0;
    }
    const Candidate& c = candidates[best];

    const bool pairOnZ = (c.mass >= kZMassLo && c.mass <= kZMassHi);
    if (pairOnZ) {
        return OSSF3LInfo{1, c.i, c.j, c.mass};
    }

    const double m3l = (leptons[0] + leptons[1] + leptons[2]).M();
    const bool m3lOnZ = (m3l >= kZMassLo && m3l <= kZMassHi);

    return OSSF3LInfo{m3lOnZ ? 2 : 3, c.i, c.j, c.mass};
}

int getTopLeptonIndex3L(const OSSF3LInfo& info) {
    if (info.category != 1) return -1;
    for (int i = 0; i < 3; ++i) {
        if (i != info.idx1 && i != info.idx2) return i;
    }
    return -1;
}

FourVector selectLeptonByIndex(const FourVectorRVec& leptons, int idx) {
    return idx >= 0 ? leptons[idx] : FourVector{};
}

TLorentzVector TL4VecFromFourVec(const FourVector& v) {
    TLorentzVector tlv;
    tlv.SetPtEtaPhiM(v.Pt(), v.Eta(), v.Phi(), v.M());
    return tlv;
}

int getOSSF3LCategory(const OSSF3LInfo& info) { return info.category; }

double getZBosonMass(const OSSF3LInfo& info) {
    return info.category == 1 ? info.mass : -1.0;
}

double getNonZOSSFMass(const OSSF3LInfo& info) {
    return (info.category == 2 || info.category == 3) ? info.mass : -1.0;
}

double getMassOf3GoodLeptons4BG(const OSSF3LInfo& info, const FourVectorRVec& leptons) {
    if ((info.category == 2 || info.category == 3) && leptons.size() == 3)
        return (leptons[0] + leptons[1] + leptons[2]).M();
    return -1.0;
}

double getM3L(const FourVectorRVec& leptons) {
    if (leptons.size() != 3) return -1.0;
    return (leptons[0] + leptons[1] + leptons[2]).M();
}

bool isMaskCat1OutsideZ3L(const OSSF3LInfo& info, double m3l) {
    return info.category == 1 && std::abs(m3l - 91.1876) > 15.0;
}






namespace {
struct PairDef  { int i, j; };
struct PairEval { bool isOSSF; double mass; };

// The 6 possible pairs among 4 leptons. Complement of pair k is always
// pair (5-k) -- e.g. (0,1) <-> (2,3), (0,2) <-> (1,3), (0,3) <-> (1,2).
// Fixed lookup, not a search: no recursion/backtracking needed at n=4.
constexpr PairDef kPairDefs4L[6] = {
    {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3}
};
}  // namespace



// ---------------------------------------------------------------------
// 4-lepton OSSF categorization. category: 0 = no valid Z pair,
// 1 = one on-Z pair + two leftover leptons, 2 = two non-overlapping
// on-Z pairs (ZZ candidate). idx1/idx2/mass1 describe the best
// ("Z1") pair; idx3/idx4/mass2 describe the other two leptons --
// mass2 is populated whenever category >= 1, regardless of whether
// that pair is itself OSSF or on-Z (it's the leftover pair's mass,
// meaningful even when they don't form a Z).
// ---------------------------------------------------------------------


OSSF4LInfo computeOSSF4LInfo(const FourVectorRVec& leptons, const ints& pdgId) {
    OSSF4LInfo result{0, -1, -1, -1.0, -1, -1, -1.0};

    if (leptons.size() != 4 || pdgId.size() != 4) return result;

    PairEval evals[6];
    for (int k = 0; k < 6; ++k) {
        const auto& p = kPairDefs4L[k];
        evals[k].isOSSF = isOSSF(pdgId[p.i], pdgId[p.j]);
        evals[k].mass = evals[k].isOSSF ? (leptons[p.i] + leptons[p.j]).M() : -1.0;
    }

    // Best pair: OSSF and closest to the Z pole.
    int bestIdx = -1;
    double bestDiff = 1e9;
    for (int k = 0; k < 6; ++k) {
        if (!evals[k].isOSSF) continue;
        double diff = std::abs(evals[k].mass - kZPole);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIdx = k;
        }
    }

    if (bestIdx == -1) return result;  // no OSSF pair at all -> category 0

    const bool bestOnZ = (evals[bestIdx].mass >= kZMassLo && evals[bestIdx].mass <= kZMassHi);
    if (!bestOnZ) return result;  // no fallback at 4L, unlike the 3L m3l case -> category 0

    const auto& bestPair = kPairDefs4L[bestIdx];
    const int complementIdx = 5 - bestIdx;
    const auto& otherPair = kPairDefs4L[complementIdx];

    // Leftover pair's mass is always meaningful (fake-lepton / non-Z-pair
    // diagnostics), so compute it even if that pair isn't itself OSSF.
    const double otherMass = evals[complementIdx].isOSSF
        ? evals[complementIdx].mass
        : (leptons[otherPair.i] + leptons[otherPair.j]).M();

    const bool secondOnZ = evals[complementIdx].isOSSF &&
                           otherMass >= kZMassLo && otherMass <= kZMassHi;

    result.category = secondOnZ ? 2 : 1;
    result.idx1 = bestPair.i;
    result.idx2 = bestPair.j;
    result.mass1 = evals[bestIdx].mass;
    result.idx3 = otherPair.i;
    result.idx4 = otherPair.j;
    result.mass2 = otherMass;
    return result;
}

int getOSSF4LCategory(const OSSF4LInfo& info) { return info.category; }

double getOSSF4LBestZMass(const OSSF4LInfo& info) {
    return info.category >= 1 ? info.mass1 : -1.0;
}

double getOSSF4LSecondZMass(const OSSF4LInfo& info) {
    return info.category == 2 ? info.mass2 : -1.0;
}

int getLeftoverLepton1Index(const OSSF4LInfo& info) {
    return info.category == 1 ? info.idx3 : -1;
}

int getLeftoverLepton2Index(const OSSF4LInfo& info) {
    return info.category == 1 ? info.idx4 : -1;
}

double getLeftoverPairMass(const OSSF4LInfo& info) {
    return info.category == 1 ? info.mass2 : -1.0;
}





// A single branch spec: output field name, the RDF expression to read
// when the region (and optional extra gate) is true, and the default
// value when it's false.
struct RegionBranchSpec {
    std::string field;
    std::string sourceExpr;
    std::string defaultExpr;
    std::string extraGate;  // empty means "no extra condition beyond the region flag"
};

// The full common branch list, in one place. Add/remove a branch by
// editing this list only -- the function below never needs to change.
static const std::vector<RegionBranchSpec>& regionBranchSpecs() {
    static const std::vector<RegionBranchSpec> specs = {
        // ---- leptons (pt-gated) ----
        {"leadingLepton_pt",     "leadingLepton_pt",     "-999.f", "leadingLepton_pt > 0"},
        {"subleadingLepton_pt",  "subleadingLepton_pt",  "-999.f", "subleadingLepton_pt > 0"},
        {"trailingLepton_pt",    "trailingLepton_pt",    "-999.f", "trailingLepton_pt > 0"},
        {"leadingLepton_eta",    "leadingLepton_eta",    "-999.f", "leadingLepton_pt > 0"},
        {"subleadingLepton_eta", "subleadingLepton_eta", "-999.f", "subleadingLepton_pt > 0"},
        {"trailingLepton_eta",   "trailingLepton_eta",   "-999.f", "trailingLepton_pt > 0"},

        // ---- jets & b-jets ----
        {"leadingJet_pt",        "leadingJet_pt",        "-999.f", ""},
        {"subleadingJet_pt",     "subleadingJet_pt",     "-999.f", ""},
        {"leadingbJet_pt",       "leadingbJet_pt",       "-999.f", ""},
        {"subleadingbJet_pt",    "subleadingbJet_pt",    "-999.f", ""},
        {"leadingJet_eta",       "leadingJet_eta",       "-999.f", ""},
        {"subleadingJet_eta",    "subleadingJet_eta",    "-999.f", ""},
        {"leadingbJet_eta",      "leadingbJet_eta",      "-999.f", ""},
        {"subleadingbJet_eta",   "subleadingbJet_eta",   "-999.f", ""},
        {"leadingJet_phi",       "leadingJet_phi",       "-999.f", ""},
        {"subleadingJet_phi",    "subleadingJet_phi",    "-999.f", ""},
        {"leadingbJet_phi",      "leadingbJet_phi",      "-999.f", ""},
        {"subleadingbJet_phi",   "subleadingbJet_phi",   "-999.f", ""},

        // ---- counts ----
        {"nJets",  "int(Selected_jetpt.size())",  "-1", ""},
        {"nbJets", "int(Selected_bjetpt.size())", "-1", ""},
        {"muon_multiplicity", "sum_goodLepton_flavor", "-1", ""},

        // ---- MET ----
        {"goodMET_pt",   "goodMET_pt",   "std::numeric_limits<float>::quiet_NaN()", ""},
        {"goodMET_phi",  "goodMET_phi",  "std::numeric_limits<float>::quiet_NaN()", ""},
        {"PuppiMET_pt",  "PuppiMET_pt",  "std::numeric_limits<float>::quiet_NaN()", ""},
        {"PuppiMET_phi", "PuppiMET_phi", "std::numeric_limits<float>::quiet_NaN()", ""},
    };
    return specs;
}

ROOT::RDF::RNode defineRegionObjectBranches(ROOT::RDF::RNode df,
                                             const std::string& regionCol,
                                             const std::string& outPrefix) {
    for (const auto& spec : regionBranchSpecs()) {
        const std::string cond = spec.extraGate.empty()
            ? regionCol
            : (regionCol + " && " + spec.extraGate);
        const std::string expr = cond + " ? " + spec.sourceExpr + " : " + spec.defaultExpr;
        df = df.Define(outPrefix + "_" + spec.field, expr);
    }
    return df;
}




// =====================================================================
// utility_ff_additions.cpp
//
// Append the contents of this file into utility.cpp (anywhere after
// the includes, e.g. right after countOSSFZPairs()).
// =====================================================================

ClosestJetInfo nearbyJetInfo(const floats &lepPt, const floats &lepEta, const floats &lepPhi,
                              const shorts &jetIdx, const floats &jetPt,
                              const floats &jetEta, const floats &jetPhi)
{
    ClosestJetInfo out;
    out.dR.reserve(lepPt.size());
    out.jetPt.reserve(lepPt.size());

    for (size_t i = 0; i < lepPt.size(); i++)
    {
        int idx = jetIdx[i];
        if (idx < 0 || idx >= (int)jetPt.size())
        {
            out.dR.push_back(999.f);
            out.jetPt.push_back(-1.f);
            continue;
        }
        float dphi = std::abs(lepPhi[i] - jetPhi[idx]);
        if (dphi > M_PI) dphi = 2 * M_PI - dphi;
        float deta = lepEta[i] - jetEta[idx];
        out.dR.push_back(std::sqrt(deta * deta + dphi * dphi));
        out.jetPt.push_back(jetPt[idx]);
    }
    return out;
}

floats jetDeepFlavBByIdx(const ints &jetIdx, const floats &jetDeepFlavB)
{
    floats out;
    out.reserve(jetIdx.size());
    for (auto idx : jetIdx)
    {
        out.push_back((idx >= 0 && idx < (int)jetDeepFlavB.size()) ? jetDeepFlavB[idx] : -1.f);
    }
    return out;
}

floats jetPtRatioByIdx(const floats &lepPt, const ints &jetIdx, const floats &jetPt)
{
    floats out;
    out.reserve(lepPt.size());
    for (size_t i = 0; i < lepPt.size(); i++)
    {
        int idx = jetIdx[i];
        if (idx < 0 || idx >= (int)jetPt.size() || jetPt[idx] <= 0.f)
        {
            out.push_back(1.f);
        }
        else
        {
            out.push_back(lepPt[i] / jetPt[idx]);
        }
    }
    return out;
}

floats coneCorrectedPt(const floats &lepPt,
                        const ints &isTight,
                        const floats &pfRelIso,
                        const floats &closestJetDR,
                        const floats &closestJetPt,
                        float x,
                        float dRmatch)
{
    floats out;
    out.reserve(lepPt.size());

    for (size_t i = 0; i < lepPt.size(); i++)
    {
        float ptcone;
        if (isTight[i])
        {
            ptcone = lepPt[i];
        }
        else if (closestJetDR[i] < dRmatch && closestJetPt[i] > 0.f)
        {
            ptcone = x * closestJetPt[i];
        }
        else
        {
            ptcone = x * lepPt[i] * (1.f + pfRelIso[i]);
        }
        out.push_back(ptcone);
    }
    return out;
}

FFTriggerResult evaluateFFTrigger(const std::vector<bool>  &hltFired,
                                   const std::vector<float> &lepPtMinTab,
                                   const std::vector<float> &jetPtMinTab,
                                   const std::vector<float> &coneLoTab,
                                   const std::vector<float> &coneHiTab,
                                   const std::vector<float> &prescaleTab,
                                   float lepPt,
                                   float jetPt,
                                   float lepPtCone,
                                   bool  isMC)
{
    FFTriggerResult result{false, 1.0};

    double prodTerm = 1.0; // running product for Eq. 29
    bool   anyFired = false;

    size_t n = hltFired.size();
    for (size_t i = 0; i < n; i++)
    {
        if (!hltFired[i]) continue;
        if (lepPt     < lepPtMinTab[i]) continue;
        if (jetPt     < jetPtMinTab[i]) continue;
        if (lepPtCone < coneLoTab[i] || lepPtCone > coneHiTab[i]) continue;

        anyFired = true;
        if (isMC && prescaleTab[i] > 0.f)
        {
            prodTerm *= (1.0 - 1.0 / prescaleTab[i]);
        }
    }

    result.passed = anyFired;
    if (anyFired && isMC)
    {
        result.weight = 1.0 - prodTerm;
    }
    return result;
}
floats nearbyJetPtRatio(const floats& relIso)
{
    floats out;
    out.reserve(relIso.size());

    for (float x : relIso)
        out.push_back(x <= -1.f ? 1.f : 1.f / (1.f + x));

    return out;
}

namespace LeptonTruth {
namespace {
Result Finish(Result r) {
    r.isPrompt = r.truthClass == int(TruthClass::Prompt);
    r.isNonPrompt = r.truthClass == int(TruthClass::NonPrompt);
    return r;
}
}
/**
 * @brief Classify one NanoAOD-associated reconstructed electron or muon (MC only).
 *
 * Uses original genPartIdx/genPartFlav, never geometric matching. Walks the
 * GenPart mother chain with bounds checks and a visited set; signed same-PDG
 * electron, muon, tau and boson copies are collapsed for path interpretation.
 * Traversal is bounded by the GenPart count. Original associations are unchanged.
 *
 * TruthClass integer values: 0 Unresolved, 1 Prompt, 2 NonPrompt,
 * 3 PhotonAssociated, 4 PromptOtherOrUnresolvedOrigin. PromptOrigin values:
 * 0 NotApplicable, 1 DirectW, 2 DirectZ, 3 DirectGammaStar, 4 PromptTauFromW,
 * 5 PromptTauFromZ, 6 PromptTauFromGammaStar, 7 PromptOtherOrUnresolvedOrigin.
 *
 * Flav 1 requires lepton copies followed immediately by W/Z/gamma* copies;
 * flav 15 requires lepton copies -> same-sign tau copies -> W/Z/gamma* copies.
 * NanoAOD flav 15 supplies prompt-tau evidence; a tau ancestor alone does not.
 * W decay charge must be consistent. W/Z require no top or on-shell mass.
 * A gamma* requires PDG 22 and direct opposite-sign same-flavor daughters
 * after photon-copy collapse. Preserve the established hard-photon route:
 * a non-final copy with finite positive stored mass plus hard-process bit 7/8.
 * Also accept a non-final isPrompt photon copy with isPrompt daughters of both
 * charges, supported by NanoAOD flav 1 (or flav 15 for prompt-tau daughters).
 * This second route recognizes secondary prompt QED pair splitting without
 * requiring a reliable stored photon mass or a hard-process photon. Two direct
 * massive lepton daughters establish pair production; their four-momenta are
 * independent validation evidence, not an added selection or mass threshold.
 * Incomplete pair/prompt evidence remains category 4. Flav 22 reconstructed
 * electrons still return PhotonAssociated before either photon route.
 * DirectGammaStar and isFromGammaStar describe the nearest accepted photon
 * mediation, whether hard or secondary; neither identifies associated hard
 * production or separates Z/gamma interference. The flag assignment is unchanged.
 * A hadron, heavy quark, photon outside this gamma* path, or another intervening
 * species blocks a direct prompt interpretation. Distant W/Z/top never overrides
 * nonprompt evidence. TopW requires a top immediately above the W copy chain.
 *
 * Flav 4/5 is NonPrompt without separate heavy-flavor output categories. Flav 3
 * remains Unresolved (light OR unknown); no extra nonprompt inference is made.
 * Electron flav 22 is PhotonAssociated, never direct gamma* by this classifier.
 * Flav 0 is Unresolved. Matched electron photons are allowed for flav 22/3/4/5;
 * an electron-flav-1 photon match cannot establish a prompt lepton decay path.
 * Charge flips at reconstruction are allowed: matching checks absolute flavor.
 *
 * Missing indices retain category 4 for flav 1/15, class 2 for flav 4/5 and class
 * 3 for electron flav 22, with diagnostics. Invalid positive/negative indices,
 * incompatible flavors, cycles, array mismatches or contradictory evidence give
 * class 0. A normal -1 mother terminates ancestry; absent accepted mediator gives
 * category 4 with IncompleteAncestry. Both isPrompt/isNonPrompt are zero for 0/3/4.
 *
 * originStatus bit positions (OR-combinable, zero means no issue):
 * 0 NoGenIndex; 1 InvalidGenIndex; 2 ArrayLengthMismatch; 3 InvalidMotherIndex;
 * 4 AncestryCycle; 5 IncompleteAncestry; 6 IncompatibleFlavor;
 * 7 ContradictoryEvidence; 8 UnsupportedFlav; 9 NoAcceptedMediator;
 * 10 UnestablishedGammaStar; 11 InterveningParticle; 12 TauEvidenceMismatch.
 *
 * Known limits: NanoAOD has one pruned mother pointer; lost parent/daughter or
 * prompt/path information cannot be invented. Stored zero photon mass alone
 * does not exclude virtual pair splitting. No ancestry-only override of
 * NanoAOD flav is attempted. isFromZ is ancestry, not associated-production Z.
 * No separate bottom/charm/light classification branches are produced.
 */
Result ClassifyRecoLeptonOrigin(int recoPdgId, int genPartIdx,
    unsigned char genPartFlav, const ints& pdg, const shorts& mother,
    const ushorts& flags, const ints& status, const floats& mass) {
    Result r;
    const int flavor = std::abs(recoPdgId);
    const bool candidate = genPartFlav == 1 || genPartFlav == 15;
    if (candidate) { r.truthClass=4; r.promptOrigin=7; }
    else if (genPartFlav == 4 || genPartFlav == 5) r.truthClass=2;
    else if (genPartFlav == 22 && flavor == 11) {
        r.truthClass=3; r.isPhotonAssociated=1;
    } else if (genPartFlav != 0 && genPartFlav != 3) r.originStatus |= UnsupportedFlav;
    auto invalid = [&](unsigned int diagnostic) {
        r.truthClass=0; r.promptOrigin=0;
        r.isFromPromptTau=r.isFromTopW=r.isFromZ=r.isFromGammaStar=0;
        r.originStatus |= diagnostic;
        return Finish(r);
    };
    if (flavor != 11 && flavor != 13) return invalid(IncompatibleFlavor);
    const size_t n=pdg.size();
    if (mother.size()!=n || flags.size()!=n || status.size()!=n || mass.size()!=n)
        return invalid(ArrayLengthMismatch);
    if (genPartIdx == -1) {
        r.originStatus |= NoGenIndex;
        if (candidate) r.originStatus |= IncompleteAncestry | NoAcceptedMediator;
        return Finish(r);
    }
    if (genPartIdx < -1 || size_t(genPartIdx)>=n) return invalid(InvalidGenIndex);
    const int matched=std::abs(pdg[genPartIdx]);
    const bool photonMatch=flavor==11 && matched==22;
    if (photonMatch) r.isPhotonAssociated=1;
    if (matched!=flavor && !(photonMatch && (genPartFlav==22 || genPartFlav==3 || genPartFlav==4 || genPartFlav==5)))
        return invalid(IncompatibleFlavor | (candidate ? ContradictoryEvidence : 0u));
    // Validate the complete reachable chain, including copies above the mediator.
    std::vector<int> chain;
    std::set<int> visited;
    int cur=genPartIdx;
    while (cur!=-1) {
        if (cur<0 || size_t(cur)>=n) return invalid(InvalidMotherIndex);
        if (!visited.insert(cur).second || chain.size()>=n) return invalid(AncestryCycle);
        chain.push_back(cur);
        cur=mother[cur];
    }
    if (!candidate) {
        // Flav 0/3 never becomes prompt merely because a boson was encountered.
        return Finish(r);
    }
    size_t k=0;
    const int leptonId=pdg[chain[0]];
    while (k<chain.size() && pdg[chain[k]]==leptonId) ++k;
    bool tau=false;
    int decayId=leptonId;
    if (k<chain.size() && std::abs(pdg[chain[k]])==15) {
        tau=true; decayId=pdg[chain[k]];
        if ((decayId>0)!=(leptonId>0)) return invalid(ContradictoryEvidence);
        while (k<chain.size() && pdg[chain[k]]==decayId) ++k;
    }
    if ((genPartFlav==15)!=tau) {
        r.originStatus |= TauEvidenceMismatch;
        if (tau) return invalid(ContradictoryEvidence);
        // NanoAOD supplies prompt-tau evidence, but a pruned/lost tau link
        // cannot establish one of the accepted paths: retain category 4.
        r.originStatus |= IncompleteAncestry | NoAcceptedMediator;
        return Finish(r);
    }
    if (k==chain.size()) {
        r.originStatus |= IncompleteAncestry | NoAcceptedMediator;
        return Finish(r);
    }
    const int bosonIdx=chain[k], bosonId=pdg[bosonIdx], boson=std::abs(bosonId);
    if (boson!=24 && boson!=23 && boson!=22) {
        r.originStatus |= InterveningParticle | NoAcceptedMediator;
        // A known hadronic path contradicts flav 1/15. Quark/radiation or
        // missing-boson records otherwise remain the separate category 4.
        if (boson>100) return invalid(ContradictoryEvidence);
        return Finish(r);
    }
    size_t above=k;
    while (above<chain.size() && pdg[chain[above]]==bosonId) ++above;
    if (boson==24 && (bosonId>0)==(decayId>0)) return invalid(ContradictoryEvidence);
    if (boson==22) {
        bool virtualPhoton=false, hard=false, plus=false, minus=false;
        bool promptPhoton=false, promptPlus=false, promptMinus=false;
        std::set<int> photonCopies;
        for (size_t j=k; j<above; ++j) {
            int b=chain[j]; photonCopies.insert(b);
            virtualPhoton |= std::isfinite(mass[b]) && mass[b]>0 && status[b]!=1;
            hard |= (flags[b] & ((1u<<7)|(1u<<8))) != 0;
            promptPhoton |= status[b]!=1 && (flags[b] & 1u)!=0;
        }
        // Direct daughters of any copy must be the decaying lepton or tau
        // flavor, with both charges. No hadron/photon barrier is skipped.
        for (size_t j=0; j<n; ++j) if (photonCopies.count(mother[j])) {
            plus |= pdg[j]==std::abs(decayId);
            minus |= pdg[j]==-std::abs(decayId);
            promptPlus |= pdg[j]==std::abs(decayId) && (flags[j] & 1u)!=0;
            promptMinus |= pdg[j]==-std::abs(decayId) && (flags[j] & 1u)!=0;
        }
        const bool hardPair=virtualPhoton && hard;
        const bool promptPair=promptPhoton && promptPlus && promptMinus;
        if (!plus || !minus || !(hardPair || promptPair)) {
            r.originStatus |= UnestablishedGammaStar | NoAcceptedMediator;
            return Finish(r);
        }
    }
    // Hadron ancestry above a claimed prompt mediator is contradictory.
    // Incoming heavy partons are allowed above an established hard mediator.
    for (size_t j=above; j<chain.size(); ++j) {
        const int a=std::abs(pdg[chain[j]]);
        if (a>100) return invalid(ContradictoryEvidence);
    }
    r.truthClass=1;
    r.promptOrigin=(boson==24 ? 1 : boson==23 ? 2 : 3)+(tau ? 3 : 0);
    r.isFromPromptTau=tau;
    r.isFromZ=boson==23;
    r.isFromGammaStar=boson==22;
    r.isFromTopW=boson==24 && above<chain.size()
        && std::abs(pdg[chain[above]])==6
        && ((pdg[chain[above]]>0)==(bosonId>0));
    return Finish(r);
}

/**
 * @brief MC-only batch wrapper preserving the final goodLepton index order.
 *
 * Input recoPdgId, genPartIdx and genPartFlav must already use the same masks,
 * electron-muon overlap removal, merging and pT sorting as goodLepton_pt,
 * goodLepton_isTight and goodLepton_isLNT. Unequal input lengths throw; no sort,
 * filtering, truncation or rematching is performed here. Exactly one result per
 * selected lepton is returned, even for unmatched or unresolved objects.
 *
 * BaseAnalyser projects these fields to Gen_goodLepton_{truthClass,promptOrigin,
 * originStatus,isPrompt,isNonPrompt,isFromPromptTau,isFromTopW,isFromZ,
 * isFromGammaStar,isPhotonAssociated}. truthClass and promptOrigin use the exact
 * enum integers documented on ClassifyRecoLeptonOrigin; originStatus uses its
 * documented bits 0--12. Other outputs are 0/1 ints: confirmed class 1, confirmed
 * class 2, accepted prompt tau path, prompt top-W path, prompt Z path, accepted
 * prompt gamma* path, and photon association respectively. Class 4 has both
 * isPrompt and isNonPrompt zero. Tau/Z/gamma/top flags require confirmed prompt.
 * The transient Columns object is not saved; only RVec<int> field projections
 * and RVec<unsigned int> originStatus are stored by Gen_goodLepton_.*.
 * See the scalar heading for physics definitions, conservative handling of
 * NanoAOD flav 0/3/22, status bits, copy handling and pruning limitations.
 */
Columns ClassifyRecoLeptonOrigins(const ints& recoPdgId,
    const shorts& genPartIdx, const uchars& genPartFlav, const ints& pdg,
    const shorts& mother, const ushorts& flags, const ints& status,
    const floats& mass) {
    if (recoPdgId.size()!=genPartIdx.size() || recoPdgId.size()!=genPartFlav.size())
        throw std::invalid_argument("LeptonTruth: mismatched selected lepton vector lengths");
    Columns c;
    for (size_t i=0; i<recoPdgId.size(); ++i) {
        const auto r=ClassifyRecoLeptonOrigin(recoPdgId[i],genPartIdx[i],genPartFlav[i],
                                            pdg,mother,flags,status,mass);
#define APPEND(field) c.field.push_back(r.field)
        APPEND(truthClass); APPEND(promptOrigin); APPEND(originStatus);
        APPEND(isPrompt); APPEND(isNonPrompt); APPEND(isFromPromptTau);
        APPEND(isFromTopW); APPEND(isFromZ); APPEND(isFromGammaStar);
        APPEND(isPhotonAssociated);
#undef APPEND
    }
    return c;
}
} // namespace LeptonTruth
