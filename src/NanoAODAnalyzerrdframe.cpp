/*
 * NanoAODAnalyzerrdframe.cpp
 *
 *  Created on: Sep 30, 2018
 *      Author: suyong
 *  Updated on: 10Oct, 2023
 *      Author: Arnab PUROHIT, IP2I, Lyon 
 */

#include "NanoAODAnalyzerrdframe.h"
#include <iostream>
#include <algorithm>
#include <typeinfo>
#include <random>

#include "TCanvas.h"
#include "Math/GenVector/VectorUtil.h"
#include <vector>
#include <fstream>
#include "utility.h"
#include <regex>
#include "ROOT/RDFHelpers.hxx"
#include <TRandom3.h>
correction::CorrectionSet* muon_scalsmear_corrector = nullptr;
#include "MuonScaRe.cc"
using namespace std;

NanoAODAnalyzerrdframe::NanoAODAnalyzerrdframe(TTree *atree, std::string outfilename)
:_rd(*atree), _jsonOK(false),_outfilename(outfilename)
	, _outrootfile(0), _rlm(_rd)
	, _rnt(&_rlm) //PDFWeights(103, 0.0) 
{
	_atree=atree;
	//cout<< " run year=====" << _year <<endl;
	// if genWeight column exists, then it is not real data

}

NanoAODAnalyzerrdframe::~NanoAODAnalyzerrdframe() {
	// TODO Auto-generated destructor stub
}

bool NanoAODAnalyzerrdframe::isDefined(string v)
{
	auto result = std::find(_originalvars.begin(), _originalvars.end(), v);
	if (result != _originalvars.end()) return true;
	else return false;
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

	this->setupAnalysis();
}


void NanoAODAnalyzerrdframe::setupAnalysis()
{
	// Event weight for data it's always one. For MC, it depends on the sign
 	//cout<<"year===="<< _year<< "==runtype=== " <<  _runtype <<endl;
	_rlm = _rlm.Define("one", "1.0");
	if (_isData && !isDefined("evWeight"))
	{
		_rlm = _rlm.Define("evWeight", [](){
				return 1.0;
			}, {} );
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
	

	setupCuts_and_Hists();
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
				for (auto &v: jsonroot[key])
				{
					if (v[0]<=lumisection && lumisection <=v[1]) goodeventflag = true;
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


void NanoAODAnalyzerrdframe::setupJetMETCorrection(string fname, string jettag,string jettagMC,string JER_tag,string JER_tag_res) //data
{

    cout << "SETUP JETMET correction" << endl;
	_correction_jerc = correction::CorrectionSet::from_file(fname);//jercfname=json
	assert(_correction_jerc->validate()); 
	cout<<"JERC JSON file : " << fname<<endl;
    if (_isData){
        _jetCorrector = _correction_jerc->compound().at(jettag);//jerctag#JSON (JEC,compound)compoundLevel="L1L2L3Res"
    }
    else {
        cout<<"JERC JSON file : " << fname<<endl;
        _jetCorrector = _correction_jerc->compound().at(jettagMC);
    }
	cout<< "JET tag in JSON : " << jettag << endl;
    for (const auto& tag : _jercunctag){
        _jetCorrectionUnc.emplace_back(tag, _correction_jerc->at(tag));
    }
    for (const auto& tag : _jercunctag) {
    cout << "JET uncertainty tag in JSON : " << tag << endl;
    }
    cout<< "JER tag in json: " << JER_tag << endl;
    _jer_corrector = _correction_jerc->at(JER_tag);
    _jer_resolution = _correction_jerc->at(JER_tag_res);
	std::cout<< "================================//=================================" << std::endl;
}



void NanoAODAnalyzerrdframe::applyJetMETCorrections()
{
    std::cout << "Applying JET/MET corrections" << std::endl;

    using ROOT::VecOps::RVec;
    using floats = RVec<float>;

    //------------------------------------------------------------------
    // MET Propagation (Type-1 PUPPI MET correction)
    //------------------------------------------------------------------
    auto propagatePuppiMET = [](
            floats jetpt_corr,
            floats jeteta,
            floats jetphi,
            floats jetrawf,
            floats jetpt_nano,
            float  met_pt,
            float  met_phi) -> floats
    {
        float met_x = met_pt * std::cos(met_phi);
        float met_y = met_pt * std::sin(met_phi);

        for (size_t i = 0; i < jetpt_corr.size(); i++)
        {
            float rawpt   = jetpt_nano[i] * (1.f - jetrawf[i]);
            float corr_pt = jetpt_corr[i];

            if (corr_pt < 15.f || std::abs(jeteta[i]) > 5.2f) continue;

            float dpx = (corr_pt - rawpt) * std::cos(jetphi[i]);
            float dpy = (corr_pt - rawpt) * std::sin(jetphi[i]);

            met_x -= dpx;
            met_y -= dpy;
        }

        return floats{ std::sqrt(met_x*met_x + met_y*met_y),
                       std::atan2(met_y, met_x) };
    };

    //------------------------------------------------------------------
    // Helper to define a MET variant column pair from a jet pT column
    //------------------------------------------------------------------
    auto defineMETVariant = [&](const std::string& jetCol,
                                const std::string& metPtCol,
                                const std::string& metPhiCol)
    {
        std::string vecCol = metPtCol + "_vec";

        _rlm = _rlm.Define(vecCol,
            [propagatePuppiMET](floats jpt_corr, floats jeta, floats jphi,
                                floats jrawf,    floats jpt,
                                float  met_pt,   float  met_phi)
            {
                return propagatePuppiMET(jpt_corr, jeta, jphi, jrawf, jpt, met_pt, met_phi);
            },
            {jetCol, "Jet_eta", "Jet_phi", "Jet_rawFactor",
             "Jet_pt", "PuppiMET_pt", "PuppiMET_phi"});

        _rlm = _rlm.Define(metPtCol,  vecCol + "[0]");
        _rlm = _rlm.Define(metPhiCol, vecCol + "[1]");

        std::cout << "Defined MET columns: " << metPtCol
                  << ", " << metPhiCol << std::endl;
    };

    if (_jetCorrector == nullptr) return;

    //------------------------------------------------------------------
    // DATA path
    //------------------------------------------------------------------
    if (_isData)
    {
        // Vectorised run branch needed by the corrector
        _rlm = _rlm.Define("run_f",
            [](unsigned int run, const floats& jetpts) {
                return floats(jetpts.size(), float(run));
            },
            {"run", "Jet_pt"});

        auto jetCorrLambda_Data =
            [this](floats jetpts,
                   floats jetetas,
                   floats jetAreas,
                   floats jetrawf,
                   float  rho,
                   floats jetphis,
                   floats run_f) -> floats
        {
            floats out;
            out.reserve(jetpts.size());

            for (size_t i = 0; i < jetpts.size(); i++)
            {
                float rawpt = jetpts[i] * (1.f - jetrawf[i]);
                float corr;

                if ( _year == 2024)
                    corr = _jetCorrector->evaluate({jetAreas[i], jetetas[i], rawpt, rho, jetphis[i], run_f[i]});
                else
                    corr = _jetCorrector->evaluate({jetAreas[i], jetetas[i], rawpt, rho});
                out.emplace_back(rawpt * corr);
            }
            return out;
        };

        _rlm = _rlm.Define("Jet_pt_corr",
            jetCorrLambda_Data,
            {"Jet_pt", "Jet_eta", "Jet_area", "Jet_rawFactor",
             "Rho_fixedGridRhoFastjetAll", "Jet_phi", "run_f"});

        // Nominal MET propagation only — no JER/JEC systematics for Data
        defineMETVariant("Jet_pt_corr", "PuppiMET_pt_corr", "PuppiMET_phi_corr");
    }
    //------------------------------------------------------------------
    // MC path
    //------------------------------------------------------------------
    else
    {
        // ------------------------------------------
        // Step 1: JEC
        // ------------------------------------------
        auto jetCorrLambda_MC =
            [this](floats jetpts,
                   floats jetetas,
                   floats jetAreas,
                   floats jetrawf,
                   floats jetphis,
                   float  rho) -> floats
        {
            floats out;
            out.reserve(jetpts.size());

            for (size_t i = 0; i < jetpts.size(); i++)
            {
                float rawpt = jetpts[i] * (1.f - jetrawf[i]);
                float corr  = (_year == 2023 || _year == 2024)
                    ? _jetCorrector->evaluate({jetAreas[i], jetetas[i], rawpt, rho, jetphis[i]})
                    : _jetCorrector->evaluate({jetAreas[i], jetetas[i], rawpt, rho});

                out.emplace_back(rawpt * corr);
            }
            return out;
        };

        _rlm = _rlm.Define("Jet_pt_JEC",
            jetCorrLambda_MC,
            {"Jet_pt", "Jet_eta", "Jet_area", "Jet_rawFactor",
             "Jet_phi", "Rho_fixedGridRhoFastjetAll"});

        // ------------------------------------------
        // Step 2: JER smearing (nom / up / down)
        // ------------------------------------------
        auto jerSmearLambda =
            [this](floats jetpts,
                   floats jetetas,
                   floats jetgenpt,
                   float  rho,
                   unsigned int       run_n,
                   unsigned int       lumi,
                   unsigned long long event,
                   std::string        variation) -> floats
        {
            floats out;
            out.reserve(jetpts.size());

            std::normal_distribution<float> gauss(0.f, 1.f);

            for (size_t i = 0; i < jetpts.size(); i++)
            {
                float pt    = jetpts[i];
                float eta   = jetetas[i];
                float genpt = jetgenpt[i];

                uint64_t seed =
                    (uint64_t(run_n) << 32) ^
                    (uint64_t(lumi)  << 16) ^
                    (uint64_t(event))       ^
                    uint64_t(i);

                seed ^= (seed >> 33);
                seed *= 0xff51afd7ed558ccdULL;
                seed ^= (seed >> 33);
                seed *= 0xc4ceb9fe1a85ec53ULL;
                seed ^= (seed >> 33);

                std::mt19937_64 gen(seed);

                float resolution = _jer_resolution->evaluate({eta, pt, rho});
                float sf         = _jer_corrector->evaluate({eta, pt, variation});
                float smeared_pt = pt;

                if (genpt > 0)
                {
                    smeared_pt = std::max(0.f, genpt + sf * (pt - genpt));
                }
                else
                {
                    float sigma    = resolution * std::sqrt(std::max(sf*sf - 1.f, 0.f));
                    float gaus_val = gauss(gen);
                    smeared_pt     = pt * (1.f + sigma * gaus_val);
                }

                out.emplace_back(smeared_pt);
            }
            return out;
        };

        // GenJet matching
        _rlm = _rlm.Define("Jet_genJetPt",
            [](const floats& GenJet_pt,
               const ROOT::VecOps::RVec<short>& Jet_genJetIdx)
            {
                floats out;
                out.reserve(Jet_genJetIdx.size());
                for (size_t i = 0; i < Jet_genJetIdx.size(); i++)
                {
                    int idx = Jet_genJetIdx[i];
                    out.emplace_back((idx >= 0 && idx < (int)GenJet_pt.size())
                                     ? GenJet_pt[idx] : -1.f);
                }
                return out;
            },
            {"GenJet_pt", "Jet_genJetIdx"});

        // Shared column list for all JER defines
        const std::vector<std::string> jerCols = {
            "Jet_pt_JEC", "Jet_eta", "Jet_genJetPt",
            "Rho_fixedGridRhoFastjetAll", "run", "luminosityBlock", "event"
        };

        for (const auto& var : std::vector<std::pair<std::string,std::string>>{
                {"nom",  "Jet_pt_corr"},
                {"up",   "Jet_pt_corr_jer_up"},
                {"down", "Jet_pt_corr_jer_down"}})
        {
            const std::string variation = var.first;
            const std::string colName   = var.second;

            _rlm = _rlm.Define(colName,
                [jerSmearLambda, variation](
                    floats pt, floats eta, floats genpt, float rho,
                    unsigned int run_n, unsigned int lumi, unsigned long long event)
                {
                    return jerSmearLambda(pt, eta, genpt, rho, run_n, lumi, event, variation);
                },
                jerCols);
        }

        // ------------------------------------------
        // Step 3: JEC uncertainties
        // ------------------------------------------
        for (const auto& [tag, unc] : _jetCorrectionUnc)
        {
            std::string colBase;
            size_t mc_pos  = tag.find("MC_");
            size_t ak4_pos = tag.find("_AK4");

            if (mc_pos != std::string::npos && ak4_pos != std::string::npos)
                colBase = tag.substr(mc_pos + 3, ak4_pos - (mc_pos + 3));
            else
                colBase = tag;

            std::replace_if(colBase.begin(), colBase.end(),
                [](char c){ return !std::isalnum(c); }, '_');

            const std::string colUp   = "Jet_pt_corr_" + colBase + "_up";
            const std::string colDown = "Jet_pt_corr_" + colBase + "_down";
            auto unc_copy = unc;

            _rlm = _rlm.Define(colUp,
                [unc_copy](floats jetpts, floats jetetas) -> floats {
                    floats out;
                    out.reserve(jetpts.size());
                    for (size_t i = 0; i < jetpts.size(); i++)
                        out.emplace_back(jetpts[i] * (1.f + unc_copy->evaluate({jetetas[i], jetpts[i]})));
                    return out;
                },
                {"Jet_pt_JEC", "Jet_eta"});

            _rlm = _rlm.Define(colDown,
                [unc_copy](floats jetpts, floats jetetas) -> floats {
                    floats out;
                    out.reserve(jetpts.size());
                    for (size_t i = 0; i < jetpts.size(); i++)
                        out.emplace_back(jetpts[i] * (1.f - unc_copy->evaluate({jetetas[i], jetpts[i]})));
                    return out;
                },
                {"Jet_pt_JEC", "Jet_eta"});

            std::cout << "Defined uncertainty columns: "
                      << colUp << ", " << colDown << std::endl;
        }

        // ------------------------------------------
        // Step 4: Propagate all MC variants to MET
        // ------------------------------------------
        // Nominal + JER
        for (const auto& var : std::vector<std::string>{"", "_jer_up", "_jer_down"})
        {
            defineMETVariant(
                "Jet_pt_corr" + var,
                "PuppiMET_pt_corr"  + var,
                "PuppiMET_phi_corr" + var);
        }

        // JEC uncertainty variants
        for (const auto& [tag, unc] : _jetCorrectionUnc)
        {
            std::string colBase;
            size_t mc_pos  = tag.find("MC_");
            size_t ak4_pos = tag.find("_AK4");

            if (mc_pos != std::string::npos && ak4_pos != std::string::npos)
                colBase = tag.substr(mc_pos + 3, ak4_pos - (mc_pos + 3));
            else
                colBase = tag;

            std::replace_if(colBase.begin(), colBase.end(),
                [](char c){ return !std::isalnum(c); }, '_');

            for (const auto& ud : std::vector<std::string>{"_up", "_down"})
            {
                defineMETVariant(
                    "Jet_pt_corr_" + colBase + ud,
                    "PuppiMET_pt_corr_"  + colBase + ud,
                    "PuppiMET_phi_corr_" + colBase + ud);
            }
        }
    } // end MC path
}

void NanoAODAnalyzerrdframe::applyMuPtCorrection()
{
    cout << "Applying Muon Pt correction using correctionlib" << endl;

    // Create helper object that captures the CorrectionSet
    auto muonHelper = std::make_shared<MuonCorrectionHelper>(_muon_scalsmear_corrector.get());

    if (_isData) {
        // Data: Only apply scale corrections
        auto lambdaf_data = [muonHelper](const ROOT::VecOps::RVec<int>& mu_charges,
                                         const ROOT::VecOps::RVec<float>& mu_pts,
                                         const ROOT::VecOps::RVec<float>& mu_etas,
                                         const ROOT::VecOps::RVec<float>& mu_phis)
        {
            ROOT::VecOps::RVec<float> corrMuPts;
            corrMuPts.reserve(mu_pts.size());

            for (size_t i = 0; i < mu_pts.size(); i++) {
                float corrected_pt = muonHelper->pt_scale(true, mu_pts[i], mu_etas[i],
                                                         mu_phis[i], mu_charges[i]);
                corrMuPts.emplace_back(corrected_pt);
            }
            return corrMuPts;
        };

        _rlm = _rlm.Define("Muon_pt_corr", lambdaf_data,
                          {"Muon_charge", "Muon_pt", "Muon_eta", "Muon_phi"});
    }
    else {
        // MC: Apply both scale and resolution corrections
        auto lambdaf_mc = [muonHelper](const ROOT::VecOps::RVec<int>& mu_charges,
                                       const ROOT::VecOps::RVec<float>& mu_pts,
                                       const ROOT::VecOps::RVec<float>& mu_etas,
                                       const ROOT::VecOps::RVec<float>& mu_phis,
                                       const ROOT::VecOps::RVec<UChar_t>& nls,
                                       ULong64_t event,
                                       UInt_t lumi)
        {
            ROOT::VecOps::RVec<float> corrMuPts;
            corrMuPts.reserve(mu_pts.size());

            for (size_t i = 0; i < mu_pts.size(); i++) {
                float nTrackerLayers = static_cast<float>(nls[i]);

                // Step 1: Apply scale correction
                float pt_scaled = muonHelper->pt_scale(false, mu_pts[i], mu_etas[i],
                                                      mu_phis[i], mu_charges[i]);

                // Step 2: Apply resolution smearing
                float corrected_pt = muonHelper->pt_resol(pt_scaled, mu_etas[i], mu_phis[i],
                                                         nTrackerLayers,
                                                         static_cast<int>(event),
                                                         static_cast<int>(lumi));

                corrMuPts.emplace_back(corrected_pt);
            }
            return corrMuPts;
        };

        _rlm = _rlm.Define("Muon_pt_corr", lambdaf_mc,
                          {"Muon_charge", "Muon_pt", "Muon_eta", "Muon_phi",
                           "Muon_nTrackerLayers", "event", "luminosityBlock"});

        // --- MC ONLY: Add systematic uncertainty variations ---
        cout << "Adding Muon Pt correction uncertainties for MC" << endl;

        // First, create the intermediate scaled pt (before resolution)
        auto lambdaf_scaled = [muonHelper](const ROOT::VecOps::RVec<int>& mu_charges,
                                           const ROOT::VecOps::RVec<float>& mu_pts,
                                           const ROOT::VecOps::RVec<float>& mu_etas,
                                           const ROOT::VecOps::RVec<float>& mu_phis)
        {
            ROOT::VecOps::RVec<float> scaledMuPts;
            scaledMuPts.reserve(mu_pts.size());

            for (size_t i = 0; i < mu_pts.size(); i++) {
                float pt_scaled = muonHelper->pt_scale(false, mu_pts[i], mu_etas[i],
                                                      mu_phis[i], mu_charges[i]);
                scaledMuPts.emplace_back(pt_scaled);
            }
            return scaledMuPts;
        };

        _rlm = _rlm.Define("Muon_pt_scaled", lambdaf_scaled,
                          {"Muon_charge", "Muon_pt", "Muon_eta", "Muon_phi"});

        // Scale variations (Up)
        auto lambdaf_scale_up = [muonHelper](const ROOT::VecOps::RVec<float>& mu_pts_corr,
                                             const ROOT::VecOps::RVec<float>& mu_etas,
                                             const ROOT::VecOps::RVec<float>& mu_phis,
                                             const ROOT::VecOps::RVec<int>& mu_charges)
        {
            ROOT::VecOps::RVec<float> varMuPts;
            varMuPts.reserve(mu_pts_corr.size());

            for (size_t i = 0; i < mu_pts_corr.size(); i++) {
                float pt_var = muonHelper->pt_scale_var(mu_pts_corr[i], mu_etas[i],
                                                       mu_phis[i], mu_charges[i], "up");
                varMuPts.emplace_back(pt_var);
            }
            return varMuPts;
        };

        _rlm = _rlm.Define("Muon_pt_corr_scaleUp", lambdaf_scale_up,
                          {"Muon_pt_corr", "Muon_eta", "Muon_phi", "Muon_charge"});

        // Scale variations (Down)
        auto lambdaf_scale_dn = [muonHelper](const ROOT::VecOps::RVec<float>& mu_pts_corr,
                                             const ROOT::VecOps::RVec<float>& mu_etas,
                                             const ROOT::VecOps::RVec<float>& mu_phis,
                                             const ROOT::VecOps::RVec<int>& mu_charges)
        {
            ROOT::VecOps::RVec<float> varMuPts;
            varMuPts.reserve(mu_pts_corr.size());

            for (size_t i = 0; i < mu_pts_corr.size(); i++) {
                float pt_var = muonHelper->pt_scale_var(mu_pts_corr[i], mu_etas[i],
                                                       mu_phis[i], mu_charges[i], "dn");
                varMuPts.emplace_back(pt_var);
            }
            return varMuPts;
        };

        _rlm = _rlm.Define("Muon_pt_corr_scaleDn", lambdaf_scale_dn,
                          {"Muon_pt_corr", "Muon_eta", "Muon_phi", "Muon_charge"});

        // Resolution variations (Up)
        auto lambdaf_resol_up = [muonHelper](const ROOT::VecOps::RVec<float>& pt_scaled,
                                             const ROOT::VecOps::RVec<float>& pt_corr,
                                             const ROOT::VecOps::RVec<float>& mu_etas)
        {
            ROOT::VecOps::RVec<float> varMuPts;
            varMuPts.reserve(pt_corr.size());

            for (size_t i = 0; i < pt_corr.size(); i++) {
                float pt_var = muonHelper->pt_resol_var(pt_scaled[i], pt_corr[i],
                                                       mu_etas[i], "up");
                varMuPts.emplace_back(pt_var);
            }
            return varMuPts;
        };

        _rlm = _rlm.Define("Muon_pt_corr_resolUp", lambdaf_resol_up,
                          {"Muon_pt_scaled", "Muon_pt_corr", "Muon_eta"});

        // Resolution variations (Down)
        auto lambdaf_resol_dn = [muonHelper](const ROOT::VecOps::RVec<float>& pt_scaled,
                                             const ROOT::VecOps::RVec<float>& pt_corr,
                                             const ROOT::VecOps::RVec<float>& mu_etas)
        {
            ROOT::VecOps::RVec<float> varMuPts;
            varMuPts.reserve(pt_corr.size());

            for (size_t i = 0; i < pt_corr.size(); i++) {
                float pt_var = muonHelper->pt_resol_var(pt_scaled[i], pt_corr[i],
                                                       mu_etas[i], "dn");
                varMuPts.emplace_back(pt_var);
            }
            return varMuPts;
        };

        _rlm = _rlm.Define("Muon_pt_corr_resolDn", lambdaf_resol_dn,
                          {"Muon_pt_scaled", "Muon_pt_corr", "Muon_eta"});
    }

    cout << "Muon Pt correction applied successfully" << endl;
}


void NanoAODAnalyzerrdframe::applyElectronPtCorrection()
{
    std::cout << "Apply Electron Pt correction" << std::endl;

    if (!_correction_electronss) {
        std::cerr << "Electron corrections file not loaded!" << std::endl;
        return;
    }

    using ROOT::VecOps::RVec;
    using floats = RVec<float>;
    auto smear_corr = _correction_electronss->at("SmearAndSyst");
    auto scale_corr = _correction_electronss->compound().at("Scale");
    _rlm = _rlm.Define("Electron_eta_supercluster",
                   "Electron_eta + Electron_deltaEtaSC");


    // Define supercluster eta once (used by both DATA and MC)

    // =====================================================
    // DATA - Scale corrections
    // =====================================================
    if (_isData) {
        auto scale_corr = _correction_electronss->compound().at("Scale");

        // Create a std::function with explicit signature
        std::function<floats(const floats&, const floats&, const floats&,
                             const ROOT::VecOps::RVec<UChar_t>&, unsigned int, 
                             const std::string&)> scale_lambda =
            [scale_corr](const floats &pt,
                         const floats &scEta,
                         const floats &r9,
                         const ROOT::VecOps::RVec<UChar_t> &seedGain,
                         unsigned int run,
                         const std::string &variation) -> floats
        {
            floats out;
            out.reserve(pt.size());

            for (size_t i = 0; i < pt.size(); ++i) {
                const double pt_v    = pt[i];
                const double scEta_v = scEta[i];
                const double r9_v    = r9[i];

                // Validation checks (fixed the bug from original line 24)
                if (pt_v < 20 ||
                    !std::isfinite(scEta_v) ||
                    !std::isfinite(r9_v) ||
                    std::abs(scEta_v) >= 2.5)
                {
                    out.emplace_back(pt[i]);
                    continue;
                }

                const double scale = scale_corr->evaluate({
                    variation,  // "scale", "scale_up", or "scale_down"
                    static_cast<double>(run),
                    scEta_v,
                    r9_v,
                    pt_v,
                    static_cast<double>(seedGain[i])
                });

                out.emplace_back(pt[i] * scale);
            }
            return out;
        };

        // Define all scale variations with explicit return types
        _rlm = _rlm
            .Define("Electron_pt_corr",
                    [scale_lambda](const floats &pt, const floats &scEta, 
                                  const floats &r9,
                                  const ROOT::VecOps::RVec<UChar_t> &seedGain, 
                                  unsigned int run) -> floats {
                        return scale_lambda(pt, scEta, r9, seedGain, run, "scale");
                    },
                    {"Electron_pt", "Electron_eta_supercluster", "Electron_r9",
                     "Electron_seedGain", "run"})
            .Define("Electron_pt_corr_scaleUp",
                    [scale_lambda](const floats &pt, const floats &scEta, 
                                  const floats &r9,
                                  const ROOT::VecOps::RVec<UChar_t> &seedGain, 
                                  unsigned int run) -> floats {
                        return scale_lambda(pt, scEta, r9, seedGain, run, "scale_up");
                    },
                    {"Electron_pt", "Electron_eta_supercluster", "Electron_r9",
                     "Electron_seedGain", "run"})
            .Define("Electron_pt_corr_scaleDown",
                    [scale_lambda](const floats &pt, const floats &scEta, 
                                  const floats &r9,
                                  const ROOT::VecOps::RVec<UChar_t> &seedGain, 
                                  unsigned int run) -> floats {
                        return scale_lambda(pt, scEta, r9, seedGain, run, "scale_down");
                    },
                    {"Electron_pt", "Electron_eta_supercluster", "Electron_r9",
                     "Electron_seedGain", "run"});
    }

    // =====================================================
    // MC - Smear and Scale corrections
    // =====================================================
    else {
        // For MC: apply Smearing corrections
        auto smear_lambda =
            [smear_corr](const floats &pt,
                    const floats &scEta,
                    const floats &r9,
                    const UInt_t run,
                    const UInt_t lumi,
                    const ULong64_t event)
            -> std::tuple<floats, floats, floats>
            {
                floats nominal, smear_up, smear_down;
                size_t N = pt.size();

                nominal.reserve(N);
                smear_up.reserve(N);
                smear_down.reserve(N);

                std::normal_distribution<float> gauss(0.0, 1.0);

                for (size_t i = 0; i < N; ++i) {

                    /* -----------------------------------------
                       Deterministic seed per electron
                       ----------------------------------------- */
                    uint64_t seed =
                        (uint64_t(run)  << 32) ^
                        (uint64_t(lumi) << 16) ^
                        (uint64_t(event)) ^
                        uint64_t(i);   // electron index

                    std::mt19937 gen(seed);
                    float rand = gauss(gen);

                    try {
                        float smear_val = smear_corr->evaluate({
                                "smear",
                                static_cast<double>(pt[i]),
                                static_cast<double>(r9[i]),
                                static_cast<double>(scEta[i])
                                });

                        float smear_unc_up = smear_corr->evaluate({
                                "smear_up",
                                static_cast<double>(pt[i]),
                                static_cast<double>(r9[i]),
                                static_cast<double>(scEta[i])
                                });

                        float smear_unc_down = smear_corr->evaluate({
                                "smear_down",
                                static_cast<double>(pt[i]),
                                static_cast<double>(r9[i]),
                                static_cast<double>(scEta[i])
                                });

                        nominal.emplace_back(pt[i] * (1.0f + smear_val * rand));
                        smear_up.emplace_back(pt[i] * (1.0f + smear_unc_up * rand));
                        smear_down.emplace_back(pt[i] * (1.0f + smear_unc_down * rand));
                    }
                    catch (const std::exception &e) {
                        std::cerr << "Smearing error at index " << i
                            << ": " << e.what() << std::endl;
                        nominal.emplace_back(pt[i]);
                        smear_up.emplace_back(pt[i]);
                        smear_down.emplace_back(pt[i]);
                    }
                }

                return std::make_tuple(nominal, smear_up, smear_down);
            };

_rlm = _rlm.Define(
            "Electron_pt_corr_triple",
            smear_lambda,
            {
                "Electron_pt",
                "Electron_eta_supercluster",
                "Electron_r9",
                "run",
                "luminosityBlock",
                "event"
            })
        .Define("Electron_pt_corr",
                "std::get<0>(Electron_pt_corr_triple)")
        .Define("Electron_pt_corr_smearUp",
                "std::get<1>(Electron_pt_corr_triple)")
        .Define("Electron_pt_corr_smearDown",
                "std::get<2>(Electron_pt_corr_triple)");

    }
}

void NanoAODAnalyzerrdframe::applyMETPtPhiCorrection() //data and MC
{
  cout << "apply MET Pt and Phi correction" << endl;
  
  if(_isData){
    
    auto lambdaf_met_data = [this](float met_pt, float met_phi, unsigned char npvGood)->std::pair<float, float>
      {
        // Get corrected pt
        float met_pt_corr = _correction_MET_pt_corrector->at("met_xy_corrections")->evaluate({"pt", "PuppiMET", "2023", "DATA", "nom", 
                                                       met_pt, met_phi, static_cast<float>(npvGood)});
        
        // Get corrected phi
        float met_phi_corr = _correction_MET_pt_corrector->at("met_xy_corrections")->evaluate({"phi", "PuppiMET", "2023", "DATA", "nom", 
                                                        met_pt, met_phi, static_cast<float>(npvGood)});
        
        return std::make_pair(met_pt_corr, met_phi_corr);
      };
    
    _rlm = _rlm.Define("MET_pt_phi_corr", lambdaf_met_data, {"PuppiMET_pt", "PuppiMET_phi", "PV_npvsGood"});
    _rlm = _rlm.Define("PuppiMET_pt_corr_type2", "MET_pt_phi_corr.first");
    _rlm = _rlm.Define("PuppiMET_phi_corr_type2", "MET_pt_phi_corr.second");
  }
  else{
    
    auto lambdaf_met_mc = [this](float met_pt, float met_phi, unsigned char npvGood)->std::pair<float, float>
      {
        // Get corrected pt
        float met_pt_corr = _correction_MET_pt_corrector->at("met_xy_corrections")->evaluate({"pt", "PuppiMET", "2023", "MC", "nom", 
                                                       met_pt, met_phi, static_cast<float>(npvGood)});
        
        // Get corrected phi
        float met_phi_corr = _correction_MET_pt_corrector->at("met_xy_corrections")->evaluate({"phi", "PuppiMET", "2023", "MC", "nom", 
                                                        met_pt, met_phi, static_cast<float>(npvGood)});
        
        return std::make_pair(met_pt_corr, met_phi_corr);
      };
    
    _rlm = _rlm.Define("MET_pt_phi_corr", lambdaf_met_mc, {"PuppiMET_pt", "PuppiMET_phi", "PV_npvsGood"});
    _rlm = _rlm.Define("PuppiMET_pt_corr_type2", "MET_pt_phi_corr.first");
    _rlm = _rlm.Define("PuppiMET_phi_corr_type2", "MET_pt_phi_corr.second");
  }
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
		string muon_roch_fname, 
		string muon_fname, 
		string muonhlttype,
		string muonidtype,
		string muonisotype,
		string electron_fname,
		string electronHlt_fname,
		string electronHlt_type, 
		string electron_reco_type1, 
		string electron_reco_type2,
		string electron_reco_type3, 
		string electron_id_type, 
		string jercfname, 
		string jerctag, 
		string jerctagMC, 
		vector<string> jercunctag,
		string jet_veto_f_name,
		string jet_veto_tag, 
		string electron_SSF,
		string metpt_fname,
		string jetidfname,
		string jetid_workingpoint,
		string JER_tag,
		string JER_tag_res)
//In this function the correction is evaluated for each jet, Muon, Electron and MET. The correction depends on the momentum, pseudorapidity, energy, and cone area of the jet, as well as the value of rho(the average momentum per area) and number of interactions in the event. The correction is used to scale the momentum of the jet.
{
        cout << "set up Corrections!" << endl;
        _correction_electronss = correction::CorrectionSet::from_file(electron_SSF);
        cout<< "Electron scaling and smearing filename   : " << electron_SSF << endl;
        _electron_SSF=electron_SSF;


        // _correction_MET_pt_corrector = correction::CorrectionSet::from_file(metpt_fname);
	    // cout<< "met pt correction file name    : " << metpt_fname<< endl;
        // _metpt_fname=metpt_fname;
        // assert(_correction_MET_pt_corrector->validate());

	   _muon_scalsmear_corrector = correction::CorrectionSet::from_file(muon_roch_fname);
	    cout<<"muon scaling and smearing filename :"<< muon_roch_fname<<endl;
	    _muon_roch_fname=muon_roch_fname;
	    assert(_muon_scalsmear_corrector ->validate());


	if (_isData) _jsonOK = readgoodjson(goodjsonfname); // read golden json file
        _correction_jetveto = correction::CorrectionSet::from_file(jet_veto_f_name);
        cout<< "Jrt veto JSON FILE : " <<  jet_veto_f_name << endl;
        assert(_correction_jetveto->validate());
        _jet_veto_tag = jet_veto_tag;


	if (!_isData) {
        // using correctionlib
        //Muon corrections
        _correction_muon = correction::CorrectionSet::from_file(muon_fname);
        _muon_hlt_type = muonhlttype;
        // _muon_reco_type = muonrecotype;
        _muon_id_type = muonidtype;
        _muon_iso_type = muonisotype;
        std::cout<< "================================//=================================" << std::endl;
        cout<< "MUON JSON FILE : " <<  muon_fname << endl;
        cout<< "MUON HLT type in JSON  : " << _muon_hlt_type << endl;
        cout<< "MUON ID type in JSON  : " << _muon_id_type << endl;
        cout<< "MUON ISO type in JSON  : " << _muon_iso_type << endl;
        assert(_correction_muon->validate());
  
	    //Electron corrections
	    _correction_electron = correction::CorrectionSet::from_file(electron_fname);
        cout<< "above line has problem" << endl;
        _correction_electronHlt = correction::CorrectionSet::from_file(electronHlt_fname);

    //  _electron_reco_type = electron_reco_type;
        _electron_reco_type1=electron_reco_type1;
        _electron_reco_type2=electron_reco_type2;
        _electron_reco_type3=electron_reco_type3;
        _electron_id_type = electron_id_type;
        _electronHlt_type =electronHlt_type;
        std::cout<< "================================//=================================" << std::endl;
        cout<< "ELECTRON JSON FILE : " << electron_fname << endl;
        cout<< "ELECTRON RECO type in JSON  : " << _electron_reco_type1 << endl;
        cout<< "ELECTRONID type in JSON  : " << _electron_id_type << endl;
        assert(_correction_electron->validate());
        assert(_correction_electronHlt->validate());
	  
        // btag corrections
        _correction_btag1 = correction::CorrectionSet::from_file(btvfname);
        _btvtype = btvtype;
        assert(_correction_btag1->validate());

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
        auto punominal = [this](float x) { return pucorrection(_correction_pu, _putag, "nominal", x); };
        auto puplus = [this](float x) { return pucorrection(_correction_pu, _putag, "up", x); };
        auto puminus = [this](float x) { return pucorrection(_correction_pu, _putag, "down", x); };

        if (!isDefined("puWeight")) _rlm = _rlm.Define("puWeight", punominal, {"Pileup_nTrueInt"});
        if (!isDefined("puWeight_plus")) _rlm = _rlm.Define("puWeight_plus", puplus, {"Pileup_nTrueInt"});
        if (!isDefined("puWeight_minus")) _rlm = _rlm.Define("puWeight_minus", puminus, {"Pileup_nTrueInt"});
	  
	  
	  if (!isDefined("pugenWeight"))
	    {
	      _rlm = _rlm.Define("pugenWeight", [this](float x, float y){
		  return x*y;
		}, {"genWeight", "puWeight"});
	    }
	}
	_jerctag = jerctag;
	_jerctagMC=jerctagMC;
	_jercunctag = jercunctag;
	_JER_tag = JER_tag;
	_JER_tag_res=JER_tag_res;
	
	setupJetMETCorrection(jercfname, _jerctag, _jerctagMC, _JER_tag,_JER_tag_res);
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
	applyJetMETCorrections();
	applyElectronPtCorrection();
	applyMuPtCorrection();
	// applyMETPtPhiCorrection();
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
			if (std::abs(etas[i]) > 2.4999 || pts[i] < 25.000001 || hadflav[i] == 0) continue;
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
					//int flavor_code = (hadflav[i] == 4 || hadflav[i] == 5) ? 5 : int(hadflav[i]);
					double bcjets_weights = _correction_btag1->at("UParTAK4_comb")->evaluate({variation, _BTagWP, int(hadflav[i]), std::fabs(etas[i]), pts[i]});
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
					//int flavor_code = (hadflav[i] == 4 || hadflav[i] == 5) ? 5 : int(hadflav[i]);
					double bcjets_weights = _correction_btag1->at("UParTAK4_comb")->evaluate({variation, _BTagWP, int(hadflav[i]), std::fabs(etas[i]), pts[i]});
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
        if (std::abs(etas[i]) > 2.4999 || pts[i] < 25.000001 || hadflav[i] != 0) continue;
    	if (_year == 2016) continue;
        try {
          if (btag_scores[i] >= btag_cut) {
            double lightjets_weights = _correction_btag1->at("UParTAK4_light")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
	  		btagWeight_lflav *= lightjets_weights;
			// std::cout<<"BTag SF for light-flavor jet " << i << " with pt = " << pts[i] << " and eta = " << etas[i] << " is applied with weight: " << lightjets_weights << std::endl;
			
          } else {
			double lightjets_weights = _correction_btag1->at("UParTAK4_light")->evaluate({variation, _BTagWP, hadflav[i], std::fabs(etas[i]), pts[i]});
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

      	 if (_year == 2022 || _year == 2023 || _year == 2024)
			variations = {"central", "up", "down", "up_correlated", "down_correlated", "up_uncorrelated", "down_uncorrelated", "up_statistic", "down_statistic"};
		else if (_year == 2016)
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




ROOT::RDF::RNode NanoAODAnalyzerrdframe::calculateMuSF(RNode _rlm, std::vector<std::string> Muon_vars, std::string output_var)
{

    //=====================================================Muon SF and eventweight============================================================// 
    //muontype= for thight: NUM_TightID_DEN_genTracks //for medium: NUM_MediumID_DEN_TrackerMuons
    //Muon MediumID ISO UL type: NUM_TightRelIso_DEN_MediumID && thightID:NUM_TightRelIso_DEN_TightIDandIPCut --> the type can be found in json file
    //--> As an example Medium wp is used 
    //===============================================================================================================================================//
  auto muon_weightgenerator = [this](const std::string& muon_type, const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<float>& pts, const std::string& variation) -> float {
      double muonHLT_w = 1.0;

      for (std::size_t i = 0; i < pts.size(); i++) {
	//std::cout << "Muon abs_eta:" << std::fabs(etas[i]) << " pt: " << pts[i] << std::endl;
	//double w = _correction_muon->at(muon_type)->evaluate({std::to_string(_year)+"_"+_runtype, std::fabs(etas[i]), pts[i], variation}); //here the year and the runtype is also being used 
	double w = _correction_muon->at(muon_type)->evaluate({std::fabs(etas[i]), pts[i], variation});
	muonHLT_w *= w;
	//std::cout << "Individual HLT weight (muon " << i << "): " << w << std::endl;
	//std::cout << "Cumulative HLT weight after muon " << i << ": " << muonHLT_w << std::endl;
      }
      return muonHLT_w;
    };

    //'sf' is nominal, and 'systup' and 'systdown' are up/down variations with total stat+-syst uncertainties. Individual systs are also available (in these cases syst only, not sf +/- syst
    std::vector<std::string> variations = {"nominal", "systup", "systdown","syst"};


    //cout<<"Generate MUONHLT weight"<<endl;
    //muonHLT sf and systematics with up/down variations
    //===========//===========//===========//===========//===========
    // define muon HLT weight sf/systs for each variation individually
    for (const std::string& variation : variations) {
      std::string column_name_hlt = output_var+"hlt_" + variation;
      _rlm = _rlm.Define(column_name_hlt, [this, muon_weightgenerator, variation](const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<float>& pts) {
	  float weight = muon_weightgenerator(_muon_hlt_type, etas, pts, variation); // Get the weight for the corresponding variation
	  //std::cout << "Muon HLT weight (" << variation << "): " << weight << std::endl;
	  return weight;
	}, Muon_vars);


/*      std::string column_name_reco = output_var+"reco_" + variation;
      _rlm = _rlm.Define(column_name_reco, [this, muon_weightgenerator, variation](const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<float>& pts) {
	  float weight = muon_weightgenerator(_muon_reco_type, etas, pts, variation); // Get the weight for the corresponding variation
	  //std::cout << "Muon HLT weight (" << variation << "): " << weight << std::endl;
	  return weight;
	}, Muon_vars);   */
	

      std::string column_name_id = output_var+"id_" + variation;
      _rlm = _rlm.Define(column_name_id, [this, muon_weightgenerator, variation](const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<float>& pts) {
	  float weight = muon_weightgenerator(_muon_id_type, etas, pts, variation); // Get the weight for the corresponding variation
	  //std::cout << "Muon HLT weight (" << variation << "): " << weight << std::endl;
	  return weight;
	}, Muon_vars);


      std::string column_name_iso = output_var+"iso_" + variation;
      _rlm = _rlm.Define(column_name_iso, [this, muon_weightgenerator, variation](const ROOT::VecOps::RVec<float>& etas, const ROOT::VecOps::RVec<float>& pts) {
	  float weight = muon_weightgenerator(_muon_iso_type, etas, pts, variation); // Get the weight for the corresponding variation
	  //std::cout << "Muon HLT weight (" << variation << "): " << weight << std::endl;
	  return weight;
	}, Muon_vars);

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
      else{
	column_name += "syst";
      }

	//std::string sf_definition = column_name_hlt+" * "+column_name_reco+" * "+column_name_id+" * "+column_name_iso;
	std::string sf_definition = column_name_id+" * "+column_name_iso;
	_rlm = _rlm.Define(column_name, sf_definition);
	std::cout<< "Muon SF column name: " << column_name << std::endl;
    }
    return _rlm;
}


ROOT::RDF::RNode NanoAODAnalyzerrdframe::calculateEleSF(
        RNode _rlm,
        std::vector<std::string> Ele_vars,
        std::string output_var)
{
    // electron RECO & ID scale factor generator
    auto electron_weightgenerator = [this](
            const std::string eletype,
            const ROOT::VecOps::RVec<float>& etas,
            const ROOT::VecOps::RVec<float>& pts,
            const ROOT::VecOps::RVec<float>& phis,
            const std::string& variation) -> float 
    {

        double w_tot = 1.0;
        for (size_t i = 0; i < pts.size(); i++)
        {

            double w = 1.0;

            if (_year == 2023 && _runtype =="PreBPix")
            {
                w = _correction_electron
                    ->at("Electron-ID-SF")
                    ->evaluate({"2023PromptC", variation, eletype,
                            std::fabs(etas[i]), pts[i], phis[i]});
            }
            else if (_year == 2022 && _runtype =="PreEE")
            {
                w = _correction_electron
                    ->at("Electron-ID-SF")
                    ->evaluate({"2022Re-recoBCD", variation, eletype,
                            std::fabs(etas[i]), pts[i]});
            }
            else if (_year == 2022 && _runtype =="PostEE")
            {
                w = _correction_electron
                    ->at("Electron-ID-SF")
                    ->evaluate({"2022Re-recoE+PromptFG", variation, eletype,
                            std::fabs(etas[i]), pts[i]});
            }
            else if (_year == 2023 && _runtype =="PostBPix")
            {
                w = _correction_electron
                    ->at("Electron-ID-SF")
                    ->evaluate({"2023PromptD", variation, eletype,
                            std::fabs(etas[i]), pts[i], phis[i]});
            }
            else if (_year == 2024 )
            {
                w = _correction_electron
                    ->at("Electron-ID-SF")
                    ->evaluate({"2024Prompt", variation, eletype,
                            std::fabs(etas[i]), pts[i]});
            }


            w_tot *= w;
        }

        return w_tot;
    };

    // variations
    std::vector<std::string> variations_elec = {"sf", "sfup", "sfdown"};

    for (const std::string& variation : variations_elec)
    {
        // ======================================================
        // 1) ELECTRON RECO SCALE FACTOR
        // ======================================================
        std::string column_name_reco = output_var + "reco_" + variation;

	_rlm = _rlm.Define(
	    column_name_reco,
	    [this, electron_weightgenerator, variation](
		const ROOT::VecOps::RVec<float>& etas,
		const ROOT::VecOps::RVec<float>& pts,
		const ROOT::VecOps::RVec<float>& phis)
	    {
		ROOT::VecOps::RVec<float> weights(pts.size(), 1.0f);

		for (size_t i = 0; i < pts.size(); ++i) {

		    std::string reco_type;
		    if (pts[i] > 75.0) {
			reco_type = _electron_reco_type1;
		    }
		    else if (pts[i] > 20.0 && pts[i] <= 75.0) {
			reco_type = _electron_reco_type2;
		    }
		    else {  // pts[i] <= 20.0
			reco_type = _electron_reco_type3;
		    }

		    ROOT::VecOps::RVec<float> eta1 = {etas[i]};
		    ROOT::VecOps::RVec<float> pt1  = {pts[i]};
		    ROOT::VecOps::RVec<float> phi1 = {phis[i]};

		    weights[i] = electron_weightgenerator(
			reco_type, eta1, pt1, phi1, variation);
		}

		return std::accumulate(
		    weights.begin(),
		    weights.end(),
		    1.0f,
		    std::multiplies<float>());
	    },
	    Ele_vars
	);

        // ======================================================
        // 2) ELECTRON ID SCALE FACTOR
        // ======================================================
        std::string column_name_id = output_var + "id_" + variation;

        _rlm = _rlm.Define(
            column_name_id,
            [this, electron_weightgenerator, variation](
                const ROOT::VecOps::RVec<float>& etas,
                const ROOT::VecOps::RVec<float>& pts,
                const ROOT::VecOps::RVec<float>& phis)
            {
                return electron_weightgenerator(
                    _electron_id_type, etas, pts, phis, variation);
            },
            Ele_vars
        );

        // ======================================================
        // 3) COMBINE: RECO * ID 
        // ======================================================
        std::string column_name = output_var;
        if (variation == "sf")
            column_name += "central";
        else if (variation == "sfup")
            column_name += "up";
        else
            column_name += "down";
        std::cout << "Electron SF column name: " << column_name << std::endl;

        _rlm = _rlm.Define(
            column_name,
            column_name_reco + " * " + column_name_id 
        );
    }

    return _rlm;
}



ROOT::RDF::RNode NanoAODAnalyzerrdframe::applyJetVetoMap(ROOT::RDF::RNode _rlm,
                                                          const std::string& eta_var,
                                                          const std::string& phi_var,
                                                          const std::string& output_var) {
    std::cout << "Applying Jet veto map..." << std::endl;

    auto is_vetoed_event = [this](const ROOT::VecOps::RVec<float>& etas,
                                  const ROOT::VecOps::RVec<float>& phis) -> bool {
        // Get the correction object
        auto veto_corr = _correction_jetveto->at(_jet_veto_tag);
        std::string veto_type = "jetvetomap";

        for (size_t i = 0; i < etas.size(); ++i) {
            double veto_val = veto_corr->evaluate({veto_type, etas[i], phis[i]});
            if (veto_val != 0) {
                return true;  // At least one jet in vetoed region
            }
        }
        return false;  // No jet in vetoed region
    };

    // Define a new column with a single boolean per event
    return _rlm.Define(output_var, is_vetoed_event, {eta_var, phi_var});

}



bool NanoAODAnalyzerrdframe::helper_1DHistCreator(std::string hname, std::string title, const int nbins, const double xlow, const double xhi, std::string rdfvar, std::string evWeight, RNode *anode)
{
	//cout << "1DHistCreator " << hname  << endl;

	RDF1DHist histojets = anode->Histo1D({hname.c_str(), title.c_str(), nbins, xlow, xhi}, rdfvar, evWeight); // Fill with weight given by evWeight
	_th1dhistos[hname] = histojets;
	//histojets.GetPtr()->Print("all");
	return true;
}

//for 2D histograms//
bool NanoAODAnalyzerrdframe::helper_2DHistCreator(std::string hname, std::string title, const int nbinsx, const double xlow, const double xhi, const int nbinsy, const double ylow, const double yhi,std::string rdfvarx,std::string rdfvary, std::string evWeight, RNode *anode)
{
	//cout << "1DHistCreator " << hname  << endl;

	RDF2DHist histojets = anode->Histo2D({hname.c_str(), title.c_str(), nbinsx, xlow, xhi,nbinsy, ylow, yhi}, rdfvarx,rdfvary, evWeight); // Fill with weight given by evWeight
	_th2dhistos[hname] = histojets;
	histojets.GetPtr()->Print("all");
	return true;
}

// Automatically loop to create
void NanoAODAnalyzerrdframe::setupCuts_and_Hists()
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
		std::string cutname = "cut"+ acut.idx;
		std::string hpost = "_"+cutname;
		RNode *r = _rnt.getParent(acut.idx)->getRNode();
		auto rnext = new RNode(r->Define(cutname, acut.cutdefinition));
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
		_rnt.addDaughter(rnext, acut.idx);

	}
}

void NanoAODAnalyzerrdframe::add1DHist(TH1DModel histdef, std::string variable, std::string weight,string mincutstep)
{
	_hist1dinfovector.push_back({histdef, variable, weight, mincutstep});
}
//for 2DHistograms
void NanoAODAnalyzerrdframe::add2DHist(TH2DModel histdef, std::string variable1,std::string variable2, std::string weight,string mincutstep)
{
	_hist2dinfovector.push_back({histdef, variable1,variable2, weight, mincutstep});
}


void NanoAODAnalyzerrdframe::drawHists(RNode t)
{
	cout << "processing" <<endl;
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
	for (auto arnt: rntends)
	{
		RNode *arnode = arnt->getRNode();
		string nodename = arnt->getIndex();
		vector<string> varforthistree;
		std::map<string, int> varused;

		for (auto varname: _varstostore)
		{
			bool foundmatch = false;
			std::regex b(varname);
			for (auto a: arnode->GetColumnNames())
			{
				if (std::regex_match(a, b) && varused[a]==0)
				{
					varforthistree.push_back(a);
					varused[a]++;
					foundmatch = true;
				}
			}
			if (!foundmatch)
			{
				cout << varname << " not found at "<< nodename << endl;
			}

		}
		_varstostorepertree[nodename]  = varforthistree;
	}

}

void NanoAODAnalyzerrdframe::addCuts(string cut, string idx)
{
	_cutinfovector.push_back({cut, idx});
}


void NanoAODAnalyzerrdframe::run(bool saveAll, string outtreename)
{
	vector<RNodeTree *> rntends;
	_rnt.getRNodeLeafs(rntends);
	_rnt.Print();
	 cout << rntends.size() << endl;
	for (auto arnt : rntends)
	{
		string nodename = arnt->getIndex();
		cout <<" nodename" <<endl;
		string outname = _outfilename;
		if (rntends.size() > 1)
			outname.replace(outname.find(".root"), 5, "_" + nodename + ".root");
		_outrootfilenames.push_back(outname);
		RNode *arnode = arnt->getRNode();
		std::cout << "-------------------------------------------------------------------" << std::endl;
		cout << "cut : ";
		cout << arnt->getIndex();
		if (saveAll)
		{
			arnode->Snapshot(outtreename, outname);
		}
		else
		{
			cout << " --writing branches" << endl;
			std::cout << "-------------------------------------------------------------------" << std::endl;
			for (auto bname : _varstostorepertree[nodename])
			{
				cout << bname << endl;
			        cout << "-----branch stored" << endl;
			}
                        cout << "before snapshot" <<endl;       
		  	arnode->Snapshot(outtreename, outname, _varstostorepertree[nodename]);
		        cout << "after snapshot" <<endl;	
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
		//TH1F* hPDFWeights = new TH1F("LHEPdfWeightSum", "LHEPdfWeightSum", 103, 0, 1);
		//for (size_t i=0; i<PDFWeights.size(); i++){
		//	hPDFWeights->SetBinContent(i+1, PDFWeights[i]);
	//	}
		_outrootfile->Write(0, TObject::kOverwrite);
		_outrootfile->Close();
	}
	std::cout << "-------------------------------------------------------------------" << std::endl;
	std::cout << "END...  :) " << std::endl;
}

void NanoAODAnalyzerrdframe::setParams(int year, string runtype, int datatype)
{
    /*if(debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }*/
	_year=year;
	_runtype=runtype;
	_datatype=datatype;
	

	if(_year==2017) {
        cout << "Analysing through Run 2017" << endl;
    }else if(_year==2022) {
        cout << "Analysing through Run 2022" << endl;
    }else if(_year==2023){
        cout << "Analysing through Run 2023" << endl;
    }else if(_year==2024){
        cout << "Analysing through Run 2024" << endl;
    }


    if(_runtype.find("PreEE") != std::string::npos){
        _isPreEE = true;
        cout << "PreEE Selected " << endl;
        std::cout<< "-------------------------------------------------------------------" << std::endl;
    }
    else if(_runtype.find("PostEE") != std::string::npos){
        _isPostEE = true;
        cout << " PostEE  Selected!" << endl;
        std::cout<< "-------------------------------------------------------------------" << std::endl;
    }
    else if(_runtype.find("PreBPix") != std::string::npos) {
        _isPreBPix = true;
        cout << "PreBPix Selected" << endl;
        cout << "-------------------------------------------------------------------" << endl;
    }
    else if(_runtype.find("PostBPix") != std::string::npos) {
        _isPostBPix = true;
        cout << "PostBPix Selected!" << endl;
        cout << "-------------------------------------------------------------------" << endl;
    }
    else if(!_isPreEE && !_isPostEE && !_isPreBPix && !_isPostBPix){
        std::cout<< "Default run version : PreEE , PostEE , PreBPix, PostBPix is not selected! "<< std::endl;
        std::cout<< "-------------------------------------------------------------------" << std::endl;
    }


	if (_datatype==0){
		_isData = false;
        std::cout << " MC input files Selected!! "<<std::endl;
        std::cout<< "-------------------------------------------------------------------" << std::endl;

    }else if(_datatype==1){
        _isData = true;
        std::cout << " DATA input files Selected!!" <<std::endl;
        std::cout<< "-------------------------------------------------------------------" << std::endl;
    }if(_datatype==-1){
		std::cout<< "Default root version :checking out gen branches! "<< std::endl;
        std::cout<< "-------------------------------------------------------------------" << std::endl;
                if (_atree == nullptr) {
                    std::cerr << "[ERROR] _atree is null! Cannot inspect genWeight branch." << std::endl;
                    return;
                }
		if (_atree->GetBranch("genWeight") == nullptr) {
			_isData = true;
			cout << "input file is DATA" <<endl;
		}
		else
		{
			_isData = false;
			cout << "input file is MC" <<endl;
		}
	}
	std::cout << "Before crash!!" <<std::endl;
	TObjArray *allbranches = _atree->GetListOfBranches();
	for (int i =0; i<allbranches->GetSize(); i++)
	{
		TBranch *abranch = dynamic_cast<TBranch *>(allbranches->At(i));
		if (abranch!= nullptr){
			//cout << abranch->GetName() << endl;
			_originalvars.push_back(abranch->GetName());
		}
	}


}
//Checking HLTs in the input root file
std::string NanoAODAnalyzerrdframe::setHLT(std::string str_HLT){
    if(debug){
    std::cout<< "================================//=================================" << std::endl;
    std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
    std::cout<< "================================//=================================" << std::endl;
    }
    if(str_HLT != "" ){

        bool ctrl_HLT =isDefined(str_HLT);
        std::string output;
        if(ctrl_HLT){
            output=str_HLT;
            std::cout<<"HLT : " <<  str_HLT  << " : SUCCESSFULLY FOUND!!"<< std::endl;
        }else{
            std::cout<<"HLT : " <<  str_HLT  << " : CAN NOT BE FOUND "<< std::endl;
            std::cout<< "Check HLT branches in the input root file!!" << std::endl;
            std::cout<< "EXITING PROGRAM!!" << std::endl;

            exit(1);
        }
        return output;

    }else{ // fill the HLT names in a vector according to each year
            std::vector<string> V_output;
            if(_year==2016){
                HLTGlobalNames=HLT2016Names;
            }else if (_year==2017){
                HLTGlobalNames=HLT2017Names;
            }else if(_year==2018){
                HLTGlobalNames=HLT2018Names;
            }else if(_year==2022){
		if(_runtype == "PreEE"){
                   HLTGlobalNames = HLT2022Names;
                }else if(_runtype == "PostEE"){
                   HLTGlobalNames = HLT2022EENames;
                }
            }else if(_year==2023){
                HLTGlobalNames=HLT2023Names;
            }else if(_year==2024){
                HLTGlobalNames=HLT2022EENames;
            }

            //loop on HLTs
            for (size_t i = 0; i < HLTGlobalNames.size(); i++)
            {
                /* code */
                bool ctrl_HLT = isDefined(HLTGlobalNames[i]);
                if(ctrl_HLT){
                    V_output.push_back(HLTGlobalNames[i]);
                }else{
    		    std::cout << "HLT NOT FOUND: " << HLTGlobalNames[i] << std::endl;
		}

            }
            std::string output_HLT;
            if(!V_output.empty()){
                for (size_t i = 0; i < V_output.size() ; i++)
                {
                    if(i!=V_output.size()-1){
                    output_HLT += V_output[i] + "==1 || " ;
                    }else{
                        output_HLT += V_output[i] + "==1 " ;
                    }
                }
            }else{
                std::cout<< " Not matched with any HLT Triggers! Please check the HLT Names in the inputfile " << std::endl;
                std::cout<< "EXITING PROGRAM!!" << std::endl;
                exit(1);
            }
            std::cout<< " HLT names =  " <<  output_HLT  << std::endl;
            return output_HLT;

    }

}
//control all branch names using in the addCuts function
std::string NanoAODAnalyzerrdframe::ctrlBranchName(std::string str_Branch){

    if(debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

    bool ctrl_Branch =isDefined(str_Branch);
    std::string output;
    if(ctrl_Branch){
        output=str_Branch;
    }else{
        std::cout<<"Branch : " <<  str_Branch  << " : CAN NOT BE FOUND "<< std::endl;
        std::cout<< "Check your branches in the input root file!!" << std::endl;
        std::cout<< "EXITING PROGRAM!!" << std::endl;

        exit(1);
    }
    return output;
}



//cut-based ID Fall17 V2 (0:fail, 1:veto, 2:loose, 3:medium, 4:tight)
std::string NanoAODAnalyzerrdframe::ElectronID(int cutbasedID){

    if(debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }
    //double Electron_eta;
    //double Electron_pt;
    if (cutbasedID==1)std::cout<< " VETO Electron ID requested    == " << cutbasedID <<std::endl;
    if (cutbasedID==2)std::cout<< " LOOSE Electron ID requested   == " << cutbasedID <<std::endl;
    if (cutbasedID==3)std::cout<< " MEDIUM Electron ID requested  == " << cutbasedID <<std::endl;
    if (cutbasedID==4)std::cout<< " TIGHT Electron ID requested   == " << cutbasedID <<std::endl;
    std::cout<< "-------------------------------------------------------------------" << std::endl;

    if (cutbasedID<0 || cutbasedID>4){
        std::cout<< "ERROR!! Wrong Electron ID requested  == " << cutbasedID << "!! Can't be applied" <<std::endl;
        std::cout<< "Please select ElectronID from 1 to 4 " <<std::endl;
        std::cout<< "-------------------------------------------------------------------" << std::endl;
        std::cout<< "EXITING PROGRAM!!" << std::endl;
        exit(1);
    }
//Rdataframe look for the variables in the intput Ttree..
std::string output = Form("Electron_cutBased == %d ",cutbasedID);
//std::string output = Form("Electron_cutBased == %d &&  abs(Electron_eta)<%f && Electron_pt<%f",cutbasedID,  Electron_eta, Electron_pt);

return output;
}

std::string NanoAODAnalyzerrdframe::MuonID(int cutbasedID){
    
    if(debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }
    
    //  cut-based ID Fall17 V2 (0:fail, 1:veto, 2:loose, 3:medium, 4:tight)
    if (cutbasedID==1)std::cout<< " Veto Muon ID requested   == " << cutbasedID <<std::endl;
    if (cutbasedID==2)std::cout<< " LOOSE Muon ID requested  == " << cutbasedID <<std::endl;
    if (cutbasedID==3)std::cout<< " MEDIUM Muon ID requested == " << cutbasedID <<std::endl;
    if (cutbasedID==4)std::cout<< " TIGHT Muon ID requested as == " << cutbasedID <<std::endl;
    std::cout<< "-------------------------------------------------------------------" << std::endl;

    if (cutbasedID<1 || cutbasedID>4){
        std::cout<< "ERROR!! Wrong Muon ID requested  == " << cutbasedID << "!! Can't be applied" <<std::endl;
        std::cout<< "Please select Muon ID from 2 to 4 " <<std::endl;
        std::cout<< "-------------------------------------------------------------------" << std::endl;
        std::cout<< "EXITING PROGRAM!!" << std::endl;

        exit(1);
    }

    string Muon_cutBased_ID;
    
    if (cutbasedID==1){
        Muon_cutBased_ID = "Muon_looseId";
        std::cout<< " VETO Muon ID requested == " << cutbasedID <<", but it doesn't exist in the nanoAOD branches. It is moved to loose MuonID. " << cutbasedID <<std::endl;
    }
    if (cutbasedID == 2){
        Muon_cutBased_ID = "Muon_looseId";
    
    }else if(cutbasedID == 3){
        Muon_cutBased_ID = "Muon_mediumId";

    }else if(cutbasedID == 4){
        Muon_cutBased_ID = "Muon_tightId";
    }
    string output;
    output = Form ("%s==true",Muon_cutBased_ID.c_str());
    return output;
}

/*
std::string NanoAODAnalyzerrdframe::JetID(int cutbasedID){

    if(debug){
        std::cout<< "================================//=================================" << std::endl;
        std::cout<< "Line : "<< __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout<< "================================//=================================" << std::endl;
    }

    if(cutbasedID<1 || cutbasedID>7){
        std::cout<< "Error Wrong JET ID requested == " << cutbasedID << "!! Can't be applied" <<std::endl;
        std::cout<< "Please select number from 1 to 7 " <<std::endl;

    }else{
	    std::cout<< " JET ID requested  == " << cutbasedID <<std::endl;
    }

    string output;
    output = Form ("Jet_jetId==%d",cutbasedID);
    return output;
}



*/


std::string NanoAODAnalyzerrdframe::JetID(int cutbasedID)
{
	if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    std::string output;

	if (_correction_jetid && cutbasedID == 6 && !_jetid_workingpoint.empty())
	{
		std::string column_name = "jetid_" + _jetid_workingpoint + "_mask";
		if (!_jetid_mask_defined)
		{
			auto jetid_lambda = [this](const ROOT::VecOps::RVec<float> &eta,
			                           const ROOT::VecOps::RVec<float> &chHEF,
			                           const ROOT::VecOps::RVec<float> &neHEF,
			                           const ROOT::VecOps::RVec<float> &chEmEF,
			                           const ROOT::VecOps::RVec<float> &neEmEF,
			                           const ROOT::VecOps::RVec<float> &muEF,
			                           const ROOT::VecOps::RVec<UChar_t> &chMultiplicity,
			                           const ROOT::VecOps::RVec<UChar_t> &neMultiplicity) -> ROOT::VecOps::RVec<char>
			{
				ROOT::VecOps::RVec<char> passMask(eta.size(), 0);
				if (!_correction_jetid)
					return passMask;

				auto criteria = _correction_jetid->at(_jetid_workingpoint);
				for (std::size_t i = 0; i < eta.size(); ++i)
				{
					try
					{
						const int chMult = static_cast<int>(chMultiplicity[i]);
						const int neMult = static_cast<int>(neMultiplicity[i]);
						const int multiplicity = chMult + neMult;
						std::vector<correction::Variable::Type> jetid_inputs;
						jetid_inputs.reserve(9);
						jetid_inputs.emplace_back(static_cast<double>(eta[i]));
						jetid_inputs.emplace_back(static_cast<double>(chHEF[i]));
						jetid_inputs.emplace_back(static_cast<double>(neHEF[i]));
						jetid_inputs.emplace_back(static_cast<double>(chEmEF[i]));
						jetid_inputs.emplace_back(static_cast<double>(neEmEF[i]));
						jetid_inputs.emplace_back(static_cast<double>(muEF[i]));
						jetid_inputs.emplace_back(static_cast<int>(chMult));
						jetid_inputs.emplace_back(static_cast<int>(neMult));
						jetid_inputs.emplace_back(static_cast<int>(multiplicity));
						const double result = criteria->evaluate(jetid_inputs);
						passMask[i] = (result > 0.5);
					}
					catch (const std::exception &e)
					{
						std::cerr << "[JetID] evaluation error: " << e.what()
						          << " (eta=" << i << ")" << std::endl;
						passMask[i] = 0;
					}
				}
				return passMask;
			};

			_rlm = _rlm.Define(column_name, jetid_lambda,
				{"Jet_eta", "Jet_chHEF", "Jet_neHEF", "Jet_chEmEF", "Jet_neEmEF",
				 "Jet_muEF", "Jet_chMultiplicity", "Jet_neMultiplicity"});
			_jetid_mask_defined = true;
		}
		output = column_name;
	}
	else if (cutbasedID == 6) {
		// Apply bugfix recipe from POG : https://twiki.cern.ch/twiki/bin/viewauth/CMS/JetID13p6TeV#nanoAOD_Flags
		output =
			"( (abs(Jet_eta) <= 2.7 && ((Jet_jetId & (1 << 1)) != 0)) || "
            "(abs(Jet_eta) > 2.7 && abs(Jet_eta) <= 3.0 && ((Jet_jetId & (1 << 1)) != 0) && Jet_neHEF < 0.99) || "
            "(abs(Jet_eta) > 3.0 && ((Jet_jetId & (1 << 1)) != 0) && Jet_neEmEF < 0.4) )"
            " && Jet_muEF < 0.8 && Jet_chEmEF < 0.8";
    } else if (cutbasedID == 2) {
        output = "Jet_jetId == 2";  // old use case
    } else {
        std::cerr << "Unsupported JetID requested: " << cutbasedID << std::endl;
    }

	//  // ----------- NanoAODv15+: Use correctionlib JSON 2024 -----------
    // Load evaluator once per job
    // evaluator["AK4PUPPI_TightLeptonVeto"].evaluate(...)

    return output;
}

