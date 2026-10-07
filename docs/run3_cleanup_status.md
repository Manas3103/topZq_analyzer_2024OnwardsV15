# Run-3 Cleanup Status

## 1. Purpose

This handoff records the checkpoint of `cleanup/run3-remove-legacy-run2` so a fresh Codex session can continue safely without conversational memory. **Git + repository documentation are the source of truth.** Historical investigation notes describe their original baselines; verify current code before acting on them.

The intended scope is CMS Run 3 (2022–2025), currently centered on 2024 NanoAODv15. This scope does not imply complete implementation of every year.

## 2. Current checkpoint

- Branch: `cleanup/run3-remove-legacy-run2`
- HEAD before this documentation commit: `4a43a0cbc31e5eb64b7a1a7fda04cb50e38fab78`
- Starting working tree: clean.

HEAD will advance by one documentation-only commit when this file is committed. Use Git to determine that new SHA; this document deliberately records the pre-documentation checkpoint.

## 3. Cleanup commits completed

Verified against Git, in chronological order:

| SHA | Commit message | Purpose |
|---|---|---|
| `f59b5408a5130037b5822a9678ff42135fc215bf` | docs: add Run-2 legacy audit | Static classification and exact Phase-1 input manifest. |
| `13f2b14e8179268a157aec02004caa54e261a331` | cleanup: remove standalone Run-2 legacy inputs | Remove exactly 78 approved tracked inputs, including the 2017 pileup macro. |
| `138a6f25931977efb2fc65df803be0e1753850d6` | cleanup: remove unused legacy analysis files | Remove SkimEvents and obsolete b-tag binning module after build/runtime validation. |
| `9187777a997cce31459dd995ba56188801107fd9` | cleanup: retire obsolete per-file processing path | Retire per-file dispatch/processor and isolated reference code; retain single-chain production. |
| `4a43a0cbc31e5eb64b7a1a7fda04cb50e38fab78` | cleanup: remove obsolete allinone configuration | Remove redundant keys, examples and logging; update misleading README usage. |

Earlier orientation commits:

- `33cd64c0e707bef23ce0cb1d7296072c86093aaa` — `docs: record repository architecture and baseline before Codex-assisted development`.
- `ac8fee688cdc215bbe51914404c7233635ec4e0b` — `docs: add repository-level Codex development instructions`.

## 4. What has been removed

- Standalone Run-2 legacy certification/calibration/input batch: 77 tracked manifest inputs plus `data/make2017MCpileup.C`. See section 10 of [the original audit](run2_legacy_audit.md) and the exact deletion list in commit `13f2b14`; directory names were not blanket deletion authorization. Ignored/untracked calibration files were excluded.
- `src/SkimEvents.cpp`, `src/SkimEvents.h`, `btagging_efficiency_binning.py`.
- `processonefile.py`.
- `src/used_function/used_func.cpp`, `src/used_function/EventShape.cpp`, `src/used_function/EventShape.h`.
- Obsolete `allinone=False` dispatch and its exclusive per-file processor implementation.
- Behaviorally redundant `allinone` keys/comments in both active 2024 configs, processor logging, and obsolete README examples.

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

## 6. Validation established

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

Relevant ROOT outputs opened normally, were not zombie/recovered, and recorded schemas/histogram/weight quantities used for comparisons matched. The latest production output contained a tree and no histograms; histogram comparisons apply only where present. No byte-identical ROOT output claim is made. These are bounded regression checks, not comprehensive physics validation.

Clean builds and both standalone runtimes passed for the compiled-code/per-file retirement batches. The later redundant-key cleanup used Python/static checks and production before/after validation; it did not rerun a C++ build because compiled files were untouched.

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

For production-processor/configuration changes, also run the same production before/after regression test when appropriate and authorized. Documentation-only or clearly non-executable artifact cleanup does not automatically require expensive ROOT runtime validation.

## 8. Important repository/Git behavior

Canonical Git repository path used for this handoff:

```text
/uscms_data/d3/msahoo/Project_tzq/CMSSW_13_3_3/src/single_top_analyser
```

Another displayed path may resolve through the CMSLPC filesystem/symlink setup. Verify the actual Git root rather than assuming a separate checkout.

Existing remotes are intentional. `git push origin` intentionally pushes to BOTH:

- GitHub: `https://github.com/Manas3103/topZq_analyzer_2024OnwardsV15.git`
- CERN GitLab: `ssh://git@gitlab.cern.ch:7999/msahoo/top_tzq_analyzer.git`

Do not modify/simplify remotes or upstream configuration. Verify both push results. Branch creation, staging, commits and pushes require user authorization.

## 9. Known issues intentionally parked

These are follow-up items, **not authorized fixes**:

- Three broken historical tracked JetID symlinks: `data/JERC/2023_Summer23/jetid.json.gz`, `data/JERC/2023_Summer23BPix/jetid.json.gz`, and `data/JERC/2024_Winter24/jetid.json.gz`, each pointing to `../2022_Summer22/jetid.json.gz`. The target is absent. They predate Phase 1 and were intentionally untouched pending provenance/physics review.
- Current 2024 Summer24 configuration instead selects the separate real `data/JERC/2024_Summer24/jetid.json`. Payload loading and actual selection evaluation are distinct; the earlier audit describes manual 2024 JetID selection. Do not infer the correct physics prescription from filenames or repair the historical links automatically.
- Recurring missing ROOT dictionary warning for `OSSF4LInfo`; validation still completed. Investigate separately rather than changing dictionary registrations during cleanup.
- Dormant explicit `RoccoR.o` Makefile recipe referencing absent RoccoR sources; do not confuse it with active Run-3 MuonScaRe corrections.
- Dormant `rootdicttmp.C`/`libtest.so` Makefile rules referencing missing temporary test inputs; still present, outside default targets.
- Historical quantile/study utilities retained for possible reference use.
- `normalized_hist` alternatives/tests require separate workflow and normalization review.
- Active physics/architecture concerns in the repository analysis and Run-2 audit remain review items, not silent cleanup targets.

Names containing historical years are insufficient evidence for deleting active/shared behavior. Preserve Run-3 calibration inputs, BTag efficiency ROOT inputs and ignored/untracked material unless separately authorized.

## 10. Likely next cleanup candidates

Candidates for INVESTIGATION only:

- Tracked `.DS_Store`/artifact files under historical study directories.
- `GetQuantile_Method` historical utilities and whether they should remain as reference methodology.
- Dormant RoccoR Makefile recipe.
- Dormant `rootdicttmp`/`libtest` Makefile recipes.
- Other isolated non-production artifacts identified in the original Run-2 audit.

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
