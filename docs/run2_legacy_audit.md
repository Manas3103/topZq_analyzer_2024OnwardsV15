# Run-2 Legacy Audit

Audit date: 2026-10-07. Source baseline: `ac8fee688cdc215bbe51914404c7233635ec4e0b` on `cleanup/run3-remove-legacy-run2`.

## 1. Executive Summary

This is a static audit, not a cleanup approval or a physics validation. No Run-2 code or inputs were removed. The only authorized repository content change is this report.

The development target is CMS Run 3 (2022–2025), currently centered on 2024 NanoAODv15. `processnanoaod_v.py` is production; the two standalone executables are debugging/test paths. Existing Run-3 support is uneven: targeting 2025 does not mean that 2025 dispatch, schemas, triggers, and corrections are implemented.

The strongest standalone removal candidates are isolated Run-2 calibration/certification collections and `data/make2017MCpileup.C`. Checked-in production configurations and debugging mains select Run-3 payloads; repository searches did not identify code loading the named Run-2 inputs. External configurations and historical reproducibility remain user decisions, so these are candidates, not proven universally unused files.

Important exclusions from a simple deletion set:

- `processonefile.py` is conditionally reachable from production with `allinone=False`, despite its obsolete interface.
- `src/SkimEvents.*` is a Run-2-oriented subclass with no current production caller, but its `.cpp` is compiled by the Makefile wildcard.
- `src/utility.*`, the framework, RDF tree machinery, truth helpers, muon correction helper, and dictionary interfaces provide active Run-3 functions.
- Non-2024 MVA/JetID/JEC/JER fallbacks cover planned Run-3 years as well as historical cases. They cannot be removed as Run-2-only branches.
- The 2016-named halo filter is an active event-quality requirement. Its name is not evidence for deletion.
- FF coefficients explicitly described as 2018 placeholders affect current 2024 results and require physics review.
- Old histogram implementations, quantile studies, archives, and the alternate BTag framework copy need a user decision; several are Run-3-capable or general algorithms rather than Run-2-only code.

## 2. Method and Scope

Read `AGENTS.md` and used `docs/codex_repository_analysis.md` as investigation context, then rechecked relevant implementation. Starting Git status was clean. The current branch and HEAD were inspected without changing them.

Methods: tracked-file and filesystem inventories; repository-wide `rg` searches for 2016/2017/2018, Run2/RunII, UL, Legacy, old taggers and calibration paths; reads of source, headers, configurations, submit files, wrappers, and documentation; tracing imports, includes, callers, Makefile dependencies, dictionary registrations, and output consumers. Local JSON/gzip JSON metadata and CSV headers were inspected with Python standard-library file reads, without ROOT or correctionlib imports. Archive member names were listed without extraction.

Search matches were filtered conceptually: `Run2024`, `RunIII`, numerical calibration values, author dates, and generic `ULong64_t` types are not Run-2 evidence. Payload metadata examples establish family/schema identity, not the correctness of every numerical entry.

Reachability terminology:

- **Production-current:** reachable with the checked-in 2024 configurations (`allinone=True`).
- **Production-conditional:** reachable through supported argument/configuration choices, including `allinone=False` or a different analyser/year.
- **Build-only:** compiled/linked even if not selected by production.
- **Standalone/manual:** only a separate macro, helper, or debugging entry point was found.
- **No in-repository consumer found:** static reference absence, not proof against external/manual consumers.

Confidence is HIGH/MEDIUM/LOW for the stated evidence/classification, not an unconditional safety guarantee. Actions are KEEP, REMOVE CANDIDATE, REFACTOR CANDIDATE, NEEDS PHYSICS REVIEW, or NEEDS USER DECISION. Grouped findings apply the same dependency/impact assessment to their listed members. Exact Run-2 input paths are inventoried in section 10.

### Production and dependency map

```text
job_2024*.submit → runjob_24_v2.sh → processnanoaod_v.py
  ├─ import configuration module; load correctionlib/MathMore/analyser library
  ├─ allinone=True → input list/DAS/EOS/local discovery → Events TChain
  │    → BaseAnalyser or FakeFactorAnalyser
  │    → setParams → setupCorrections → setupObjects → setupAnalysis → run
  └─ allinone=False → Nanoaodprocessor → processonefile.py
       → btagging_efficiency_binning.py → BaseAnalyser + older correction call

Makefile: src/*.cpp except two mains + rootdict.o
  → libnanoadrdframe.so and two debugging executables
  → SkimEvents.cpp included; BTag/framework copy and src/used_function excluded

Framework → utility / RNodeTree / correctionlib / MuonScaRe.cc
BaseAnalyser → framework / utility / GenParticleHelper
FakeFactorAnalyser → BaseAnalyser / framework / utility
Linkdef.h + analyser headers → generated rootdict.C → Python class reflection
Snapshots: outputTree_<cut-index>, currently outputTree_00000 at final leaf
  → normalized_hist JSON/schema/weight consumers
```

## 3. Standalone Run-2 Legacy / Removal Candidates

| ID | Path / section | Evidence and reachability | Dependencies and consumers | Effect / confidence / action |
|---|---|---|---|---|
| S1 | `data/BTV/2017_UL/`, `data/BTV/2018_UL/` | Run-2 tagging families; inspected correction names include deepCSV/deepJet-style fixed-WP calibrations. No named loader found in current config/mains. | Generic configurable BTV loader could load another file; historical/external configurations remain possible. | Loses historical tagging inputs; no identified 2024 input change. HIGH Run-2 identity, MEDIUM removal safety. REMOVE CANDIDATE. |
| S2 | `data/EGM/2016preVFP_UL/`, `2016postVFP_UL/`, `2017_UL/`, `2018_UL/`; `data/ELECTRON/2018_UL/` | Electron/photon UL payload metadata includes `UL-Electron-ID-SF`/`UL-Photon-*`; distinct from current `Electron-ID-SF` Run-3 evaluator. No named consumer found. | Generic file configuration and possible external macros. | Removes Run-2 electron/photon calibrations only if externally unused. HIGH identity, MEDIUM safety. REMOVE CANDIDATE. |
| S3 | `data/MUO/{2016preVFP_UL,2016postVFP_UL,2017_UL,2018_UL}/` | UL muon JSONs and RoccoR text files; inspected UL SF schema includes year input. Current momentum correction uses Run-3 scale/smear JSON and MuonScaRe helper. | No active RoccoR class implementation/include found; dormant Makefile recipe is not a loader. Generic external configuration is possible. | Historical SF/Rochester inputs disappear; current MUO/2024 inputs must remain. HIGH identity, MEDIUM safety. REMOVE CANDIDATE. |
| S4 | `data/MUON/2017_UL/`, `2018_UL/`, `data/MUON/UL/` | Separate older muon collections organized by 2016 pre/postVFP, 2017 and 2018; no named production reference found. Some are older JSON schemas rather than current correctionset format. | Historical consumers not recoverable from checked-in code. | Removes old calibration material; do not delete all MUO/MUON by broad substring. HIGH identity, MEDIUM safety. REMOVE CANDIDATE. |
| S5 | `data/LUM/{2016preVFP_UL,2016postVFP_UL,2017_UL,2018_UL}/` | UL pileup tags such as `Collisions16_UltraLegacy_goldenJSON`; current config loads `data/LUM/2024/puWeights_BCDEFGHI.json`. | Shared `pucorrection()` accepts configured tags/files. | Removes historical inputs, not the shared pileup function. HIGH identity, MEDIUM safety. REMOVE CANDIDATE. |
| S6 | `data/Legacy_RunII/`; three root-level `Cert_*Collisions16/17/18*` text files | Certification run ranges and names identify Run-2 data. Current GoldenJSON configuration selects 2024; no named active loader found. | Generic `readgoodjson()` can load user-specified certification files. | Removes old data certification records; preserve 2022/2023/2024 GoldenJSON files. HIGH identity, MEDIUM safety. REMOVE CANDIDATE. |
| S7 | `data/JERC/UL17_jerc.json` | Inspected metadata identifies Summer19UL17 AK4PFchs JEC/JER. Current config uses Summer24 AK4PFPuppi. | Generic JERC loader, no named reference found. | Removes historical payload, not general JEC/JER dispatch. HIGH identity, MEDIUM safety. REMOVE CANDIDATE. |
| S8 | `data/CSVv2_*.csv`, `data/DeepCSV_*.csv`, `data/reshaping_deepCSV_106XUL18_v2.csv` | Old calibration names and CSV headers; current loader is correctionlib JSON, not a CSV reader. No corresponding CSV input reference found. | External historical tools unknown; generic shared b-tag helpers are not evidence that these CSVs are loaded. | Removes old tagging calibration formats. MEDIUM/HIGH identity, MEDIUM safety. REMOVE CANDIDATE. |
| S9 | `data/make2017MCpileup.C`, `make2017MCPileup()` | Fixed 2017 MC pileup distribution, CMSSW_10_4_X reference, and `PileupMC2017v4.root` output; no caller/build target found. | ROOT histogram/file classes; manually invoked macro; no output consumer found in repository. | Removes an isolated Run-2 input-generation macro. HIGH. REMOVE CANDIDATE. |
| S10 | `src/SkimEvents.cpp/.h`, `defineCuts()` | Explicit 2016 hadronic triggers; old `Sel_*` and fat-jet/skim schema. No current class selection or caller found; dictionary pragma is commented. | **Build-only** via `src/*.cpp`; includes framework and utility. | Removing source changes library symbols/build objects; cannot delete framework dependencies. HIGH legacy identity, MEDIUM removal safety. REFACTOR CANDIDATE for phase 2, not first input-only cleanup. |

No current production configuration explicitly selects a Run-2 year or payload. This does not authorize deletion and does not establish that all dynamic/manual usage has been found.

## 4. Run-2 Logic Embedded in Active Run-3 Code

| ID | Path / function / section | Specific evidence / reachability | Dependencies / downstream effect | Confidence / action |
|---|---|---|---|---|
| E1 | `src/NanoAODAnalyzerrdframe.cpp:setHLT()` (~2645); `.h:HLT2016/17/18Names` | Historical year dispatch remains in production framework. Main 2024 dispatch uses `HLT2022EENames`; Run-2 lists are not populated by the checked-in active constructors. | `BaseAnalyser::defineCuts()` calls setHLT; output filter is cut `0`. Deleting public members changes class layout/dictionary and any external subclass contract. Preserve 2022/2023 branches. | HIGH. REFACTOR CANDIDATE. |
| E2 | Same `.cpp:setParams()` (~2544) | 2017 log branch remains; `_runtype` now controls PreEE/PostEE/PreBPix/PostBPix, not an active UL/ReReco selector. | Called before correction setup/data-MC inspection. Do not delete runtype argument or era flags because documentation still describes UL. | HIGH. REFACTOR CANDIDATE for old log branch only; KEEP Run-3 era interface. |
| E3 | Same `.cpp:calculateBTagSF()` (~1758,1784–1787) | 2016 light-flavor skip and 2016 variation list remain. Nearby comment says “for 2024” on the 2016 branch and conflicts with dispatch. Current main calls case 1 only after a 2024 WP check. | Creates `btag_SF_*` columns consumed by vectors and `evWeight`; removal must preserve 2024 names/order and future-year behavior. `_redefine` path and case 3 are separate interfaces. | HIGH code evidence. REFACTOR CANDIDATE; variation prescription NEEDS PHYSICS REVIEW. |
| E4 | `src/BaseAnalyser.cpp:selectElectrons()/selectMuons()` (~197,315) | 2024 uses `*_promptMVA`; **all other years** use `*_mvaTTH`. Historical schema-looking fallback is reachable for 2022, 2023, 2025, not established Run-2-only. | Tight flags, `baselineElectrons_mvaTTH` alias, merged prompt arrays, stored branches and region variables. Branch replacement can change physics/schema; maintain masks/permutations. | HIGH reachability, LOW validity for untested years. NEEDS PHYSICS REVIEW. |
| E5 | `src/NanoAODAnalyzerrdframe.cpp:ElectronID()/MuonID()` (~2722,2751); Base call ~237 | Fall17-style ID comment/log conventions coexist with generic NanoAOD IDs. `MuonID(2)` is called despite a “tight” caller comment; implementation selects loose ID. | Defines `goodMuonsID`; baseline uses separate explicit tightId cuts. Need downstream usage trace before changing IDs; old comments do not justify selection changes. | HIGH. KEEP helper functionality; REFACTOR CANDIDATE for reviewed documentation/interface clarity. |
| E6 | `src/FakeFactorAnalyser.cpp:selectFakeableMuons()` (~133–189) | Active `sliding_a=0.025`, `sliding_b=0.015` explicitly described as a 2018 placeholder. Uses corrected pT approximation and `Muon_jetDF` in the sliding cut. | Direct production FF path; changes fakeable mask, cone-pT, MR acceptance and numerator/denominator histograms. | HIGH. NEEDS PHYSICS REVIEW; not dead code. |
| E7 | Same `.cpp:selectFakeableElectrons()` and `.h` comments | Commented Fall17/MVA-TOP branch assumptions and 0.4 tight threshold disagree with active `Electron_promptMVA`, `Electron_mvaIso_WP90` and >0.9 tight condition; analogous muon comments disagree with >0.64. | Active fakeable masks consume `Electron_jetDF`/jet ratios; thresholds determine FF columns and cuts. | HIGH discrepancy, LOW prescribed replacement. NEEDS PHYSICS REVIEW. |
| E8 | Same `.cpp:defineFFTriggerWeight()` (~324–377); `.h` | Header says Run-2 prescale placeholders; implementation labels numbers updated for 2024. Values: electrons 9116/1540; muons 21900/5214/322/1095/497. | Seven direct HLT inputs plus per-path pT/cone/jet windows → `FFTrig`, `passFFTrigger`, `FFTrigWeight`, FF weights/hists. Constants are live. | HIGH inconsistency, LOW provenance. NEEDS PHYSICS REVIEW. |
| E9 | `jobconfiganalysis_2024*.py` comments; `README.md` examples; framework header RoccoR comment | UL/ReReco and 2018 examples persist while real configs use year 2024, empty runtype and Run-3 muon JSON under `muon_roch_fname`. | Python/C++ positional correction interface consumes that key; naming alone is not a Rochester dependency. README also covers general build/use. | HIGH. REFACTOR CANDIDATE for scoped documentation; KEEP files/interfaces until separately approved. |

`Flag_globalSuperTightHalo2016Filter` appears in both active cut definitions and stored-branch patterns. Its consumer is current event-quality filtering, not a Run-2-only conditional. **KEEP** (HIGH reachability); changing the event-quality prescription requires physics review. Similarly, creation dates such as 2018 on utility/RNodeTree/Linkdef headers carry no removal implication.

## 5. Shared Run-2 / Run-3 Components

| ID | Path / section | Reachability and dependencies | Removal/change effect | Confidence / action |
|---|---|---|---|---|
| H1 | `src/NanoAODAnalyzerrdframe.*` | Production framework; RDF root/node, corrections, data/MC, cuts/hists/snapshots. Contains historical fields and generic interfaces. | Whole-file deletion destroys production. Separate explicit legacy clauses from shared implementation. | HIGH / KEEP. |
| H2 | `src/BaseAnalyser.*` | Main production analyser and FF base; uses utility/truth/framework. | Objects, cleaning, masks, sorting, reconstruction, regions, weights and output schema all affected. | HIGH / KEEP. |
| H3 | `src/FakeFactorAnalyser.*` | Configured production FF class; inherited jets/corrections plus active legacy-derived constants. | Deletion removes requested Run-3 FF workflow. | HIGH / KEEP; constants require review. |
| H4 | `src/utility.cpp/.h` | Compiled library; active four-vector, PU, neutrino, OSSF, truth-origin, region branches and FF helpers. | Whole-file deletion breaks active columns/callers. `pucorrection()` is payload-neutral. | HIGH / KEEP. |
| H5 | Utility `btv_case1/2`, `btvcorrection`, `muoncorrection` (~136–340) | Old deepJet keys/2018 comment/year+runtype signature; declarations and definitions found, no call found from current main/FF or alternate BTag framework. Compiled/exportable, generic taggers not uniquely Run-2. | Function-level removal changes public helper API; do not remove nearby used reconstruction or PU helpers. External callers unknown. | MEDIUM / REFACTOR CANDIDATE after user/API decision. |
| H6 | `src/GenParticleHelper.*` | Main MC selects prompt generator leptons and matches reconstructed leptons; status flags/ancestry are shared NanoAOD logic. | Truth branch definitions and matching change. No actual Run-2-only behavior identified. | HIGH / KEEP. |
| H7 | `src/MuonScaRe.*` | Framework includes `.cc` directly and instantiates helper from Run-3 scale/smearing JSON. | Changes nominal and varied Muon_pt columns, deterministic random behavior, selections and SF inputs. RoccoR filename remnants do not make this legacy. | HIGH / KEEP. |
| H8 | `src/RNodeTree.*`, `src/Linkdef.h`, `src/rootdict.C` | Cut topology and reflected classes are active; dictionary includes framework/Base/FF headers, not SkimEvents. | Cut index/leaf/outputTree contracts, Python construction and serialization can break. Dictionary is generated and tracked. | HIGH / KEEP; any ABI/interface cleanup needs an authorized build later. |
| H9 | `Makefile` | Common source wildcard includes active and old SkimEvents source. Dictionary, correctionlib and ROOT link paths support production. | RoccoR explicit target references missing `src/RoccoR.cpp/.h` and is not in current OBJS; dormant test dictionary target also references absent files. Removing target is build cleanup, not a physics fix. | HIGH / KEEP main build; REFACTOR CANDIDATE for dormant recipes. |
| H10 | `BTag/btag_efficiency.C`, `btag.C`, filelists and efficiency ROOT inputs | 2022/EE, 2023/BPix and 2024 WPs; macro default is 2024, tagger UParT for 2024 and PNet otherwise. btag.C produces six efficiency hist names consumed by framework. | Input-generation changes can alter b-tag event weights. An unknown year falls back to numerical values labeled 2023; not a Run-2-only fallback. | HIGH / KEEP; future-year/fallback changes NEEDS PHYSICS REVIEW. |
| H11 | `processnanoaod_v.py`, current configs, submitanalysisjob.py, submit/wrapper/status/resubmission scripts | Main current/conditional orchestration; wrappers may set environments, stage/delete outputs. Author dates/UL comments are not enough to classify scripts as Run-2-only. | Input discovery, batch arguments, outputs/logs and conditional single-file dispatch affected. | HIGH / KEEP. |
| H12 | `DataMC_txt/` Run-3 lists, splitter/preparation, SumOfGenWeight_calculator.py and temp helpers | 2022/2023/2024 samples and generic file batching/normalization tools. | Queue path prefixes and Runs/Events weight bookkeeping feed production/downstream. No Run-2-only workflow established for these helpers. | HIGH Run-3/generic role, MEDIUM external usage / KEEP. |
| H13 | `normalized_hist/Python_rdf_norm/` active generator/configuration/processor and sample JSON/text | Main/FF submits invoke create_hist_rdf.py; exact branch names/tree and normalization inputs matter. | Changes downstream histogram content or normalization; “old” alternative versions are not proven Run-2-only. | HIGH active role / KEEP. |
| H14 | `data/` 2022/2023/2024 families; GoldenJSON; generic JERC metadata | Planned/active Run-3 inputs. `data/JERC/README.md` describes general input contracts alongside Run-2 examples. | Removing unused-in-2024 2022/2023 files harms intended Run-3 scope. | HIGH / KEEP. |

## 6. Ambiguous Items Requiring User or Physics Decisions

| ID | Path / section | Evidence, dependencies, reachability | Possible effect | Confidence / action |
|---|---|---|---|---|
| A1 | `processonefile.py`, `btagging_efficiency_binning.py` | **Production-conditional** through allinone=False. Dictionary contains a 13 TeV t-channel sample and filename suffix. Old config keys and shorter setupCorrections call mismatch current signatures; class hard-coded BaseAnalyser. | Deleting breaks conditional mode; modernizing requires choosing its support scope, SF interface and handling data/no dataset match. Old interface does not prove Run-2-only. | HIGH caller/mismatch, MEDIUM historical identity / NEEDS USER DECISION. |
| A2 | `BTag/NanoAODAnalyzerrdframe_sneh.cpp` | Alternate framework implementation includes MuonScaRe and Run-2/Run-3 year dispatch, including 2024; UL/ReReco flags. Excluded by top-level wildcard; no named caller found. | May be manually used reference or calibration-generation source; wholesale deletion is not Run-2 removal. Duplicate class definitions would conflict if added to build. | HIGH mixed-era code, LOW external role / NEEDS USER DECISION. |
| A3 | `GetQuantile_Method/Quantiles_jj.C`, `Mergebins500.C`, README and support dirs | Hard-coded 2018UL Tprime/B2G histogram studies (`TPMassvsDRjjW_CD`, `TPMassvsRelative_THT_CD`), no production/build caller. Quantile/merging algorithms themselves are year-neutral. | No identified current output impact; loses reusable/reference methodology and historical study. Entire directory contains .DS_Store artifacts, not just Run-2 logic. | HIGH standalone wiring, MEDIUM retirement safety / NEEDS USER DECISION; conditional REMOVE CANDIDATE after retirement approval. |
| A4 | `src/used_function/used_func.cpp`, `EventShape.*` | Excluded from build; repeated calculateZBosonMass definitions and general event-shape algorithm. Base header retains declarations, but active reconstruction uses different helpers. No actual Run-2 year restriction established. | Future/manual reuse unknown; deletion is general unused-code cleanup, not proven Run-2-only. | MEDIUM / NEEDS USER DECISION. |
| A5 | `normalized_hist/normalization_with_C/`, Python old/unused variants and Test_yield | C++ submit uses 2024 queue and transfers 2022 material; wrappers/macros have different weight formulas/tree defaults. Separate Run-3 downstream path despite legacy appearance. | Could remove still-used production tools or change expected normalization. | HIGH mixed/generic evidence, LOW current operational usage / NEEDS USER DECISION. |
| A6 | `normalized_hist/Python_rdf_norm/my_ploting_project.tar.gz`, generated outputs/caches/logs/.root_hist/.DS_Store | Archive members include create_hist_rdf.py, 2024_MC.txt, submit_jobs.sh, job_hist.submit, hist_config.json. Archive contents not fully audited; binary ROOT files not opened. | Archive or output deletion may remove provenance/calibration/reference material; no Run-2 classification from age alone. | LOW / NEEDS USER DECISION. |
| A7 | `data/JERC/jer_smear.json.gz`, 2024 Winter/Summer alternates and backup; `data/JERC/README.md`, `data/README_JSONfiles.md`, top-level README | Generic/alternate payloads and mixed documentation; the latter notes 2018_UL but includes generic payload/certification references. No exclusive Run-2 proof for generic smear or whole README. | Broad directory cleanup could remove useful API docs or future inputs. | MEDIUM / KEEP pending specific decision; documentation REFACTOR CANDIDATE. |
| A8 | Planned 2025 and partial 2022/2023 support | No explicit 2025 dispatch found in inspected active classes/configs. Main calculateEvWeight returns for non-2024; MVA, JEC/JER, trigger and BTag variants have incomplete future-year handling. | Generic else branches can silently use inappropriate assumptions or miss columns. Run-2 removal cannot establish Run-3 support by itself. | HIGH static absence/dispatch evidence, LOW runtime validity / NEEDS PHYSICS REVIEW and user scope decision. |
| A9 | Ignored `data/BTV/2017_UL/BtaggingEfficiency.root` | Present on disk inside a Run-2 payload directory, but not in tracked-file manifest. No named source/configuration reference found; contents were not opened. | Could be a historical efficiency input. Do not remove it incidentally when retiring tracked JSON files or deleting the parent directory. | LOW content/usage confidence / NEEDS USER DECISION. |

## 7. Dependency and Build Risks

1. Never delete a compiled core file because one function/comment is historical. `src/*.cpp` is the actual common build set; `.cc` MuonScaRe is included directly, whereas `src/used_function` and BTag's alternate framework are outside that set.
2. SkimEvents retirement needs a source/header/include/dictionary/public API check and later authorized build validation. Its commented Linkdef entry is not equivalent to absence from the library.
3. Public member/function removal changes class layout or exported symbols. Header changes require regeneration of tracked rootdict.C and PCM/library artifacts through an explicitly authorized build, not manual dictionary editing. The Makefile's dependency coverage should be reviewed separately; old binaries do not validate new source.
4. The dormant RoccoR rule does not put RoccoR in OBJS when sources are absent. Removing its payloads and removing its recipe are separate actions.
5. Conditional Python paths matter: allinone=False is an actual subprocess edge to processonefile.py. Dynamic configuration-module and analyser lookup mean static absence cannot exclude external consumers.
6. Calibration paths are string/configuration contracts. ROOT BTag efficiencies may be ignored by Git but are required runtime inputs. Do not apply a blanket removal to `*.root` or whole data/BTag trees.
7. Current selected-branch snapshots use outputTree_00000 through five nested cut indices; downstream wrappers read that exact name. Branch regexes and histogram JSONs form another contract. Legacy-looking aliases such as baselineElectrons_mvaTTH can hold 2024 promptMVA values.
8. Main Condor input transfer is `.`; candidates are incidentally packaged even when not loaded. Reduced packaging is not evidence that physics behavior is preserved. Do not edit queues or execute wrappers during audit.

## 8. Run-3 Physics/Behavior Risks

These are review boundaries, not authorized bugs/fixes:

- **Triggers:** 2024 uses the populated 2022EE-named menu; 2023 menu is declared but no assignment was found in active constructors; 2025 has no explicit branch. FF replaces 2022EE-named vector with its seven triggers, but trigger evaluation also directly requires all seven columns. Renaming/deleting old-year-named vectors can break 2024.
- **Objects/schema:** non-2024 mvaTTH fallback and JetID fallback must be mapped to actual 2022/2023/2025 NanoAOD schemas. `JetID(6)` fallback cites 13.6 TeV NanoAOD bugfix criteria; it is not established Run-2 code. Current 2024 selectJets builds a manual ID mask and does not use that correctionlib helper branch.
- **JEC/JER:** data non-2024 JEC uses four inputs; MC adds phi for 2023/2024 and uses four inputs otherwise (including 2022). These are shared schema decisions. JER includes a variation input except for 2024's two-input SF evaluation. Removing else paths would affect Run-3, not just Run-2.
- **MET:** propagation consumes nominal/varied jet columns and PUPPI MET. Commented alternative propagation is not necessarily Run-2; XY loader/application are disabled while 2024 config names a 2023 BPix XY file. No automatic replacement prescription is justified.
- **Lepton corrections:** `muon_roch_fname` supplies current scale/smear JSON. MuonScaRe and deterministic seeds must survive naming/interface cleanup. Run-2 Rochester text files are a separate input family.
- **BTag:** 2016 clauses are separable historical branches, but 2022/2023/2024 heavy-flavor dispatch and unconditionally UParTAK4 light-flavor evaluation need physics review for multi-year use. Main weight setup currently accepts only 2024. Variation names, vector ordering, selected hadflavor alignment and efficiencies are protected.
- **Weights/normalization:** main nominal evWeight includes PU/gen × BTag × lepton factors; muon central currently uses ISO only. Current downstream histogram processor instead uses normalization × genWeight (other factors commented). The old processor uses a different formula. Do not synchronize them under a Run-2 cleanup label. Chain-level genEventSumw is not automatically a merged dataset denominator.
- **Fake factors:** Run-2-derived sliding coefficients, conflicting MVA documentation and prescale provenance directly affect current masks/weights. Unweighted FF numerator/denominator all use one; weighted eta plots use signed eta with nonnegative ranges. These earlier investigation concerns remain review notes, not cleanup tasks.
- **RDF/indexing/output:** masks, permutations, merged vectors and stored columns must remain aligned. The documented repeated permutation in goodLepton_ptCut is independent of Run-2 retirement. Do not fix it incidentally. Preserve region flags/masked branches and cutflow topology.
- **Data/MC:** keep certification only for data and MC truth/smearing/gen/PU/SF handling only for MC. A Run-2 payload retirement must not remove generic certification or weighting functionality.

## 9. Recommended Cleanup Sequence

Every phase below needs a separate user-authorized task; this report authorizes none of them.

### Phase 1 — safest standalone cleanup

Confirm archival/external-use policy, then approve an exact manifest of isolated Run-2 input files and the 2017 pileup macro. Preserve all Run-3 payloads, generic helpers, ROOT BTag inputs, scripts, current documentation and source interfaces. Review the diff and repeat reference inventory; do not claim runtime preservation from static checks alone.

### Phase 2 — dependency/build cleanup

Decide SkimEvents and the dormant RoccoR/test build-target retention separately. Decide the conditionally reachable single-file processor before any deletion. Review headers, reflection, class ABI, library symbols and callers. Build/dictionary regeneration requires explicit execution authorization. Do not include alternate Run-3 workflows merely because they are old.

### Phase 3 — active-code Run-2 refactoring

Remove only explicitly approved Run-2 dispatch/fields and obsolete helper interfaces after proving the retained Run-3 path and output/API contracts. Preserve Run-3 era routing and all needed fallback branches. Separate documentation correction from behavior changes. Do not propagate changes back into old Run-2 implementations.

### Phase 4 — physics-sensitive cleanup

With approved per-year schemas and prescriptions, address FF constants, trigger menus, object IDs/MVA, JEC/JER/MET, tagging/weights and normalization. Each is a separate physics-reviewed change. Plan bounded authorized validation against reference cutflows, branches and yields; compilation alone is insufficient.

## 10. Proposed First Safe Cleanup Set

**Proposed, not approved or removed.** The conservative initial set is S1–S9: the individually inventoried Run-2 input files below and `data/make2017MCpileup.C`. “Safe” means no identified checked-in current Run-3 loader/build edge, subject to historical/external-use confirmation; it does not assert runtime validation.

Exclude SkimEvents, processonefile.py, its binning dictionary, shared src files, generated dictionary, Makefile, BTag macros/ROOT inputs, all 2022/2023/2024 inputs, all normalization alternatives, archives, README files and active FF code from this first phase. Quantile macros may be a later standalone set only if the user explicitly retires the study.

The payload groups comprise 77 tracked input files at the audit baseline: BTV 5, EGM 8, ELECTRON 1, MUO 8, MUON 37, LUM 5, JERC 1, Legacy_RunII 3, root-level certification 3, and CSV 6. The macro is an additional file, giving 78 tracked first-phase candidates. The filesystem also contains the ignored BTag ROOT input classified as A9; it is excluded. Group counts describe candidate identity, not approval to delete a directory.

### Exact input manifest

The following manifest is appended from tracked-file inventory; each path inherits its S1–S8 finding's dependency, effect, confidence and recommended action. Paths are quoted to preserve the trailing space on the Legacy2018 certification filename.

```text
'data/BTV/2017_UL/btagging.json'
'data/BTV/2017_UL/cjets.json'
'data/BTV/2018_UL/btagging.json'
'data/BTV/2018_UL/btagging_v01.json'
'data/BTV/2018_UL/cjets.json'
'data/CSVv2_94XSF_V2_B_F.csv'
'data/CSVv2_Moriond17_B_H.csv'
'data/Cert_271036-284044_13TeV_23Sep2016ReReco_Collisions16_JSON.txt'
'data/Cert_294927-306462_13TeV_EOY2017ReReco_Collisions17_JSON.txt'
'data/Cert_314472-325175_13TeV_17SeptEarlyReReco2018ABC_PromptEraD_Collisions18_JSON.txt'
'data/DeepCSV_102XSF_V1.csv'
'data/DeepCSV_2016LegacySF_V1.csv'
'data/DeepCSV_94XSF_V4_B_F.csv'
'data/EGM/2016postVFP_UL/electron.json.gz'
'data/EGM/2016postVFP_UL/photon.json.gz'
'data/EGM/2016preVFP_UL/electron.json.gz'
'data/EGM/2016preVFP_UL/photon.json.gz'
'data/EGM/2017_UL/electron.json.gz'
'data/EGM/2017_UL/photon.json.gz'
'data/EGM/2018_UL/electron.json.gz'
'data/EGM/2018_UL/photon.json.gz'
'data/ELECTRON/2018_UL/electron.json'
'data/JERC/UL17_jerc.json'
'data/LUM/2016postVFP_UL/puWeights.json'
'data/LUM/2016preVFP_UL/puWeights.json'
'data/LUM/2017_UL/puWeights.json'
'data/LUM/2018_UL/puWeights.json'
'data/LUM/2018_UL/puWeights_v01.json'
'data/Legacy_RunII/Cert_271036-284044_13TeV_Legacy2016_Collisions16_JSON.txt'
'data/Legacy_RunII/Cert_294927-306462_13TeV_UL2017_Collisions17_GoldenJSON.txt'
'data/Legacy_RunII/Cert_314472-325175_13TeV_Legacy2018_Collisions18_JSON.txt '
'data/MUO/2016postVFP_UL/RoccoR2016bUL.txt'
'data/MUO/2016postVFP_UL/muon_Z.json.gz'
'data/MUO/2016preVFP_UL/RoccoR2016aUL.txt'
'data/MUO/2016preVFP_UL/muon_Z.json.gz'
'data/MUO/2017_UL/RoccoR2017UL.txt'
'data/MUO/2017_UL/muon_Z.json.gz'
'data/MUO/2018_UL/RoccoR2018UL.txt'
'data/MUO/2018_UL/muon_Z.json.gz'
'data/MUON/2017_UL/Efficiencies_muon_generalTracks_Z_Run2017_UL_ID.json'
'data/MUON/2017_UL/Efficiencies_muon_generalTracks_Z_Run2017_UL_ISO.json'
'data/MUON/2017_UL/Efficiencies_muon_generalTracks_Z_Run2017_UL_SingleMuonTriggers_schemaV2.json'
'data/MUON/2017_UL/NUM_TrackerMuons_DEN_genTracks_Z_abseta_pt.json'
'data/MUON/2018_UL/muon_Z.json'
'data/MUON/UL/2016_postVFP/2016_postVFP_Jpsi/Efficiency_muon_trackerMuon_Run2016postVFP_UL_ID.json'
'data/MUON/UL/2016_postVFP/2016_postVFP_Z/Efficiencies_muon_generalTracks_Z_Run2016_UL_ID.json'
'data/MUON/UL/2016_postVFP/2016_postVFP_Z/Efficiencies_muon_generalTracks_Z_Run2016_UL_ISO.json'
'data/MUON/UL/2016_postVFP/2016_postVFP_trigger/Efficiencies_muon_generalTracks_Z_Run2016_UL_SingleMuonTriggers_schemaV2.json'
'data/MUON/UL/2016_postVFP/Efficiency_muon_generalTracks_Run2016postVFP_UL_trackerMuon.json'
'data/MUON/UL/2016_postVFP/Efficiency_muon_generalTracks_Run2016postVFP_UL_trackerMuon_avg.json'
'data/MUON/UL/2016_postVFP/NUM_TrackerMuons_DEN_genTracks_Z_abseta_pt.json'
'data/MUON/UL/2016_postVFP/NUM_TrackerMuons_DEN_genTracks_Z_abseta_pt_schemaV1.json'
'data/MUON/UL/2016_preVFP/2016_preVFP_Jpsi/Efficiency_muon_trackerMuon_Run2016preVFP_UL_ID.json'
'data/MUON/UL/2016_preVFP/2016_preVFP_Z/Efficiencies_muon_generalTracks_Z_Run2016_UL_HIPM_ID.json'
'data/MUON/UL/2016_preVFP/2016_preVFP_Z/Efficiencies_muon_generalTracks_Z_Run2016_UL_HIPM_ISO.json'
'data/MUON/UL/2016_preVFP/2016_preVFP_trigger/Efficiencies_muon_generalTracks_Z_Run2016_UL_HIPM_SingleMuonTriggers_schemaV2.json'
'data/MUON/UL/2016_preVFP/Efficiency_muon_generalTracks_Run2016preVFP_UL_trackerMuon.json'
'data/MUON/UL/2016_preVFP/Efficiency_muon_generalTracks_Run2016preVFP_UL_trackerMuon_avg.json'
'data/MUON/UL/2016_preVFP/NUM_TrackerMuons_DEN_genTracks_Z_abseta_pt.json'
'data/MUON/UL/2016_preVFP/NUM_TrackerMuons_DEN_genTracks_Z_abseta_pt_schemaV1.json'
'data/MUON/UL/2017/2017_Jpsi/Efficiency_muon_trackerMuon_Run2017_UL_ID.json'
'data/MUON/UL/2017/2017_Z/Efficiencies_muon_generalTracks_Z_Run2017_UL_ID.json'
'data/MUON/UL/2017/2017_Z/Efficiencies_muon_generalTracks_Z_Run2017_UL_ISO.json'
'data/MUON/UL/2017/2017_trigger/Efficiencies_muon_generalTracks_Z_Run2017_UL_SingleMuonTriggers_schemaV2.json'
'data/MUON/UL/2017/Efficiency_muon_generalTracks_Run2017_UL_trackerMuon.json'
'data/MUON/UL/2017/Efficiency_muon_generalTracks_Run2017_UL_trackerMuon_avg.json'
'data/MUON/UL/2017/NUM_TrackerMuons_DEN_genTracks_Z_abseta_pt.json'
'data/MUON/UL/2017/NUM_TrackerMuons_DEN_genTracks_Z_abseta_pt_schemaV1.json'
'data/MUON/UL/2018/2018_Jpsi/Efficiency_muon_trackerMuon_Run2018_UL_ID.json'
'data/MUON/UL/2018/2018_Z/Efficiencies_muon_generalTracks_Z_Run2018_UL_ID.json'
'data/MUON/UL/2018/2018_Z/Efficiencies_muon_generalTracks_Z_Run2018_UL_ISO.json'
'data/MUON/UL/2018/2018_trigger/Efficiencies_muon_generalTracks_Z_Run2018_UL_SingleMuonTriggers_schemaV2.json'
'data/MUON/UL/2018/Efficiency_muon_generalTracks_Run2018_UL_trackerMuon.json'
'data/MUON/UL/2018/Efficiency_muon_generalTracks_Run2018_UL_trackerMuon_avg.json'
'data/MUON/UL/2018/NUM_TrackerMuons_DEN_genTracks_Z_abseta_pt.json'
'data/MUON/UL/2018/NUM_TrackerMuons_DEN_genTracks_Z_abseta_pt_schemaV1.json'
'data/reshaping_deepCSV_106XUL18_v2.csv'
```

## 11. Later Active-Code Refactor Candidates

- E1/E2: explicit historical trigger dispatch/public vectors and 2017 diagnostic branch; preserve Run-3 era argument and the vector actually selected for 2024.
- E3: 2016 BTag skip/variation clause; keep 2024 RDF columns/vector conventions and independently review other years.
- H5: legacy-shaped btv/muon utility APIs with no local caller; function-level/API retirement only, not utility-file deletion.
- H9/S10: dormant RoccoR/test recipes and compiled SkimEvents retirement with reflection/build checks.
- A1: decide and refactor/remove the production-conditional older single-file interface without silently discarding a processor mode.
- E9: update Run-2 examples/UL labels and Rochester naming only after mapping active configuration/positional consumers; naming cleanup must not change payload selection.
- E4/E6/E7/E8/A8: explicit per-year fallbacks and FF physics prescriptions need physics/user decisions before implementation.

## 12. Questions Requiring User Decision

1. May Run-2 calibration/certification files and the isolated 2017 pileup macro be retired, or must reproducibility inputs remain in-tree or in an archive?
2. Are there external configuration modules, ROOT macros, notebooks or downstream users that load these paths or public helper APIs?
3. Is allinone=False production processing still required? Should processonefile.py be repaired, retired with its dispatch, or kept as historical reference?
4. Is SkimEvents still needed by any external C++ caller? May its compiled subclass and dormant dictionary comment be retired in a later build-focused task?
5. Are GetQuantile_Method studies and the alternate BTag framework copy still reference material or used manually?
6. Which normalization/histogram implementations and archive are operationally authoritative? Their Run-3 input use prevents blanket Run-2 classification.
7. Which NanoAOD versions/schemas, trigger menus and payload campaigns are required for each 2022/2023 era and 2025?
8. What approved Run-3 FF coefficients, electron/muon definitions, prescale tables and provenance replace or justify the documented placeholders?
9. What multi-year JEC/JER/MET, tagging and SF prescriptions are approved, including systematic propagation?
10. Which units, full-dataset denominators and correction terms should downstream normalization use?
11. What reference samples, expected cutflows/branches/yields and execution limits should a later authorized validation task use?

## 13. Verification Limitations

No compilation, make/make clean, ROOT execution, ROOT-file processing, RDF event loop, analysis/histogram/FF execution, runtime validation, Condor submission/resubmission or other Condor operation, EOS write, credential renewal, or physics validation was performed. No environment reconfiguration was performed.

Static reads covered the repository inventory and relevant source/configuration/call/build paths; they cannot prove the absence of dynamic or external users. Binary ROOT outputs, executable/library contents, and archived script bodies were not executed or validated. Payload schema examples were read but full numerical calibrations were not physics-audited. Ignored/generated artifacts were inventoried where relevant, not treated as authoritative source or safe deletion material.

Run-3 target years are a development policy, not a statement of validated implementation. “No consumer found” and “removal candidate” are deliberately conditional. No recommendation in this document is a cleanup instruction.

Before the documentation commit, verify this report is the sole Git change and stage only this file. The task authorizes the documentation commit `docs: add Run-2 legacy audit` and pushing the existing current branch through existing origin; it does not authorize any cleanup, configuration, branch, remote or upstream change.
