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
};

#endif
