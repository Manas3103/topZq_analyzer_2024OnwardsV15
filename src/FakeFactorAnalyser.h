#ifndef FAKEFACTORANALYSER_H_
#define FAKEFACTORANALYSER_H_

#include "BaseAnalyser.h"

class FakeFactorAnalyser : public BaseAnalyser
{
public:
    FakeFactorAnalyser(TTree *t, std::string outfilename);

    // Trial workflow
    void setupObjects() override;
    void setupAnalysis() override;

    // Trial cut
    void defineCuts() override;

    void defineMoreVars();
    void bookHists();

private:
    // Fakeable/tight lepton selection for the FF measurement region.
    // Deliberately separate from BaseAnalyser::selectElectrons()/selectMuons()
    // (those build the analysis-region 3-lepton baseline objects and are not
    // touched here).
    void selectFakeableElectrons();
    void selectFakeableMuons();

    // ClosestJet dR/pT (Eq. 28 jet-matched branch, and the MR jet-lepton
    // separation cut), and the cone-pT correction itself.
    void defineConePt();

    // Exactly one fakeable lepton (e xor mu), >=1 jet pT>25 |eta|<2.5 with
    // DeltaR(jet,lepton) > 0.7.
    void defineMeasurementRegion();

    // Table 25/26: per-path (lepton pT min, jet pT min, cone-pT window,
    // prescale). Applied via ::evaluateFFTrigger. NOTE: the prescale
    // numbers below are the 2018 Run-2 values from the TOP fake-factor
    // note as a structural placeholder -- replace with brilcalc numbers
    // for your actual 2024 run once available.
    void defineFFTriggerWeight();
};

#endif
