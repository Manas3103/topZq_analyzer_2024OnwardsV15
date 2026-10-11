# CP-16H-A — Offline ROOT output-validation foundation

## Parent checkpoint and scope

Repository: `Manas3103/topZq_analyzer_2024OnwardsV15` (`single_top_analyser`).
Branch: `cleanup/run3-remove-legacy-run2`.
Authoritative parent: [CP-16G](CP-16G.md), commit
**`7a324f40f38fdcf0ee3da75b0de25c5afed9a01f`**.
Resolve this checkpoint's committed revision with
`git log -1 --format=%H -- docs/checkpoints/CP-16H-A.md`; the delivery report records
the exact commit and remote synchronization results.

Batch 16H-A provides read-only inspection of explicitly inventoried approved local
ROOT files. It emits structured evidence independently of Batch 16F acceptance and
Batch 16G scheduler monitoring. It does not establish production readiness, output
origin, physics correctness or compatibility with live EOS/XRootD storage.
Parent physics and campaign-validation limitations remain in force.

## Files and responsibilities

| File | Responsibility |
|---|---|
| `tools/root_output_validation.py` | Inventory/profile validation, local-path checks, subprocess control, strict worker-evidence reconciliation and JSON CLI. |
| `tools/root_output_worker.py` | Isolated PyROOT READ inspection and content-coverage evidence. |
| `tests/test_root_output_validation.py` | Offline contracts, mocked failures, protocol regressions and tiny real ROOT fixtures. |
| `docs/root_output_validation.md` | Interface, semantics, production-convention evidence and limitations. |
| `docs/root_validation_profiles/main-data.json` | Configurable metadata-only main data example. |
| `docs/root_validation_profiles/main-mc.json` | Main MC example with generator-weight branch requirement. |
| `docs/root_validation_profiles/fakefactor-data.json` | FF data example with FF branches and four flavor numerator/denominator TH2 objects. |
| `docs/root_validation_profiles/fakefactor-mc.json` | FF MC example, including generator-weight branch requirement. |
| `docs/checkpoints/CP-16H-A.md` | This checkpoint record. |

All nine files are new. Existing campaign schemas, acceptance logic/records,
monitoring code, production submit/worker scripts, physics and processing configs
are unchanged.

## Inventory, profiles and architecture

Version-1 `root_output_inventory` requires 1..100 explicit records identifying
campaign, workflow (`main`/`fakefactor`), dataset/sample, datatype (`data`/`mc`),
chunk ID/index, attempt, exact absolute local path and profile ID. Optional job IDs
and evidence references are declarations only. Repeated resolved paths/jobs,
duplicate or contradictory attempts and inconsistent dataset/chunk declarations
are rejected. No filename inference establishes campaign/attempt provenance.
`--local-root` explicitly approves existing local storage; URLs, traversal,
`/eos` paths and resolved symlink escapes are rejected.

Version-1 profiles configure tree, required branches, optional branch types,
required object names/classes, zero-entry policy, dictionary-sensitive branches
and explicit metadata/sample/full read policy. Examples use `outputTree_00000`
following the current 2024 production configs and writer. They are minimal
configurable examples, not universal production schema definitions. Types are
unspecified in examples. Main and FF requirements remain separate.

Python 3.9+, PyROOT and Linux resource controls support actual inspection. The
controller does not import ROOT. Each sequential child opens TFile in READ mode,
with wall timeout 1..60 seconds (default 20), CPU soft/hard limits of timeout and
timeout+1 seconds, address space 512..8192 MiB (default 4096), and disabled core
dumps. Timeout/crash/dependency limitations are UNKNOWN. JSON goes only to stdout;
there is no automatic report-file replacement.

## Checks, coverage and worker protocol

File checks distinguish missing/nonregular/zero-byte files, access failures and
detectable mutation using pre/post size, timestamps, device, inode and mode.
ROOT checks cover open refusal/null/zombie, recovery flag, required TTree,
entry count, required branches/types and required object existence/class matching.
Zero selected entries are allowed by default; an explicit profile can forbid them.

Metadata mode does not read tree entries. Sample mode reads configured branches
at deterministic bounded indices and records exact successful coverage. Full mode
reads configured branches over every entry and records half-open ranges. Allowed
zero-entry full coverage is vacuous. Coverage does not certify unconfigured
branches, all objects or numerical physics content. External baskets/TChains are
unsupported rather than followed during configured content inspection.

Worker JSON rejects duplicate keys, nonfinite numbers, unexpected fields and
incorrect types. PASS requires affirmative, internally consistent evidence for
the requested profile and read mode, including branch/object results and complete
configured coverage. Valid early failures do not require impossible downstream
evidence. Invalid/incomplete/contradictory responses become `worker_protocol`
UNKNOWN with safe diagnostic stderr tails retained.

The worker derives `dictionary_dependent_branches` from actual nonempty ROOT
`GetClassName()` metadata, including object/container branches. The controller
requires those names in branch metadata and an exact dictionary-map inventory
covering derived identities plus profile-listed branches. Null is valid only for
absent branches; present entries must be boolean. Missing/null contradictory
evidence cannot establish PASS. Unavailable dictionaries blocking configured
reads are UNKNOWN. Ordinary scalar leaf branches require no dictionary evidence.
OSSF4L_info is neither silently removed nor replaced; metadata-only PASS does not
establish its content readability. No dictionaries are built or repaired.

The corrections close the identified empty-evidence, contradictory-null and
missing-unlisted-dictionary false-PASS paths. Protocol checks establish reported
consistency, not independent proof that the worker read particular bytes.

## Status, provenance and acceptance

PASS means requested checks completed; FAIL means a conclusive requested-check
failure; UNKNOWN means incomplete inspection. FAIL dominates UNKNOWN while all
issues remain recorded. CLI exits: 0 PASS, 1 FAIL, 3 UNKNOWN, 2 invalid input/usage.
Root-open refusal does not establish its underlying cause as corruption.

Every report preserves provenance UNKNOWN with caller-declared association and
campaign acceptance `not_evaluated`; `acceptance_changed=false`. Structural or
content PASS never verifies campaign origin, scheduler success or accepted chunks.
Batch 16F still requires its independent evidence and explicit acceptance decision.
No Batch 16E/16F/16G contract is changed.

## Offline validation

Environment retained: CMSSW_13_3_3, Python 3.9.14, ROOT 6.26/11.
Final checkpoint suite command:

```bash
PYTHONDONTWRITEBYTECODE=1 timeout 60s python3 -m unittest discover -s tests
```

Result: **180 passed, 0 failed, 0 skipped**: 99 existing campaign/monitor tests and
81 output-validation tests (21 controller, 14 mocked ROOT, 32 protocol regressions,
14 real local ROOT integration tests). Real fixtures are tiny temporary scalar/
vector trees and histograms, including corrupt bytes; recovery, unavailable
custom dictionaries, timeouts/crashes and difficult read failures use mocks.

The bounded deterministic adversarial controller/CLI matrix passed **46 cases**:
listed/unlisted dictionary true/false/null/missing, required branch/object presence,
zero-entry policy, metadata/sample/full mode, requested/reported branches, missing/
duplicate/out-of-range sampled indices, incomplete full coverage, contradictory
completion and malformed fields. This separately executed temporary harness is not
a new repository testing framework; persistent regressions cover confirmed defects.
These tested cases establish no identified remaining false-PASS, not exhaustiveness.

Synthetic metadata/sample/full CLI examples each returned PASS/exit 0, provenance
UNKNOWN and acceptance unchanged. Reproduce with the temporary-fixture instructions
in [root_output_validation.md](../root_output_validation.md), then run:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 tools/root_output_validation.py \
  --inventory /tmp/approved-example/inventory.json \
  --profile docs/root_validation_profiles/main-data.json \
  --local-root /tmp/approved-example
```

The example requires separately created synthetic files, never production outputs.
`git diff --check`, per-new-file whitespace checks and staged whitespace review
are part of the authorized checkpoint gate.

## Limitations and deferred work

- No live EOS/XRootD compatibility or production-scale validation is established.
- Top-level class metadata is not recursive member-dictionary proof; actual
  OSSF4L_info and broad split/composite deserialization remain unverified.
- File integrity is size/open/recovery evidence, not checksum or whole-file proof.
  Pre/post metadata is not a lock or immutable snapshot. Hardlink aliases are not
  deduplicated and arbitrary EOS mount aliases require caller discipline.
- Per-child limits do not bound controller filesystem operations or total inventory
  duration. Captured logs are buffered. Isolation is not an adversarial-file sandbox.
- Profile examples do not check all branches, histogram numerical content or
  full physics schemas. Inventory association is not byte-bound provenance.
- Actual resource exhaustion/crashes use mocked tests; site operational behavior
  and processing correctness are not established by this offline checkpoint.

Batch 16H-B remote discovery/access and broader validation remain deferred.
Worker/staging provenance, execution-instance identities and retry planning remain
separate later work. No Batch 16H-B implementation has started.

No production processing, live ROOT-output inspection, EOS/XRootD access, Condor
queries/submissions, staging, retry, acceptance mutation, physics change, credential
renewal or environment reinitialization occurred. Only authorized synthetic local
ROOT fixtures were created/read for tests. Commit and ordinary remote synchronization
are explicitly authorized in this task; their outcomes belong in the delivery report.
