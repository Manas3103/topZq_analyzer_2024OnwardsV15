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
// ints GetLeptonOrigin(
//     const shorts& Lepton_genPartIdx,
//     const ints& GenPart_pdgId,
//     const shorts& GenPart_genPartIdxMother)
// {
//     ints origin(Lepton_genPartIdx.size(), 0);

//     for (size_t i = 0; i < Lepton_genPartIdx.size(); ++i) {

//         int idx = Lepton_genPartIdx[i];

//         while (idx >= 0 && idx < static_cast<int>(GenPart_pdgId.size())) {

//             int pdg = std::abs(GenPart_pdgId[idx]);

//             if (pdg == 15) {
//                 origin[i] = 1;   // tau
//                 break;
//             }

//             if (pdg == 23) {
//                 origin[i] = 2;   // Z
//                 break;
//             }

//             if (pdg == 24) {
//                 origin[i] = 3;   // W
//                 break;
//             }

//             idx = GenPart_genPartIdxMother[idx];
//         }
//     }

//     return origin;
// }

// #include <set>


bool HasBit(int flags, int bit)
{
    return (flags & (1 << bit)) != 0;
}

constexpr int BIT_FROM_HARD_PROCESS = 8;
constexpr int BIT_IS_LAST_COPY = 13;


ints GetLeptonOrigin(
    const shorts& Lepton_genPartIdx,
    const ints& GenPart_pdgId,
    const ushorts& GenPart_statusFlags,
    const shorts& GenPart_genPartIdxMother)
{
    // Origin codes:
    //
    //   0 = No identified W/Z/tau origin
    //
    //   1 = Lepton from tau, where tau came from W
    //   2 = Lepton from tau, where tau came from Z
    //   3 = Lepton from tau, but tau did not come from W or Z
    //
    //   4 = Direct W -> lepton, where W came from top
    //   5 = Direct Z -> lepton
    //   6 = Direct W -> lepton, but W did NOT come from top

    ints origin(Lepton_genPartIdx.size(), 0);

    // Loop over all leptons and trace their generator-particle ancestry.
    for (size_t i = 0; i < Lepton_genPartIdx.size(); ++i) {

        int idx = Lepton_genPartIdx[i];

        bool foundTau = false;
        int tauIdx = -1;

        // ------------------------------------------------------------
        // Step 1:
        // Walk upward from the lepton through the mother chain.
        //
        // We first look for a tau. If no tau is encountered, we look
        // for a W or Z directly associated with the lepton ancestry.
        // ------------------------------------------------------------
        while (idx >= 0 &&
               idx < static_cast<int>(GenPart_pdgId.size())) {

            int pdg = std::abs(GenPart_pdgId[idx]);
            unsigned short flags = GenPart_statusFlags[idx];

            bool isLastCopy = HasBit(flags, BIT_IS_LAST_COPY);
            bool isHardProc = HasBit(flags, BIT_FROM_HARD_PROCESS);

            // --------------------------------------------------------
            // A tau was found in the lepton ancestry.
            //
            // Save the tau index and handle its origin separately.
            // --------------------------------------------------------
            if (pdg == 15 && isLastCopy) {
                foundTau = true;
                tauIdx = idx;
                break;
            }

            // --------------------------------------------------------
            // A W was found in the lepton ancestry.
            //
            // We now need to determine whether this W came from
            // a top quark:
            //
            //        top -> W -> lepton
            //
            // or whether the W came from some other source.
            // --------------------------------------------------------
            if (pdg == 24 && isLastCopy && isHardProc) {

                int wMother = GenPart_genPartIdxMother[idx];
                bool wFromTop = false;

                // Follow the W mother chain and look for a top quark.
                while (wMother >= 0 &&
                       wMother < static_cast<int>(GenPart_pdgId.size())) {

                    int motherPdg = std::abs(GenPart_pdgId[wMother]);

                    unsigned short motherFlags =
                        GenPart_statusFlags[wMother];

                    bool motherIsLastCopy =
                        HasBit(motherFlags, BIT_IS_LAST_COPY);

                    // Top quark found in the W ancestry.
                    if (motherPdg == 6 && motherIsLastCopy) {
                        wFromTop = true;
                        break;
                    }

                    // Continue following the W mother's ancestry.
                    wMother =
                        GenPart_genPartIdxMother[wMother];
                }

                // Distinguish W from top from W produced elsewhere.
                if (wFromTop) {
                    origin[i] = 4;   // top -> W -> lepton
                }
                else {
                    origin[i] = 6;   // W -> lepton, not from top
                }

                break;
            }

            // --------------------------------------------------------
            // A Z boson was found in the lepton ancestry.
            // --------------------------------------------------------
            if (pdg == 23 && isLastCopy && isHardProc) {
                origin[i] = 5;       // Z -> lepton
                break;
            }

            // Move to the next particle in the mother chain.
            idx = GenPart_genPartIdxMother[idx];
        }

        // ------------------------------------------------------------
        // Step 2:
        // If the lepton came from a tau, determine the origin of
        // that tau by following the tau's mother chain.
        // ------------------------------------------------------------
        if (foundTau) {

            int mother = GenPart_genPartIdxMother[tauIdx];

            bool foundTauOrigin = false;

            // Follow the tau's ancestry looking for a hard-process
            // W or Z boson.
            while (mother >= 0 &&
                   mother < static_cast<int>(GenPart_pdgId.size())) {

                int pdg = std::abs(GenPart_pdgId[mother]);

                unsigned short flags =
                    GenPart_statusFlags[mother];

                bool isLastCopy =
                    HasBit(flags, BIT_IS_LAST_COPY);

                bool isHardProc =
                    HasBit(flags, BIT_FROM_HARD_PROCESS);

                // ----------------------------------------------------
                // Tau came from a hard-process W.
                //
                // This corresponds to:
                //
                //       W -> tau -> lepton
                // ----------------------------------------------------
                if (pdg == 24 && isLastCopy && isHardProc) {
                    origin[i] = 1;
                    foundTauOrigin = true;
                    break;
                }

                // ----------------------------------------------------
                // Tau came from a hard-process Z.
                //
                // This corresponds to:
                //
                //       Z -> tau -> lepton
                // ----------------------------------------------------
                if (pdg == 23 && isLastCopy && isHardProc) {
                    origin[i] = 2;
                    foundTauOrigin = true;
                    break;
                }

                // Continue following the tau's mother chain.
                mother = GenPart_genPartIdxMother[mother];
            }

            // --------------------------------------------------------
            // Tau was found, but no hard-process W or Z was found
            // in its ancestry.
            // --------------------------------------------------------
            if (!foundTauOrigin) {
                origin[i] = 3;
            }
        }
    }

    return origin;
}



int CountUniqueOrigin3(
    const shorts& genIdx,
    const ints& origin)
{
    std::set<int> unique;

    for (size_t i = 0; i < genIdx.size(); ++i) {

        if (origin[i] != 3)
            continue;

        if (genIdx[i] < 0)
            continue;

        unique.insert(genIdx[i]);
    }

    return unique.size();
}

inline float DeltaPhi(float phi1, float phi2) {
    float dphi = phi1 - phi2;
    while (dphi >  M_PI) dphi -= 2 * M_PI;
    while (dphi < -M_PI) dphi += 2 * M_PI;
    return dphi;
}

inline float DeltaR(float eta1, float phi1, float eta2, float phi2) {
    float deta = eta1 - eta2;
    float dphi = DeltaPhi(phi1, phi2);
    return std::sqrt(deta * deta + dphi * dphi);
}

struct RecoMuonMatch {
    int   genIdx  = -1;    // matched gen muon index, -1 if none
    float dR      = -1.f;
    bool  fromW   = false; // mother chain (skipping W copies) starts with a W
    bool  fromTop = false; // and above that W sits a top
};

// ------------------------------------------------------------
// Walk up the mother chain starting from `startIdx`, skipping
// over any run of W copies (|pdgId| == 24), and report:
//   - whether a W was found at all in the chain
//   - whether a top sits directly above that W run
// ------------------------------------------------------------
inline void WalkUpThroughW(
    int startIdx,
    const ROOT::VecOps::RVec<int>& pdg,
    const ROOT::VecOps::RVec<short>& mother,
    bool& sawW, bool& topAbove)
{
    sawW = false;
    topAbove = false;

    int cur = mother[startIdx];
    while (cur >= 0) {
        int apdg = std::abs(pdg[cur]);
        if (apdg == 24) {
            sawW = true;
            cur = mother[cur];   // keep climbing through W copies
            continue;
        }
        if (sawW) topAbove = (apdg == 6);
        break;
    }
}

// ------------------------------------------------------------
// Single-muon version (kept for reuse / testing individual muons)
// ------------------------------------------------------------
RecoMuonMatch MatchRecoMuonToTopChain(
    float recoPt, float recoEta, float recoPhi,
    const ROOT::VecOps::RVec<int>& pdg,
    const ROOT::VecOps::RVec<short>& mother,
    const ROOT::VecOps::RVec<float>& genEta,
    const ROOT::VecOps::RVec<float>& genPhi,
    float maxDR = 0.3f)
{
    RecoMuonMatch out;

    int bestIdx = -1;
    float bestDR = maxDR;
    for (int j = 0; j < (int)pdg.size(); ++j) {
        if (std::abs(pdg[j]) != 13) continue;
        float dr = DeltaR(recoEta, recoPhi, genEta[j], genPhi[j]);
        if (dr < bestDR) { bestDR = dr; bestIdx = j; }
    }
    if (bestIdx < 0) return out;

    out.genIdx = bestIdx;
    out.dR = bestDR;

    bool sawW = false, topAbove = false;
    WalkUpThroughW(bestIdx, pdg, mother, sawW, topAbove);

    out.fromW   = sawW;
    out.fromTop = sawW && topAbove;

    return out;
}

// ------------------------------------------------------------
// VECTORIZED: loops over every reconstructed muon in the event
// automatically. Pass the full Muon_* and GenPart_* branches;
// get back one RVec<int> (0/1 flag) per muon, in the same order
// as the input Muon collection.
//
// Also provides an "all fields" variant returning genIdx/dR/fromW
// for anyone who wants more than just the final flag.
// ------------------------------------------------------------
ROOT::RVec<int> Muon_FromTopW(
    const ROOT::VecOps::RVec<float>& muPt,
    const ROOT::VecOps::RVec<float>& muEta,
    const ROOT::VecOps::RVec<float>& muPhi,
    const ROOT::VecOps::RVec<int>&   pdg,
    const ROOT::VecOps::RVec<short>& mother,
    const ROOT::VecOps::RVec<float>& genEta,
    const ROOT::VecOps::RVec<float>& genPhi,
    float maxDR = 0.3f)
{
    ROOT::RVec<int> out(muPt.size(), 0);
    for (size_t i = 0; i < muPt.size(); ++i) {
        auto m = MatchRecoMuonToTopChain(muPt[i], muEta[i], muPhi[i],
                                          pdg, mother, genEta, genPhi, maxDR);
        out[i] = m.fromTop ? 1 : 0;
    }
    return out;
}

// Optional: full diagnostic output per muon (genIdx, dR, fromW, fromTop)
// packed into parallel RVecs, useful for debugging / cutflow studies.
struct MuonMatchArrays {
    ROOT::RVec<int>   genIdx;
    ROOT::RVec<float> dR;
    ROOT::RVec<int>   fromW;
    ROOT::RVec<int>   fromTop;
};

MuonMatchArrays Muon_TopWMatchInfo(
    const ROOT::VecOps::RVec<float>& muPt,
    const ROOT::VecOps::RVec<float>& muEta,
    const ROOT::VecOps::RVec<float>& muPhi,
    const ROOT::VecOps::RVec<int>&   pdg,
    const ROOT::VecOps::RVec<short>& mother,
    const ROOT::VecOps::RVec<float>& genEta,
    const ROOT::VecOps::RVec<float>& genPhi,
    float maxDR = 0.3f)
{
    MuonMatchArrays out;
    size_t n = muPt.size();
    out.genIdx.resize(n, -1);
    out.dR.resize(n, -1.f);
    out.fromW.resize(n, 0);
    out.fromTop.resize(n, 0);

    for (size_t i = 0; i < n; ++i) {
        auto m = MatchRecoMuonToTopChain(muPt[i], muEta[i], muPhi[i],
                                          pdg, mother, genEta, genPhi, maxDR);
        out.genIdx[i]  = m.genIdx;
        out.dR[i]      = m.dR;
        out.fromW[i]   = m.fromW ? 1 : 0;
        out.fromTop[i] = m.fromTop ? 1 : 0;
    }
    return out;
}

// ------------------------------------------------------------
// TRUTH-LEVEL ONLY: check whether a given GenPart muon's mother
// chain is W and that W's ancestor (skipping W copies) is a top.
// No reco objects / dR matching involved anywhere here.
// ------------------------------------------------------------

// Single-index check: given one GenPart index (already known to be
// a muon, |pdgId|==13), classify its ancestry.
// Returns true only if: muon -> (chain of W copies) -> top
bool IsGenMuonFromTopW(
    int muIdx,
    const ROOT::VecOps::RVec<int>&   pdg,
    const ROOT::VecOps::RVec<short>& mother)
{
    if (muIdx < 0 || muIdx >= (int)pdg.size()) return false;
    if (std::abs(pdg[muIdx]) != 13) return false; // not a muon

    bool sawW = false;
    int cur = mother[muIdx];
    while (cur >= 0) {
        int apdg = std::abs(pdg[cur]);
        if (apdg == 24) {
            sawW = true;
            cur = mother[cur];   // climb through W copies
            continue;
        }
        // first non-W ancestor reached
        return sawW && (apdg == 6);
    }
    return false; // reached top of record (mother == -1) without a top
}

// ------------------------------------------------------------
// VECTORIZED: scan the full GenPart collection of an event and
// return a 0/1 flag PER GenPart ENTRY (same length as GenPart_pdgId),
// flagging every gen muon whose ancestry is W -> top.
// Non-muon entries are always 0.
// ------------------------------------------------------------
ROOT::RVec<int> GenPart_MuonFromTopW(
    const ROOT::VecOps::RVec<int>&   pdg,
    const ROOT::VecOps::RVec<short>& mother)
{
    ROOT::RVec<int> out(pdg.size(), 0);
    for (int i = 0; i < (int)pdg.size(); ++i) {
        if (std::abs(pdg[i]) != 13) continue; // only tag muons
        out[i] = IsGenMuonFromTopW(i, pdg, mother) ? 1 : 0;
    }
    return out;
}

// ------------------------------------------------------------
// CONVENIENCE: does THIS EVENT contain at least one gen muon
// coming from a top-quark W decay? Useful as an event-level flag
// or for a Filter().
// ------------------------------------------------------------
bool Event_HasGenMuonFromTopW(
    const ROOT::VecOps::RVec<int>&   pdg,
    const ROOT::VecOps::RVec<short>& mother)
{
    for (int i = 0; i < (int)pdg.size(); ++i) {
        if (std::abs(pdg[i]) != 13) continue;
        if (IsGenMuonFromTopW(i, pdg, mother)) return true;
    }
    return false;
}

// ------------------------------------------------------------
// Cross-check: does GetLeptonOrigin's code==4 (top->W->lepton)
// agree with GenPart_muFromTopW at the lepton's matched GenPart index?
//
// Returns a per-lepton comparison code (same length as Lepton_genPartIdx):
//    1  = both agree the lepton IS from top->W
//    0  = both agree the lepton is NOT from top->W
//   -1  = disagreement: origin==4 but GenPart flag says 0
//   -2  = disagreement: origin!=4 but GenPart flag says 1
//   -9  = lepton has no valid gen match (genPartIdx < 0), can't compare
// ------------------------------------------------------------
ints CompareLeptonOriginToGenPartFlag(
    const shorts& Lepton_genPartIdx,
    const ints&   origin,              // output of GetLeptonOrigin
    const ints&   GenPart_muFromTopW)  // output of GenPart_MuonFromTopW
{
    ints cmp(Lepton_genPartIdx.size(), -9);

    for (size_t i = 0; i < Lepton_genPartIdx.size(); ++i) {
        int gi = Lepton_genPartIdx[i];
        if (gi < 0 || gi >= (int)GenPart_muFromTopW.size()) {
            cmp[i] = -9; // no valid gen match, skip
            continue;
        }

        bool originSaysTopW  = (origin[i] == 4);
        bool genPartSaysTopW = (GenPart_muFromTopW[gi] == 1);

        if (originSaysTopW && genPartSaysTopW)        cmp[i] = 1;
        else if (!originSaysTopW && !genPartSaysTopW)  cmp[i] = 0;
        else if (originSaysTopW && !genPartSaysTopW)   cmp[i] = -1; // origin flagged, genpart didn't
        else                                            cmp[i] = -2; // genpart flagged, origin didn't
    }

    return cmp;
}
