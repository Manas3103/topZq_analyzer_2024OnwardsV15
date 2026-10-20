#include "GenParticleHelper.h"

#include <ROOT/RVec.hxx>
#include <cmath>
#include <set>

using ROOT::VecOps::RVec;
namespace GenParticleHelper
{

    // const int BIT_FROM_HARD_PROCESS = 8;
    // const int BIT_IS_LAST_COPY      = 13;
    inline bool HasBit(int flags, int bit) { return (flags >> bit) & 1; }

    int FindLastCopyTop(const ROOT::VecOps::RVec<int>& pdg,
                         const ROOT::VecOps::RVec<unsigned short>& statusFlags) {
        for (int i = 0; i < (int)pdg.size(); ++i)
            if (std::abs(pdg[i]) == 6 && HasBit(statusFlags[i], BIT_IS_LAST_COPY))
                return i;
        return -1;
    }

    int FindLastCopyZ(const ROOT::VecOps::RVec<int>& pdg,
                       const ROOT::VecOps::RVec<unsigned short>& statusFlags) {
        for (int i = 0; i < (int)pdg.size(); ++i)
            if (pdg[i] == 23 && HasBit(statusFlags[i], BIT_IS_LAST_COPY) &&
                HasBit(statusFlags[i], BIT_FROM_HARD_PROCESS))
                return i;
        for (int i = 0; i < (int)pdg.size(); ++i)
            if (pdg[i] == 23 && HasBit(statusFlags[i], BIT_IS_LAST_COPY))
                return i;
        return -1;
    }

    int WalkToLastCopy(int startIdx, int targetPdg,
                        const ROOT::VecOps::RVec<int>& pdg,
                        const ROOT::VecOps::RVec<short>& mother,
                        const ROOT::VecOps::RVec<unsigned short>& statusFlags) {
        int idx = startIdx, n = (int)pdg.size();
        while (!HasBit(statusFlags[idx], BIT_IS_LAST_COPY)) {
            int nextIdx = -1;
            for (int j = 0; j < n; ++j)
                if (mother[j] == idx && pdg[j] == targetPdg) { nextIdx = j; break; }
            if (nextIdx < 0) break;
            idx = nextIdx;
        }
        return idx;
    }

    // 1=e, 2=mu, 3=tau, 0=hadronic, -1=no top found
    int ClassifyTopWDecay(const ROOT::VecOps::RVec<int>& pdg,
                           const ROOT::VecOps::RVec<short>& mother,
                           const ROOT::VecOps::RVec<unsigned short>& statusFlags) {
        int topIdx = FindLastCopyTop(pdg, statusFlags);
        if (topIdx < 0) return -1;
        int n = (int)pdg.size();
        int wIdx = -1;
        for (int j = 0; j < n; ++j)
            if (mother[j] == topIdx && std::abs(pdg[j]) == 24) { wIdx = j; break; }
        if (wIdx < 0) return 0;
        wIdx = WalkToLastCopy(wIdx, pdg[wIdx], pdg, mother, statusFlags);
        for (int j = 0; j < n; ++j) {
            if (mother[j] != wIdx) continue;
            int apdg = std::abs(pdg[j]);
            if (apdg == 11) return 1;
            if (apdg == 13) return 2;
            if (apdg == 15) return 3;
        }
        return 0;
    }

    // 1=ee, 2=mumu, 3=tautau, 0=other, -1=no Z found
    int ClassifyZDecay(const ROOT::VecOps::RVec<int>& pdg,
                        const ROOT::VecOps::RVec<short>& mother,
                        const ROOT::VecOps::RVec<unsigned short>& statusFlags) {
        int zIdx = FindLastCopyZ(pdg, statusFlags);
        if (zIdx < 0) return -1;
        int n = (int)pdg.size();
        int nE = 0, nMu = 0, nTau = 0;
        for (int j = 0; j < n; ++j) {
            if (mother[j] != zIdx) continue;
            int apdg = std::abs(pdg[j]);
            if (apdg == 11) nE++;
            else if (apdg == 13) nMu++;
            else if (apdg == 15) nTau++;
        }
        if (nE   >= 2) return 1;
        if (nMu  >= 2) return 2;
        if (nTau >= 2) return 3;
        return 0;
    }


    // ============================================================
    // NanoAOD GenPart statusFlags
    //
    // bit 12 = isFirstCopy
    // bit 13 = isLastCopy
    // ============================================================

    // constexpr unsigned int IS_FIRST_COPY = (1u << 12);
    // constexpr unsigned int IS_LAST_COPY  = (1u << 13);


    bool IsFirstCopy(int flags)
    {
        return (flags & IS_FIRST_COPY) != 0;
    }


    bool IsLastCopy(int flags)
    {
        return (flags & IS_LAST_COPY) != 0;
    }


}
