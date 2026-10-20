#ifndef GENPARTICLEHELPER_H
#define GENPARTICLEHELPER_H

#include <ROOT/RVec.hxx>

namespace GenParticleHelper
{
    using ROOT::VecOps::RVec;

    // Status flag bits
    constexpr int BIT_FROM_HARD_PROCESS = 8;
    constexpr int BIT_IS_LAST_COPY      = 13;

    constexpr unsigned int IS_FIRST_COPY = (1u << 12);
    constexpr unsigned int IS_LAST_COPY  = (1u << 13);

    // Bit helpers
    bool HasBit(int flags, int bit);
    bool IsFirstCopy(int flags);
    bool IsLastCopy(int flags);

    // Last-copy helpers
    int FindLastCopyTop(const RVec<int>& pdg,
                        const RVec<unsigned short>& statusFlags);

    int FindLastCopyZ(const RVec<int>& pdg,
                      const RVec<unsigned short>& statusFlags);

    int WalkToLastCopy(int startIdx,
                       int targetPdg,
                       const RVec<int>& pdg,
                       const RVec<short>& mother,
                       const RVec<unsigned short>& statusFlags);

    // Decay classification
    int ClassifyTopWDecay(const RVec<int>& pdg,
                          const RVec<short>& mother,
                          const RVec<unsigned short>& statusFlags);

    int ClassifyZDecay(const RVec<int>& pdg,
                       const RVec<short>& mother,
                       const RVec<unsigned short>& statusFlags);

}

#endif
