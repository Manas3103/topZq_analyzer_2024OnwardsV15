/*
 * Linkdef.h
 *
 *  Created on: Oct 16, 2018
 *      Author: suyong
 */
#if defined(__CLING__)

// #pragma link C++ class ROOT::Math::PtEtaPhiMVector+;
#pragma link C++ class std::vector<ROOT::Math::PtEtaPhiMVector>+;
// #pragma link C++ class ROOT::VecOps::RVec<ROOT::Math::PtEtaPhiMVector>+;
#pragma link C++ class ROOT::VecOps::RVec<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>+;
#pragma link C++ class std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>+;


#pragma link C++ class NanoAODAnalyzerrdframe +;
#pragma link C++ class BaseAnalyser +;
//#pragma link C++ class SkimEvents +;

#endif /* LINKDEF_H_ */
