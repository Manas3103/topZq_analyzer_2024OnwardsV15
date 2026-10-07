# Repository architecture analysis

Investigation date: 2026-10-07.

The investigation used read-only inspection. No files were edited, the project was not compiled, the analysis was not run, and no Condor jobs were submitted. Git status was clean at the end of the investigation. This document was subsequently saved at the user's request; `AGENTS.md` has not been created.

This report describes behavior established from source code and configuration. Existing binaries, ROOT file contents, remote EOS outputs, and numerical physics correctness were not validated by execution. Uncertainties below must not be treated as established physics conclusions.

## Environment

| Item | Observed value |
|---|---|
| Shell working directory | `/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_13_3_3/src/single_top_analyser` |
| Git repository root | `/uscms_data/d3/msahoo/Project_tzq/CMSSW_13_3_3/src/single_top_analyser` |
| `CMSSW_BASE` | `/uscms_data/d3/msahoo/Project_tzq/CMSSW_13_3_3` |
| `CMSSW_VERSION` | `CMSSW_13_3_3` |
| ROOT | `6.26/11` |
| Compiler | GCC `12.3.1`, dated `20230527` |
| Python / Python 3 | Both report `3.9.14` |

The resolved `root-config`, `g++`, and `python3` paths use the CMSSW/CVMFS `el9_amd64_gcc12` environment. Batch wrappers instead specify `el9_amd64_gcc10`; that discrepancy deserves verification.

## A. Repository architecture summary

The repository is a standalone C++/ROOT NanoAOD analysis framework, with Python orchestration and HTCondor wrappers. Its current configuration focuses on **2024 Summer24 NanoAODv15**.

The inheritance structure is:

```text
NanoAODAnalyzerrdframe
├── BaseAnalyser
│   └── FakeFactorAnalyser
└── SkimEvents
```

`NanoAODAnalyzerrdframe` supplies dataframe management, corrections, scale factors, cut-tree construction, histogram actions, and ROOT output. `BaseAnalyser` implements the multilepton analysis. `FakeFactorAnalyser` replaces the object and analysis setup for a single-lepton fake-factor measurement.

| Directory | Purpose |
|---|---|
| `src/` | Main C++ implementation, headers, ROOT dictionary, and build artifacts. |
| `src/used_function/` | Additional event-shape code and alternative/example analysis functions; not included by the current top-level source wildcard. |
| `data/` | Correctionlib payloads, certification JSONs, older calibration CSVs, and supporting material. |
| `data/EGM`, `MUO`, `JERC`, `BTV`, `LUM` | Electron, muon, jet/MET, tagging, and pileup corrections. |
| `data/MUON`, `ELECTRON`, `Legacy_RunII` | Older payload collections alongside newer naming conventions. |
| `DataMC_txt/` | Dataset lists, chunk preparation, queue lists, and generator-weight bookkeeping. |
| `DataMC_txt/filelists`, `queues` | Generated per-job input lists and Condor queue rows. |
| `DataMC_txt/temp/` | Alternative batching and bookkeeping utilities. |
| `BTag/` | Tagging-efficiency production macros, dataset lists, and efficiency ROOT inputs. |
| `normalized_hist/Python_rdf_norm/` | Downstream normalized histogram production, JSON configuration, sample bookkeeping, and Condor submission. |
| `normalized_hist/normalization_with_C/` | Separate C++ histogram/normalization implementation and wrappers. |
| `GetQuantile_Method/` | Quantile-based cut studies and low-statistics bin merging. |
| `Analyzed/`, `logs/` | Analysis output staging and job logs. |

There are two processing stages: NanoAOD-to-analysis-tree production, then analysis-tree-to-normalized-histogram production.

## B. Analysis execution/data-flow diagram

```text
Input discovery
  ├── DAS dataset → dasgoclient → XRootD file URLs
  ├── text file containing ROOT URLs
  ├── local ROOT directory
  └── supported /store/user/msahoo/ EOS path
          │
          ▼
processnanoaod_v.py
  load correctionlib, MathMore, libnanoadrdframe.so
  import job configuration
  construct TChain("Events")
  select BaseAnalyser or FakeFactorAnalyser
          │
          ▼
NanoAODAnalyzerrdframe constructor
  _rd = ROOT::RDataFrame(*inputTree)
  _rlm = RNode(_rd)
          │
          ▼
setParams(year, runtype, datatype)
  inspect original branches
  determine data/MC
          │
          ▼
setupCorrections()
  data: golden-JSON filter
  load correction payloads
  define corrected jet/electron/muon columns
  propagate jet corrections to PUPPI MET
  MC: define pileup × generator weight
          │
          ├── BaseAnalyser::setupObjects()
          │     electrons → muons → jets → MET
          │     overlap cleaning → merged/sorted leptons
          │     OSSF/Z classification → W → top reconstruction
          │     signal/control-region columns
          │     MC event scale factors
          │
          └── FakeFactorAnalyser::setupObjects()
                jets → fakeable electrons/muons
                cone-pT → measurement region → trigger weight
          │
          ▼
setupAnalysis()
  define unit/data weights
  MC: sum original genWeight and define genEventSumw
  register cuts and output columns
  FF path: register histograms
          │
          ▼
setupCuts_and_Hists()
  construct filtered RNodeTree
  attach Count/Histo1D/Histo2D actions
          │
          ▼
setupTree()
  expand output-column regexes against each leaf's columns
          │
          ▼
run()
  Snapshot each leaf into outputTree_<cut-index>
  print cutflow counts
  write booked histograms into the same ROOT file
          │
          ▼
Optional downstream histogram processing
  analysis tree + histogram JSON + xsec/lumi/sumw
  → normalized *_hist.root → EOS
```

For both current analysis classes, the final cut index is `00000`, so the configured output tree becomes **`outputTree_00000`**.

RDataFrame transformations repeatedly replace `_rlm` with the returned node. Original `_rd` remains available for preselection generator-weight summation. Region definitions generally create columns and masked values; they do not create separate region-filtered trees.

Execution is not entirely deferred until `run()`: dereferencing `Sum("genWeight")` and diagnostic `Count()` results already triggers event loops during setup.

### Selections, objects, cuts, and output

The main object selectors live in `BaseAnalyser.cpp`: `selectElectrons()`, `selectMuons()`, `selectJets()`, `selectMET()`, and `removeOverlaps()`. Electrons and muons use corrected pT, baseline ID/quality masks, and separate prompt-MVA tight flags. Selected object arrays are derived using masks; jets and merged leptons are sorted with corresponding aligned arrays.

`mergeLeptons()` combines electrons and muons. `defineInZPeak()` uses utility functions for OSSF pairing and three-/four-lepton categories. `reconstructWboson()` reconstructs a neutrino from MET and the selected top lepton. `reconstructTop()` selects the b-jet/W candidate closest to the reference top mass of 172.76 GeV. `defineSignalRegion()` defines multilepton signal/control-region flags and masked region-object branches.

The active main cut chain in `defineCuts()` is:

```text
0      available-trigger OR
00     MET/event-quality filters
000    at least three baseline electrons + muons
0000   PV_npvsGood >= 1 and NgoodLepton >= 3
00000  no vetoed jets
```

Signal/control regions are additional columns. Examples include three-lepton Z regions, a base region requiring at least two clean jets and one b jet, tZq/ttZ splitting at four central jets, WZ regions, off-Z/nonprompt regions, and four-lepton ZZ/ttZ regions.

The active FF cut chain is event-quality filters → exactly one fakeable lepton → at least one measurement-region jet → FF trigger acceptance → no vetoed jets. Measurement-region jets require pT above 25 GeV, |eta| below 2.5, and lepton separation above 0.7.

Derived ROOT columns are created through `Define()`/`Redefine()`. `defineMoreVars()` registers output-column patterns with `addVartoStore()`, and `setupTree()` expands those regexes against each leaf's columns. `setupCuts_and_Hists()` registers cut counts and histogram actions. `run()` snapshots leaf trees and prints cutflow counts, then opens the ROOT file in UPDATE mode and writes booked histograms. The selected-branch snapshot path uses UPDATE and overwrite options; the save-all path does not pass those options.

The main `BaseAnalyser::setupAnalysis()` currently comments out `bookHists()`. The FF setup calls it, producing numerator/denominator histograms at the no-cut and applicable cut stages.

## C. Important files and their responsibilities

| File | Responsibility |
|---|---|
| `src/NanoAODAnalyzerrdframe.cpp` / `.h` | Core RDF framework, correction loading/evaluation, data/MC detection, trigger expression construction, output schema, cutflows, snapshots, and histogram writing. |
| `src/BaseAnalyser.cpp` / `.h` | Multilepton object selection, cleaning, lepton merging, reconstruction, region definitions, nominal event weights, and stored branches. |
| `src/FakeFactorAnalyser.cpp` / `.h` | Fakeable/tight lepton definitions, cone-pT, measurement-region selection, prescale acceptance weighting, and numerator/denominator histograms. |
| `src/utility.cpp` / `.h` | Four-vectors, angular variables, correction helpers, neutrino reconstruction, OSSF classification, region-object branches, truth-origin helpers, cone-pT, and FF trigger evaluation. |
| `src/GenParticleHelper.cpp` / `.h` | Prompt generator-lepton identification and reconstructed-to-generator matching. |
| `src/MuonScaRe.cc` / `.h` | Muon scale/resolution correction helper and deterministic smearing. The `.cc` is directly included by the framework implementation. |
| `src/RNodeTree.cpp` / `.h` | Parent/child relationships between cut nodes and identification of snapshot leaves. |
| `src/SkimEvents.cpp` / `.h` | Older skim subclass with 2016 jet triggers and older branch conventions. It is compiled but not exposed by the active dictionary pragmas. |
| `src/nanoaodrdataframe.cpp` | Standalone main-analysis executable with hard-coded input and output. |
| `src/fakefactorframe.cpp` | Standalone FF executable with hard-coded input and output. |
| `src/Linkdef.h` | ROOT reflection/serialization declarations for the main analysis classes and vector types. |
| `src/rootdict.C` | Generated ROOT dictionary source, although currently tracked. |
| `src/json.hpp`, `json_fwd.hpp` | Bundled JSON headers; the main framework also includes `nlohmann/json.hpp`. |
| `src/used_function/EventShape.*` | Additional event-shape implementation outside the active build source list. |
| `src/used_function/used_func.cpp` | Alternative repeated Z-reconstruction implementations; not part of the current build. |

### Important Python scripts

| Script | Responsibility |
|---|---|
| `processnanoaod_v.py` | Main configurable processor: input discovery, TChain construction, analyser selection, corrections, setup, and execution. |
| `jobconfiganalysis_2024.py` | Main-analysis payload paths, correction tags, year/type, and processing options. |
| `jobconfiganalysis_2024_ff.py` | Equivalent configuration selecting `FakeFactorAnalyser`. |
| `processonefile.py` | Older single-file processor; currently uses obsolete correction arguments/configuration names. |
| `submitanalysisjob.py` | Starts background local processing commands from configuration tuples; it does not submit Condor jobs. |
| `check_job_status.py` | Compares queue expectations with EOS files and writes a CSV; optional ROOT-open validity check. |
| `DataMC_txt/split_dataset.py` | Resolves DAS datasets and writes fixed-size chunk lists plus queue files. |
| `DataMC_txt/SumOfGenWeight_calculator.py` | Sums `Runs/genEventSumw` across accessible dataset files and writes coverage information. |
| `DataMC_txt/temp/check_sumgenweight.py` | Cross-checks Runs bookkeeping against Events `Generator_weight`. |
| `DataMC_txt/temp/make_batches.py` | Alternative DAS-to-batched-JSON workflow. |
| `DataMC_txt/temp/make_job_list_v2.py` | Converts batching JSON to dataset/batch job rows, referencing another processor not present here. |
| `DataMC_txt/temp/make_queue_list.py` | Converts batching JSON to text filelists and Condor queue rows. |
| `BTag/list_dataset_files.py` | DAS-to-XRootD file-list utility. |
| `btagging_efficiency_binning.py` | Dataset-specific efficiency suffix/binning information used by the older single-file processor. |
| `normalized_hist/Python_rdf_norm/create_hist_rdf.py` | Creates weighted 1D/2D histograms from output trees and JSON definitions. |
| `normalized_hist/Python_rdf_norm/generate_hist_config.py` | Generates histogram definitions from region/variable tables. |
| `normalized_hist/Python_rdf_norm/json_to_general.py` | Validates sample bookkeeping JSON and converts it to submission text. |
| `normalized_hist/Python_rdf_norm/Test_yield/Calculate_yield.py` | Separate normalized-yield diagnostic. |
| `create_hist_rdf_old.py`, `unused_txt_file/*` | Alternate/older histogram and labeling implementations. |
| `data/EGM/2024_Summer24/python_check.py` | Simple correction-payload loading check. |

### Supporting macros and external inputs

`BTag/btag_efficiency.C` fills total and tagged heavy/light-flavor jet histograms, using `Jet_btagUParTAK4B` for 2024 and older taggers for earlier years. `BTag/btag.C` divides tagged histograms by total histograms to produce the six efficiency histograms consumed by the framework. The configured `BTag/btag_efficiency_2024_UParT.root` exists locally; its contents were not opened during investigation.

`GetQuantile_Method/Quantiles_jj.C` studies quantiles from projections of 2D distributions. `Mergebins500.C` merges low-statistics bins. The C++ normalization directory contains a separate normalized histogram macro and an unnormalized histogram macro. `data/make2017MCpileup.C` is an older pileup-support macro.

Configurations select 2024 Golden JSON, pileup weights, EGM electron corrections, MUO scale/smear and muon SF payloads, Summer24 JERC/jet veto/JetID payloads, and BTV corrections. Older Run-II and Run-3 payloads coexist. Sample bookkeeping appears in `samples_2024.json`, submission text files, dataset lists, and sumgenweight summaries. Cross sections and sum-of-weights values are external inputs to downstream normalization. Their provenance and units were not established confidently.

## D. Build and execution workflow

`Makefile` is the build definition. It obtains:

- ROOT flags/libraries from `root-config`.
- Correctionlib include/library directories from `correction config`.
- Common sources from `src/*.cpp`, excluding the two executable mains.
- Reflection code from `rootcling` using analyser headers and `Linkdef.h`.

The default target builds:

```text
nanoaodrdataframe
fakefactorframe
libnanoadrdframe.so
```

Compilation uses `-O0 -g -Wall -fPIC` plus ROOT flags. Executables link ROOT, `MathMore`, `GenVector`, and correctionlib. The shared-library link recipe lists ROOT libraries only; Python explicitly loads correctionlib and MathMore before loading the analyser library.

The documented build command is `make`. It was **not run**.

Configurable execution is:

```text
./processnanoaod_v.py <input> <output.root> <configuration-module>
```

Standalone executables bypass Python configuration. Their current hard-coded examples differ from the Python settings. For example, `fakefactorframe.cpp` reads a W+jets input while naming the output `QCD_bcToE.root`.

Important relationships:

- Python configuration keys must match the long positional `setupCorrections()` interface.
- Class names in configuration must match ROOT dictionary declarations.
- Stored branch names must match downstream histogram JSON keys.
- Cut indices determine tree names consumed by histogram wrappers.
- BTag efficiency histogram names must match the six histograms retrieved by correction setup.

## E. HTCondor workflow

The primary workflow is:

```text
dataset list
  → prepare_all_datasets.sh / split_dataset.py
  → DataMC_txt/filelists/*.txt
  → three-column queue:
      input_filelist output.root processing.out
  → .submit
  → runjob_24_v2.sh
  → processnanoaod_v.py
  → Analyzed/output.root
  → xrdcp to EOS
```

Current submission configuration:

| Submit file | Configuration | Active queue |
|---|---|---|
| `job_2024_v2.submit` | `jobconfiganalysis_2024` | `resubmit_queue.txt` |
| `job_2024_fakeFactor.submit` | `jobconfiganalysis_2024_ff` | `DataMC_txt/Fake_factor_MC_path.txt` |

Both pass four arguments:

```text
<input_dir> <output_file> <stderr_file> <jobconfmod>
```

Both request one CPU, 8 GB memory, and 50 GB disk, transfer `.` as input, and request output transfer on exit or eviction.

The wrapper sets up CMSSW in worker scratch, creates `Analyzed/`, redirects processor output into the requested processing log, then copies ROOT output/logs to:

- Main: `/eos/uscms/store/user/msahoo/2024_test`
- FF: `/eos/uscms/store/user/msahoo/2024_fakeFactor`

Condor logs are:

```text
logs/<ClusterId>.<Process>.out
logs/<ClusterId>.<Process>.err
logs/<ClusterId>.log
```

### Shell scripts and resubmission helpers

- `DataMC_txt/prepare_all_datasets.sh`: runs chunk preparation for every listed dataset and concatenates queue files; `PATH_PREFIX` controls recorded input-list paths.
- `runjob_24_v2.sh`: worker environment setup, analysis launch, EOS staging, and scratch cleanup.
- `resubmit_missing.sh`: writes queue rows corresponding to non-OK CSV results.
- `resubmit.sbng`: runs checking, creates a queue, may edit the submit file, then calls `condor_submit`.
- `setupldpath.csh`: adds correctionlib's directory to `LD_LIBRARY_PATH`.
- `normalized_hist/Python_rdf_norm/runjob_hist_py.sh`: histogram worker setup, Python histogram execution, local staging, and EOS copy/verification.
- `normalized_hist/Python_rdf_norm/submit_jobs.sh`: renews credentials if needed, deletes old logs, and submits normal/data histogram jobs.
- `normalized_hist/normalization_with_C/runjob_hist.sh`: separate ROOT-macro histogram workflow and EOS staging.
- `normalized_hist/Python_rdf_norm/Test_yield/runjob_hist_py.sh`: wrapper for the yield diagnostic workflow.

There is a separate histogram Condor workflow under `normalized_hist/`, with normal, FF, and data submit variants. Its Python wrapper reads `outputTree_00000`, uses luminosity `12.75`, and stages histograms to separate main/FF EOS destinations. Normal/FF histogram submission passes filename, cross section, sum generator weight, sample type, histogram JSON, and output mode.

None of these scripts was executed.

## F. Physics-sensitive code

These areas should require an explicit physics request and review before changes:

- Electron/muon baseline, fakeable, and tight definitions; ID, isolation, impact-parameter, conversion, and prompt-MVA requirements.
- Jet ID, forward-jet thresholds, veto maps, tagging discriminator, working points, and efficiency inputs.
- Electron–muon and jet–lepton overlap removal and aligned vector sorting.
- Trigger menus, dataset overlap handling, prescales, and certification JSON coverage.
- JEC/JER tags, raw-factor treatment, generator matching, smearing seeds, and MET propagation.
- Electron scale/smear and muon scale/resolution corrections.
- OSSF pairing/category definitions, Z windows, neutrino solutions, and top candidate selection.
- Signal/control-region definitions.
- Generator-weight signs, pileup, scale-factor products, cross sections, luminosity units, and sum-of-weights denominators.
- FF cone-pT, trigger acceptance formula, numerator/denominator definitions, and binning.
- Systematic branch naming and propagation through selections and reconstruction.

The main nominal MC event weight is:

```text
genWeight × puWeight
× btag_SF_bcflav_central × btag_SF_lflav_central
× muon_SF_central × ele_SF_central
```

Despite its name, `muon_SF_central` currently combines **ISO only**. RECO/ID variation columns are defined separately. Electron central SF combines RECO and ID. Muon trigger SF evaluation is commented out.

Data receives golden-JSON filtering, data correction paths, and unit event weight; MC receives truth columns, smearing, pileup, and SF calculations. `datatype` is 0 for MC, 1 for data, and -1 for automatic detection by the presence of `genWeight`.

Correction loading and evaluation live principally in `NanoAODAnalyzerrdframe.cpp`: `setupCorrections()`, `setupJetMETCorrection()`, `applyJetMETCorrections()`, `applyElectronPtCorrection()`, `applyMuPtCorrection()`, `calculateBTagSF()`, `calculateMuSF()`, and `calculateEleSF()`. `BaseAnalyser::calculateEvWeight()` combines nominal terms. Muon correction internals live in `MuonScaRe.cc`; utility functions provide additional correction/reconstruction helpers.

Systematics are implemented principally as explicit alternative columns: JEC/JER jet and MET variants, electron/muon momentum variants, pileup variations, and SF variations. I did not find an active `Vary`/`VariationsFor` workflow rebuilding the complete analysis for these shifts.

FF weighting differs from the main analysis. `evaluateFFTrigger()` computes MC trigger acceptance as `1 - product(1 - 1/prescale)` over applicable fired paths; data uses unit weight. FF numerator weights are zero for non-tight leptons. The base FF histogram weights do not include the main analysis's full nominal MC correction product.

## G. Potential issues and uncertainties

These are observations for review, not fixes.

1. **Lepton threshold indexing appears incorrect.** `goodLepton_pt` is already sorted, but `goodLepton_ptCut` indexes it using the original sorting permutation again. This can test the wrong lepton as leading. See `src/BaseAnalyser.cpp`, around line 967 at investigation time.

2. **2024 JER variation labels do not change the SF evaluation.** The 2024 branch evaluates `{eta, pt}` without the variation argument. Nominal/up/down columns therefore share the same SF prescription. Their names alone do not establish meaningful uncertainty shifts.

3. **MET propagation needs physics validation.** The active function starts from `PuppiMET_pt/phi` and subtracts corrected-minus-raw jet contributions. It does not use the more elaborate commented implementation involving raw MET and `CorrT1METJet`. Whether this repeats existing NanoAOD corrections cannot be established from repository code alone.

4. **JEC uncertainty columns start from JEC-only jet pT**, while nominal MC uses JER-smeared jet pT. Full systematic consistency needs review.

5. **MET XY correction is disabled.** Its loader and application call are commented out. Configuration still points to a 2023 BPix payload in the 2024 analysis.

6. **Jet veto threshold is constrained by an earlier mask.** The veto mask requests pT above 15, but the 2024 `goodJetsID` mask already rejects pT at or below 25. Also, jet selection explicitly excludes `eta == 0` and exact boundaries at `|eta| == 2.5` and `3.0`.

7. **Downstream weighting differs materially from C++ weighting.** Current `create_hist_rdf.py` uses luminosity × cross section / sumw, multiplied by `genWeight` when available. PU, electron/muon SFs, and b-tag SF terms are commented out. It does not consume `evWeight` or FF trigger weights.

8. **Chunk normalization fallback is unsafe for merged samples.** The C++ `genEventSumw` column represents the current input chain. Taking its maximum after merging unequal chunks does not produce a dataset-wide sum. Supplying an explicit total denominator avoids that particular fallback issue.

9. **FF histogram definitions have concrete discrepancies.** Weighted eta histograms are labeled `|eta|` with nonnegative ranges but use signed `MRLepton_eta`. The “unweighted” numerator and denominator definitions all use `one` without tight/flavor weights, so they do not implement distinct tight/flavor-specific counts.

10. **FF documentation conflicts with implementation.** The header describes prescales as Run-2 placeholders; implementation comments label hard-coded numbers as updated for 2024. Other FF comments explicitly mark sliding-cut values and binning as placeholders. Provenance is unclear.

11. **Old single-file processing is incompatible with current configuration.** It hard-codes `BaseAnalyser`, references old keys/signatures, and may use an uninitialized `dataset_name` when input parsing fails.

12. **Batch success does not guarantee successful staging.** The main wrapper warns on missing filelists without exiting, does not reliably fail the job on copy failures, and deletes scratch contents afterward. Processing log paths can also be listed twice for copying.

13. **Some histogram submission variants appear stale.** The data submit file supplies only two arguments to a wrapper expecting additional configuration. The C++ histogram wrapper searches for literal `_hist.root`, which may disagree with produced filenames.

14. **The yield diagnostic uses the weight as its histogram coordinate.** Its nominal range is 0–1; negative or larger weights enter underflow/overflow. Yield interpretation depends on which bins are integrated.

15. **Class state and lifecycle need care.** `BaseAnalyser.h` redeclares several framework member names. `isDefined()` checks original input branches, not all newly defined columns. `setTree()` does not visibly reset all cut/variable bookkeeping. `RNodeTree` relies on valid digit indices and unchecked daughter access.

16. **Calibration loading has fragile assumptions.** BTag ROOT inputs and histogram pointers are retrieved without strong visible validation. Golden-JSON failure returns false, but I did not see setup abort on that result. `readgoodjson()` also forms a pointer from a temporary string's `c_str()`.

17. **Build and documentation are inconsistent.** Generated `rootdict.C` is tracked; `make clean` deletes it. Header dependency coverage is limited, and subdirectory sources are excluded. README examples reference older/missing names and schemas.

18. **Data stream overlap removal is not established.** Trigger OR construction checks available branches and reuses the 2022 post-EE menu for 2024. I did not establish primary-dataset exclusivity or event deduplication.

Generated artifacts include executables, `.so`, `.o`, `.pcm`, dictionary output, ROOT outputs, ACLiC `.d` files, caches, logs, queue/filelist products, status CSVs, and output directories. However, **BTag efficiency ROOT files are also required calibration inputs**, so “generated” does not mean disposable.

## H. Recommended eventual AGENTS.md rules

These are proposed rules, not an existing `AGENTS.md` policy.

- Default to read-only investigation unless modification or execution is requested.
- Require explicit authorization for analysis runs, Condor submission/resubmission, EOS writes, credential changes, and cleanup.
- Identify the active analyser, configuration, year/era, NanoAOD version, and entry path before changing behavior.
- Treat physics selections, corrections, weights, normalization, and systematic propagation as protected behavior.
- Preserve aligned object vectors through masking and sorting.
- Maintain exact contracts among C++ columns, stored-branch regexes, histogram JSONs, and output tree names.
- Do not infer production validity from comments or existing binaries.
- Do not edit generated dictionaries or build products manually.
- Do not use `make clean` casually: it removes tracked/generated material.
- Keep calibration payloads and efficiency ROOT inputs intact unless replacement is explicitly requested.
- Distinguish current workflows from legacy alternatives; do not silently switch paths.
- Make normalization units and denominator scope explicit.
- Preserve deterministic smearing and document seed changes.
- Use bounded, explicitly authorized validation samples for future runtime checks.
- Never combine data/MC or distinct calibration eras in one chain without an established policy.
- Record unresolved physics questions rather than making corrective assumptions.

## I. Questions requiring user input

1. Which path is authoritative: Python-driven production, standalone executables, or both?
2. Which years/eras and NanoAOD versions must remain supported?
3. What analysis note or approved specification defines selections, regions, and reconstruction?
4. Are downstream histograms intentionally generator-weight-only, or should they use the full correction weight?
5. What are the cross-section and luminosity units, and is `12.75` the intended luminosity for the current data selection?
6. Which dataset-wide sum-of-weights source is authoritative after chunk merging?
7. Are the FF prescales approved 2024 values? What is their source and run coverage?
8. Which FF thresholds/binning remain provisional?
9. What data-stream overlap-removal policy is intended?
10. Which JEC/JER/MET prescription is approved, including the disabled XY correction?
11. Are systematic columns intended only for storage, or should shifted selections/reconstruction be produced?
12. Which EOS destinations, queue files, and histogram workflow are current production targets?
13. Which legacy scripts should be supported, and which are retained only as references?
14. What small reference samples and expected cutflows/yields should future changes be checked against?
