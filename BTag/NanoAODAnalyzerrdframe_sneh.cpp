/*
 * NanoAODAnalyzerrdframe.cpp
 *
 *  Created on: Sep 30, 2018
 *      Author: suyong
 *  Updated on: 10Oct, 2023
 *      Author: Arnab PUROHIT, IP2I, Lyon
 * Updated on : 4Oct, 2024
 * 		Author: Sneh Shuchi, NISER, Bhubaneswar
 */

#include "NanoAODAnalyzerrdframe.h"
#include "MuonScaRe.h"
#include <iostream>
#include <algorithm>
#include <typeinfo>
#include <random>
#include <chrono>
#include <utility>
#include <atomic>
#include <tuple>
#include "TCanvas.h"
#include "Math/GenVector/VectorUtil.h"
#include <vector>
#include <array>
#include <fstream>
#include "utility.h"
#include <regex>
#include "ROOT/RDFHelpers.hxx"

using namespace std;

NanoAODAnalyzerrdframe::NanoAODAnalyzerrdframe(TTree *atree, std::string outfilename)
	: _rd(*atree), _jsonOK(false), _outfilename(outfilename), _outrootfile(0), _rlm(_rd), _rnt(&_rlm) // PDFWeights(103, 0.0)
{
	// ROOT::EnableImplicitMT();
	_atree = atree;
	// cout<< " run year=====" << _year <<endl;
	//  if genWeight column exists, then it is not real data
	//
}


NanoAODAnalyzerrdframe::~NanoAODAnalyzerrdframe()
{
	// TODO Auto-generated destructor stub
}


bool NanoAODAnalyzerrdframe::isDefined(string v)
{
	auto result = std::find(_originalvars.begin(), _originalvars.end(), v);
	if (result != _originalvars.end())
		return true;
	else
		return false;
}


void NanoAODAnalyzerrdframe::setTree(TTree *t, std::string outfilename)
{
	_rd = ROOT::RDataFrame(*t);
	_rlm = RNode(_rd);
	_outfilename = outfilename;
	_hist1dinfovector.clear();
	_th1dhistos.clear();
	_varstostore.clear();
	//_hist1dinfovector.clear();
	_hist2dinfovector.clear();
	_th2dhistos.clear();
	_selections.clear();
	_extraHists.clear();

	this->setupAnalysis();
}


void NanoAODAnalyzerrdframe::setupAnalysis()
{
	// Event weight for data it's always one. For MC, it depends on the sign
	// cout<<"year===="<< _year<< "==runtype=== " <<  _runtype <<endl;
	_rlm = _rlm.Define("one", "1.0");
	if (_isData && !isDefined("evWeight"))
	{
		_rlm = _rlm.Define("evWeight", []()
						   { return 1.0; }, {});
	}
	// Store PDF sum of weights
	/*if(!_isData){
			 auto storeWeights = [this](floats weights)->floats {

				 for (unsigned int i=0; i<weights.size(); i++)
					 PDFWeights[i] += weights[i];

				 return PDFWeights;
			 };
			 try {
				 _rlm.Foreach(storeWeights, {"LHEPdfWeight"});
			 } catch (exception& e) {
				 cout << e.what() << endl;
				 cout << "No PDF weight in this root file!" << endl;
			 }
	 }*/

	// setupCuts_and_Hists();
	void setupCuts_and_Hists(std::string jes_unc_level_start_idx);
	setupTree();
}


bool NanoAODAnalyzerrdframe::readgoodjson(string goodjsonfname)
{
	auto isgoodjsonevent = [this](unsigned int runnumber, unsigned int lumisection)
	{
		auto key = std::to_string(runnumber).c_str();

		bool goodeventflag = false;

		if (jsonroot.contains(key))
		{
			for (auto &v : jsonroot[key])
			{
				if (v[0] <= lumisection && lumisection <= v[1])
					goodeventflag = true;
			}
		}
		return goodeventflag;
	};

	if (goodjsonfname != "")
	{
		std::ifstream jsoninfile;
		jsoninfile.open(goodjsonfname);

		if (jsoninfile.good())
		{
			// using rapidjson
			// rapidjson::IStreamWrapper s(jsoninfile);
			// jsonroot.ParseStream(s);

			// using jsoncpp
			jsoninfile >> jsonroot;
			_rlm = _rlm.Define("goodjsonevent", isgoodjsonevent, {"run", "luminosityBlock"}).Filter("goodjsonevent");
			_jsonOK = true;
			return true;
		}
		else
		{
			cout << "Problem reading json file " << goodjsonfname << endl;
			return false;
		}
	}
	else
	{
		cout << "no JSON file given" << endl;
		return true;
	}
}


void NanoAODAnalyzerrdframe::setupJetMETCorrection(const std::string& jercfname,
	const std::string& jerctag,
	const std::vector<std::string>& jercUncTags,
	const std::string& jertag,
	const std::string& fname_metphimod,
	const std::string& jetvetofname,
	const std::string& jetvetotag)
{
	std::cout << "===== SETUP JETMET CORRECTIONS =====" << std::endl;

	// Load JERC JSON
	try {
		std::cout << "Loading JERC JSON file: " << jercfname << std::endl;
		_correction_jerc = correction::CorrectionSet::from_file(jercfname);
		if (!_correction_jerc->validate()) {
			throw std::runtime_error("Failed to validate JERC correction set: " + jercfname);
		}
		std::cout << "JERC JSON validated successfully." << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "[ERROR] Failed to load/validate JERC JSON: " << e.what() << std::endl;
		throw;
	}


	const std::string level = "_L1L2L3Res";
	const std::string algo = "_AK4PFPuppi"; 
	std::string jec_compound = jerctag + level + algo;

	try {
		std::cout << "Setting up Jet Energy Corrector with tag: " << jec_compound << std::endl;
		_jetCorrector = _correction_jerc->compound().at(jec_compound);
		std::cout << "Jet corrector loaded." << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "[ERROR] Failed to get jet corrector: " << e.what() << std::endl;
		throw;
	}

	// Load JEC uncertainties if MC
	if (!_isData && !jercUncTags.empty()) {
		std::cout << "Loading JEC uncertainties..." << std::endl;
		_jetCorrectionUnc.reserve(jercUncTags.size());
		for (const std::string& junc : jercUncTags) {
			if (junc == "JER") continue;
			std::string unc_tag = jerctag + "_" + junc + algo;
			try {
				std::cout << "  → Loading JEC uncertainty: " << unc_tag << std::endl;
				_jetCorrectionUnc.emplace_back(_correction_jerc->at(unc_tag));
			} catch (const std::exception& e) {
				std::cerr << "[WARNING] Failed to load JEC uncertainty (" << unc_tag << "): " << e.what() << std::endl;
			}
		}
	}

	// Load JER resolution and scale factor
	try {
		std::string ptres_tag = jertag + "_PtResolution" + algo;
		std::string sf_tag = jertag + "_ScaleFactor" + algo;
		std::cout << "Loading JER tags:\n  PtResolution: " << ptres_tag << "\n  ScaleFactor: " << sf_tag << std::endl;
		_jetResCorrector = _correction_jerc->at(ptres_tag);
		_jetResCorrector_SF = _correction_jerc->at(sf_tag);
		std::cout << "JER resolution and scale factors loaded successfully." << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "[ERROR] Failed to load JER resolution or scale factors: " << e.what() << std::endl;
		throw;
	}

	// Load MET Phi modulation corrections
	if (fname_metphimod.empty()) {
		std::cout << "No MET Phi Modulation JSON provided; skipping MET phi corrections." << std::endl;
		_metphiCorrector.reset();
	} else {
		try {
			std::cout << "Loading MET Phi Modulation JSON: " << fname_metphimod << std::endl;
			_metphiCorrector = correction::CorrectionSet::from_file(fname_metphimod);
			assert(_metphiCorrector != nullptr);
			std::cout << "MET Phi Modulation loaded." << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "[WARNING] Failed to load MET Phi modulation JSON: " << e.what()
			          << " (skipping MET phi corrections)" << std::endl;
			_metphiCorrector.reset();
		}
	}


	// Load Jet Veto Map
	try {
		std::cout << "Loading Jet Veto Map JSON: " << jetvetofname << std::endl;
		_correction_jetvetomap = correction::CorrectionSet::from_file(jetvetofname);
		assert(_correction_jetvetomap != nullptr);
		if (!_correction_jetvetomap->validate()) {
			throw std::runtime_error("Failed to validate Jet Veto Map");
		}
		std::cout << "Jet Veto Map validated." << std::endl;

		// Confirm the requested veto tag exists
		_correction_jetvetomap->at(jetvetotag);
		_jetveto_tag = jetvetotag;
		std::cout << "Using Jet Veto Map Tag: " << _jetveto_tag << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "[ERROR] Jet Veto Map setup failed: " << e.what() << std::endl;
		throw;
	}
    std::cout << "--------------------------------------------------------" << std::endl;
	std::cout << "============= DONE SETTING UP JETMET CORRECTIONS =======" << std::endl;
	std::cout << "--------------------------------------------------------" << std::endl;
}



void NanoAODAnalyzerrdframe::applyJetMETCorrections() 
{
  cout << "apply JETMET correction" << endl;
  	using AppCorrLambda = std::function<floats(const floats&, const floats&, const floats&, const floats&, const floats&, const floats&, const float&, const unsigned int)>;
	using JecUncertaintyLambda = std::function<floats(const floats&, const floats&, const floats&, const floats&, const floats&, const floats&, const float&, const int&, const std::string&, const unsigned int)>;

	AppCorrLambda appcorrlambdaf;
    JecUncertaintyLambda jecuncertaintylambdaf;

  	const bool isRun3_2023BPix = (_year == 2023 && _yr_ext == "BPix");
	const bool isRun3_2024 = (_year == 2024);
  
	// this is for 2023BPix, where the JEC uncertainty is different

	if (isRun3_2023BPix || isRun3_2024)
	{
		std::cout <<"[INFO] Using 2023BPix/2024 JEC/JER lambda definitions." << std::endl;
	

		appcorrlambdaf = [_jetCorrector = this->_jetCorrector, _isData = this->_isData](const floats& jetpts, const floats& jetetas, const floats& jetphis, const floats& jetAreas, const floats& jetrawf, const floats& vars_to_corr, const float& rho, const unsigned int run = 1) -> floats 
		{
			floats corrfactors;
			corrfactors.reserve(jetpts.size());
			
			for (size_t i = 0; i < jetpts.size(); ++i) {
				float rawjetpt = jetpts[i] * (1.0 - jetrawf[i]);
				float rawjetvar = vars_to_corr[i] * (1.0 - jetrawf[i]);
				float corrfactor = 1;
				
				try {
					if (rawjetvar < 6500) {
						// Create variant vector for JEC correction
						// Data needs 6 params: JetA, JetEta, JetPt, Rho, JetPhi, run
						// MC needs 5 params: JetA, JetEta, JetPt, Rho, JetPhi
						std::vector<std::variant<int, double, std::string>> params = {
						
							static_cast<double>(jetAreas[i]),   // JetA
							static_cast<double>(jetetas[i]),    // JetEta
							static_cast<double>(rawjetpt),      // JetPt
							static_cast<double>(rho),            // Rho
							static_cast<double>(jetphis[i])		//JetPhi
						};
							//Data requires run number as 6th argument
						if (_isData){
							params.push_back(static_cast<double>(run));
						}
						
						corrfactor = _jetCorrector->evaluate(params);
					} else {
						std::cerr << "[WARNING] Raw jet variable is too large: " << rawjetvar << std::endl;
					}
				} catch (const std::exception& e) {
					std::cerr << "[ERROR] Jet correction failed (data=" << _isData << ", run=" << run << "): " << e.what() << std::endl;
				}
				corrfactors.emplace_back(rawjetvar * corrfactor);
			}
			return corrfactors;
		};
	
		jecuncertaintylambdaf = [_jetCorrector = this->_jetCorrector, _jetCorrectionUnc = this->_jetCorrectionUnc, _isData = this->_isData, isRun3_2023BPix, isRun3_2024](const floats& jetpts, const floats& jetetas, const floats& jetphis, const floats& jetAreas, const floats& jetrawf, const floats& vartocorr, const float& rho, const int& jec_unc_idx, const std::string& unc_type, const unsigned int run = 1) -> floats 
		{
			floats uncertainties;
			uncertainties.reserve(jetpts.size());
			
			for (size_t i = 0; i < jetpts.size(); ++i) {
				float rawjetpt = jetpts[i] * (1.0 - jetrawf[i]);
				float rawjetvar = vartocorr[i] * (1.0 - jetrawf[i]);
				float corrfactor = 1;
				float corrfactor_unc = 1;
				
				try {
					if (rawjetpt < 6500) {
						// JEC correction with variant parameters
						std::vector<std::variant<int, double, std::string>> jec_params = {
							static_cast<double>(jetAreas[i]), 
							static_cast<double>(jetetas[i]), 
							static_cast<double>(rawjetpt), 
							static_cast<double>(rho),
							static_cast<double>(jetphis[i])
						};

						// Data requires run number as 6th parameter
						if (_isData) {
							jec_params.push_back(static_cast<double>(run));
						}
						corrfactor = _jetCorrector->evaluate(jec_params);
						
						// Uncertainty evaluation with variant parameters
						std::vector<std::variant<int, double, std::string>> unc_params = {
							static_cast<double>(jetetas[i]), 
							static_cast<double>(rawjetpt)
						};
						float unc_value = _jetCorrectionUnc[jec_unc_idx]->evaluate(unc_params);
						
						if (unc_type == "up") {
							corrfactor_unc = corrfactor + unc_value;
						} else if (unc_type == "down") {
							corrfactor_unc = corrfactor - unc_value;
						}
					}
				} catch (const std::exception& e) {
					std::cerr << "[WARNING] Jet uncertainty correction failed for jet " << i 
							<< ": " << e.what() << " (eta=" << jetetas[i] << ", pt=" << rawjetpt << ")" << std::endl;
					corrfactor_unc = corrfactor;
				}
				
				uncertainties.emplace_back(rawjetvar * corrfactor_unc);
			}
			return uncertainties;
		};

	}
	

	// --------------------------------------------------------------------------------------------------
	// uncomment down for 2023 and before
	else
	{

		appcorrlambdaf = [_jetCorrector = this->_jetCorrector](const floats& jetpts, const floats& jetetas,  const floats& jetphis, const floats& jetAreas, const floats& jetrawf, const floats& vars_to_corr, const float& rho, const unsigned int run = 1) -> floats {
			floats corrfactors;
			corrfactors.reserve(jetpts.size());
			for (size_t i = 0; i < jetpts.size(); ++i) {
			float rawjetpt = jetpts[i] * (1.0 - jetrawf[i]);
			float rawjetvar = vars_to_corr[i] * (1.0 - jetrawf[i]);
			float corrfactor = 1;
			try {
				if (rawjetvar < 6500) {
				corrfactor = _jetCorrector->evaluate({jetAreas[i], jetetas[i], rawjetpt, rho});
				//   std::cout << "[appcorrlambdaf] Jet pt: "  << jetpts[i] << ", eta: " << jetetas[i] << ", area: " << jetAreas[i] << ", raw factor: " << jetrawf[i] << ", rho: " << rho << "correction" << corrfactor << std::endl;
				} else {
				std::cerr << "[WARNING] Raw jet variable is too large: " << rawjetvar << std::endl;
				}
			} catch (const std::exception& e) {
				std::cerr << "[ERROR] Jet correction failed: " << e.what() << std::endl;
				
			}
			corrfactors.emplace_back(rawjetvar * corrfactor);
			}
			return corrfactors;
		};



		jecuncertaintylambdaf = [_jetCorrector = this->_jetCorrector, _jetCorrectionUnc = this->_jetCorrectionUnc, isRun3_2023BPix, isRun3_2024](const floats& jetpts, const floats& jetetas, const floats& jetphis, const floats& jetAreas, const floats& jetrawf, const floats& vartocorr, const float& rho, const int& jec_unc_idx, const std::string& unc_type, const unsigned int run = 1) -> floats {
			floats uncertainties;
			uncertainties.reserve(jetpts.size());
			for (size_t i = 0; i < jetpts.size(); ++i) {
				float rawjetpt = jetpts[i] * (1.0 - jetrawf[i]);
				float corrfactor = 1;
				float corrfactor_unc = 1;
				try {
					if (rawjetpt < 6500) {
					corrfactor = _jetCorrector->evaluate({jetAreas[i], jetetas[i], rawjetpt, rho});
					if (unc_type == "up") {
						corrfactor_unc = corrfactor + ((isRun3_2023BPix || isRun3_2024)
							? _jetCorrectionUnc[jec_unc_idx]->evaluate({jetetas[i], jetphis[i], rawjetpt})
							: _jetCorrectionUnc[jec_unc_idx]->evaluate({jetetas[i], rawjetpt}));
					} else if (unc_type == "down") {
						corrfactor_unc = corrfactor - ((isRun3_2023BPix || isRun3_2024)
							? _jetCorrectionUnc[jec_unc_idx]->evaluate({jetetas[i], jetphis[i], rawjetpt})
							: _jetCorrectionUnc[jec_unc_idx]->evaluate({jetetas[i], rawjetpt}));
					}
					}
				} catch (const std::exception& e) {
					std::cerr << "[WARNING] Jet uncertainty correction failed: " << e.what() << std::endl;
				}
				uncertainties.emplace_back(vartocorr[i] * (1.0 - jetrawf[i]) * corrfactor_unc);
				}
			return uncertainties;
		};
	}

	auto appresolutioncorrlambdaf = [_jetResCorrector_SF = this->_jetResCorrector_SF, _jetResCorrector = this->_jetResCorrector]
		(const floats& jetpts, const floats& jetetas, const floats& jetphis,
		const float& rho, const floats& GenJetpts, const floats& GenJetetas, const floats& GenJetphis,
		std::string type_name) -> floats 
	{
		floats correction_factors;
		correction_factors.reserve(jetpts.size());

		for (size_t i = 0; i < jetpts.size(); ++i) 
		{
			float corrfactor = 1.0;
			float matched_genJet_pt = -1;
			float min_dR = 0.2;

			try 
			{
				// Validate inputs
				if (i >= jetetas.size() || i >= jetphis.size()) 
				{
					std::cerr << "[WARN] Index " << i << " out of bounds for jet eta or phi.\n";
					correction_factors.emplace_back(1.0);
					continue;
				}

				float eta = jetetas[i];
				float pt = jetpts[i];
				float phi = jetphis[i];

				if (pt < 15 || std::isnan(pt) || std::isnan(eta) || std::isnan(rho)) 
				{
					// std::cerr << "[WARN] Skipping jet with pt=" << pt << ", eta=" << eta << ", rho=" << rho << " at index " << i << std::endl;
					correction_factors.emplace_back(1.0);
					continue;
				}

			
				float sf = _jetResCorrector_SF->evaluate({eta, pt, type_name}); //SF also expect pt argument
				float ptres = _jetResCorrector->evaluate({eta, pt, rho});
				
				for (size_t j = 0; j < GenJetpts.size(); ++j) 
				{
					float dr = ROOT::VecOps::DeltaR<float>(eta, GenJetetas[j], phi, GenJetphis[j]);
					if (dr < min_dR && std::abs(pt - GenJetpts[j]) < 3 * pt * ptres) {
						matched_genJet_pt = GenJetpts[j];
						break;
					}
				}

				if (matched_genJet_pt > 0) 
				{
					corrfactor = 1 + (sf - 1) * (pt - matched_genJet_pt) / pt;
				} else 
				{
					double smear = gRandom->Gaus(0, ptres);
					corrfactor = 1 + smear * std::sqrt(std::max(sf * sf - 1.0, 0.0));
				}

				if (corrfactor < 0) corrfactor = 0;

				// std::cout << "[DEBUG] JER correction | eta=" << eta << ", pt=" << pt << ", rho=" << rho << ", sf=" << sf << ", ptres=" << ptres << ", corr=" << corrfactor << std::endl;

			} 
			catch (const std::exception& e) 
			{
				// std::cerr << "[ERROR] Jet resolution correction failed: " << e.what();
				// 		  << "\n        Inputs -> eta: " << jetetas[i]
				// 		  << ", pt: " << jetpts[i]
				// 		  << ", phi: " << jetphis[i]
				// 		  << ", rho: " << rho
				// 		  << "\n        JetRes SF object valid: " << (_jetResCorrector_SF != nullptr)
				// 		  << ", JetRes PtRes object valid: " << (_jetResCorrector != nullptr)
				// 		  << "\n        GenJet size: " << GenJetpts.size()
				// 		  << ", Type name: " << type_name
				// 		  << std::endl;
			}
			

			correction_factors.emplace_back(corrfactor);
		}

		return correction_factors;
	};


  	auto metcorrlambdaf = [](const float& met, const float& metphi, const floats& jetptsbefore, const floats& jetptsafter, const floats& jetphis)->float
    {
      auto metx = met * cos(metphi);
      auto mety = met * sin(metphi);
      for (auto i=0; i<int(jetphis.size()); i++)
		{
		if (jetptsafter[i]>15.0)
			{
			metx += (jetptsafter[i] - jetptsbefore[i])*cos(jetphis[i]);
			mety += (jetptsafter[i] - jetptsbefore[i])*sin(jetphis[i]);
			}
		}
      float met_corr = float(sqrt(metx*metx + mety*mety));
      return met_corr;
    };
  
  
	auto metphicorrlambdaf = [](const float& met, const float& metphi, const floats& jetptsbefore, const floats& jetptsafter, const floats& jetphis)->float
    {
      auto metx = met * cos(metphi);
      auto mety = met * sin(metphi);
      for (auto i=0; i<int(jetphis.size()); i++)
		{
		if (jetptsafter[i]>15.0)
			{
			metx += (jetptsafter[i] - jetptsbefore[i])*cos(jetphis[i]);
			mety += (jetptsafter[i] - jetptsbefore[i])*sin(jetphis[i]);
			}
		}
      return float(atan2(mety, metx));
    };

	std::cout << "[INFO] Jet/MET correction lambdas ready. Proceeding with RDataFrame definitions." << std::endl;

		if (_jetCorrector != 0)
	    {
	      

			if (isRun3_2023BPix)
			{
				_rlm = _rlm.Define("Jet_pt_corr_nom", appcorrlambdaf, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_area", "Jet_rawFactor", "Jet_pt", "Rho_fixedGridRhoFastjetAll", "run"});
				_rlm = _rlm.Define("Jet_mass_corr_nom", appcorrlambdaf, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_area", "Jet_rawFactor", "Jet_mass", "Rho_fixedGridRhoFastjetAll", "run"});
			}
			else
			{
				_rlm = _rlm.Define("Jet_pt_corr_nom", appcorrlambdaf, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_area", "Jet_rawFactor", "Jet_pt", "Rho_fixedGridRhoFastjetAll", "run"});
				_rlm = _rlm.Define("Jet_mass_corr_nom", appcorrlambdaf, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_area", "Jet_rawFactor", "Jet_mass", "Rho_fixedGridRhoFastjetAll", "run"});
				
			}
		}
		
	      
		_rlm = _rlm.Define("Jet_pt_corr", "Jet_pt_corr_nom");
		_rlm = _rlm.Define("Jet_mass_corr", "Jet_mass_corr_nom");
		_rlm = _rlm.Define("MET_pt_corr", metcorrlambdaf, {"MET_pt_phimodcorr", "MET_phi_phimodcorr", "Jet_pt", "Jet_pt_corr_nom", "Jet_phi"});
		_rlm = _rlm.Define("MET_phi_corr", metphicorrlambdaf, {"MET_pt_phimodcorr", "MET_phi_phimodcorr", "Jet_pt", "Jet_pt_corr_nom", "Jet_phi"});

	if(!_isData)
	{
		_rlm = _rlm.Define("Jet_energy_res_corr_fact", [appresolutioncorrlambdaf](const floats& jetspt, const floats& jetseta, const floats& jetsphi, const float& rho, const floats& genjetspt, const floats& genjetseta, const floats& genjetsphi)
		{
			floats corr_fact = appresolutioncorrlambdaf(jetspt, jetseta, jetsphi, rho, genjetspt, genjetseta, genjetsphi, "nom");
			return corr_fact;
		}, {"Jet_pt_corr_nom", "Jet_eta", "Jet_phi", "Rho_fixedGridRhoFastjetAll", "GenJet_pt", "GenJet_eta", "GenJet_phi"});
	
		_rlm = _rlm.Define("Jet_pt_res_corr", "Jet_pt_corr_nom * Jet_energy_res_corr_fact");
		_rlm = _rlm.Define("Jet_mass_res_corr", "Jet_mass_corr_nom * Jet_energy_res_corr_fact");
		
		
		_rlm = _rlm.Define("MET_pt_res_corr", metcorrlambdaf, {"MET_pt_corr", "MET_phi_corr", "Jet_pt_corr_nom", "Jet_pt_res_corr", "Jet_phi"});
		_rlm = _rlm.Define("MET_phi_res_corr", metphicorrlambdaf, {"MET_pt_corr", "MET_phi_corr", "Jet_pt_corr_nom", "Jet_pt_res_corr", "Jet_phi"});
		
		_rlm = _rlm.Redefine("Jet_pt_corr", "Jet_pt_res_corr");
		_rlm = _rlm.Redefine("Jet_mass_corr", "Jet_mass_res_corr");
		_rlm = _rlm.Redefine("MET_pt_corr", "MET_pt_res_corr");
		_rlm = _rlm.Redefine("MET_phi_corr", "MET_phi_res_corr");
	}

	int unc_idx = 0;
	if(_jercunctag.size()>0 && !_isData)
	{
		for(auto _unc_name : _jercunctag)
		{	    
			if(_unc_name=="JER") continue;
			string jet_pt_name = "Jet_pt_"+_unc_name;
			string jet_mass_name = "Jet_mass_"+_unc_name;
	
			_rlm = _rlm.Define(jet_pt_name+"_up", [jecuncertaintylambdaf, unc_idx](const floats& jetspt, const floats& jetseta, const floats& jetphis, const floats& jetsarea, const floats& jetsrawf, const floats& vartocorr, const float& rho, const unsigned int run)
			{
				floats jet_pt_up = jecuncertaintylambdaf(jetspt, jetseta, jetphis, jetsarea, jetsrawf, vartocorr, rho, unc_idx, "up", run);
				return jet_pt_up;
			}, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_area", "Jet_rawFactor", "Jet_pt", "Rho_fixedGridRhoFastjetAll", "run"});
			_rlm = _rlm.Redefine(jet_pt_name+"_up", jet_pt_name+"_up * Jet_energy_res_corr_fact");

			_rlm = _rlm.Define(jet_pt_name+"_down", [jecuncertaintylambdaf, unc_idx](const floats& jetspt, const floats& jetseta, const floats& jetphis, const floats& jetsarea, const floats& jetsrawf, const floats& vartocorr, const float& rho, const unsigned int run)
			{
				floats jet_pt_down = jecuncertaintylambdaf(jetspt, jetseta, jetphis, jetsarea, jetsrawf, vartocorr, rho, unc_idx, "down", run);
				return jet_pt_down;
			}, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_area", "Jet_rawFactor", "Jet_pt", "Rho_fixedGridRhoFastjetAll", "run"});

			_rlm = _rlm.Redefine(jet_pt_name+"_down", jet_pt_name+"_down * Jet_energy_res_corr_fact");

			_rlm = _rlm.Define(jet_mass_name+"_up", [jecuncertaintylambdaf, unc_idx](const floats& jetspt, const floats& jetseta, const floats& jetphis, const floats& jetsarea, const floats& jetsrawf, const floats& vartocorr, const float& rho, const unsigned int run)
			{
				floats jet_mass_up = jecuncertaintylambdaf(jetspt, jetseta, jetphis,  jetsarea, jetsrawf, vartocorr, rho, unc_idx, "up", run);
				return jet_mass_up;
			}, {"Jet_pt", "Jet_eta", "Jet_phi" , "Jet_area", "Jet_rawFactor", "Jet_mass", "Rho_fixedGridRhoFastjetAll", "run"});
			_rlm = _rlm.Redefine(jet_mass_name+"_up", jet_mass_name+"_up * Jet_energy_res_corr_fact");
			
			_rlm = _rlm.Define(jet_mass_name+"_down", [jecuncertaintylambdaf, unc_idx](const floats& jetspt, const floats& jetseta, const floats& jetphis, const floats& jetsarea, const floats& jetsrawf, const floats& vartocorr, const float& rho, const unsigned int run)
			{
				floats jet_mass_down = jecuncertaintylambdaf(jetspt, jetseta, jetphis, jetsarea, jetsrawf, vartocorr, rho, unc_idx, "down", run);
				return jet_mass_down;
			}, {"Jet_pt", "Jet_eta", "Jet_phi" , "Jet_area", "Jet_rawFactor", "Jet_mass", "Rho_fixedGridRhoFastjetAll", "run"});
			_rlm = _rlm.Redefine(jet_mass_name+"_down", jet_mass_name+"_down * Jet_energy_res_corr_fact");
	
	
			string met_pt_name = "MET_pt_corr_"+_unc_name;
			string met_phi_name = "MET_phi_corr_"+_unc_name;
			_rlm = _rlm.Define(met_pt_name+"_up", metcorrlambdaf, {"MET_pt_phimodcorr", "MET_phi_phimodcorr", "Jet_pt", jet_pt_name+"_up", "Jet_phi"});
			_rlm = _rlm.Define(met_phi_name+"_up", metphicorrlambdaf, {"MET_pt_phimodcorr", "MET_phi_phimodcorr", "Jet_pt", jet_pt_name+"_up", "Jet_phi"});
			_rlm = _rlm.Define(met_pt_name+"_down", metcorrlambdaf, {"MET_pt_phimodcorr", "MET_phi_phimodcorr", "Jet_pt", jet_pt_name+"_down", "Jet_phi"});
			_rlm = _rlm.Define(met_phi_name+"_down", metphicorrlambdaf, {"MET_pt_phimodcorr", "MET_phi_phimodcorr", "Jet_pt", jet_pt_name+"_down", "Jet_phi"});
			unc_idx +=1;
		}

		//For JER Uncertainties
		
		_rlm = _rlm.Define("Jet_energy_res_corr_fact_up", [appresolutioncorrlambdaf](const floats& jetspt, const floats& jetseta, const floats& jetsphi, const float& rho, const floats& genjetspt, const floats& genjetseta, const floats& genjetsphi)
		{
			floats corr_fact = appresolutioncorrlambdaf(jetspt, jetseta, jetsphi, rho, genjetspt, genjetseta, genjetsphi, "up");
			return corr_fact;
		}, {"Jet_pt_corr_nom", "Jet_eta", "Jet_phi", "Rho_fixedGridRhoFastjetAll", "GenJet_pt", "GenJet_eta", "GenJet_phi"});
		
		_rlm = _rlm.Define("Jet_pt_JER_up", "Jet_pt_corr_nom * Jet_energy_res_corr_fact_up");
		_rlm = _rlm.Define("Jet_mass_JER_up", "Jet_mass_corr_nom * Jet_energy_res_corr_fact_up");

		_rlm = _rlm.Define("MET_pt_corr_JER_up", metcorrlambdaf, {"MET_pt_corr", "MET_phi_corr", "Jet_pt_corr", "Jet_pt_JER_up", "Jet_phi"});
		_rlm = _rlm.Define("MET_phi_corr_JER_up", metphicorrlambdaf, {"MET_pt_corr", "MET_phi_corr", "Jet_pt_corr", "Jet_pt_JER_up", "Jet_phi"});

		_rlm = _rlm.Define("Jet_energy_res_corr_fact_down", [appresolutioncorrlambdaf](const floats& jetspt, const floats& jetseta, const floats& jetsphi, const float& rho, const floats& genjetspt, const floats& genjetseta, const floats& genjetsphi)
		{
			floats corr_fact = appresolutioncorrlambdaf(jetspt, jetseta, jetsphi, rho, genjetspt, genjetseta, genjetsphi, "down");
			return corr_fact;
		}, {"Jet_pt_corr_nom", "Jet_eta", "Jet_phi", "Rho_fixedGridRhoFastjetAll", "GenJet_pt", "GenJet_eta", "GenJet_phi"});
		
		_rlm = _rlm.Define("Jet_pt_JER_down", "Jet_pt_corr_nom * Jet_energy_res_corr_fact_down");
		_rlm = _rlm.Define("Jet_mass_JER_down", "Jet_mass_corr_nom * Jet_energy_res_corr_fact_down");
		
		
		_rlm = _rlm.Define("MET_pt_corr_JER_down", metcorrlambdaf, {"MET_pt_corr", "MET_phi_corr", "Jet_pt_corr", "Jet_pt_JER_down", "Jet_phi"});
		_rlm = _rlm.Define("MET_phi_corr_JER_down", metphicorrlambdaf, {"MET_pt_corr", "MET_phi_corr", "Jet_pt_corr", "Jet_pt_JER_down", "Jet_phi"});
		
	}
    
  
}





//=================================Define regions=================================================//
void NanoAODAnalyzerrdframe::defineJESUncRegion(const string&  pt_cut)
{
    if (debug){
    std::cout<<std::endl;
    std::cout<< "================================//=================================" << std::endl;
    std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
    std::cout<< "================================//=================================" << std::endl;
    std::cout<<"Define Region Based on Jets pt cut for all JES Unc variations"<<std::endl;
    }

    const float ptCut = std::stof(pt_cut);
    auto checkJetsAbovePtCut = [ptCut](const ROOT::RVec<float> &jetPts) -> int {
      return std::any_of(jetPts.begin(), jetPts.end(), [ptCut](float pt) { return pt > ptCut; }) ? 1 : 0;
    };
    //_rlm.Range(10).Snapshot("debugTree", "debug.root", {"Jet_pt_corr"});

    //_rlm = _rlm.Define("region_Jet_pt_nom", "Jet_pt_corr>"+pt_cut);
    //_rlm = _rlm.Define("region_Jet_pt_nom", [checkJetsAbovePtCut, ptCut](const ROOT::RVec<float> &jetPts) { return checkJetsAbovePtCut(jetPts, ptCut); }, {"Jet_pt_corr"});
    _rlm = _rlm.Define("region_Jet_pt_nom", checkJetsAbovePtCut, {"Jet_pt_corr"});
    //string reg_def = "region_Jet_pt_nom == 1 ? 0.0 : ";
    //string new_jet_pt_corr_def = "region_jes_unc == 0.0 ? Jet_pt_corr : ";
    
    if(!_jercunctag.empty() && !_isData){
      int jes_unc_idx = 1;
      for(const auto& _unc_name : _jercunctag){	    
	const string jet_pt_name_up = "Jet_pt_" + _unc_name + "_up";
	const string jet_pt_name_down = "Jet_pt_" + _unc_name + "_down";
	const string region_name_up = "region_jes_" + _unc_name + "_up";
	const string region_name_down = "region_jes_" + _unc_name + "_down";

	//_rlm = _rlm.Define(region_name_up, jet_pt_name_up+">"+pt_cut);
	//_rlm = _rlm.Define(region_name_down, jet_pt_name_down+">"+pt_cut);
	//_rlm = _rlm.Define(region_name_up, [checkJetsAbovePtCut, ptCut](const ROOT::RVec<float> &jetPts) { return checkJetsAbovePtCut(jetPts, ptCut); }, {jet_pt_name_up});
	//_rlm = _rlm.Define(region_name_down, [checkJetsAbovePtCut, ptCut](const ROOT::RVec<float> &jetPts) { return checkJetsAbovePtCut(jetPts, ptCut); }, {jet_pt_name_down});
	_rlm = _rlm.Define(region_name_up, checkJetsAbovePtCut, {jet_pt_name_up});
	_rlm = _rlm.Define(region_name_down, checkJetsAbovePtCut, {jet_pt_name_down});

	//reg_def = reg_def+ region_name_up + "==1 ? "+std::to_string(jes_unc_idx)+" : ";
	//reg_def = reg_def+ region_name_down + "==1 ? "+std::to_string(jes_unc_idx+1)+" : ";
	jes_unc_idx +=2;
      }
    }
    //reg_def += "-1";
    
    //_rlm = _rlm.Define("region_jes_unc", reg_def);

    // _rlm = _rlm.Redefine("Jet_pt_corr", "region_jes_unc == 0.0 ? Jet_pt_corr : ");



}


void NanoAODAnalyzerrdframe::applyMETPhiCorrections()
{
    std::cout << "[INFO] Applying MET XY corrections..." << std::endl;

    // Define a lambda to evaluate corrections
    auto appmetcorrlambdaf = [this](float met, float metphi, unsigned char npvs, unsigned int run,
                                    const std::string& ptphi, const std::string& met_type,
                                    const std::string& epoch, const std::string& dtmc,
                                    const std::string& variation) -> float
    {
        if (!_metphiCorrector) {
            std::cerr << "[ERROR] MET phi correction set not loaded!" << std::endl;
            return met;  // return uncorrected
        }

        // Select the correction node
        auto corr = _metphiCorrector->at("met_xy_corrections");

        // evaluate() takes *positional arguments*, not a single initializer list
        float corr_val = 0.0;
        try {
            std::vector<correction::Variable::Type> inputs;
            inputs.reserve(8);
            inputs.emplace_back(ptphi);
            inputs.emplace_back(met_type);
            inputs.emplace_back(epoch);
            inputs.emplace_back(dtmc);
            inputs.emplace_back(variation);
            inputs.emplace_back(static_cast<double>(met));
            inputs.emplace_back(static_cast<double>(metphi));
            inputs.emplace_back(static_cast<double>(npvs));
            corr_val = corr->evaluate(inputs);
        } catch (const std::exception& e) {
            std::cerr << "[WARNING] Correction evaluation failed: " << e.what()
                      << " (npvs=" << static_cast<int>(npvs) << ", met=" << met << ", metphi=" << metphi << ")\n";
            return met;  // fallback
        }
        return corr_val;
    };

    // -----------------------------------------------
    // Define corrected MET variables for Data and MC
    // -----------------------------------------------
    const std::string met_type = "PuppiMET";
    const std::string epoch    = std::to_string(_year) + _yr_ext;  
    const std::string variation = "nom";
    static const std::array<std::string, 2> components{{"pt", "phi"}};

    if (_isData) {
        const std::string dtmc = "DATA";
        for (const auto& v : components) {
            std::string out_name = "MET_" + v + "_phimodcorr";
            _rlm = _rlm.Define(out_name,
                [appmetcorrlambdaf, v, met_type, epoch, dtmc, variation](float met, float metphi, unsigned char npvs, unsigned int run) {
                    return appmetcorrlambdaf(met, metphi, npvs, run, v, met_type, epoch, dtmc, variation);
                },
                {"PuppiMET_pt", "PuppiMET_phi", "PV_npvsGood", "run"});
        }
    }
    else {
        const std::string dtmc = "MC";
        for (const auto& v : components) {
            std::string out_name = "MET_" + v + "_phimodcorr";
            _rlm = _rlm.Define(out_name,
                [appmetcorrlambdaf, v, met_type, epoch, dtmc, variation](float met, float metphi, unsigned char npvs) {
                    unsigned int dummy_run = 1;
                    return appmetcorrlambdaf(met, metphi, npvs, dummy_run, v, met_type, epoch, dtmc, variation);
                },
                {"PuppiMET_pt", "PuppiMET_phi", "PV_npvsGood"});
        }
    }


    std::cout << "[INFO] MET corrections applied successfully." << std::endl;
}


void NanoAODAnalyzerrdframe::applyMuPtCorrection() // data and MC
{
	std::cout << "Applying muon momentum scale and smearing corrections" << std::endl;

	if (_isData)
	{
		auto lambdaf_data = [](const ROOT::VecOps::RVec<int> &mu_charges,
							   const ROOT::VecOps::RVec<float> &mu_pts,
							   const ROOT::VecOps::RVec<float> &mu_etas,
							   const ROOT::VecOps::RVec<float> &mu_phis) -> ROOT::VecOps::RVec<float>
		{
			ROOT::VecOps::RVec<float> corrMuPts;
			corrMuPts.reserve(mu_pts.size());
			for (size_t i = 0; i < mu_pts.size(); ++i)
			{
				double corrected = pt_scale(true, mu_pts[i], mu_etas[i], mu_phis[i], mu_charges[i]);
				corrMuPts.emplace_back(static_cast<float>(corrected));
			}
			return corrMuPts;
		};

		_rlm = _rlm.Define("Muon_pt_corr", lambdaf_data, {"Muon_charge", "Muon_pt", "Muon_eta", "Muon_phi"});
	}
	else
	{
		auto scale_lambda = [](const ROOT::VecOps::RVec<int> &mu_charges,
							   const ROOT::VecOps::RVec<float> &mu_pts,
							   const ROOT::VecOps::RVec<float> &mu_etas,
							   const ROOT::VecOps::RVec<float> &mu_phis) -> ROOT::VecOps::RVec<float>
		{
			ROOT::VecOps::RVec<float> scaledPts;
			scaledPts.reserve(mu_pts.size());
			for (size_t i = 0; i < mu_pts.size(); ++i)
			{
				double scaled = pt_scale(false, mu_pts[i], mu_etas[i], mu_phis[i], mu_charges[i]);
				scaledPts.emplace_back(static_cast<float>(scaled));
			}
			return scaledPts;
		};

		auto smear_lambda = [](const ROOT::VecOps::RVec<float> &scaled_pts,
							   const ROOT::VecOps::RVec<float> &mu_etas,
							   const ROOT::VecOps::RVec<float> &mu_phis,
							   const ROOT::VecOps::RVec<unsigned char> &n_tracker_layers,
							   unsigned long long event,
							   unsigned int lumi) -> ROOT::VecOps::RVec<float>
		{
			ROOT::VecOps::RVec<float> smearedPts;
			smearedPts.reserve(scaled_pts.size());
			for (size_t i = 0; i < scaled_pts.size(); ++i)
			{
				double smeared = pt_resol(scaled_pts[i],
										  mu_etas[i],
										  mu_phis[i],
										  static_cast<float>(n_tracker_layers[i]),
										  static_cast<long long>(event),
										  lumi);
				smearedPts.emplace_back(static_cast<float>(smeared));
			}
			return smearedPts;
		};

		auto scale_var_up_lambda = [](const ROOT::VecOps::RVec<float> &mu_pts_corr,
									  const ROOT::VecOps::RVec<float> &mu_etas,
									  const ROOT::VecOps::RVec<float> &mu_phis,
									  const ROOT::VecOps::RVec<int> &mu_charges) -> ROOT::VecOps::RVec<float>
		{
			ROOT::VecOps::RVec<float> out;
			out.reserve(mu_pts_corr.size());
			for (size_t i = 0; i < mu_pts_corr.size(); ++i)
			{
				out.emplace_back(static_cast<float>(pt_scale_var(mu_pts_corr[i], mu_etas[i], mu_phis[i], mu_charges[i], "up")));
			}
			return out;
		};

		auto scale_var_down_lambda = [](const ROOT::VecOps::RVec<float> &mu_pts_corr,
										const ROOT::VecOps::RVec<float> &mu_etas,
										const ROOT::VecOps::RVec<float> &mu_phis,
										const ROOT::VecOps::RVec<int> &mu_charges) -> ROOT::VecOps::RVec<float>
		{
			ROOT::VecOps::RVec<float> out;
			out.reserve(mu_pts_corr.size());
			for (size_t i = 0; i < mu_pts_corr.size(); ++i)
			{
				out.emplace_back(static_cast<float>(pt_scale_var(mu_pts_corr[i], mu_etas[i], mu_phis[i], mu_charges[i], "dn")));
			}
			return out;
		};

		auto resol_var_up_lambda = [](const ROOT::VecOps::RVec<float> &mu_pts_scale,
									  const ROOT::VecOps::RVec<float> &mu_pts_corr,
									  const ROOT::VecOps::RVec<float> &mu_etas) -> ROOT::VecOps::RVec<float>
		{
			ROOT::VecOps::RVec<float> out;
			out.reserve(mu_pts_corr.size());
			for (size_t i = 0; i < mu_pts_corr.size(); ++i)
			{
				out.emplace_back(static_cast<float>(pt_resol_var(mu_pts_scale[i], mu_pts_corr[i], mu_etas[i], "up")));
			}
			return out;
		};

		auto resol_var_down_lambda = [](const ROOT::VecOps::RVec<float> &mu_pts_scale,
										const ROOT::VecOps::RVec<float> &mu_pts_corr,
										const ROOT::VecOps::RVec<float> &mu_etas) -> ROOT::VecOps::RVec<float>
		{
			ROOT::VecOps::RVec<float> out;
			out.reserve(mu_pts_corr.size());
			for (size_t i = 0; i < mu_pts_corr.size(); ++i)
			{
				out.emplace_back(static_cast<float>(pt_resol_var(mu_pts_scale[i], mu_pts_corr[i], mu_etas[i], "dn")));
			}
			return out;
		};

		_rlm = _rlm.Define("Muon_pt_scaleCorr",scale_lambda,{"Muon_charge", "Muon_pt", "Muon_eta", "Muon_phi"})
					   .Define("Muon_pt_corr",smear_lambda,{"Muon_pt_scaleCorr", "Muon_eta", "Muon_phi", "Muon_nTrackerLayers", "event", "luminosityBlock"})
					   .Define("Muon_pt_corr_scaleUp",scale_var_up_lambda,{"Muon_pt_corr", "Muon_eta", "Muon_phi", "Muon_charge"})
					   .Define("Muon_pt_corr_scaleDown",scale_var_down_lambda,{"Muon_pt_corr", "Muon_eta", "Muon_phi", "Muon_charge"})
					   .Define("Muon_pt_corr_resolUp",resol_var_up_lambda,{"Muon_pt_scaleCorr", "Muon_pt_corr", "Muon_eta"})
					   .Define("Muon_pt_corr_resolDown",resol_var_down_lambda,{"Muon_pt_scaleCorr", "Muon_pt_corr", "Muon_eta"});
	}

	const unsigned int max_debug_events = 10;
	auto muon_debug_printer = [max_debug_events](unsigned long long event,
												 const ROOT::VecOps::RVec<float> &pt_raw,
												 const ROOT::VecOps::RVec<float> &pt_corr)
	{
		static std::atomic<unsigned int> printed{0};
		unsigned int idx = printed.fetch_add(1);
		if (idx >= max_debug_events)
			return;

		float raw_first = pt_raw.empty() ? -1.0f : pt_raw[0];
		float corr_first = pt_corr.empty() ? -1.0f : pt_corr[0];
		std::cout << "[MuonPtDebug] Event " << event
				  << " raw_pt(first muon)=" << raw_first
				  << " -> corr_pt=" << corr_first << std::endl;
	};

	_rlm.Foreach(muon_debug_printer, {"event", "Muon_pt", "Muon_pt_corr"});
}

void NanoAODAnalyzerrdframe::setupCorrections(
    string goodjsonfname,
    string pufname,
    string putag,
    string btvfname,
    string btvtype,
    string fname_btagEff,
    string hname_Loose_btagEff_bcflav,
    string hname_Loose_btagEff_lflav,
    string hname_Medium_btagEff_bcflav,
    string hname_Medium_btagEff_lflav,
    string hname_Tight_btagEff_bcflav,
    string hname_Tight_btagEff_lflav,
    string muon_corr_fname,
    string muon_fname,
    string muonhlttype,
    // string muonrecotype,
    string muonidtype,
    string muonisotype,
    string electron_fname,
    string electron_reco_fname,
    string electron_reco_type,
    string electron_id_type,
	string electron_hltfname,
	string electron_hlt_type,
	string electron_ssfname,
	string electron_ss_type,
    string photon_fname,
    string photon_id_type,
	string photon_vetofname,
	string photon_ssfname,
	string photon_ss_type,
    string jercfname,
    string jerctag,
    std::vector<std::string> jercunctag,
    string jertag,
	string fname_metphimod,
	string jetvetofname,
	string jetvetotag,
	string jetidfname,
	string jetid_workingpoint)

// In this function the correction is evaluated for each jet, Muon, Electron and MET. The correction depends on the momentum, pseudorapidity, energy, and cone area of the jet, as well as the value of “rho” (the average momentum per area) and number of interactions in the event. The correction is used to scale the momentum of the jet.
{
	cout << "set up Corrections!" << endl;

	if (debug){
    std::cout<<std::endl;
    std::cout<< "================================//=================================" << std::endl;
    std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
    std::cout<< "================================//=================================" << std::endl;
    cout << "set up Corrections!" << endl;
    auto Nentry = _rlm.Count();
    // This is how you can express a range of the first 100 entries
    // _rlm = _rlm.Range(0, 1000000);
    // auto Nentry_100 = _rlm.Count();
    std::cout<< "-------------------------------------------------------------------" << std::endl;
    cout << "In Set up Corrections!:\n"
	 << " - All entries: " << *Nentry << endl;
    //<< " - Entries from 0 to 100: " << *Nentry_100 << endl;
    std::cout<< "-------------------------------------------------------------------" << std::endl;
    }

	if (_isData)
		_jsonOK = readgoodjson(goodjsonfname); 

	// std::cout << "Rochester correction files: " << muon_roch_fname << std::endl;
	// _Roch_corr.init(muon_roch_fname);

	if(_year == 2022 || _year == 2023 || _year == 2024)
		{_correction_electronss = correction::CorrectionSet::from_file(electron_ssfname);}
	_electron_ss_type = electron_ss_type;
	assert(_correction_electronss->validate());

	_correction_photonss = correction::CorrectionSet::from_file(photon_ssfname);
	_photon_ss_type = photon_ss_type;
	assert(_correction_photonss->validate());

	initMuonScaRe(muon_corr_fname);
	applyMuPtCorrection();


	if (!_isData)
	{
		// using correctionlib
		// Muon corrections
		_correction_muon = correction::CorrectionSet::from_file(muon_fname);
		_muon_hlt_type = muonhlttype;
		// _muon_reco_type = muonrecotype;
		_muon_id_type = muonidtype;
		_muon_iso_type = muonisotype;
		std::cout << "================================//=================================" << std::endl;
		cout << "MUON JSON FILE : " << muon_fname << endl;
		cout << "MUON HLT type in JSON  : " << _muon_hlt_type << endl;
		// cout << "MUON RECO type in JSON  : " << _muon_reco_type << endl;
		cout << "MUON ID type in JSON  : " << _muon_id_type << endl;
		cout << "MUON ISO type in JSON  : " << _muon_iso_type << endl;
		assert(_correction_muon->validate());

		// Electron corrections
		_correction_electron = correction::CorrectionSet::from_file(electron_fname);
		if (!electron_reco_fname.empty())
		{
			_correction_electron_reco = correction::CorrectionSet::from_file(electron_reco_fname);
		}
		else
		{
			_correction_electron_reco = correction::CorrectionSet::from_file(electron_fname);
		}
		if (!electron_hltfname.empty())
		{
			_correction_electronhlt = correction::CorrectionSet::from_file(electron_hltfname);
		}
		else
		{
			_correction_electronhlt.reset();
		}
		
		_electron_reco_type = electron_reco_type;
		_electron_id_type = electron_id_type;
		_electron_hlt_type = electron_hlt_type;
		
		
		std::cout << "================================//=================================" << std::endl;
		cout << "ELECTRON ID JSON FILE : " << electron_fname << endl;
		cout << "ELECTRON RECO JSON FILE : " << (electron_reco_fname.empty() ? electron_fname : electron_reco_fname) << endl;
		cout << "ELECTRON RECO type in JSON  : " << _electron_reco_type << endl;
		cout << "ELECTRONID type in JSON  : " << _electron_id_type << endl;
		cout << "ELECTRON HLT JSON FILE : " << (electron_hltfname.empty() ? "N/A" : electron_hltfname) << endl;
		cout << "ELECTRON HLT type in JSON  : " << _electron_hlt_type << endl;
		cout << "ELECTRON SMEARING type in JSON  : " << _electron_ss_type << endl;
		cout << "================================//=================================" << std::endl;
		assert(_correction_electron->validate());
		assert(_correction_electron_reco->validate());
		if (_correction_electronhlt)
			assert(_correction_electronhlt->validate());
		

		//  photon corrections
		_correction_photon = correction::CorrectionSet::from_file(photon_fname);
		if (!photon_vetofname.empty())
		{
			_correction_photon_veto = correction::CorrectionSet::from_file(photon_vetofname);
		}
		else
		{
			_correction_photon_veto = correction::CorrectionSet::from_file(photon_fname);
		}
		_photon_id_type = photon_id_type;
		
		assert(_correction_photon->validate());
		assert(_correction_photon_veto->validate());
		
		std::cout << "================================//=================================" << std::endl;
		cout << "PHOTON ID JSON FILE : " << photon_fname << endl;
		cout << "PHOTON VETO JSON FILE : " << (photon_vetofname.empty() ? photon_fname : photon_vetofname) << endl;
		cout << "PHOTON ID type in JSON  : " << _photon_id_type << endl;
		cout << "PHOTON SMEARING  JSON FILE : " << photon_ssfname << endl;
		cout << "PHOTON SMEARING type in JSON  : " << _photon_ss_type << endl;
		cout << "================================//=================================" << std::endl;


		// btag corrections
		_correction_btag1 = correction::CorrectionSet::from_file(btvfname);
		_btvtype = btvtype;
		assert(_correction_btag1->validate());
		std::cout << "================================//=================================" << std::endl;
		cout << "BTag JSON FILE : " << btvfname << endl;
		cout << "BTag type in JSON  : " << _btvtype << endl;
		std::cout << "================================//=================================" << std::endl;

		// btagging efficiency
		if(!_doBtagEff){
		f_btagEff = new TFile(fname_btagEff.c_str(), "READ");
		hist_Loose_btagEff_bcflav = dynamic_cast<TH2D*>(f_btagEff->Get(hname_Loose_btagEff_bcflav.c_str()));
		hist_Loose_btagEff_lflav = dynamic_cast<TH2D*>(f_btagEff->Get(hname_Loose_btagEff_lflav.c_str()));
		hist_Medium_btagEff_bcflav = dynamic_cast<TH2D*>(f_btagEff->Get(hname_Medium_btagEff_bcflav.c_str()));
		hist_Medium_btagEff_lflav = dynamic_cast<TH2D*>(f_btagEff->Get(hname_Medium_btagEff_lflav.c_str()));
		hist_Tight_btagEff_bcflav = dynamic_cast<TH2D*>(f_btagEff->Get(hname_Tight_btagEff_bcflav.c_str()));
		hist_Tight_btagEff_lflav = dynamic_cast<TH2D*>(f_btagEff->Get(hname_Tight_btagEff_lflav.c_str()));
		}
    

		// pile up weights
		_correction_pu = correction::CorrectionSet::from_file(pufname);
		assert(_correction_pu->validate());
		_putag = putag;
		auto punominal = [this](float x)
		{ return pucorrection(_correction_pu, _putag, "nominal", x); };
		auto puplus = [this](float x)
		{ return pucorrection(_correction_pu, _putag, "up", x); };
		auto puminus = [this](float x)
		{ return pucorrection(_correction_pu, _putag, "down", x); };

		if (!isDefined("puWeight"))
			_rlm = _rlm.Define("puWeight", punominal, {"Pileup_nTrueInt"});
		if (!isDefined("puWeight_plus"))
			_rlm = _rlm.Define("puWeight_plus", puplus, {"Pileup_nTrueInt"});
		if (!isDefined("puWeight_minus"))
			_rlm = _rlm.Define("puWeight_minus", puminus, {"Pileup_nTrueInt"});

			if (!isDefined("pugenWeight"))
			{
				_rlm = _rlm.Define("pugenWeight", [this](float x, float y)
			
			{ return x * y; },{"Generator_weight", "puWeight"}); 
				// { return x * y; },{"genWeight", "puWeight"}); 

			_rlm = _rlm.Define("pugenWeight_plus", [this](float x, float y)
			{ return x * y; },{"Generator_weight", "puWeight_plus"});
			// { return x * y; },{"genWeight", "puWeight_plus"});

			_rlm = _rlm.Define("pugenWeight_minus", [this](float x, float y)
			{ return x * y; },{"Generator_weight", "puWeight_minus"});
			// { return x * y; },{"genWeight", "puWeight_minus"});
				  
			}
		}
	
	_jerctag = jerctag;
	_jercunctag = jercunctag;


	setupJetMETCorrection(jercfname, _jerctag, _jercunctag, jertag, fname_metphimod,  jetvetofname, jetvetotag);
	if (!jetidfname.empty())
	{
		try
		{
			_correction_jetid = correction::CorrectionSet::from_file(jetidfname);
			assert(_correction_jetid->validate());
			std::cout << "Loaded JetID JSON file: " << jetidfname << std::endl;
		}
		catch (const std::exception &e)
		{
			std::cerr << "[JetID] Failed to load " << jetidfname << ": " << e.what() << std::endl;
			_correction_jetid.reset();
		}
	}
	else
	{
		_correction_jetid.reset();
	}
	_jetid_workingpoint = jetid_workingpoint;
	_jetid_mask_defined = false;
 	applyMETPhiCorrections();
	applyJetMETCorrections();
	applyElectronScaleAndSmear();
	applyPhotonScaleAndSmear();
}


double NanoAODAnalyzerrdframe::getBTaggingEff(double hadflav, double eta, double pt, std::string _BTaggingWP){
  double efficiency = 1.0;
  if(_doBtagEff){
    return efficiency;
  }

  int binX = -1;
  int binY = -1;
  if(hadflav!=0){

    if(_BTaggingWP=="L"){
      // Get the bin number corresponding to the provided x and y values
      binX = hist_Loose_btagEff_bcflav->GetXaxis()->FindBin(pt);
      binY = hist_Loose_btagEff_bcflav->GetYaxis()->FindBin(std::abs(eta));
      
      efficiency = hist_Loose_btagEff_bcflav->GetBinContent(binX, binY);
    }
    else if(_BTaggingWP=="M"){
      // Get the bin number corresponding to the provided x and y values
      binX = hist_Medium_btagEff_bcflav->GetXaxis()->FindBin(pt);
      binY = hist_Medium_btagEff_bcflav->GetYaxis()->FindBin(std::abs(eta));
      
      efficiency = hist_Medium_btagEff_bcflav->GetBinContent(binX, binY);
	//   std::cout<<"The BTagging efficiency for hadron Flavor ="<<hadflav<<" with eta = "<<eta<<" and pt = "<<pt<<" is "<<efficiency<<std::endl;
	  if(efficiency<0.0){
		std::cout<<"Warning: BTagging efficiency is negative for hadron Flavor ="<<hadflav<<" with eta = "<<eta<<" and pt = "<<pt<<std::endl;
		efficiency = 0.0;
    }}
    else if(_BTaggingWP=="T"){
      // Get the bin number corresponding to the provided x and y values
      binX = hist_Tight_btagEff_bcflav->GetXaxis()->FindBin(pt);
      binY = hist_Tight_btagEff_bcflav->GetYaxis()->FindBin(std::abs(eta));
      
      efficiency = hist_Tight_btagEff_bcflav->GetBinContent(binX, binY);
    }
  }
  else{
  
    if(_BTaggingWP=="L"){
      // Get the bin number corresponding to the provided x and y values
      binX = hist_Loose_btagEff_lflav->GetXaxis()->FindBin(pt);
      binY = hist_Loose_btagEff_lflav->GetYaxis()->FindBin(std::abs(eta));
      
      efficiency = hist_Loose_btagEff_lflav->GetBinContent(binX, binY);
    }
    else if(_BTaggingWP=="M"){
      // Get the bin number corresponding to the provided x and y values
      binX = hist_Medium_btagEff_lflav->GetXaxis()->FindBin(pt);
      binY = hist_Medium_btagEff_lflav->GetYaxis()->FindBin(std::abs(eta));
      
      efficiency = hist_Medium_btagEff_lflav->GetBinContent(binX, binY);
	//   std::cout<<"The BTagging efficiency for hadron Flavor ="<<hadflav<<" with eta = "<<eta<<" and pt = "<<pt<<" is "<<efficiency<<std::endl;
	  if(efficiency<0.0){
		std::cout<<"Warning: BTagging efficiency is negative for hadron Flavor ="<<hadflav<<" with eta = "<<eta<<" and pt = "<<pt<<std::endl;
		efficiency = 0.0;
	  }
    }
    else if(_BTaggingWP=="T"){
      // Get the bin number corresponding to the provided x and y values
      binX = hist_Tight_btagEff_lflav->GetXaxis()->FindBin(pt);
      binY = hist_Tight_btagEff_lflav->GetYaxis()->FindBin(std::abs(eta));
      
      efficiency = hist_Tight_btagEff_lflav->GetBinContent(binX, binY);
    }
  }
//   std::cout<<"The BTagging efficiency for hadron Flavor ="<<hadflav<<" with eta = "<<eta<<" and pt = "<<pt<<" is "<<efficiency<<std::endl;
  return efficiency;
}


ROOT::RDF::RNode NanoAODAnalyzerrdframe::calculateBTagSF(ROOT::RDF::RNode &_rlm, std::vector<std::string> Jets_vars_names, const int&_case, const double btag_cut, std::string _BTaggingWP, std::string output_var, const int&_redefine)
{
  if (_case == 1)
  {
    auto btagweightgenerator_bcflav_case1 = [this](const ROOT::VecOps::RVec<unsigned char>& hadflav,
                                                   const ROOT::VecOps::RVec<float>& etas,
                                                   const ROOT::VecOps::RVec<float>& pts,
                                                   const ROOT::VecOps::RVec<float>& btag_scores,
                                                   const double btag_cut,
                                                   std::string _BTagWP,
                                                   const std::string& variation) -> float {
      double btagWeight_bcflav = 1.0;
      for (std::size_t i = 0; i < pts.size(); i++) 
	  	{
			if (std::abs(etas[i]) > 2.4999 || pts[i] < 30.000001 || hadflav[i] == 0) continue;
			try 
			{
				if (btag_scores[i] >= btag_cut) 
				{
					// double bcjets_weights = _correction_btag1->at("deepJet_mujets")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
					if (_year == 2022) {
					double bcjets_weights = _correction_btag1->at("particleNet_comb")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
					// let us use comb for 2022 for now too
					btagWeight_bcflav *= bcjets_weights;
					// std::cout<<"BTag SF for bc-flavor jet " << i << " with pt = " << pts[i] << " and eta = " << etas[i] << " is applied with weight: " << bcjets_weights << std::endl;
					}
					else if (_year == 2023) {
					double bcjets_weights = _correction_btag1->at("particleNet_comb")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
					btagWeight_bcflav *= bcjets_weights;
					// std::cout<<"BTag SF for bc-flavor jet " << i << " with pt = " << pts[i] << " and eta = " << etas[i] << " is applied with weight: " << bcjets_weights << std::endl;
					}
					else if (_year == 2024) {
					// UParTAK4_kinfit expects flavor: 0=light, 5=bc combined
					int flavor_code = (hadflav[i] == 4 || hadflav[i] == 5) ? 5 : int(hadflav[i]);
					double bcjets_weights = _correction_btag1->at("UParTAK4_kinfit")->evaluate({variation, _BTagWP, flavor_code, std::fabs(etas[i]), pts[i]});
					btagWeight_bcflav *= bcjets_weights;
					// std::cout<<"BTag SF for bc-flavor jet " << i << " with pt = " << pts[i] << " and eta = " << etas[i] << " is applied with weight: " << bcjets_weights << std::endl;
					}

				} 
				else 
				{
					if (_year == 2022) {
					double bcjets_weights = _correction_btag1->at("particleNet_comb")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
					double eff = getBTaggingEff(hadflav[i], etas[i], pts[i], _BTagWP);
					btagWeight_bcflav *= (1 - bcjets_weights*eff)/(1-eff);
					// std::cout<<"BTag SF for bc-flavor jet " << i << " with pt = " << pts[i] << " and eta = " << etas[i] << " is applied with weight: " << bcjets_weights << " and efficiency: " << eff << std::endl;
					}
					else if (_year == 2023) {
					double bcjets_weights = _correction_btag1->at("particleNet_comb")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
					double eff = getBTaggingEff(hadflav[i], etas[i], pts[i], _BTagWP);
					btagWeight_bcflav *= (1 - bcjets_weights*eff)/(1-eff);
					// std::cout<<"BTag SF for bc-flavor jet " << i << " with pt = " << pts[i] << " and eta = " << etas[i] << " is applied with weight: " << bcjets_weights << " and efficiency: " << eff << std::endl;
					}
					else if (_year == 2024) {
					// UParTAK4_kinfit expects flavor: 0=light, 5=bc combined
					int flavor_code = (hadflav[i] == 4 || hadflav[i] == 5) ? 5 : int(hadflav[i]);
					double bcjets_weights = _correction_btag1->at("UParTAK4_kinfit")->evaluate({variation, _BTagWP, flavor_code, std::fabs(etas[i]), pts[i]});
					double eff = getBTaggingEff(hadflav[i], etas[i], pts[i], _BTagWP);
					btagWeight_bcflav *= (1 - bcjets_weights*eff)/(1-eff);
					}

				}
			}
			catch (const std::exception &e) 
			{
			std::cerr << "BTag SF failed for bc-flavor jet " << i << ": " << e.what() << std::endl;
        	}
      	}
      return btagWeight_bcflav;
    };

    auto btagweightgenerator_lflav_case1 = [this](const ROOT::VecOps::RVec<unsigned char>& hadflav,
                                                  const ROOT::VecOps::RVec<float>& etas,
                                                  const ROOT::VecOps::RVec<float>& pts,
                                                  const ROOT::VecOps::RVec<float>& btag_scores,
                                                  const double btag_cut,
                                                  std::string _BTagWP,
                                                  const std::string& variation) -> float {
      double btagWeight_lflav = 1.0;
      for (std::size_t i = 0; i < pts.size(); i++) {
        if (std::abs(etas[i]) > 2.4999 || pts[i] < 30.000001 || hadflav[i] != 0) continue;
		// 2024: mistag SFs not available yet -> do nothing (weight = 1)
    	if (_year == 2024) continue;
        try {
        //   double sf = _correction_btag1->at("deepJet_incl")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
          if (btag_scores[i] >= btag_cut) {
            double lightjets_weights = _correction_btag1->at("particleNet_light")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
            // double lightjets_weights = _correction_btag1->at("deepJet_light")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
	  		btagWeight_lflav *= lightjets_weights;
			// std::cout<<"BTag SF for light-flavor jet " << i << " with pt = " << pts[i] << " and eta = " << etas[i] << " is applied with weight: " << lightjets_weights << std::endl;
			
          } else {
			double lightjets_weights = _correction_btag1->at("particleNet_light")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
			// double lightjets_weights = _correction_btag1->at("deepJet_light")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
			double eff = getBTaggingEff(hadflav[i], etas[i], pts[i], _BTagWP);
			btagWeight_lflav *= (1 - lightjets_weights*eff)/(1-eff);
			// std::cout<<"BTag SF for light-flavor jet " << i << " with pt = " << pts[i] << " and eta = " << etas[i] << " is applied with weight: " << lightjets_weights << std::endl;
	  
          }
        } catch (const std::exception &e) {
          std::cerr << "BTag SF failed for light-flavor jet " << i << ": " << e.what() << std::endl;
        }
      }
      return btagWeight_lflav;
    };

    std::vector<std::string> variations = {};
    std::string column_name_bcflav = "";
    std::string column_name_lflav = "";
    if(_redefine!=1){

      	 if (_year == 2022 || _year == 2023)
			variations = {"central", "up", "down", "up_correlated", "down_correlated", "up_uncorrelated", "down_uncorrelated", "up_statistic", "down_statistic"};
		else if (_year == 2024)
      		variations = {"central", "up", "down", "up_statistic", "down_statistic"}; //this is for 2024, will update when available

		for (const std::string& variation : variations) 
		{
			column_name_bcflav = output_var + "bcflav_" +variation;
			_rlm = _rlm.Define(column_name_bcflav, [btagweightgenerator_bcflav_case1, variation, btag_cut, _BTaggingWP](const ROOT::VecOps::RVec<unsigned char>& hadflav, const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<float>& pts, const ROOT::VecOps::RVec<float>& btag_scores) 
			{
				float weight = btagweightgenerator_bcflav_case1(hadflav, etas, pts, btag_scores, btag_cut, _BTaggingWP, variation);// Get the weight for the corresponding variation
				return weight;
			}, Jets_vars_names); //after all cuts, remove overlapped
	
			column_name_lflav = output_var + "lflav_" +variation;
			_rlm = _rlm.Define(column_name_lflav, [btagweightgenerator_lflav_case1, variation, btag_cut, _BTaggingWP](const ROOT::VecOps::RVec<unsigned char>& hadflav, const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<float>& pts, const ROOT::VecOps::RVec<float>& btag_scores) 
			{
				float weight = btagweightgenerator_lflav_case1(hadflav, etas, pts, btag_scores, btag_cut, _BTaggingWP, variation);// Get the weight for the corresponding variation
				return weight;
			}, Jets_vars_names); //after all cuts, remove overlapped
			std::cout<< "BJet SF column name: " << column_name_bcflav<< " and " << column_name_lflav  << std::endl;
			if(isDefined(column_name_bcflav)){
			std::cout<< "BJet SF column: " << column_name_bcflav << " is saved in the Node."<< std::endl;
			}
			if(isDefined(column_name_lflav)){
			std::cout<< "BJet SF column: " << column_name_lflav << " is saved in the Node."<< std::endl;
			}
			std::cout << "[INFO] Defined column: " << column_name_bcflav << std::endl;
			std::cout << "[INFO] Defined column: " << column_name_lflav << std::endl;

		}
    }
    else
	{
      variations = {"central"}; 
      for (const std::string& variation : variations) 
	  {
			column_name_bcflav = output_var + "bcflav_" +variation;
			_rlm = _rlm.Redefine(column_name_bcflav, [btagweightgenerator_bcflav_case1, variation, btag_cut, _BTaggingWP](const ROOT::VecOps::RVec<unsigned char>& hadflav, const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<float>& pts, const ROOT::VecOps::RVec<float>& btag_scores) {
				float weight = btagweightgenerator_bcflav_case1(hadflav, etas, pts, btag_scores, btag_cut, _BTaggingWP, variation);// Get the weight for the corresponding variation
				return weight;
			}, Jets_vars_names); //after all cuts, remove overlapped
			
			column_name_lflav = output_var + "lflav_" +variation;
			_rlm = _rlm.Redefine(column_name_lflav, [btagweightgenerator_lflav_case1, variation, btag_cut, _BTaggingWP](const ROOT::VecOps::RVec<unsigned char>& hadflav, const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<float>& pts, const ROOT::VecOps::RVec<float>& btag_scores) {
				float weight = btagweightgenerator_lflav_case1(hadflav, etas, pts, btag_scores, btag_cut, _BTaggingWP, variation);// Get the weight for the corresponding variation
				return weight;
			}, Jets_vars_names); //after all cuts, remove overlapped
			std::cout<< "BJet SF column name: " << column_name_bcflav<< " and " << column_name_lflav  << std::endl;
			if(isDefined(column_name_bcflav)){
			std::cout<< "BJet SF column: " << column_name_bcflav << " is Changed in the Node."<< std::endl;
			}
			if(isDefined(column_name_lflav)){
			std::cout<< "BJet SF column: " << column_name_lflav << " is Changed in the Node."<< std::endl;
			
			}
			std::cout << "[INFO] Defined column: " << column_name_bcflav << std::endl;
			std::cout << "[INFO] Defined column: " << column_name_lflav << std::endl;

      	}

    }
  }


  else if (_case == 3)
  {
    std::cout << "Case 3: Shape correction for BTag SF" << std::endl;
    auto btagweightgenerator3 = [this](const ints &hadflav, const floats &etas, const floats &pts, const floats &btags) -> float {
      double bweight = 1.0;
      for (std::size_t i = 0; i < pts.size(); ++i) {
        if (std::abs(etas[i]) > 2.5 || pts[i] < 30.000001) continue;
        try {
          double w = _correction_btag1->at(_btvtype)->evaluate({"central", int(hadflav[i]), fabs(float(etas[i])), float(pts[i]), float(btags[i])});
          bweight *= w;

        } catch (const std::exception &e) {
          std::cerr << "Shape SF failed for jet " << i << ": " << e.what() << std::endl;
        }
      }
      return bweight;
    };
    std::string column_name = output_var + "case3";
    _rlm = _rlm.Define(column_name, btagweightgenerator3, Jets_vars_names);
  }
  return _rlm;
}


ROOT::RDF::RNode NanoAODAnalyzerrdframe::calculateMuSF(RNode _rlm, std::vector<std::string> Muon_vars, std::string output_var = "muon_SF_")
{
	std::cout << "Muon SF for MC" << std::endl;

	// Variations defined by Muon POG JSON structure
	// std::vector<std::string> variations = {"nominal", "systup", "systdown"};  //'sf' is nominal in Run2
	std::vector<std::string> variations = {"nominal", "systup", "systdown","syst", "stat"};

	auto muon_weightgenerator = [this](const std::string &muon_type,
									   const ROOT::VecOps::RVec<float> &etas,
									   const ROOT::VecOps::RVec<float> &pts,
									   const std::string &variation) -> float
	{
		double muon_w = 1.0;

		for (std::size_t i = 0; i < pts.size(); ++i)
		{
			try
			{
				double w = _correction_muon->at(muon_type)->evaluate({
					std::fabs(etas[i]),
					pts[i],
					variation
				});
				muon_w *= w;
				// std::cout << "[Muon SF] Type: " << muon_type
				// 		  << ", Eta: " << etas[i]
				// 		  << ", Pt: " << pts[i]
				// 		  << ", Var: " << variation
				// 		  << ", Weight: " << w << std::endl;
			}
			catch (const std::exception &e)
			{
				std::cerr << "[Muon SF ERROR] " << e.what()
						  << " (Eta=" << etas[i]
						  << ", Pt=" << pts[i]
						  << ", Var=" << variation
						  << ", Type=" << muon_type << ")" << std::endl;
				continue;
			}
		}

		return muon_w;
	};

	for (const std::string &variation : variations)
	{
		// HLT SF
		std::string column_name_hlt = output_var + "hlt_" + variation;
		std::cout << "[INFO] Defining column: " << column_name_hlt << std::endl;
		_rlm = _rlm.Define(column_name_hlt,
			[this, muon_weightgenerator, variation](const ROOT::VecOps::RVec<float> &etas, const ROOT::VecOps::RVec<float> &pts)
			{
				return muon_weightgenerator(_muon_hlt_type, etas, pts, variation);
			}, Muon_vars);

		// ID SF
		std::string column_name_id = output_var + "id_" + variation;
		std::cout << "[INFO] Defining column: " << column_name_id << std::endl;
		_rlm = _rlm.Define(column_name_id,
			[this, muon_weightgenerator, variation](const ROOT::VecOps::RVec<float> &etas, const ROOT::VecOps::RVec<float> &pts)
			{
				return muon_weightgenerator(_muon_id_type, etas, pts, variation);
			}, Muon_vars);

		// ISO SF
		std::string column_name_iso = output_var + "iso_" + variation;
		std::cout << "[INFO] Defining column: " << column_name_iso << std::endl;
		_rlm = _rlm.Define(column_name_iso,
			[this, muon_weightgenerator, variation](const ROOT::VecOps::RVec<float> &etas, const ROOT::VecOps::RVec<float> &pts)
			{
				return muon_weightgenerator(_muon_iso_type, etas, pts, variation);
			}, Muon_vars);

		// Combined SF
		// std::string column_name = output_var;
		// if (variation == "nominal")
		// 	column_name += "central";
		// else if (variation == "systup")
		// 	column_name += "up";
		// else
		// 	column_name += "down";

		std::string column_name = output_var;
		if(variation=="nominal"){
			column_name += "central";
		}
		else if(variation=="systup"){
			column_name += "up";
		}
		else if(variation=="systdown"){
			column_name += "down";
		}
		else if(variation=="syst"){
			column_name += "syst";
		}
		else if(variation=="stat"){
			column_name += "stat";
		}
			

		std::string sf_formula = column_name_hlt + " * " + column_name_id + " * " + column_name_iso;
		_rlm = _rlm.Define(column_name, sf_formula);
		std::cout << "Muon SF column name: " << column_name << std::endl;
	}

	return _rlm;
}


ROOT::RDF::RNode NanoAODAnalyzerrdframe::calculateEleSF(RNode _rlm, std::vector<std::string> Ele_vars, std::string output_var)
{
	std::cout << "Electron SF for MC" << std::endl;

	// ========== Electron ID & Reco SF ========== //
	auto electron_weightgenerator = [this](const std::string &eletype,
	                                       const ROOT::VecOps::RVec<float> &etas,
	                                       const ROOT::VecOps::RVec<float> &pts,
	                                       const ROOT::VecOps::RVec<float> &phis,
	                                       const std::string &variation,
	                                       bool isReco) -> float
	{
		const correction::CorrectionSet *corr_set = isReco ? _correction_electron_reco.get() : _correction_electron.get();
		if (!corr_set)
		{
			return 1.0;
		}
		auto correction = corr_set->at("Electron-ID-SF");
		const bool requiresPhi = (_year == 2023);

		double electron_weight = 1.0;

		for (std::size_t i = 0; i < pts.size(); i++)
		{
			try
			{
				const std::string reco_key = (pts[i] > 75.0) ? "RecoAbove75" : "Reco20to75";
				const std::string wp = isReco ? reco_key : eletype;
				std::string year_key;

				if (_year == 2022 && _yr_ext == "")          year_key = "2022Re-recoBCD";
				else if (_year == 2022 && _yr_ext == "EE")   year_key = "2022Re-recoE+PromptFG";
				else if (_year == 2023 && _yr_ext == "")     year_key = "2023PromptC";
				else if (_year == 2023 && _yr_ext == "BPix") year_key = "2023PromptD";
				else if (_year == 2024 && _yr_ext == "")     year_key = isReco ? "2024Prompt" : "2024";
				else throw std::runtime_error("[ElectronSF] Unsupported year/extension combination.");

				double w = 1.0;
				if (requiresPhi) {
					w = correction->evaluate({year_key, variation, wp, std::fabs(etas[i]), pts[i], phis[i]});
				} else {
					w = correction->evaluate({year_key, variation, wp, std::fabs(etas[i]), pts[i]});
				}
				electron_weight *= w;
			}
			catch (const std::exception &e)
			{
				std::cerr << "[Electron SF ERROR] " << e.what()
				          << " (eta=" << etas[i]
				          << ", pt=" << pts[i]
				          << ", wp=" << (isReco ? "RecoX" : eletype)
				          << ", var=" << variation << ")" << std::endl;
			}
		}

		return electron_weight;
	};

	// ========== Electron HLT SF ========== //
	auto electron_hlt_weight = [this](const ROOT::VecOps::RVec<float> &etas,
	                                  const ROOT::VecOps::RVec<float> &pts,
	                                  const std::string &variation) -> float
	{
		if (!_correction_electronhlt)
			return 1.0;
		double hlt_weight = 1.0;
		for (std::size_t i = 0; i < pts.size(); ++i)
		{
			try
			{
				std::string year_key;
				if (_year == 2022 && _yr_ext == "")         year_key = "2022Re-recoBCD";
				else if (_year == 2022 && _yr_ext == "EE")  year_key = "2022Re-recoE+PromptFG";
				else if (_year == 2023 && _yr_ext == "")    year_key = "2023PromptC";
				else if (_year == 2023 && _yr_ext == "BPix")year_key = "2023PromptD";
				else if (_year == 2024 && _yr_ext == "")		 year_key = "2024Prompt";
				else throw std::runtime_error("[ElectronHLTSF] Unsupported year/extension.");

				double w = _correction_electronhlt->at("Electron-HLT-SF")->evaluate({year_key, variation, _electron_hlt_type, std::fabs(etas[i]), pts[i]});
				hlt_weight *= w;
			}
			catch (const std::exception &e)
			{
				std::cerr << "[Electron HLT SF ERROR] " << e.what()
				          << " (eta=" << etas[i]
				          << ", pt=" << pts[i]
				          << ", path=" << _electron_hlt_type
				          << ", var=" << variation << ")" << std::endl;
			}
		}
		return hlt_weight;
	};



	// ========== Loop Over Variations ========== //
	std::vector<std::string> variations = {"sf", "sfup", "sfdown"};

	for (const std::string &variation : variations)
	{
		std::string column_name_reco = output_var + "reco_" + variation;
		std::string column_name_id   = output_var + "id_"   + variation;
		std::string column_name_hlt  = output_var + "hlt_"  + variation;

		_rlm = _rlm.Define(column_name_reco,
			[this, electron_weightgenerator, variation](const ROOT::VecOps::RVec<float> &etas,
			                                             const ROOT::VecOps::RVec<float> &pts,
			                                             const ROOT::VecOps::RVec<float> &phis)
			{
				return electron_weightgenerator("", etas, pts, phis, variation, true);
			}, {Ele_vars[0], Ele_vars[1], Ele_vars[2]});

		_rlm = _rlm.Define(column_name_id,
			[this, electron_weightgenerator, variation](const ROOT::VecOps::RVec<float> &etas,
			                                             const ROOT::VecOps::RVec<float> &pts,
			                                             const ROOT::VecOps::RVec<float> &phis)
			{
				return electron_weightgenerator(_electron_id_type, etas, pts, phis, variation, false);
			}, {Ele_vars[0], Ele_vars[1], Ele_vars[2]});

		_rlm = _rlm.Define(column_name_hlt,
			[this, electron_hlt_weight, variation](const ROOT::VecOps::RVec<float> &etas,
			                                       const ROOT::VecOps::RVec<float> &pts)
			{
				return electron_hlt_weight(etas, pts, variation);
			}, {Ele_vars[0], Ele_vars[1]});
	

		std::string final_column = output_var + (
			variation == "sf" ? "central" :
			(variation == "sfup" ? "up" : "down"));

		_rlm = _rlm.Define(final_column,
			column_name_reco + " * " + column_name_id + " * " + column_name_hlt );

		std::cout << "[INFO] Defined Electron SF column: " << final_column << std::endl;
	}

	return _rlm;
}


ROOT::RDF::RNode NanoAODAnalyzerrdframe::calculatePhoSF(RNode _rlm,std::vector<std::string> Pho_vars,std::string output_var)
{
	auto photon_weightgenerator = [this](const std::string& year_key,
	                                     const std::string& id_wp,
	                                     const ROOT::VecOps::RVec<float>& etas,
	                                     const ROOT::VecOps::RVec<float>& pts,
	                                     const ROOT::VecOps::RVec<float>& r9s,
	                                     const ROOT::VecOps::RVec<float>& phis,
	                                     const std::string& variation) -> std::array<double, 4>
	{
		double phoSF_weight = 1.0;
		double phoSF_id     = 1.0;
		double phoSF_csev   = 1.0;
		double phoSF_pix    = 1.0;

		for (std::size_t i = 0; i < pts.size(); ++i) 
		{
			float absEta = std::fabs(etas[i]);
			float pt     = pts[i];
			float phi    = phis[i];
			float r9     = r9s[i];

			// double sf_id      = _correction_photon->at("Photon-ID-SF")->evaluate({year_key, variation, id_wp, absEta, pt});

			double sf_id = 1.0;
			if (_year == 2023) {
				sf_id = _correction_photon->at("Photon-ID-SF")->evaluate({year_key, variation, id_wp, absEta, pt, phi});
			} else {
				sf_id = _correction_photon->at("Photon-ID-SF")->evaluate({year_key, variation, id_wp, absEta, pt});
			}
			double sf_csev    = _correction_photon_veto->at("Photon-CSEV-SF")->evaluate({year_key, variation, id_wp, absEta, r9});
			double sf_pixveto = _correction_photon_veto->at("Photon-PixVeto-SF")->evaluate({year_key, variation, id_wp, absEta, r9});

			phoSF_weight *= sf_id * sf_csev * sf_pixveto;
			phoSF_id     *= sf_id;
			phoSF_csev   *= sf_csev;
			phoSF_pix    *= sf_pixveto;
		}

		return {phoSF_weight, phoSF_id, phoSF_csev, phoSF_pix};
	};


	// Determine the correctionlib key
	std::string year_key;
	if (_year == 2022 && _yr_ext == "")      year_key = "2022Re-recoBCD";
	else if (_year == 2022 && _yr_ext == "EE") year_key = "2022Re-recoE+PromptFG";
	else if (_year == 2023 && _yr_ext == "") year_key = "2023PromptC";
	else if (_year == 2023 && _yr_ext == "BPix") year_key = "2023PromptD";
	else if (_year == 2024 && _yr_ext == "") year_key = "2024Prompt";
	else throw std::runtime_error("[PhotonSF] Unsupported year/extension combination.");


	std::vector<std::string> variations = {"sf", "sfup", "sfdown"};
	for (const auto& variation : variations)
	{
		std::string component_col = output_var + "components_" + variation;

		_rlm = _rlm.Define(component_col,
		[this, photon_weightgenerator, year_key, variation](const ROOT::VecOps::RVec<float>& etas,
					const ROOT::VecOps::RVec<float>& pts,
					const ROOT::VecOps::RVec<float>& r9s,
					const ROOT::VecOps::RVec<float>& phis)
		{
		return photon_weightgenerator(year_key, _photon_id_type, etas, pts,r9s,phis,variation);
		}, Pho_vars);

		// Final scale factor column names
		std::string final_col = output_var + (
		variation == "sf" ? "central" :
		(variation == "sfup" ? "up" : "down")
		);
		
		// also save individual components if needed : ID, PIXVeto, CSEV
		std::string col_id = output_var + "id_" + variation;
		std::string col_pixveto = output_var + "pixveto_" + variation;
		std::string col_csev = output_var + "csev_" + variation;

		_rlm = _rlm.Define(final_col,
			[](const std::array<double, 4>& comps) { return static_cast<float>(comps[0]); },
			{component_col});
		_rlm = _rlm.Define(col_id,
			[](const std::array<double, 4>& comps) { return static_cast<float>(comps[1]); },
			{component_col});
		_rlm = _rlm.Define(col_csev,
			[](const std::array<double, 4>& comps) { return static_cast<float>(comps[2]); },
			{component_col});
		_rlm = _rlm.Define(col_pixveto,
			[](const std::array<double, 4>& comps) { return static_cast<float>(comps[3]); },
			{component_col});
		std::cout << "[INFO] Defined photon SF column: " << final_col << std::endl;

		// _rlm = _rlm.Define(pho_smear_weight, col_ss );
		// std::cout << "[INFO] Defined photon SS column: " << col_ss << std::endl;
		
	}
	// print the defined above columns
	std::cout << "[INFO] Defined photon SF columns: " << std::endl;
	for (const auto& variation : variations)
	{
		std::string final_col = output_var + (
			variation == "sf" ? "central" :			(variation == "sfup" ? "up" : "down")
		);
		std::cout << "  - " << final_col << std::endl;
	}
	return _rlm;
}


ROOT::RDF::RNode NanoAODAnalyzerrdframe::applyPrefiringWeight(ROOT::RDF::RNode &_rlm, std::string output_var)
{

	std::vector<std::string> variations = {"Nom", "Up", "Dn"};
	std::vector<std::string> output_variations = {"central", "up", "down"};
	for (int i = 0; i < int(variations.size()); i++)
	{
		std::string input_column_name = "L1PreFiringWeight_" + variations[i];
		std::string output_column_name = output_var + output_variations[i];
		_rlm = _rlm.Define(output_column_name, input_column_name);
	}
	return _rlm;
}
//TO DO
// Apply Top PT reweighting for TTGamma, TTbar and SingleTop


bool NanoAODAnalyzerrdframe::helper_1DHistCreator(std::string hname, std::string title, const int nbins, const double xlow, const double xhi, std::string rdfvar, std::string evWeight, RNode *anode)
{
	// cout << "1DHistCreator " << hname  << endl;

	RDF1DHist histojets = anode->Histo1D({hname.c_str(), title.c_str(), nbins, xlow, xhi}, rdfvar, evWeight); // Fill with weight given by evWeight
	_th1dhistos[hname] = histojets;
	// histojets.GetPtr()->Print("all");
	return true;
}

// for 2D histograms//
bool NanoAODAnalyzerrdframe::helper_2DHistCreator(std::string hname, std::string title, const int nbinsx, const double xlow, const double xhi, const int nbinsy, const double ylow, const double yhi, std::string rdfvarx, std::string rdfvary, std::string evWeight, RNode *anode)
{
	// cout << "1DHistCreator " << hname  << endl;

	RDF2DHist histojets = anode->Histo2D({hname.c_str(), title.c_str(), nbinsx, xlow, xhi, nbinsy, ylow, yhi}, rdfvarx, rdfvary, evWeight); // Fill with weight given by evWeight
	_th2dhistos[hname] = histojets;
	// histojets.GetPtr()->Print("all");
	return true;
}


void NanoAODAnalyzerrdframe::setupCuts_and_Hists(std::string jes_unc_level_start_idx)
{
  
  
	
	cout << "setting up definitions, cuts, and histograms" <<endl;

	for ( auto &c : _varinfovector)
	{
		if (c.mincutstep.length()==0) _rlm = _rlm.Define(c.varname, c.vardefinition);
	}

	for (auto &x : _hist1dinfovector)
	{
		std::string hpost = "_nocut";

		if (x.mincutstep.length()==0)
		{
			helper_1DHistCreator(std::string(x.hmodel.fName)+hpost,  std::string(x.hmodel.fTitle)+hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname, &_rlm);
		}
	}

	//for 2D histograms
	for (auto &x : _hist2dinfovector)
	{
		std::string hpost = "_nocut";

		if (x.mincutstep.length()==0)
		{
			helper_2DHistCreator(std::string(x.hmodel.fName)+hpost,  std::string(x.hmodel.fTitle)+hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.hmodel.fNbinsY, x.hmodel.fYLow, x.hmodel.fYUp, x.varname1, x.varname2, x.weightname, &_rlm);
		}
	}


	_rnt.setRNode(&_rlm);

	for (auto acut : _cutinfovector)
	{
	  //std::cout<<acut.idx<<" this is the cut we are analysing"<<std::endl;
	  //std::cout<<jes_unc_level_start_idx<<" this is the jes_unc_level_start_idx we are analysing"<<std::endl;
	  size_t lastNonZeroPos_nominal = jes_unc_level_start_idx.find_last_not_of('0');
	  size_t lastNonZeroPos_jercunc = acut.idx.find_last_not_of('0');
	  std::string parent_idx;
	  string zeros_in_Nominal = std::string(jes_unc_level_start_idx.length() - lastNonZeroPos_nominal - 1, '0');
	  string zeros_in_jercunc = std::string(acut.idx.length() - lastNonZeroPos_jercunc - 1, '0');
	  //std::cout<<zeros_in_Nominal<<" this is the zeros_in_Nominal we are analysing"<<std::endl;
	  //std::cout<<zeros_in_jercunc<<" this is the zeros_in_jercunc we are analysing"<<std::endl;
	  /*

	  if (!acut.idx.empty() && acut.idx.front() == '0') {
	    parent_idx = acut.idx;
	  }

	  else if(!acut.idx.empty() && zeros_in_jercunc==zeros_in_Nominal){
	    parent_idx = std::string(jes_unc_level_start_idx.length() - lastNonZeroPos_nominal - 1, '0');
	  }
	  else{
	    parent_idx = acut.idx;
	  }
	  */
	  parent_idx = acut.idx;
	  //std::cout<<parent_idx.substr(0, parent_idx.length()-1) <<" this is the parent_idx we are analysing"<<std::endl;
	  std::string cutname = "cut_"+ acut.idx;
	  std::string hpost = "_"+cutname;
	  //std::cout<<"Inside setup cut for "<<hpost<<std::endl;
	  //RNode *r = _rnt.getParent(acut.idx)->getRNode();
	  RNode *r = _rnt.getParent(parent_idx)->getRNode();
	  auto rnext = new RNode(r->Define(cutname, acut.cutdefinition));
	  
	  if(_jercunctag.size()>0 && !_isData){
	    int jes_unc_idx = 1;
	    for(auto _unc_name : _jercunctag){	    
	      string jec_unc_reg_idx = jes_unc_level_start_idx.substr(0, jes_unc_level_start_idx.length()-1);
	      string jec_unc_up_reg_idx = jec_unc_reg_idx+std::to_string(jes_unc_idx);
	      string jec_unc_down_reg_idx = jec_unc_reg_idx+std::to_string(jes_unc_idx+1);
	      //string jec_unc_up_reg_idx = _unc_name+"_Up_000";
	      //string jec_unc_down_reg_idx = _unc_name+"_Down_000";
	      
	      string jet_pt_name;
	      string jet_mass_name;
	      
	      //if(acut.idx==jec_unc_up_reg_idx){
	      if (acut.idx.find(jec_unc_up_reg_idx) == 0 && 
		  (acut.idx.length() == jec_unc_up_reg_idx.length() || 
		   (acut.idx.length() > jec_unc_up_reg_idx.length() && acut.idx[jec_unc_up_reg_idx.length()] == '_'))){
		jet_pt_name = "Jet_pt_"+_unc_name+"_up";
		jet_mass_name = "Jet_mass_"+_unc_name+"_up";
		*rnext = rnext->Redefine("Jet_pt_corr",jet_pt_name);
		*rnext = rnext->Redefine("Jet_mass_corr",jet_mass_name);
	      }
	      else if (acut.idx.find(jec_unc_down_reg_idx) == 0 &&
		       (acut.idx.length() == jec_unc_down_reg_idx.length() ||
			(acut.idx.length() > jec_unc_down_reg_idx.length() && acut.idx[jec_unc_down_reg_idx.length()] == '_'))){
		//else if(acut.idx==jec_unc_down_reg_idx){
		jet_pt_name = "Jet_pt_"+_unc_name+"_down";
		jet_mass_name = "Jet_mass_"+_unc_name+"_down";
		*rnext = rnext->Redefine("Jet_pt_corr",jet_pt_name);
		*rnext = rnext->Redefine("Jet_mass_corr",jet_mass_name);

	      }
	      jes_unc_idx +=2;
	    }
	  }

	  *rnext = rnext->Filter(cutname);
	  
	  for ( auto &c : _varinfovector)
	    {
	      if (acut.idx.compare(c.mincutstep)==0) *rnext = rnext->Define(c.varname, c.vardefinition);
	    }
	  for (auto &x : _hist1dinfovector)
	    {
	      if (acut.idx.compare(0, x.mincutstep.length(), x.mincutstep)==0)
		{
		  helper_1DHistCreator(std::string(x.hmodel.fName)+hpost,  std::string(x.hmodel.fTitle)+hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname, rnext);
		}
	    }
	  
	  //for 2DHistograms
	  for (auto &x : _hist2dinfovector)
	    {
	      if (acut.idx.compare(0, x.mincutstep.length(), x.mincutstep)==0)
		{
		  helper_2DHistCreator(std::string(x.hmodel.fName)+hpost,  std::string(x.hmodel.fTitle)+hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.hmodel.fNbinsY, x.hmodel.fYLow, x.hmodel.fYUp, x.varname1, x.varname2, x.weightname, rnext);
		}
	    }
	  _rnt.addDaughter(rnext, acut.idx, parent_idx);
	  //std::cout<<acut.idx<<" Finished ####################################################"<<std::endl;
	}
}

// Automatically loop to create
/*
void NanoAODAnalyzerrdframe::setupCuts_and_Hists()
{
	cout << "setting up definitions, cuts, and histograms" << endl;

	for (auto &c : _varinfovector)
	{
		if (c.mincutstep.length() == 0)
		{
			_rlm = _rlm.Define(c.varname, c.vardefinition);
		}
	}

	for (auto &x : _hist1dinfovector)
	{
		std::string hpost = "_nocut";

		if (x.mincutstep.length() == 0)
		{
			helper_1DHistCreator(std::string(x.hmodel.fName) + hpost, std::string(x.hmodel.fTitle) + hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname, &_rlm);
		}
	}

	// for 2D histograms
	for (auto &x : _hist2dinfovector)
	{
		std::string hpost = "_nocut";

		if (x.mincutstep.length() == 0)
		{
			helper_2DHistCreator(std::string(x.hmodel.fName) + hpost, std::string(x.hmodel.fTitle) + hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.hmodel.fNbinsY, x.hmodel.fYLow, x.hmodel.fYUp, x.varname1, x.varname2, x.weightname, &_rlm);
		}
	}

	_rnt.setRNode(&_rlm);

	for (auto acut : _cutinfovector)
	{
		std::string cutname = "cut" + acut.idx;
		std::string hpost = "_" + cutname;
		auto parentNode = _rnt.getParent(acut.idx);

		if (!parentNode)
		{
			std::cerr << "Error: Parent node for cut index " << acut.idx << " is null!" << std::endl;
			continue;
		}
		RNode *r = _rnt.getParent(acut.idx)->getRNode();
		if (!r)
		{
			std::cerr << "Error: RNode for parent node of cut index " << acut.idx << " is null!" << std::endl;
			continue;
		}
		auto rnext = new RNode(r->Define(cutname, acut.cutdefinition));
		*rnext = rnext->Filter(cutname);

		for (auto &c : _varinfovector)
		{
			if (acut.idx.compare(c.mincutstep) == 0)
				*rnext = rnext->Define(c.varname, c.vardefinition);
		}
		for (auto &x : _hist1dinfovector)
		{
			if (acut.idx.compare(0, x.mincutstep.length(), x.mincutstep) == 0)
			{
				helper_1DHistCreator(std::string(x.hmodel.fName) + hpost, std::string(x.hmodel.fTitle) + hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.varname, x.weightname, rnext);
			}
		}

		// for 2DHistograms
		for (auto &x : _hist2dinfovector)
		{
			if (acut.idx.compare(0, x.mincutstep.length(), x.mincutstep) == 0)
			{
				helper_2DHistCreator(std::string(x.hmodel.fName) + hpost, std::string(x.hmodel.fTitle) + hpost, x.hmodel.fNbinsX, x.hmodel.fXLow, x.hmodel.fXUp, x.hmodel.fNbinsY, x.hmodel.fYLow, x.hmodel.fYUp, x.varname1, x.varname2, x.weightname, rnext);
			}
		}
		_rnt.addDaughter(rnext, acut.idx);
	}
*/


void NanoAODAnalyzerrdframe::registerExtraHist(std::unique_ptr<TH1D> hist)
{
	if (hist)
	{
		_extraHists.push_back(std::move(hist));
	}
}


void NanoAODAnalyzerrdframe::add1DHist(TH1DModel histdef, std::string variable, std::string weight, string mincutstep)
{
	_hist1dinfovector.push_back({histdef, variable, weight, mincutstep});
}

// for 2DHistograms
void NanoAODAnalyzerrdframe::add2DHist(TH2DModel histdef, std::string variable1, std::string variable2, std::string weight, string mincutstep)
{
	_hist2dinfovector.push_back({histdef, variable1, variable2, weight, mincutstep});
}


void NanoAODAnalyzerrdframe::drawHists(RNode t)
{
	cout << "processing" << endl;
	t.Count();
}


void NanoAODAnalyzerrdframe::addVar(varinfo v)
{
	_varinfovector.push_back(v);
}


void NanoAODAnalyzerrdframe::addVartoStore(string varname)
{
	// varname is assumed to be a regular expression.
	// e.g. if varname is "Muon_eta" then "Muon_eta" will be stored
	// if varname=="Muon_.*", then any branch name that starts with "Muon_" string will
	// be saved
	_varstostore.push_back(varname);
}


void NanoAODAnalyzerrdframe::setupTree()
{
	vector<RNodeTree *> rntends;
	_rnt.getRNodeLeafs(rntends);
	for (auto arnt : rntends)
	{
		RNode *arnode = arnt->getRNode();
		string nodename = arnt->getIndex();
		vector<string> varforthistree;
		std::map<string, int> varused;

		for (auto varname : _varstostore)
		{
			bool foundmatch = false;
			std::regex b(varname);
			for (auto a : arnode->GetColumnNames())
			{
				if (std::regex_match(a, b) && varused[a] == 0)
				{
					varforthistree.push_back(a);
					varused[a]++;
					foundmatch = true;
				}
			}
			if (!foundmatch)
			{
				cout << varname << " not found at " << nodename << endl;
			}
		}
		_varstostorepertree[nodename] = varforthistree;
	}
}


void NanoAODAnalyzerrdframe::addCuts(string cut, string idx)
{
	_cutinfovector.push_back({cut, idx});
}


void NanoAODAnalyzerrdframe::run(bool saveAll, string outtreename)
{
	auto start_time = std::chrono::high_resolution_clock::now();


	vector<RNodeTree *> rntends;
	_rnt.getRNodeLeafs(rntends);
	_rnt.Print();
	std::cout << "[INFO] Number of cut branches: " << rntends.size() << std::endl;
	cout << rntends.size() << endl;
	for (auto arnt : rntends)
	{
		string nodename = arnt->getIndex();
		string outname = _outfilename;
		if (rntends.size() > 1)
			outname.replace(outname.find(".root"), 5, "_" + nodename + ".root");
		_outrootfilenames.push_back(outname);
		RNode *arnode = arnt->getRNode();
		std::cout << "-------------------------------------------------------------------" << std::endl;
		std::cout << "cut : " << nodename << std::endl;
		cout << arnt->getIndex();
		if (saveAll)
		{
			std::cout << "[INFO] Saving all branches to: " << outname << std::endl;
			arnode->Snapshot(outtreename, outname);
			
		}
		else
		{
			cout << " --writing branches" << endl;
			std::cout << "-------------------------------------------------------------------" << std::endl;
			
			for (auto bname : _varstostorepertree[nodename])
			{
				cout << bname << endl;
				// cout << "-----branch stored" << endl;
			}

			arnode->Snapshot(outtreename, outname, _varstostorepertree[nodename]);
		}

				
		std::cout << "-------------------------------------------------------------------" << std::endl;
		cout << "Creating output root file :  " << endl;
		cout << outname << " ";
		cout << endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
		_outrootfile = new TFile(outname.c_str(), "UPDATE");
		cout << "Writing histograms...   " << endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
		for (auto &h : _th1dhistos)
		{
			if (h.second.GetPtr() != nullptr)
			{
				h.second.GetPtr()->Print();
				h.second.GetPtr()->Write();
			}
		}
		// for 2D histograms
		for (auto &h : _th2dhistos)
		{
			if (h.second.GetPtr() != nullptr)
			{
				h.second.GetPtr()->Print();
				h.second.GetPtr()->Write();
			}
		}
		for (auto &h : _extraHists)
		{
			if (h)
			{
				h->Print();
				h->Write();
			}
		}
		/*TH1F* hPDFWeights = new TH1F("LHEPdfWeightSum", "LHEPdfWeightSum", 103, 0, 1);
		for (size_t i=0; i<PDFWeights.size(); i++){
			hPDFWeights->SetBinContent(i+1, PDFWeights[i]);
		}*/
		_outrootfile->Write(0, TObject::kOverwrite);
		_outrootfile->Close();
	}
	auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << "Total time taken: " << duration << " seconds" << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;
    std::cout << "END...  :) " << std::endl;
}


void NanoAODAnalyzerrdframe::setParams(int year,string runtype, int datatype, string year_ext, bool doNominal, bool doQCD_DataDriven)
{
	if(debug){
		std::cout<< "================================//=================================" << std::endl;
		std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
		std::cout<< "================================//=================================" << std::endl;
	}
	_year = year;
	_yr_ext = year_ext;
	_runtype = runtype;
	_datatype = datatype;
	_doQCD_DataDriven = doQCD_DataDriven;
	_doNominal = doNominal;


	if (_year == 2016)
	{
		cout << "Analysing through Run 2016" << endl;
	}
	else if (_year == 2017)
	{
		cout << "Analysing through Run 2017" << endl;
	}
	else if (_year == 2018)
	{
		cout << "Analysing through Run 2018" << endl;
	}
	if (_year == 2022 && (_yr_ext == "" || _yr_ext == "EE"))
	{
		cout << "Analysing through Run 2022" << _yr_ext << endl; 
	}
	else if (_year == 2023 && (_yr_ext == "" || _yr_ext == "BPix"))
	{
		cout << "Analysing through Run 2023" << _yr_ext << endl;
	}
	else if (_year == 2024 && _yr_ext == "")
	{
		cout << "Analysing through Run 2024" << endl;
	}
	else
	{
		cout << "Year not supported! Please check the year and year extension!" << endl;
		exit(1);
	}
	
	
	if (_runtype.find("UL") != std::string::npos)
	{
		_isUL = true;
		cout << "Ultra Legacy Selected " << endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
	}
	else if (_runtype.find("ReReco") != std::string::npos)
	{
		_isReReco = true;
		cout << " ReReco  Selected!" << endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
	}
	if (!_isUL && !_isReReco)
	{
		std::cout << "Default run version : UL or ReReco is not selected! " << std::endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
	}
	if (_datatype == 0)
	{
		_isData = false;
		std::cout << " MC input files Selected!! " << std::endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
	}
	else if (_datatype == 1)
	{
		_isData = true;
		std::cout << " DATA input files Selected!!" << std::endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
	}
	if (_datatype == -1)
	{
		std::cout << "Default root version :checking out gen branches! " << std::endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;

		// if (_atree->GetBranch("genWeight") == nullptr)
		if (_atree->GetBranch("Generator_weight") == nullptr)
		{
			// add print statement
			std::cout << "Generator_weight branch is not found in the input file!! " << std::endl;
			// std::cout<< "genWeight branch is not found in the input file!! "<< std::endl;
			_isData = true;
			cout << "input file is DATA" << endl;
		}
		else
		{
			_isData = false;
			cout << "input file is MC" << endl;
		}
	}
	TObjArray *allbranches = _atree->GetListOfBranches();
	for (int i = 0; i < allbranches->GetSize(); i++)
	{
		TBranch *abranch = dynamic_cast<TBranch *>(allbranches->At(i));
		if (abranch != nullptr)
		{
			// cout << abranch->GetName() << endl;
			_originalvars.push_back(abranch->GetName());
		}
	}
}

// Checking HLTs in the input root file
std::string NanoAODAnalyzerrdframe::setHLT(std::string str_HLT)
{
	if (debug)
	{
		std::cout << "================================//=================================" << std::endl;
		std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
		std::cout << "================================//=================================" << std::endl;
	}
	if (str_HLT != "")
	{

		bool ctrl_HLT = isDefined(str_HLT);
		std::string output;
		if (ctrl_HLT)
		{
			output = str_HLT;
			std::cout << "HLT : " << str_HLT << " : SUCCESSFULLY FOUND!!" << std::endl;
		}
		else
		{
			std::cout << "HLT : " << str_HLT << " : CAN NOT BE FOUND " << std::endl;
			std::cout << "Check HLT branches in the input root file!!" << std::endl;
			std::cout << "EXITING PROGRAM!!" << std::endl;

			exit(1);
		}
		return output;
	}
	else
	{ // fill the HLT names in a vector according to each year
		std::vector<string> V_output;
		if (_year == 2016)
		{
			HLTGlobalNames = HLT2016Names;
		}
		else if (_year == 2017)
		{
			HLTGlobalNames = HLT2017Names;
		}
		else if (_year == 2018)
		{
			HLTGlobalNames = HLT2018Names;
		}
		else if (_year == 2022 )
		{
			HLTGlobalNames = HLT2022Names;
		}
		else if (_year == 2023)
		{
			HLTGlobalNames = HLT2023Names;
		}


		// loop on HLTs
		for (size_t i = 0; i < HLTGlobalNames.size(); i++)
		{
			/* code */
			bool ctrl_HLT = isDefined(HLTGlobalNames[i]);
			if (ctrl_HLT)
			{
				V_output.push_back(HLTGlobalNames[i]);
			}
		}
		std::string output_HLT;
		if (!V_output.empty())
		{
			for (size_t i = 0; i < V_output.size(); i++)
			{
				if (i != V_output.size() - 1)
				{
					output_HLT += V_output[i] + "==1 || ";
				}
				else
				{
					output_HLT += V_output[i] + "==1 ";
				}
			}
		}
		else
		{
			std::cout << " Not matched with any HLT Triggers! Please check the HLT Names in the inputfile " << std::endl;
			std::cout << "EXITING PROGRAM!!" << std::endl;
			exit(1);
		}
		std::cout << " HLT names =  " << output_HLT << std::endl;
		return output_HLT;
	}
}

// control all branch names using in the addCuts function
std::string NanoAODAnalyzerrdframe::ctrlBranchName(std::string str_Branch)
{

	if (debug)
	{
		std::cout << "================================//=================================" << std::endl;
		std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
		std::cout << "================================//=================================" << std::endl;
	}

	bool ctrl_Branch = isDefined(str_Branch);
	std::string output;
	if (ctrl_Branch)
	{
		output = str_Branch;
	}
	else
	{
		std::cout << "Branch : " << str_Branch << " : CAN NOT BE FOUND " << std::endl;
		std::cout << "Check your branches in the input root file!!" << std::endl;
		std::cout << "EXITING PROGRAM!!" << std::endl;

		exit(1);
	}
	return output;
}

// cut-based ID bitmap, Fall17V2, (0:fail, 1:loose, 2:medium, 3:tight)
std::string NanoAODAnalyzerrdframe::PhotonID(int cutbasedID)
{

	if (debug)
	{
		std::cout << "================================//=================================" << std::endl;
		std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
		std::cout << "================================//=================================" << std::endl;
	}

	// Print debug information based on the Photon ID
	if (cutbasedID == 1)
		std::cout << "LOOSE Photon ID requested   == " << cutbasedID << std::endl;
	if (cutbasedID == 2)
		std::cout << "MEDIUM Photon ID requested  == " << cutbasedID << std::endl;
	if (cutbasedID == 3)
		std::cout << "TIGHT Photon ID requested   == " << cutbasedID << std::endl;
	std::cout << "-------------------------------------------------------------------" << std::endl;

	// Validate the cutbasedID input
	if (cutbasedID < 0 || cutbasedID > 3)
	{
		std::cout << "ERROR!! Wrong Photon ID requested  == " << cutbasedID << "!! Can't be applied" << std::endl;
		std::cout << "Please select PhotonID from 1 to 3 " << std::endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
		std::cout << "EXITING PROGRAM!!" << std::endl;
		exit(1);
	}

	// Construct the selection string based on the cut-based ID
	// Changed the Photon_cutBased == %d to Photon_cutBased >= %d 
	std::string output = Form("Photon_cutBased >= %d", cutbasedID);

	return output;
}

// cut-based ID Fall17 V2 (0:fail, 1:veto, 2:loose, 3:medium, 4:tight)
std::string NanoAODAnalyzerrdframe::ElectronID(int cutbasedID)
{

	if (debug)
	{
		std::cout << "================================//=================================" << std::endl;
		std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
		std::cout << "================================//=================================" << std::endl;
	}
	// double Electron_eta;
	// double Electron_pt;
	if (cutbasedID == 1)
		std::cout << " VETO Electron ID requested    == " << cutbasedID << std::endl;
	if (cutbasedID == 2)
		std::cout << " LOOSE Electron ID requested   == " << cutbasedID << std::endl;
	if (cutbasedID == 3)
		std::cout << " MEDIUM Electron ID requested  == " << cutbasedID << std::endl;
	if (cutbasedID == 4)
		std::cout << " TIGHT Electron ID requested   == " << cutbasedID << std::endl;
	std::cout << "-------------------------------------------------------------------" << std::endl;

	if (cutbasedID < 0 || cutbasedID > 4)
	{
		std::cout << "ERROR!! Wrong Electron ID requested  == " << cutbasedID << "!! Can't be applied" << std::endl;
		std::cout << "Please select ElectronID from 1 to 4 " << std::endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
		std::cout << "EXITING PROGRAM!!" << std::endl;
		exit(1);
	}
	/*if(_year==2018 && _isUL){
		if(cutbasedID==2 ){
			Electron_eta=2.5;
			Electron_pt=10;
		}else if (cutbasedID==3){
			Electron_eta=2.4;
			Electron_pt=10;
		}
	}*/

	// Rdataframe look for the variables in the intput Ttree..
	std::string output = Form("Electron_cutBased >= %d ", cutbasedID);
	// std::string output = Form("Electron_cutBased == %d &&  abs(Electron_eta)<%f && Electron_pt<%f",cutbasedID,  Electron_eta, Electron_pt);

	return output;
}


std::string NanoAODAnalyzerrdframe::MuonID(int cutbasedID)
{

	if (debug)
	{
		std::cout << "================================//=================================" << std::endl;
		std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
		std::cout << "================================//=================================" << std::endl;
	}

	//  cut-based ID Fall17 V2 (0:fail, 1:veto, 2:loose, 3:medium, 4:tight)
	if (cutbasedID == 1)
		std::cout << " Veto Muon ID requested   == " << cutbasedID << std::endl;
	if (cutbasedID == 2)
		std::cout << " LOOSE Muon ID requested  == " << cutbasedID << std::endl;
	if (cutbasedID == 3)
		std::cout << " MEDIUM Muon ID requested == " << cutbasedID << std::endl;
	if (cutbasedID == 4)
		std::cout << " TIGHT Muon ID requested as == " << cutbasedID << std::endl;
	std::cout << "-------------------------------------------------------------------" << std::endl;

	if (cutbasedID < 1 || cutbasedID > 4)
	{
		std::cout << "ERROR!! Wrong Muon ID requested  == " << cutbasedID << "!! Can't be applied" << std::endl;
		std::cout << "Please select Muon ID from 2 to 4 " << std::endl;
		std::cout << "-------------------------------------------------------------------" << std::endl;
		std::cout << "EXITING PROGRAM!!" << std::endl;

		exit(1);
	}

	string Muon_cutBased_ID;

	if (cutbasedID == 1)
	{
		// Muon_cutBased_ID = "Muon_looseId";
		Muon_cutBased_ID = "Muon_isPFcand && (Muon_isGlobal || Muon_isTracker)";
		// std::cout << " VETO Muon ID requested == " << cutbasedID << ", but it doesn't exist in the nanoAOD branches. It is moved to loose MuonID. " << cutbasedID << std::endl;
	}
	if (cutbasedID == 2)
	{
		Muon_cutBased_ID = "Muon_looseId";
	}
	else if (cutbasedID == 3)
	{
		Muon_cutBased_ID = "Muon_mediumId";
	}
	else if (cutbasedID == 4)
	{
		Muon_cutBased_ID = "Muon_tightId";
	}
	string output;
	output = Form("%s==true", Muon_cutBased_ID.c_str());
	return output;
}



std::string NanoAODAnalyzerrdframe::JetID(int cutbasedID)
{
	if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    std::string output;

	if (cutbasedID == 6) {
		// Apply bugfix recipe from POG : https://twiki.cern.ch/twiki/bin/viewauth/CMS/JetID13p6TeV#nanoAOD_Flags
		if (_year== 2022 || _year == 2023) {
			output =
				"( (abs(Jet_eta) <= 2.7 && ((Jet_jetId & (1 << 1)) != 0)) || "
				"(abs(Jet_eta) > 2.7 && abs(Jet_eta) <= 3.0 && ((Jet_jetId & (1 << 1)) != 0) && Jet_neHEF < 0.99) || "
				"(abs(Jet_eta) > 3.0 && ((Jet_jetId & (1 << 1)) != 0) && Jet_neEmEF < 0.4) )"
				" && Jet_muEF < 0.8 && Jet_chEmEF < 0.8";
		}
		else if (_year == 2024) {
			output =
				// "( (abs(Jet_eta) <= 2.6 && Jet_neHEF < 0.99 && Jet_neEmEF < 0.9 && (Jet_chMultiplicity+Jet_neMultiplicity) > 1 && Jet_chHEF > 0.01 && Jet_chMultiplicity > 0) || "
				// "(abs(Jet_eta) > 2.6 && abs(Jet_eta) <= 2.7 && Jet_neHEF < 0.90 && Jet_neEmEF < 0.99) || "
				// "(abs(Jet_eta) > 2.7 && abs(Jet_eta) <= 3.0 && Jet_neHEF < 0.99) || "
				// "(Jet_neMultiplicity >= 2 && Jet_neEmEF < 0.4) )";

				"( (abs(Jet_eta) <= 2.7 && ((Jet_jetId & (1 << 1)) != 0)) || "
				"(abs(Jet_eta) > 2.7 && abs(Jet_eta) <= 3.0 && ((Jet_jetId & (1 << 1)) != 0) && Jet_neHEF < 0.99) || "
				"(abs(Jet_eta) > 3.0 && ((Jet_jetId & (1 << 1)) != 0) && Jet_neEmEF < 0.4) )"
				" && Jet_muEF < 0.8 && Jet_chEmEF < 0.8";
		}
	} else if (cutbasedID == 2) {
        output = "Jet_jetId == 2";  // old use case
    } else {
        std::cerr << "Unsupported JetID requested: " << cutbasedID << std::endl;
    }

    return output;
}


void NanoAODAnalyzerrdframe::applyElectronScaleAndSmear()
{
    std::cout << "Apply Electron Pt correction" << std::endl;

    if (!_correction_electronss) {
        std::cerr << "Electron corrections file not loaded!" << std::endl;
        return;
    }

    using ROOT::VecOps::RVec;
    using floats = RVec<float>;

    // Compute SCeta for electrons
    _rlm = _rlm.Define("ElectronSC", [](const floats &eta, const floats &deltaEtaSC) {
        floats sc(eta.size());
        for (size_t i = 0; i < eta.size(); ++i)
            sc[i] = std::abs(eta[i] + deltaEtaSC[i]);
        return sc;
    }, {"Electron_eta", "Electron_deltaEtaSC"});

    // ----------------------------------
    // Data: Apply Scale Correction
    // ----------------------------------
    if (_isData) {
        auto scale_lambda = [this](const floats &pt,
                                   const floats &scEta,
                                   const floats &r9_raw,
                                   const ROOT::VecOps::RVec<UChar_t> &seedGain,
                                   const ROOT::VecOps::RVec<UInt_t> &run) -> floats {
            floats corrected_pt;
            corrected_pt.reserve(pt.size());

            std::string key_scale;
            bool uses_abs_eta = true;
            if (_year == 2022 && _yr_ext == "")         key_scale = "EGMScale_Compound_Ele_2022preEE";
            else if (_year == 2022 && _yr_ext == "EE")  key_scale = "EGMScale_Compound_Ele_2022postEE";
            else if (_year == 2023 && _yr_ext == "")    key_scale = "EGMScale_Compound_Ele_2023preBPIX";
            else if (_year == 2023 && _yr_ext == "BPix")key_scale = "EGMScale_Compound_Ele_2023postBPIX";
            else if (_year == 2024 && _yr_ext == "") {
                key_scale = "Scale";
                uses_abs_eta = false;
            }
            else throw std::runtime_error("[Electron Scale] Unknown year/extension");

            for (size_t i = 0; i < pt.size(); ++i) {
                float r9 = r9_raw[i] < 0.0f ? 0.0f : r9_raw[i];
                try {
                    float abs_eta = std::fabs(scEta[i]);
                    float wt = 1.0f;
                    if (uses_abs_eta) {
                        wt = _correction_electronss->compound().at(key_scale)->evaluate(
                            {"scale", static_cast<double>(run[i]), scEta[i], r9, abs_eta, pt[i], static_cast<double>(seedGain[i])});
                    } else {
                        wt = _correction_electronss->compound().at(key_scale)->evaluate(
                            {"scale", static_cast<double>(run[i]), scEta[i], r9, pt[i], static_cast<double>(seedGain[i])});
                    }
                    corrected_pt.emplace_back(pt[i] * wt);
                } catch (const std::exception &e) {
                    std::cerr << "[Electron Scale ERROR] " << e.what()
                              << " (eta=" << scEta[i] << ", r9=" << r9 << ")" << std::endl;
                    corrected_pt.emplace_back(pt[i]);
                }
            }
            return corrected_pt;
        };

        _rlm = _rlm.Define("Electron_pt_corr", scale_lambda,
                           {"Electron_pt", "ElectronSC", "Electron_r9", "Electron_seedGain", "run"});
    }

    // ----------------------------------
    // MC: Apply Smearing Correction
    // ----------------------------------
    else {
        auto smear_lambda = [this](const floats &pt,
                                   const floats &scEta,
                                   const floats &r9_raw) -> std::tuple<floats, floats, floats, floats, floats> {
            floats nominal_pt, smear_up_pt, smear_down_pt, scale_up_pt, scale_down_pt;
            nominal_pt.reserve(pt.size());
            smear_up_pt.reserve(pt.size());
            smear_down_pt.reserve(pt.size());
			scale_up_pt.reserve(pt.size());
			scale_down_pt.reserve(pt.size());

            std::string key_smear;
            if (_year == 2022 && _yr_ext == "")         key_smear = "EGMSmearAndSyst_ElePTsplit_2022preEE";
            else if (_year == 2022 && _yr_ext == "EE")  key_smear = "EGMSmearAndSyst_ElePTsplit_2022postEE";
            else if (_year == 2023 && _yr_ext == "")    key_smear = "EGMSmearAndSyst_ElePTsplit_2023preBPIX";
            else if (_year == 2023 && _yr_ext == "BPix")key_smear = "EGMSmearAndSyst_ElePTsplit_2023postBPIX";
			else if (_year == 2024 && _yr_ext == "")    key_smear = "EGMSmearAndSyst_ElePT_2024";
            else throw std::runtime_error("[Electron Smearing] Unknown year/extension");

            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> gauss(0.0, 1.0);

            for (size_t i = 0; i < pt.size(); ++i) {
				float r9 = r9_raw[i] < 0.0f ? 0.0f : r9_raw[i];
                try {
                    float abs_eta = std::fabs(scEta[i]);
                    float smear      = _correction_electronss->at(key_smear)->evaluate({"smear", pt[i], r9, abs_eta});
                    float smear_up   = _correction_electronss->at(key_smear)->evaluate({"smear_up", pt[i], r9, abs_eta});
                    float smear_down = _correction_electronss->at(key_smear)->evaluate({"smear_down", pt[i], r9, abs_eta});
					float escale     = _correction_electronss->at(key_smear)->evaluate({"escale", pt[i], r9, abs_eta});

                    float rand = gauss(gen);

                    float pt_nominal = pt[i] * (1.0 + smear * rand);
                    float pt_up      = pt[i] * (1.0 + smear_up * rand);
                    float pt_down    = pt[i] * (1.0 + smear_down * rand);

					float pt_scaleUp = pt_nominal * (1.0 + escale);
					float pt_scaleDown = pt_nominal * (1.0 - escale);

                    nominal_pt.emplace_back(pt_nominal);
                    smear_up_pt.emplace_back(pt_up);
                    smear_down_pt.emplace_back(pt_down);
					scale_up_pt.emplace_back(pt_scaleUp);
					scale_down_pt.emplace_back(pt_scaleDown);

                } catch (const std::exception &e) {
                    std::cerr << "[Electron Smearing ERROR] " << e.what()
                              << " (eta=" << scEta[i] << ", r9=" << r9 << ")" << std::endl;
                    nominal_pt.emplace_back(pt[i]);
                    smear_up_pt.emplace_back(pt[i]);
                    smear_down_pt.emplace_back(pt[i]);
					scale_up_pt.emplace_back(pt[i]);
					scale_down_pt.emplace_back(pt[i]);
                }
            }

            return std::make_tuple(nominal_pt, smear_up_pt, smear_down_pt, scale_up_pt, scale_down_pt);
        };

			_rlm = _rlm.Define("Electron_pt_corr_packed", smear_lambda,
					{"Electron_pt", "ElectronSC", "Electron_r9"})
			.Define("Electron_pt_corr",          "std::get<0>(Electron_pt_corr_packed)")
			.Define("Electron_pt_corr_smearUp",  "std::get<1>(Electron_pt_corr_packed)")
			.Define("Electron_pt_corr_smearDown","std::get<2>(Electron_pt_corr_packed)")
			.Define("Electron_pt_corr_scaleUp",  "std::get<3>(Electron_pt_corr_packed)")
			.Define("Electron_pt_corr_scaleDown","std::get<4>(Electron_pt_corr_packed)");
	}
}


void NanoAODAnalyzerrdframe::applyPhotonScaleAndSmear()
{
    std::cout << "Apply Photon Pt correction" << std::endl;

    if (!_correction_photonss) {
        std::cerr << "Photon corrections file not loaded!" << std::endl;
        return;
    }

    using ROOT::VecOps::RVec;
    using floats = RVec<float>;

	_rlm = _rlm.Define("Photons_SCeta", computePhotonSCEta,
        {"Photon_eta", "Photon_phi", "Photon_isScEtaEB", "Photon_isScEtaEE", "PV_x", "PV_y", "PV_z"});
	
	// ------------------------------
    // Apply Scale for Data
    // ------------------------------
    if (_isData) {
		auto scale_lambda = [this](const floats &pt,
		                           const floats &scEta,
		                           const floats &r9_raw,
		                           const ROOT::VecOps::RVec<UChar_t> &seedGain,
		                           const ROOT::VecOps::RVec<UInt_t> &run) -> floats {
			floats result;
			result.reserve(pt.size());

			std::string key_scale;
			bool uses_abs_eta = true;
			if (_year == 2022 && _yr_ext == "")         key_scale = "EGMScale_Compound_Pho_2022preEE";
			else if (_year == 2022 && _yr_ext == "EE")  key_scale = "EGMScale_Compound_Pho_2022postEE";
			else if (_year == 2023 && _yr_ext == "")    key_scale = "EGMScale_Compound_Pho_2023preBPIX";
			else if (_year == 2023 && _yr_ext == "BPix")key_scale = "EGMScale_Compound_Pho_2023postBPIX";
			else if (_year == 2024 && _yr_ext == "") {
				key_scale = "Scale";
				uses_abs_eta = false;
			}
			else throw std::runtime_error("[Photon Scale] Unknown year/extension");

			for (size_t i = 0; i < pt.size(); ++i) {
				const float r9 = r9_raw[i] < 0.0f ? 0.0f : r9_raw[i];
				try {
					float wt = 1.0f;
					if (uses_abs_eta) {
						float abs_eta = std::fabs(scEta[i]);
						wt = _correction_photonss->compound().at(key_scale)->evaluate({
							"scale",
							static_cast<double>(run[i]),
							scEta[i],
							r9,
							abs_eta,
							pt[i],
							static_cast<double>(seedGain[i])
						});
					} else {
						wt = _correction_photonss->compound().at(key_scale)->evaluate({
							"scale",
							static_cast<double>(run[i]),
							scEta[i],
							r9,
							pt[i],
							static_cast<double>(seedGain[i])
						});
					}

					result.emplace_back(pt[i] * wt);
				} catch (const std::exception &e) {
					std::cerr << "[Photon Scale ERROR] " << e.what()
					          << " (SCeta=" << scEta[i] << ", r9=" << r9
					          << ", pt=" << pt[i] << ", gain=" << (int)seedGain[i] << ")" << std::endl;
					result.emplace_back(pt[i]);
				}
			}

			return result;
		};

		_rlm = _rlm.Define("Photon_pt_corr", scale_lambda,
		                   {"Photon_pt", "Photons_SCeta", "Photon_r9", "Photon_seedGain", "run"});
	}
	
	// ------------------------------
    // Apply Smearing for MC
    // ------------------------------
    else {
        auto smear_lambda = [this](const floats &pt,
                                   const floats &scEta,
                                   const floats &r9_raw) -> std::tuple<floats, floats, floats, floats, floats> {
            floats nominal_pt, smear_up_pt, smear_down_pt, scale_up_pt, scale_down_pt;
            nominal_pt.reserve(pt.size());
            smear_up_pt.reserve(pt.size());
            smear_down_pt.reserve(pt.size());
			scale_up_pt.reserve(pt.size());
			scale_down_pt.reserve(pt.size());

            std::string key_smear;
            if (_year == 2022 && _yr_ext == "")         key_smear = "EGMSmearAndSyst_PhoPTsplit_2022preEE";
            else if (_year == 2022 && _yr_ext == "EE")  key_smear = "EGMSmearAndSyst_PhoPTsplit_2022postEE";
            else if (_year == 2023 && _yr_ext == "")    key_smear = "EGMSmearAndSyst_PhoPTsplit_2023preBPIX";
            else if (_year == 2023 && _yr_ext == "BPix")key_smear = "EGMSmearAndSyst_PhoPTsplit_2023postBPIX";
			else if (_year == 2024 && _yr_ext == "")    key_smear = "EGMSmearAndSyst_PhoPT_2024";
            else throw std::runtime_error("[Photon Smearing] Unknown year/extension");

            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> gauss(0.0, 1.0);

            for (size_t i = 0; i < pt.size(); ++i) {
				float r9 = r9_raw[i] < 0.0f ? 0.0f : r9_raw[i];
                try {
                    float abs_eta = std::fabs(scEta[i]);
                    float smear      = _correction_photonss->at(key_smear)->evaluate({"smear", pt[i], r9, abs_eta});
                    float smear_up   = _correction_photonss->at(key_smear)->evaluate({"smear_up", pt[i], r9, abs_eta});
                    float smear_down = _correction_photonss->at(key_smear)->evaluate({"smear_down", pt[i], r9, abs_eta});
					float escale     = _correction_photonss->at(key_smear)->evaluate({"escale", pt[i], r9, abs_eta});

                    float rand = gauss(gen);

                    float pt_nominal = pt[i] * (1.0 + smear * rand);
                    float pt_up      = pt[i] * (1.0 + smear_up * rand);
                    float pt_down    = pt[i] * (1.0 + smear_down * rand);
					float pt_scaleUp = pt_nominal * (1.0 + escale);
					float pt_scaleDown = pt_nominal * (1.0 - escale);

                    nominal_pt.emplace_back(pt_nominal);
                    smear_up_pt.emplace_back(pt_up);
                    smear_down_pt.emplace_back(pt_down);
					scale_up_pt.emplace_back(pt_scaleUp);
					scale_down_pt.emplace_back(pt_scaleDown);

                } catch (const std::exception &e) {
                    std::cerr << "[Photon Smearing ERROR] " << e.what()
                              << " (eta=" << scEta[i] << ", r9=" << r9 << ")" << std::endl;
                    nominal_pt.emplace_back(pt[i]);
                    smear_up_pt.emplace_back(pt[i]);
                    smear_down_pt.emplace_back(pt[i]);
					scale_up_pt.emplace_back(pt[i]);
					scale_down_pt.emplace_back(pt[i]);
                }
            }

            return std::make_tuple(nominal_pt, smear_up_pt, smear_down_pt, scale_up_pt, scale_down_pt);
        };

        _rlm = _rlm.Define("Photon_pt_corr_corr_packed", smear_lambda,
                           {"Photon_pt", "Photons_SCeta", "Photon_r9"})
                   .Define("Photon_pt_corr",        "std::get<0>(Photon_pt_corr_corr_packed)")
                   .Define("Photon_pt_corr_smearUp","std::get<1>(Photon_pt_corr_corr_packed)")
                   .Define("Photon_pt_corr_smearDown","std::get<2>(Photon_pt_corr_corr_packed)")
				   .Define("Photon_pt_corr_scaleUp","std::get<3>(Photon_pt_corr_corr_packed)")
				   .Define("Photon_pt_corr_scaleDown","std::get<4>(Photon_pt_corr_corr_packed)");
    }
}


void NanoAODAnalyzerrdframe::topPtReweight() {

	_rlm = _rlm.Define("gentopcut", "abs(GenPart_pdgId) == 6 && GenPart_statusFlags & (1 << 13)")
				 .Define("GenPart_top_pt", "GenPart_pt[gentopcut]");
  
	auto topPtRun3 = [](const floats &toppt) -> floats {
	  floats out;
	  out.reserve(3);
  
	  if (toppt.size() != 2) {
		// if we didn't find two tops, just return unity weights
		out = {1.0, 1.0, 1.0};
	  } else {
		float pt1 = toppt[0];
		float pt2 = toppt[1];
  
		// --- 13 TeV NNLO–NLO correction ---
		auto sf13 = [](float pt) {
		  return 0.103 * std::exp(-0.0118 * pt) - 0.000134 * pt + 0.973;
		};
  
		// --- 13.6 TeV extrapolation ---
		auto sf136 = [](float pt) {
		  return 0.992469 + 0.0000607335 * pt;
		};
  
		float w1 = sf13(pt1) * sf136(pt1);
		float w2 = sf13(pt2) * sf136(pt2);
  
		float nom_weight = std::sqrt(w1 * w2);

		float delta = std::fabs(nom_weight - 1.0);
		float up    = 1.0 + delta;
		float down  = 1.0 - delta;
  
		out = {nom_weight, up, down};
	  }
  
	  return out;
	};
  
	if (_outfilename.find("TTGamma_PTG10to100") != std::string::npos ||
	    _outfilename.find("TTGamma_PTG100to200") != std::string::npos ||
	    _outfilename.find("TTGamma_PTG200") != std::string::npos ||
	    _outfilename.find("TTbar_Hadronic") != std::string::npos ||
	    _outfilename.find("TTbar_SemiLept") != std::string::npos ||
	    _outfilename.find("TTbar_Dilept") != std::string::npos)
	{
    	_rlm = _rlm.Define("TopPtWeight", topPtRun3, {"GenPart_top_pt"});
	} else 
	{
		_rlm = _rlm.Define("TopPtWeight", "floats v{1.0, 1.0, 1.0}; return v;");
	}

}
  
