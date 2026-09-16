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


    // ============================================================
    // Find the LAST COPY of a particle.
    //
    // Starting from a first-copy particle, follow daughters with
    // the same absolute PDG ID until isLastCopy is reached.
    // ============================================================

    int FindLastCopy(
        int firstIdx,
        const RVec<int>& pdgId,
        const RVec<short>& mother,
        const RVec<unsigned short>& statusFlags)
    {
        const int n = pdgId.size();

        if (firstIdx < 0 || firstIdx >= n)
            return -1;

        const int absId = std::abs(pdgId[firstIdx]);

        int current = firstIdx;

        // Safety against pathological/cyclic histories
        std::set<int> visited;

        while (current >= 0 && current < n)
        {
            if (visited.count(current))
                return current;

            visited.insert(current);

            // We have reached the last copy
            if (IsLastCopy(statusFlags[current]))
                return current;

            int next = -1;

            // Find same-PDG daughter
            for (int i = 0; i < n; ++i)
            {
                if (mother[i] != current)
                    continue;

                if (std::abs(pdgId[i]) != absId)
                    continue;

                next = i;
                break;
            }

            if (next < 0)
                return current;

            current = next;
        }

        return current;
    }


    // ============================================================
    // Save ALL electron/muon copies.
    //
    // Starting from the FIRST COPY of an e/mu, follow e/mu copies
    // until the LAST COPY and save every index.
    // ============================================================

    void SaveLeptonCopyChain(
        int firstLeptonIdx,
        const RVec<int>& pdgId,
        const RVec<short>& mother,
        const RVec<unsigned short>& statusFlags,
        RVec<int>& output)
    {
        const int n = pdgId.size();

        int current = firstLeptonIdx;

        std::set<int> visited;

        while (current >= 0 && current < n)
        {
            if (visited.count(current))
                break;

            visited.insert(current);

            const int absId = std::abs(pdgId[current]);

            // Safety: only e/mu
            if (absId != 11 && absId != 13)
                break;

            // Save this copy
            output.push_back(current);

            // Stop at final copy
            if (IsLastCopy(statusFlags[current]))
                break;

            int next = -1;

            // Find same e/mu daughter
            for (int i = 0; i < n; ++i)
            {
                if (mother[i] != current)
                    continue;

                if (std::abs(pdgId[i]) != absId)
                    continue;

                next = i;
                break;
            }

            if (next < 0)
                break;

            current = next;
        }
    }


    // ============================================================
    // Process FINAL COPY of Z.
    //
    // We require the final Z to have a DIRECT e/mu daughter.
    //
    // Z -> e/mu       ACCEPT
    // Z -> tau -> e   REJECT
    // Z -> qq         REJECT
    // ============================================================

    void ProcessZ(
        int zLast,
        const RVec<int>& pdgId,
        const RVec<short>& mother,
        const RVec<unsigned short>& statusFlags,
        RVec<int>& output)
    {
        const int n = pdgId.size();

        for (int i = 0; i < n; ++i)
        {
            // Must be a DIRECT daughter of final Z
            if (mother[i] != zLast)
                continue;

            const int absId = std::abs(pdgId[i]);

            // Only electron or muon
            if (absId != 11 && absId != 13)
                continue;

            // We want the FIRST COPY of the lepton
            if (!IsFirstCopy(statusFlags[i]))
                continue;

            // Save all copies of this lepton
            SaveLeptonCopyChain(
                i,
                pdgId,
                mother,
                statusFlags,
                output
            );
        }
    }


    // ============================================================
    // Process FINAL COPY of W.
    //
    // We require the final W to have a DIRECT e/mu daughter.
    //
    // W -> e/mu       ACCEPT
    // W -> tau -> e   REJECT
    // W -> qq         REJECT
    // ============================================================

    void ProcessW(
        int wLast,
        const RVec<int>& pdgId,
        const RVec<short>& mother,
        const RVec<unsigned short>& statusFlags,
        RVec<int>& output)
    {
        const int n = pdgId.size();

        for (int i = 0; i < n; ++i)
        {
            // Must be a DIRECT daughter of final W
            if (mother[i] != wLast)
                continue;

            const int absId = std::abs(pdgId[i]);

            // Only electron or muon
            if (absId != 11 && absId != 13)
                continue;

            // Must be FIRST COPY of the lepton
            if (!IsFirstCopy(statusFlags[i]))
                continue;

            // Save all copies
            SaveLeptonCopyChain(
                i,
                pdgId,
                mother,
                statusFlags,
                output
            );
        }
    }


    // ============================================================
    // MAIN FUNCTION
    //
    // Selection:
    //
    // 1. Z -> e/mu
    //
    //    Z(first)
    //       |
    //       v
    //    Z(last)
    //       |
    //       v
    //    e/mu(first)
    //       |
    //       v
    //    e/mu copies
    //
    // 2. top -> W -> e/mu
    //
    //    top(first)
    //       |
    //       v
    //    top(last)
    //       |
    //       v
    //    W(first)
    //       |
    //       v
    //    W(last)
    //       |
    //       v
    //    e/mu(first)
    //       |
    //       v
    //    e/mu copies
    // ============================================================

    RVec<int> GetAllPromptGenLeptonIndices(
        const RVec<int>& pdgId,
        const RVec<short>& mother,
        const RVec<unsigned short>& statusFlags)
    {
        RVec<int> output;

        const int n = pdgId.size();


        // ========================================================
        // PART 1: Z -> e/mu
        // ========================================================

        for (int i = 0; i < n; ++i)
        {
            // Find Z
            if (std::abs(pdgId[i]) != 23)
                continue;

            // Only start from FIRST COPY of Z
            if (!IsFirstCopy(statusFlags[i]))
                continue;

            // Follow Z copies to LAST COPY
            const int zLast =
                FindLastCopy(
                    i,
                    pdgId,
                    mother,
                    statusFlags
                );

            if (zLast < 0)
                continue;

            // Check direct decay of final Z
            ProcessZ(
                zLast,
                pdgId,
                mother,
                statusFlags,
                output
            );
        }


        // ========================================================
        // PART 2: top -> W -> e/mu
        // ========================================================

        for (int i = 0; i < n; ++i)
        {
            // Find top
            if (std::abs(pdgId[i]) != 6)
                continue;

            // Only start from FIRST COPY of top
            if (!IsFirstCopy(statusFlags[i]))
                continue;

            // Follow top copies to LAST COPY
            const int topLast =
                FindLastCopy(
                    i,
                    pdgId,
                    mother,
                    statusFlags
                );

            if (topLast < 0)
                continue;


            // ----------------------------------------------------
            // Find DIRECT W daughter of final top
            // ----------------------------------------------------

            for (int j = 0; j < n; ++j)
            {
                if (mother[j] != topLast)
                    continue;

                // Must be W
                if (std::abs(pdgId[j]) != 24)
                    continue;

                // Must be FIRST COPY of W
                if (!IsFirstCopy(statusFlags[j]))
                    continue;


                // ------------------------------------------------
                // Follow W copies to LAST COPY
                // ------------------------------------------------

                const int wLast =
                    FindLastCopy(
                        j,
                        pdgId,
                        mother,
                        statusFlags
                    );

                if (wLast < 0)
                    continue;


                // ------------------------------------------------
                // Check DIRECT decay of final W
                // ------------------------------------------------

                ProcessW(
                    wLast,
                    pdgId,
                    mother,
                    statusFlags,
                    output
                );
            }
        }


        // ========================================================
        // Remove duplicate indices
        // ========================================================

        RVec<int> uniqueOutput;

        std::set<int> seen;

        for (auto idx : output)
        {
            if (!seen.count(idx))
            {
                seen.insert(idx);
                uniqueOutput.push_back(idx);
            }
        }

        return uniqueOutput;
    }


    // ============================================================
    // Convert selected GenPart indices -> PDG IDs
    // ============================================================

    RVec<int> GetPromptGenLeptonPdgId(
        const RVec<int>& indices,
        const RVec<int>& pdgId)
    {
        RVec<int> output;

        for (auto idx : indices)
        {
            if (idx >= 0 && idx < (int)pdgId.size())
                output.push_back(pdgId[idx]);
        }

        return output;
    }


    RVec<bool> MatchLeptonToPromptGen(
        const RVec<short>& leptonGenPartIdx,
        const RVec<int>& promptGenLeptonIdx)
    {
        RVec<bool> output;

        for (auto idx : leptonGenPartIdx)
        {
            bool found = false;

            // -1 means no GenPart match
            if (idx >= 0)
            {
                for (auto promptIdx : promptGenLeptonIdx)
                {
                    if (idx == promptIdx)
                    {
                        found = true;
                        break;
                    }
                }
            }

            output.push_back(found);
        }

        return output;
    }
    int CountPromptMatchedLeptons(
        const ROOT::VecOps::RVec<bool>& matches)
    {
        int count = 0;

        for (bool match : matches)
        {
            if (match)
                ++count;
        }

        return count;
    }
}
