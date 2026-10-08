# Run-3 Cleanup Status

## 1. Purpose

This handoff records the checkpoint of `cleanup/run3-remove-legacy-run2` so a fresh Codex session can continue safely without conversational memory. **Git + repository documentation are the source of truth.** Historical investigation notes describe their original baselines; verify current code before acting on them.

The intended scope is CMS Run 3 (2022–2025), currently centered on 2024 NanoAODv15. This scope does not imply complete implementation of every year.

## 2. Current checkpoint

- Branch: `cleanup/run3-remove-legacy-run2`
- Current source checkpoint (2026-10-08): `1d24d86dcbdc612ef293ff482a21b772365dda54`
- Earlier reconciled checkpoint: `422a3fb04eaa74333eae0809aa6f650da361a1db`
- CLI commit `7bc8ddab` and documentation commit `0e934284` were pushed to both GitHub and CERN GitLab; Makefile cleanup `1d24d86` was also pushed successfully to both destinations.
- Original handoff pre-documentation checkpoint: `4a43a0cbc31e5eb64b7a1a7fda04cb50e38fab78`
- Starting working tree: clean.

The original handoff was committed as `5c3b944`. This reconciliation is documentation-only; HEAD advances only after a separately approved commit. Use Git to determine the latest SHA rather than treating this recorded source checkpoint as permanently current.

## 3. Cleanup commits completed

Verified against Git, in chronological order:

| SHA | Commit message | Purpose |
|---|---|---|
| `f59b5408a5130037b5822a9678ff42135fc215bf` | docs: add Run-2 legacy audit | Static classification and exact Phase-1 input manifest. |
| `13f2b14e8179268a157aec02004caa54e261a331` | cleanup: remove standalone Run-2 legacy inputs | Remove exactly 78 approved tracked inputs, including the 2017 pileup macro. |
| `138a6f25931977efb2fc65df803be0e1753850d6` | cleanup: remove unused legacy analysis files | Remove SkimEvents and obsolete b-tag binning module after build/runtime validation. |
| `9187777a997cce31459dd995ba56188801107fd9` | cleanup: retire obsolete per-file processing path | Retire per-file dispatch/processor and isolated reference code; retain single-chain production. |
| `4a43a0cbc31e5eb64b7a1a7fda04cb50e38fab78` | cleanup: remove obsolete allinone configuration | Remove redundant keys, examples and logging; update misleading README usage. |
| `5c3b9444c265c8a2bca3e584bb56fb9709575139` | docs: record Run-3 cleanup checkpoint | Original durable handoff; historical validation references. |
| `7187b6fce4b67d6d10fc3a2868ed6ad506232932` | cleanup: remove tracked macOS metadata | Delete four tracked Finder metadata files and correct the ignore-rule capitalization. |
| `422a3fb04eaa74333eae0809aa6f650da361a1db` | cleanup: stop tracking fakefactorframe executable | Remove executable from Git tracking, preserve the local executable, add exact ignore rule. |
| `0165ef1cf2b321da0adf7f6315db677d22878a7c` | docs: reconcile Run-3 cleanup status and validation | Record phase status, diagnostic limitations and pending work. |
| `aaf438b147f807d91114b4de292e732d8536abc3` | cleanup: stop tracking generated BTag ACLiC dependencies | Preserve local dependency files; retire their tracking with exact ignore rules. |
| `7bc8ddab306997be83a9a7e75bf09c27f0baab89` | feat: add sample-selectable standalone CLI | Add validated input/sample/output selection and help; preserve automatic mode and physics settings. |
| `0e9342848d9eb8eeddc7f24d4f2c21fac3249d73` | docs: update Run-3 cleanup CLI validation checkpoint | Preserve full-content MC regression and data smoke-test evidence. |
| `1d24d86dcbdc612ef293ff482a21b772365dda54` | cleanup: remove dormant legacy Makefile rules | Remove four isolated legacy/test rules; verify unchanged active build plan and successful incremental make. |

Earlier orientation commits:

- `33cd64c0e707bef23ce0cb1d7296072c86093aaa` — `docs: record repository architecture and baseline before Codex-assisted development`.
- `ac8fee688cdc215bbe51914404c7233635ec4e0b` — `docs: add repository-level Codex development instructions`.

### Phase reconciliation

The phases refer to section 9 of the original audit; later approved artifact tasks supplement that plan.

| Phase | Current status | Completed / remaining work |
|---|---|---|
| 1 — standalone inputs | Complete for the approved manifest | Exactly 78 tracked deletions in `13f2b14`; ignored BTag efficiency inputs preserved. No broader directory cleanup was approved. |
| 2 — dependency/build cleanup | Partially complete | SkimEvents, per-file processing and isolated scratch sources retired; redundant configuration removed. Metadata cleanup and binary/ACLiC artifact untracking also complete. Standalone CLI validation is complete. Four dormant Makefile rules were removed in `1d24d86` using the task-specific static/incremental validation described below. Remaining scope includes the tracked generated dictionary decision, alternate BTag/manual consumers and normalization alternatives; no blanket Phase-2 completion is claimed. Historical alternative research implementations remain retained; any proposed retirement requires a separate user decision. |
| 3 — active-code Run-2 refactoring | Not started as a dedicated approved batch | Historical trigger/year dispatch, BTag clauses and public helper interfaces remain in active/shared source. Trace Run-3 dependencies and external users before proposing narrow retirement. |
| 4 — physics-sensitive cleanup | Intentionally deferred | FF definitions, IDs/triggers, corrections, weights, normalization and systematics require separate physics approval and per-year validation. No prescription is chosen by this handoff. |

No accidentally skipped approved deletion has been established. A failed prerequisite or intentionally retained reference is not a skipped cleanup. Phase 1 used static verification; compiled/build Phase 2 changes normally require before/after builds and runtime checks; the four-rule Makefile cleanup used an explicitly authorized incremental-only exception, without runtime execution. Phase 3 also requires interface and event-level regression checks; Phase 4 requires physics review beyond software regression.

## 4. What has been removed

- Standalone Run-2 legacy certification/calibration/input batch: 77 tracked manifest inputs plus `data/make2017MCpileup.C`. See section 10 of [the original audit](run2_legacy_audit.md) and the exact deletion list in commit `13f2b14`; directory names were not blanket deletion authorization. Ignored/untracked calibration files were excluded.
- `src/SkimEvents.cpp`, `src/SkimEvents.h`, `btagging_efficiency_binning.py`.
- `processonefile.py`.
- `src/used_function/used_func.cpp`, `src/used_function/EventShape.cpp`, `src/used_function/EventShape.h`.
- Obsolete `allinone=False` dispatch and its exclusive per-file processor implementation.
- Behaviorally redundant `allinone` keys/comments in both active 2024 configs, processor logging, and obsolete README examples.

- Four tracked Finder metadata files: `GetQuantile_Method/Outputs/.DS_Store`, `GetQuantile_Method/Outputs/MergedBins500/.DS_Store`, `GetQuantile_Method/Outputs/RebinX2/.DS_Store`, `GetQuantile_Method/data/.DS_Store`. `.gitignore` changed only `.DS_store` to `.DS_Store` in that commit. Research macros and README were retained.
- Git tracking of `fakefactorframe`, using `git rm --cached`; the executable was preserved locally and is still executable at this reconciliation. Exact ignore rule: `/fakefactorframe`. Its C++ source and Makefile build rule remain tracked. Future `make clean` can still remove the local build product.

- Git tracking of `BTag/btag_C.d` and `BTag/btag_efficiency_C.d`: local copies, sizes, permissions and SHA256 checksums preserved; exact ignore rules added. BTag macros and calibration inputs were untouched.

- Four dormant Makefile rules and their dedicated headings: `src/RoccoR.o`, `src/rootdicttmp.C`, `src/rootdicttmp.o`, `libtest.so`. Active dictionary, executable/library targets, source discovery, flags and the complete clean recipe were preserved.

Historical audit references to these paths remain intentionally unchanged. They are not descriptions of the current processing structure.

## 5. Current supported processing structure

```text
processnanoaod_v.py
  -> Nanoaodprocessor_singledir
  -> current single-chain / Events TChain processing
  -> configured analyser
       -> BaseAnalyser or FakeFactorAnalyser as appropriate
  -> output analysis tree
```

`nanoaodrdataframe` and `fakefactorframe` remain standalone small-file debugging/testing executables, not production entry points. `processonefile.py` and the alternative per-file path have been retired. The `allinone` key was also removed from `jobconfiganalysis_2024.py` and `jobconfiganalysis_2024_ff.py` because it no longer controlled behavior.

Both standalone entry points now accept `--sample NAME`, `--input PATH_OR_URL`, `--output FILE`, and `-h`/`--help`. Custom input requires output and cannot be combined with a sample selector. Invalid usage returns nonzero; help exits before TChain construction. `setParams(2024, "", -1)` preserves automatic MC/data detection from `genWeight`. No-argument behavior remains TZQB -> `tzq_new.root` and W+jets -> `QCD_bcToE.root`.

Explicit main samples: `tzqb` -> `TZQB_TZQAnalysis.root`, `zz4l` -> `ZZ4L_TZQAnalysis.root`, `muoneg-c` -> `MuonEG_Era_C_Run24_TZQAnalysis.root`. Explicit fake-factor samples: `wjets` -> `WJets_4J_FakeFactor.root`, `qcd-bctoe` -> `QCD_bcToE_FakeFactor.root`, `muoneg-h` -> `MuonEG_Era_H_Run24_FakeFactor.root`, `muon1-h` -> `Muon1_Era_H_Run24_FakeFactor.root`. Output can be overridden. Existing URLs, analyser classes, triggers, correction arguments and setup/execution sequence remain unchanged.

## 6. Validation established

### Historical successful full-input references

Existing TZQB 2024 NanoAODv15 input, read remotely without downloading:

```text
root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/TZQB-Zto2L-4FS_Bin-MLL-30_TuneCP5_13p6TeV_amcatnlo-pythia8/NANOAODSIM/Madgraph_2_6_5_150X_mcRun3_2024_realistic_v2-v2/2810000/43318103-fc71-48c7-8d99-4915164b3b87.root
```

Production reference using `jobconfiganalysis_2024` and `BaseAnalyser`:

| Quantity | Validated value |
|---|---|
| Exit status | 0 |
| Input entries | 141000 |
| Output tree | outputTree_00000 |
| Selected entries | 6411 |
| Cutflow, in recorded order | 95551, 95415, 7147, 7147, 6411 |
| sum(evWeight) | 480.6132584437728 |

Before/after production validation matched for both retirement of `processonefile.py`/False dispatch and subsequent removal of redundant configuration. The latest exact invocation, from repository root, was:

```bash
PYTHONDONTWRITEBYTECODE=1 ./processnanoaod_v.py /tmp/codex-allinone-ydqo6spx/tzqb.txt /tmp/codex-allinone-ydqo6spx/production.root jobconfiganalysis_2024
```

The temporary filelist contained only the URL above. Baseline output was preserved as `baseline.root` before repeating the same command. `/tmp` evidence is temporary and must not be assumed available in a fresh session. Recreate only a validation filelist/output when explicitly authorized; do not invent physics configuration.

Standalone references from the completed compiled-code cleanup validations:

| Program | outputTree_00000 entries | Cutflow, in recorded order |
|---|---:|---|
| nanoaodrdataframe | 6411 | 95551, 95415, 7147, 7147, 6411 |
| fakefactorframe | 36539 | 115953, 54819, 47395, 36539, 36539 |

Relevant ROOT outputs opened normally, were not zombie/recovered, and recorded schemas/histogram/weight quantities used for comparisons matched. The latest production output contained a tree and no histograms; histogram comparisons apply only where present. No byte-identical ROOT output claim is made. These are summary-level full-input regression checks, not comprehensive physics validation or proof of event-by-event identity.

Clean builds and both standalone runtimes passed for the compiled-code/per-file retirement batches. The later redundant-key cleanup used Python/static checks and production before/after validation; it did not rerun a C++ build because compiled files were untouched.

### Recent incomplete baseline attempts and bounded diagnostics

These later investigations did **not** complete a new full TZQB baseline and did not implement the postponed Makefile cleanup. Results below come from the prior diagnostic session; this documentation update performed no runtime tests. Temporary evidence may expire.

- Clean builds succeeded and regenerated `src/rootdict.C` identically. Rebuilding the then-tracked `fakefactorframe` changed its binary; it was safely restored before the later explicit untracking commit.
- An initial restricted-sandbox attempt could not resolve the XRootD hostname. Outside that sandbox, DNS and `xrdfs stat` succeeded for both configured files. This establishes accessibility at those times, not reliable basket reads.
- Later `fakefactorframe` completed: exit 0, 36539 selected entries, 56 branches, 48 histograms and the reference cutflow. `nanoaodrdataframe` exceeded 15 minutes and was terminated with SIGTERM (exit 143). Its final log reached output-column reporting near Snapshot; it did not record completed output/cutflow. Evidence: `/tmp/codex-make-resume-lz8buted`.

| Diagnostic | Observed result | Scope / limitation |
|---|---|---|
| Direct TTree, first 100 events | `genWeight` read passed; all 2007 branches read passed in 35.089 seconds for the broad read; no ROOT errors. | Remote read succeeds for this small window; does not prove full-file reliability. |
| RDF and generic Snapshot, first 100 events | Both passed; `sum(genWeight) = 9.109748303890228`. RDF total 7.920 seconds; Snapshot total 6.502 seconds, four original scalar branches, valid 100-entry output. | Generic ROOT execution/writing works; not the BaseAnalyser graph. |
| BaseAnalyser, first 1000 events copied to an in-memory tree | Exit 0; 49 selected entries, 648 branches; cutflow 680, 680, 54, 54, 49; `sum(evWeight) = 4.31822320073843`; valid ROOT output. Copy 42.97 seconds, analysis setup 8.66 seconds, Snapshot/finalization 8.48 seconds, total 66.90 seconds. | Existing analyser/settings unchanged; every action bounded by the input tree. Remote I/O removed from the graph after copying. Sample-level normalization is not a full-dataset reference. |
| Planned 10000-event window `[0, 10000)` | Timed out at 900 seconds (exit 137 after forced termination), before BaseAnalyser construction. Last timestamp preceded combined `LoadTree`/`GetEntries` access; exact blocked call was not separately instrumented. | No selected entries, cutflow or completed analysis output. Planned later window `[70000, 80000)` was not run because the first failed. |
| Separately instrumented 90-second remote opening/read | `TFile::Open` 20.482 seconds; Events lookup 0.392 seconds; count 0.004 seconds (141000); `LoadTree(0)` 0.003 seconds; metadata 0.017 seconds (2007 branches). First all-branch `GetEntry(0)` did not finish before timeout (exit 137). | Redirected through Nebraska to `red-xfer1.unl.edu:1094`; subsequent 1000-entry read and W+jets comparison were not reached. No reported ROOT/XRootD error before timeout. |

Diagnostic environment: ROOT 6.26/11, XRootD 5.6.2 in the existing CMSSW environment. The RDF/generic Snapshot test used implicit MT disabled. Evidence directories include `/tmp/codex-root-read-mxioifyy`, `/tmp/codex-rdf-diagnostic-6tje05ez`, `/tmp/codex-base1000-xisj0hd5`, `/tmp/codex-base10000-p4j_2a5f`, and `/tmp/codex-xrd-open-uqbjdutg`.

**Root cause remains unconfirmed.** Low CPU usage, sleeping/waiting process state and an established socket are observations, not proof of a server/network fault. Intermittent remote I/O/redirector/storage delay is supported as a hypothesis; ROOT synchronization and client behavior remain alternatives. The 10000-event stall occurred before the analysis graph, whereas the earlier full-run log reached the Snapshot region. Do not attribute both to one confirmed cause or claim that small passing tests validate the full run.

### Historical durable regression baseline gap

At the earlier reconciliation, no durable, provenance-verified full event-by-event golden ROOT baseline had been established. The historical `/tmp/codex-allinone-ydqo6spx` production evidence was no longer available at reconciliation. Other surviving outputs are not automatically golden references: their input/configuration provenance and complete contents have not been certified. Historical count/schema/weight comparisons cannot prove that every stored event and value matched.

### Latest CLI validation and preserved regression evidence

Regression directory: `/uscms_data/d3/msahoo/Project_tzq/regression_baseline`. This is outside Git. It contains preserved references (`tzq_new.root`, `QCD_bcToE.root`), exact staged original MC inputs under `inputs/`, new outputs under `test_outputs/`, and transfer manifests, full logs, comparison helpers and reports under `reports/`. Reference SHA256 checksums remained unchanged:

- Main: `dc690df2e430080758d4533d6bf955601cdabb435a5ff0fc67e57e1042ab393c`.
- Fake factor: `5e53098728221b6345de6e2cf1aa0b353890b496203219462ec0fc54125faa78`.

The original TZQB and W+jets URLs were transferred without overwriting inputs using `xrdcp --cksum adler32`, exit 0. Transfer-time checksum verification succeeded; independent remote checksum queries were unsupported. Exact URLs, sizes and local SHA256 hashes are in the transfer manifests.

The existing Makefile built both CLI executables successfully without `make clean`; 16 main and 18 fake-factor help/invalid-usage tests passed. Both staged-input MC executions exited 0 (40.2 and 32.9 seconds). Initial slow Python/ROOT comparison attempts were incomplete; they were superseded by successful 500-entry chunked uproot/awkward comparisons:

| Validation | Result |
|---|---|
| Main MC | 6411 events, 648 top-level branches: exact full-content PASS, in original order; comparison 10.50 seconds. |
| Fake-factor MC | 36539 events, 56 top-level branches: exact full-content PASS, in original order; comparison 7.31 seconds including common histograms. |
| Fake-factor histograms | All 48 common histograms matched exactly: flow bins, errors, Sumw2, axes, labels and stored metadata. |
| Historical histogram cycles | All 48 reference histograms matched across `;1`, `;2`, `;3` (96 pairwise checks). Reference-only extra cycles are repeated-write storage artifacts, not physics-content differences. Existing output `UPDATE` mode and histogram writes explain how repeated cycles accumulate. Strict key/cycle inventories still differ. |

Comparisons covered every stored scalar, array, vector and split-object member, branch names/types and event ordering. Floating-point buffers were compared exactly, without tolerance, including NaN representations, infinities and signed zero. No independently written ROOT-file byte identity is required. The source identity of the staged inputs is recorded, but the older references still lack a complete contemporaneous environment/payload provenance manifest; verify evidence availability and freeze provenance before extending this baseline to another change.

Two predefined data selectors were tested against their existing remote inputs with separate 600-second timeouts and unique outputs:

| Data smoke test | Exit / elapsed | Selected / branches | Cutflow |
|---|---|---|---|
| Main MuonEG Run2024C (`muoneg-c`) | 0 / 259.0 seconds | 153 / 573 | 226953, 226479, 166, 166, 153 |
| Fake factor MuonEG Run2024H (`muoneg-h`) | 0 / 175.6 seconds | 880 / 54 | 1177698, 555092, 414803, 880, 880 |

Both data smoke tests PASS: non-zombie/non-recovered outputs, `outputTree_00000`, positive unique certified selected event IDs, logged DATA mode/data JEC path and consistent cutflows. Main `evWeight=1`; fake-factor `FFWeight_base=1`, with 48 finite histograms and consistent cut-stage counts. Preflight checked required object/correction branches and all configured HLT paths. Golden JSON covered all 551343 C input entries and 1181490 of 1187718 H entries; uncertified H entries were filtered.

Exact new outputs are under `test_outputs/`:

- `nanoaodrdataframe-mc-1791459446174753217.root`.
- `fakefactorframe-mc-1791459446174753217.root`.
- `data-1791461579460525387/MuonEG_Era_C_Run24_TZQAnalysis.root`.
- `data-1791461579460525387/MuonEG_Era_H_Run24_FakeFactor.root`.

Key evidence in `reports/`: `mc-run-1791459446174753217.json`, `full_mc_compare.py`, `full-mc-comparison-0.json`, `full-mc-comparison-1.json`, `full-mc-histogram-axes.json`, `historical-histogram-cycles.json`, `data-preflight.json`, `data-smoke-run-1791461579460525387.json`, `data-smoke-validation.json`, and `cli-commit-readiness.json`. The original fake-factor comparison report retains the strict cycle-inventory MISMATCH; the subsequent cycle report establishes identical physics content. Complete per-run logs and transfer manifests are stored alongside these reports.

**Scope of evidence:** MC full-content regression establishes equivalence to the preserved references for these inputs. Data tests establish execution/integrity behavior; no prior data golden outputs exist, so no data regression equivalence is claimed. Neither proves independent physics correctness or approval for other years, datasets or prescriptions. The known OSSF4LInfo dictionary warning and existing unmatched output patterns remain parked; data adds the expected absent GenPart pattern. Successful recent runs do not resolve the earlier intermittent XRootD root cause.

### Dormant Makefile cleanup validation (`1d24d86`)

Starting checkpoint: `0e9342848d9eb8eeddc7f24d4f2c21fac3249d73`, clean tree. Tracked-reference searches found no active production/script/dictionary consumer of the four rules; their RoccoR and temporary-test source/header prerequisites were absent. Documentation references and a commented RoccoR member were historical evidence, not executable dependencies.

Before/after `make -n -j8 nanoaodrdataframe fakefactorframe libnanoadrdframe.so` plans agreed. The authorized bounded command was:

```bash
timeout --signal=TERM --kill-after=10s 600s make -j8 nanoaodrdataframe fakefactorframe libnanoadrdframe.so
```

It exited 0; all three targets were already up to date. **No fresh recompilation, dictionary regeneration, clean build or runtime regression was performed.** `git diff --check` passed, and the final/staged diff contained only 27 removed Makefile lines comprising the four rules and their dedicated headings. No source, configuration or generated artifact changed. Environment inspection matched CMSSW_13_3_3, ROOT 6.26/11, GCC 12.3.1 and Python 3.9.14. This task-specific exception does not relax validation for future active-code changes. Both existing origin push destinations succeeded.

## 7. Validation policy for future cleanup

Execution still requires explicit authorization in the current task under [AGENTS.md](../AGENTS.md). This checklist does not grant standing permission to build or run analysis.

For deletion/modification under `src/` or changes affecting compiled/build code:

1. Verify clean starting tree and expected HEAD.
2. Establish and record the baseline conditions.
3. Run `make clean`.
4. Run `make -j8`.
5. Run `nanoaodrdataframe`.
6. Run `fakefactorframe`.
7. Record meaningful ROOT integrity, tree/schema, cutflow and weight quantities.
8. Make the narrow approved change only after baseline passes.
9. Run `make clean`.
10. Run `make -j8`.
11. Rerun the same executables on the same inputs.
12. Compare meaningful quantities; stop on unexplained differences.
13. Inspect exact Git diff, including generated files.
14. Commit/push only after validation passes and when explicitly authorized.

`make clean` removes tracked `src/rootdict.C`; the normal build has been verified to regenerate it identically in the current environment. Recheck clean/build behavior before future use. The clean recipe also removes build products and `.nfs*`; inspect the working environment rather than treating clean as harmless. Do not commit dictionary churn or hide unexplained changes with restore/reset.

Before active-code cleanup, establish an explicitly authorized full regression baseline in an approved durable location, with a manifest recording source SHA, exact command, ordered input files, configuration/payload versions, software environment, threading and random-seed settings, and output checksums. Preserve and compare:

- Exact event identity (`run`, `luminosityBlock`, `event`), multiplicities and order.
- Cutflows and complete tree/branch schema, including types.
- All stored scalar/vector/object values and vector alignment, including per-event weights and normalization quantities.
- All histogram contents, errors, underflow/overflow and relevant totals where present.

Agree comparison handling for floating-point values explicitly; do not silently accept differences or use byte-identical ROOT files as the criterion. Preserve existing normalization conventions and denominator scope. The standalone MC comparator and preserved evidence now exist at the location above. Complete provenance freezing and any production-processor golden baseline remain separate approved tasks; this handoff does not authorize execution.

For production-processor/configuration changes, also run the same production before/after regression test when appropriate and authorized. Documentation-only or clearly non-executable artifact cleanup does not automatically require expensive ROOT runtime validation.

## 8. Important repository/Git behavior

Canonical Git repository path used for this handoff:

```text
/uscms_data/d3/msahoo/Project_tzq/CMSSW_13_3_3/src/single_top_analyser
```

Another displayed path may resolve through the CMSLPC filesystem/symlink setup. Verify the actual Git root rather than assuming a separate checkout.

Existing origin push destinations are intentional. Pushing the current branch through origin targets BOTH:

- GitHub: `https://github.com/Manas3103/topZq_analyzer_2024OnwardsV15.git`
- CERN GitLab: `ssh://git@gitlab.cern.ch:7999/msahoo/top_tzq_analyzer.git`

Previous successful pushes used `git push origin HEAD`; plain `git push origin` encountered a missing-upstream condition. Do not create an upstream or change configuration to bypass it.

Do not modify/simplify remotes or upstream configuration. Verify both push results. Branch creation, staging, commits and pushes require user authorization.

## 9. Known issues intentionally parked

These are follow-up items, **not authorized fixes**:

- Three broken historical tracked JetID symlinks: `data/JERC/2023_Summer23/jetid.json.gz`, `data/JERC/2023_Summer23BPix/jetid.json.gz`, and `data/JERC/2024_Winter24/jetid.json.gz`, each pointing to `../2022_Summer22/jetid.json.gz`. The target is absent. They predate Phase 1 and were intentionally untouched pending provenance/physics review.
- Current 2024 Summer24 configuration instead selects the separate real `data/JERC/2024_Summer24/jetid.json`. Payload loading and actual selection evaluation are distinct; the earlier audit describes manual 2024 JetID selection. Do not infer the correct physics prescription from filenames or repair the historical links automatically.
- Recurring missing ROOT dictionary warning for `OSSF4LInfo`; validation still completed. Investigate separately rather than changing dictionary registrations during cleanup.
- The four historically postponed Makefile rules were removed in `1d24d86`; earlier failed baseline attempts remain recorded above. Active ROOT dictionary rules and MuonScaRe implementation were untouched. Historical RoccoR comments/interface naming require separate review, not a correction change.
- Intermittent remote opening/basket-read stalls remain under investigation; root cause remains unconfirmed despite the subsequent successful full local-input TZQB regression and remote data runs.
- Historical quantile/study utilities retained for possible reference use.
- `normalized_hist` alternatives/tests require separate workflow and normalization review.
- Active physics/architecture concerns in the repository analysis and Run-2 audit remain review items, not silent cleanup targets.

Names containing historical years are insufficient evidence for deleting active/shared behavior. Preserve Run-3 calibration inputs, BTag efficiency ROOT inputs and ignored/untracked material unless separately authorized.

## 10. Likely next cleanup candidates

Candidates for INVESTIGATION only:

- ACLiC dependency untracking is complete in `aaf438b`; do not repeat this cleanup or remove the preserved macros/physics inputs.
- Generated but tracked `src/rootdict.C`: changing tracking needs a separate build/reflection/reproducibility decision. It is not interchangeable with an ignored executable.
- Retained `GetQuantile_Method/Quantiles_jj.C`, `GetQuantile_Method/Mergebins500.C`, `GetQuantile_Method/README.md`: useful research/reference methods without identified production callers; retain unless the user chooses Git-history-only storage. Metadata cleanup is already complete.
- The four dormant Makefile rules are now retired; do not repeat that cleanup. Remaining Phase-2 candidates require separate classification/approval, including dictionary tracking and manual alternative workflows.
- Alternate `BTag/NanoAODAnalyzerrdframe_sneh.cpp`: outside the default source wildcard, but contains Run-3 functionality; manual/external use requires a user decision.
- `normalized_hist` alternatives, including `normalized_hist/Python_rdf_norm/create_hist_rdf_old.py`, `normalized_hist/Python_rdf_norm/unused_txt_file/`, and `normalized_hist/normalization_with_C/Analysed/`: trace manual workflows and downstream normalization before retirement. Active Python and C++ submission workflows must be preserved; differing conventions need physics review.
- Isolated history/log/archive material such as `normalized_hist/.root_hist`, `normalized_hist/Python_rdf_norm/docker_stderror`, and `normalized_hist/my_ploting_project.tar.gz`: verify provenance and consumers; archives are not automatic deletion candidates.
- Remaining historical comments/declarations and Run-2 branches inside active code: separate narrow interface/comment review from protected behavior. Triggers, BTag dispatch, helper APIs, FF fallbacks and year routing are not Phase-2 blanket deletion candidates.

Recommended order: reconcile documentation; investigate low-risk artifacts; preserve/freeze the regression evidence and complete its provenance; finish separately approved Phase 2 work; propose Phase 3 only with approval; address Phase 4 only with separate physics review.

**Investigate/classify first; obtain user approval; then modify.** None is approved for deletion by this handoff. Trace build, dictionary, production, manual-study and downstream consumers; do not maximize deletion or broaden a cleanup batch.

## 11. Instructions for a fresh Codex session

Start with:

```bash
cd /uscms_data/d3/msahoo/Project_tzq/CMSSW_13_3_3/src/single_top_analyser

git status --short
git branch --show-current
git rev-parse HEAD
git log -5 --oneline
```

Read these before proposing further cleanup:

- `AGENTS.md`
- `docs/codex_repository_analysis.md`
- `docs/run2_legacy_audit.md`
- `docs/run3_cleanup_status.md`

Do not make changes merely from this handoff document. Wait for explicit user approval for the next cleanup batch. Do not automatically initialize CMSSW, compile, execute ROOT/analysis, submit Condor, write to EOS, renew credentials, repair symlinks or alter Git configuration.
