# CP-16F — Run-3 campaign metadata and chunk acceptance handoff

## Authoritative checkpoint and scope

Repository: `single_top_analyser`.
Canonical checkout: `/uscms_data/d3/msahoo/Project_tzq/CMSSW_13_3_3/src/single_top_analyser`.
Branch: `cleanup/run3-remove-legacy-run2`.

- Phase-2 closure documentation: `d4b43433b3bc1836e3418aec6a5d16b7d01740b4`.
- Batch 16E/16E.1 implementation: `159fcbc8a53b24e956b2248daa9b8ef87fe87daf`.
- Batch 16F implementation: `b3ce2796acbdcc2c7282a68f02f57850c4fa4695`.
- **Authoritative CP-16F HEAD is the separate commit introducing this file**, not
  the implementation commit. Resolve its exact SHA with:
  `git log -1 --format=%H -- docs/checkpoints/CP-16F.md`.
  A commit cannot embed its own SHA without changing that SHA. The final delivery
  report records the exact document commit and both remote tips.

Git and repository documentation are the source of truth. This document records
verified historical results and implementation boundaries; it is not standing
permission to execute processing, cleanup, submissions or storage changes.

## Completed cleanup and validation boundary

Phase 1 standalone legacy cleanup is complete. Phase 2 is complete **for the
approved technical scope**, including obsolete dispatch/configuration/rules,
seven implementation-only source relocations, automatic compiler dependencies,
ROOT dictionary/PCM recovery, build artifact isolation, dictionary untracking,
ten backed-up top-level historical object removals, checkout runtime refresh and
bounded real Condor loading tests. See `docs/run3_cleanup_status.md` for commit
history and detailed exceptions. Phase 3 behavior cleanup has not started;
Phase 4 requires separate physics approval. Retained research/manual alternatives
and historical artifacts do not invalidate approved-scope closure.

`processnanoaod_v.py` remains authoritative production processing. Standalone
`nanoaodrdataframe` and `fakefactorframe` are debugging/small-file entry points.
They support sample/input/output/help options, preserve no-argument defaults and
automatic MC/data detection. Production and standalone configurations differ.

Active objects/dependencies are under `build/obj/`; active dictionary source,
scanner dependencies and PCM under `build/dict/`. Public library/executables remain
at repository root. Root-level PCM link points to
`build/dict/rootdict_rdict.pcm`. Historical `src/rootdict.C` is ignored/untracked;
local copies, two historical PCMs and eight nested objects remain retained.
GNU Make >= 4.3 is required for grouped dictionary targets. MuonScaRe remains
directly included, not independently compiled; public headers remain in `src/`.

Technical regression and worker smoke checks do **not** establish full
production-scale 2024 pipeline validation, independent physics correctness, all
2024 eras, or implemented 2022/2023/2025 support. Current data validation scope is
2024 **C/H only**. MC regression inputs are representative individual local files.

## Frozen regression and runtime evidence

All external paths below are rooted at:
`/uscms_data/d3/msahoo/Project_tzq/regression_baseline/` (abbreviated BASE).

- `BASE/production-48d4a5b-wigcmx95/manifest.json`: frozen production reference
  from `48d4a5b7d46b2c302c7d9826ff1b276dc72f2c60`, with inputs/configuration/payload
  hashes, environment, commands and comparisons. Main: **6411 events, 648
  branches**. Fake factor: **36539 events, 56 branches, 48 histograms**. Exact
  event identity/order, branch types and all stored values, weights, cutflows,
  histogram contents/errors/metadata were compared; file byte equality is not
  the criterion. Never overwrite these reference artifacts.
- `BASE/tzq_new.root`, `BASE/QCD_bcToE.root`, and `BASE/reports/`: preserved
  standalone CLI references/comparisons. Main 6411/648, FF 36539/56 and 48
  histograms matched exactly. Historical FF histogram cycles 1/2/3 had identical
  content and reflect repeated writes. Data smoke tests: MuonEG C main 153/573;
  MuonEG H FF 880/54. Data tests are execution checks, not golden data regressions.
- `BASE/dictionary-independence-cawfd430`: Batch 12A fresh export without
  historical dictionary/objects/PCMs passed builds, recovery, loading and exact
  production regressions; Batch 12B `28ef609` untracked the historical source.
- `BASE/historical-objects-rmt124ud/reports/postcheck.json` and
  `BASE/historical-objects-backup-2sutf7ao`: Batch 13B deletion/preservation proof.
- `BASE/checkout-runtime-refresh-3dskc42n/manifest.json`: Batch 14G checkout
  rebuild and exact main/FF production regressions PASS, with backup evidence.
- `BASE/condor-runtime-smoke-nc24imwl/repeat-diagnostic-e8d9thm2/results.json`:
  Batch 14H.2 real diagnostic job **4488686.0 exit 0**. CMSSW_13_3_3,
  el9_amd64_gcc12, ROOT 6.26/11, GCC 12.3.1, correctionlib 2.2.2;
  ROOT/PyROOT/Cling and both executable help checks passed. This was not a
  production wrapper run, EOS staging test or worker event-processing test.

The initial job 4488684.0 failed a symlink-only preflight. HTCondor transferred
root-level PCM as a regular 6200-byte file matching the generated PCM; corrected
external diagnostics accept the expected valid relative link **or** matching
regular bytes and independently validate the generated target. No worker repair
is performed. Both worker root and worker/src include paths are required by the
validated harness. Wrapper initialization patch `eb28128` selects gcc12 with
checked CMSSW setup/runtime and lookup paths.

Exactly **88 baseline-reproduced ROOT reflection diagnostics** remain unresolved
(CorrectionSet ownership-member reflection). Successful processing does not prove
arbitrary reflection/serialization correctness. OSSF4LInfo dictionary warnings
are a separate issue. Full production payload transfer, EOS failure propagation,
remote I/O reliability and campaign-scale processing remain unverified. Earlier
XRootD stalls were observed, but their underlying cause was not confirmed.

## Batch 16A–16D: pipeline and storage findings

Read detailed reports; these stages were audits/design, not pipeline execution.

| Batch | Detailed external report | Main conclusion |
|---|---|---|
| 16A | `BASE/pipeline-audit-xutxq6az/REPORT.md` | Processing, histograms and plotting have distinct contracts; no complete merged/dedup/hist/plot golden chain established. |
| 16B | `BASE/eos-merge-audit-97df7jcl/REPORT.md` | Existing EOS merge/dedup scripts can overwrite/delete outputs and fail to propagate errors; completeness/integrity gates absent. |
| 16C | `BASE/ossf-overlap-audit-mfbk3wpi/REPORT.md` | Conditional OSSF object exclusion and first-entry-wins policy; preserve scalar values and verify duplicate content. |
| 16D | `BASE/campaign-design-9j18cyk5/DESIGN.md` | Versioned campaigns, separate attempts/stages/main/FF, immutable validated outputs, explicit provenance and approval gates. |

Actual processing: ordered NanoAOD/filelists -> production TChain/analyser ->
selected `outputTree_00000` and FF measurement histograms -> sample chunk merging
-> data-only per-era stream combination/dedup -> generic normalized histograms ->
external histogram grouping/stack plots. Do not invent an implemented final
fake-background application stage: ratio extraction, prompt subtraction and
FF/(1-FF) prediction were not connected by the audit.

Legacy EOS scripts (read, never modified by metadata work):

- `/eos/uscms/store/user/msahoo/merge_root_chunks.sh`: main `2024_test/` ->
  `2024_merged_rootfile/`; FF `2024_fakeFactor/` -> its `ff_merged_rootfile/`.
  Sample/era grouping uses prefixes and natural chunk sorting; `hadd -f`
  overwrites, and per-sample failures do not reliably yield overall failure.
- `/eos/uscms/store/user/msahoo/2024_merged_rootfile/merged_removeOverlap.sbng`:
  broad wildcard deletion, C/H combinations and unchecked commands. Do not run
  this as a diagnostic or assume it is a scheduler submission description.
- Same directory `Remove_duplicates_multiple.py`: single-thread seen set keyed
  `(run, luminosityBlock, event)`, reset per era input; first encounter retained.
  Output excludes `OSSF4L_info` and does not copy all non-tree objects. Naming
  differs from some downstream sample lists; that connection remains unresolved.

No stream-specific selection is intentionally applied. Effective available-HLT
OR can still depend on input branch presence. Retain first-entry-wins with fixed,
recorded input order; no new physics stream priority is authorized. Acceptance
requires verification that duplicate retained content agrees, or a separate
approved policy for differences. Era C/H separation is intentional.

**OSSF4L_info exclusion:** conditionally safe for identified current downstream
scalar/derived-column consumers, not a full object-equivalence claim. Category,
best/second-Z masses and leftover fields are exported; idx1/idx2 and some
category-dependent indices are not completely duplicated. Verify required scalar
branches and exact retained values; keep raw merged inputs. Unknown external
consumers requiring object pair indices remain an unresolved boundary.

## Main/FF weights and generator bookkeeping

Keep main and FF sample/configuration identities independent:
`jobconfiganalysis_2024.py` versus `jobconfiganalysis_2024_ff.py`. FF correction
labels, selections and measurement weights differ and remain protected.

Recorded current tree behavior: main MC `evWeight` includes genWeight, PU,
central BTag and lepton factors; data `evWeight=1`. `genEventSumw` constructed
from current input-chain genWeight is not necessarily a dataset denominator.
External `DataMC_txt/SumOfGenWeight_calculator.py` sums Runs/genEventSumw over
accessible files; skipped files and limited printed precision need recorded
coverage/provenance, not silent acceptance.

Current simplified Python histogram formula is intentionally preserved:
`lumi * xsec / sumw`, multiplied by `genWeight` when present; data weight is 1.
PU/lepton/BTag additions are disabled in that recipe; it does not simply use
stored evWeight or FF measurement weights. Wrapper luminosity literal 12.75 is
an observed setting, **not a verified unit/era convention**. Cross-section units,
authoritative table coverage and luminosity label consistency remain unresolved.
Fallback Max of a chunk-level denominator must not be claimed dataset-wide.
Alternate C++ normalization has different weighting; do not synchronize it.
FF measurement data weight 1 and MC trigger weights/tight gating are separate
from generic histogram normalization; ongoing FF physics needs separate review.

Dataset metadata records external `sumGenWeight`, source script, exact dataset,
filelist/report hashes, coverage, provenance, cross section/units, luminosity and
formula. Verified sums require linked verified evidence; metadata checks do not
execute calculators or prove their truth. Unknown units/settings stay unknown.
No dataset-versus-chunk denominator substitution or weighting change is approved.

## Campaign design and completed metadata tooling

Proposed EOS namespace (NOT created):
`/eos/uscms/store/user/msahoo/tzq/2024/campaigns/<campaign-id>/` with:

```text
metadata/                  source/config/payload identities and manifests
main/attempts/<attempt>/   MC/data chunks, logs and job descriptions
main/stages/<stage>/       merged, data-combined, data-dedup, histograms
fakefactor/                independent equivalent stages/configuration
plots/<revision>/          separate plotting revision
validation/<comparison>/  reports
```

Campaign identity describes physics configuration; attempts describe retries;
dataset identity includes exact version/era; stages describe recipes/parents.
Example ID `2024-CH-sel01-corr01-20261010-d4b4343` is a human label, never a
substitute for full hashes. Archive via immutable catalog/reference, not automatic
large-file copying. Existing backups have uncertain provenance; preserve them.
Same-EOS version retention is not independent disaster backup.

Plotting repository: `https://github.com/Manas3103/stack_histogram`, branch
`uncertainty-band`, directory `MergeandStackHist_RDFpy`; 16D inspected revision
`8e8eb7674f87382d85a351dd90e6cf2afaba48ba`. User's current local plotting changes
are not established by that remote snapshot. Record actual revision/config,
grouping, missing members, numerical outputs and luminosity labels separately.

Batch 16E/16E.1 provides campaign/dataset/attempt/stage schemas and templates;
`tools/campaign.py` commands `plan`, `config`, `validate`, `summary`, `tree`.
Config parsing is AST literal inspection, not imports. Records distinguish
**declared**, **verified**, **unknown/unavailable**, and runtime-effective facts.
No ROOT import, DAS lookup, campaign creation or execution. Dirty paths are
recorded but not a complete patch snapshot; templates are synthetic placeholders.
Python >=3.9 and installed jsonschema Draft 2020-12 support are prerequisites.

Batch 16F adds `chunks` and four schemas: expected_chunks, chunk_inventory,
chunk_completeness_report, accepted_merge_inputs. Explicit plan embeds dataset
chunks plus submission/filelist path/hash; inventory binds each attempt, accepted
flag and output validation metadata. Exactly one accepted validated exit-0,
verified-processing-PASS attempt per expected chunk is required. Reject wrong
campaign/sample/workflow/input lists, unexpected identities, ambiguous acceptance,
missing/empty accepted files and size/SHA mismatch. Failed rejected retries stay
visible; a successful accepted retry can pass. Sort merge inputs by expected
numeric index. FAIL produces an empty merge list.

No directory scan: omitted unexpected files cannot be detected. Rejected attempt
outputs are not content-checked. ROOT validity is supplied external evidence;
checksums cannot prove ROOT semantics. Hashing reads approved local regular files
in blocks; no remote ROOT access. Relative paths must resolve inside local-root;
/eos, URL, traversal and escaping paths rejected. Arbitrary mount aliases are not
reliably identifiable: caller must supply local storage. Recheck input identity
before later merging; reports are not immutable file locks or physics approval.

Retention metadata suggests **7 days** from an explicit verified retention start,
not mtime. Eligibility needs accepted identities, verified merge PASS (and data
dedup PASS for data), elapsed period and audit evidence. This is review eligibility
only. `deletion_enabled=false`; **no deletion command exists**. Never automatically
delete accepted chunks, raw merged data, calibration inputs, goldens, manifests,
backups or unknown historical artifacts. Separate exact-path retention approval,
recovery/backup proof and storage authorization are required.

External metadata checkpoint:
`BASE/campaign-design-9j18cyk5/BATCH16E2_CHECKPOINT_159fcbc.md`.

## Tests, next stages and approval gates

CP-16F validation: all **55 synthetic tests** passed (36 metadata + 19 chunk
acceptance). Bounded CLI subprocess checks passed JSON/summary/accepted output
and missing-chunk FAIL exit 1; schema and whitespace checks passed. Fixtures use
temporary local bytes, not actual ROOT files. No fresh physics/build validation
was performed for metadata changes, and none is implied.

Next proposed batch: read-only merge planning from accepted manifests, then a
separately approved local merger with synthetic ROOT fixtures. Required design:
explicit inputs, repeated checksum/schema validation, failure propagation,
unique temporary outputs, no clobber, interrupted-stage reporting, entry/schema/
histogram verification and no deletion. EOS publication primitives require a
separate bounded backend test and explicit write approval.

Later independent stages: deterministic data deduplication and duplicate-content
checks; production output routing/runtime provenance; normalization/histogram
metadata and exact tests preserving current formulas; plotting capture/numerical
validation; immutable catalog/archive/recovery. Full C/H pipeline and expansion
to other eras remain gated. Existing EOS merge/dedup and production scripts have
not been replaced or integrated by this foundation.

Before acting, read AGENTS.md, this file, docs/run3_cleanup_status.md,
docs/campaign_metadata.md, and relevant external reports. Initial commands:

```bash
cd /uscms_data/d3/msahoo/Project_tzq/CMSSW_13_3_3/src/single_top_analyser
git status --short
git branch --show-current
git rev-parse HEAD
git log -5 --oneline
```

Use only separately authorized bounded tests. No automatic CMSSW initialization,
make clean, ROOT/event processing, Condor submission, EOS writes/moves/deletions,
credential renewal, historical cleanup, Phase 3 refactoring or physics changes.
Do not change remotes; origin intentionally pushes to GitHub and CERN GitLab.
Proposed implementation requires explicit user approval; code approval is not EOS
or submission permission. Stop on unexpected tracked changes, incomplete evidence,
unexplained physics differences or environment mismatch. Do not guess units,
correction prescriptions, dataset identities or runtime-effective settings.
