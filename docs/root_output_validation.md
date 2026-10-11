# Batch 16H-A: offline ROOT output validation

`tools/root_output_validation.py` inspects explicit local outputs and emits JSON
validation evidence to stdout. It does not discover datasets, inspect EOS/XRootD,
query Condor, stage outputs or select accepted attempts. Batch 16E/16F/16G schemas
and logic are unchanged. Python 3.9+, Linux resource controls and PyROOT are needed
for actual inspection. Only the child reader imports ROOT; the controller remains
usable without it and reports UNKNOWN dependency limitations.

## Inventory

Version 1 `root_output_inventory` has `schema_version`, `kind`, and a nonempty
`outputs` array (maximum 100). Every record requires:

```json
{
  "campaign_id": "synthetic", "workflow": "main",
  "dataset_id": "synthetic-data", "sample_name": "SyntheticData",
  "datatype": "data", "chunk_id": "c0", "index": 0,
  "attempt_id": "a0", "path": "/tmp/approved-example/tiny.root",
  "profile_id": "main-data-example"
}
```

Workflow values are the established `main` and `fakefactor`; datatype is `data` or
`mc`. Optional `job_id` is canonical ClusterId.ProcId (or null); optional `evidence`
is a list of nonempty reference strings. These are caller declarations only.
There is no scheduler lookup, filename inference or independent verification of
campaign/dataset/input origin. Chunk IDs and nonnegative integer indices remain
separate. Duplicate output or campaign/workflow/attempt identities, repeated
resolved paths/jobs and contradictory dataset/chunk declarations are invalid.
Retries may reference the same chunk under separate attempt IDs and file paths.

`--local-root` must explicitly name approved existing local storage. Output paths
must be absolute, inside that root after resolving symlinks. URLs, colon syntax,
traversal, `/eos` paths and symlink escapes are rejected. Direct mounted EOS and
known symlink aliases are rejected; arbitrary mount aliases cannot be identified
reliably. The caller must not supply an EOS mount alias as approved local storage.
Local ROOT trees referencing external basket files or TChains are unsupported;
content inspection refuses those references rather than following remote files.

## Profiles and production conventions

Version 1 `root_validation_profile` requires a profile ID, workflow/datatype, exact
`tree`, `required_branches` list, `branch_types` map, `required_objects` map,
`allow_zero_entries` boolean, `content` policy and `dictionary_branches` list.
Unknown fields/versions and duplicate names are rejected. Types refer to required
branches. Required objects map names to ROOT base classes (e.g. TH1 or TH2).

Four examples are under `docs/root_validation_profiles/`: `main-data.json`,
`main-mc.json`, `fakefactor-data.json`, `fakefactor-mc.json`. They are configurable
minimal examples, not frozen universal production schemas. Types are deliberately
unspecified in examples; scalar branch type strings (e.g. UInt_t) or object class
names can be supplied in a reviewed profile. Multi-leaf unclassed branches have
unknown type metadata and fail an explicit incompatible type expectation.

The current configs use outputTree; BaseAnalyser.cpp:117–121 and
FakeFactorAnalyser.cpp:413–417 end at cut 00000. The actual writer appends the cut
name (NanoAODAnalyzerrdframe.cpp:2341), yielding `outputTree_00000`.
Main registers IDs/evWeight at BaseAnalyser.cpp:1611–1614 and disables histogram
booking at 1974. FF registers IDs, MC generator weights, MR and FF-weight branches
at FakeFactorAnalyser.cpp:522–565 and books histograms at 623. FF example objects
are the four weighted flavor numerator/denominator `_nocut` TH2 histograms from
425–454, with suffix construction in NanoAODAnalyzerrdframe.cpp:2175–2196.
Examples distinguish main/FF and data/MC without claiming historical branch counts
are universal. Histograms are checked for existence/class, not numerical physics
correctness, complete inventory, axes, bin errors or cutflow agreement.

All example profiles allow zero selected entries. A nonempty ROOT file containing
an intact zero-entry tree may PASS. Set `allow_zero_entries=false` only with a
reviewed requirement. Empty filesystem files always FAIL.

## Content coverage and dictionaries

`content` requires `mode`, `branches`, and `sample_entries` (1..10000):

- `metadata`: no event content read; coverage is explicitly empty. Required object
  checks deserialize those objects, but do not establish tree-basket readability.
- `sample`: explicit branches at up to sample_entries deterministic evenly spaced
  entry indices, including first/last when at least two entries are sampled.
- `full`: explicitly requested branches over all tree entries, subject to the same
  time/memory bounds. This is not a claim that unconfigured branches or all file
  objects are readable.

Each branch is read separately and must report positive bytes for a selected entry.
Coverage records per-branch checked sample indices or half-open full entry ranges,
plus fully completed entries. Failure preserves partial coverage. A zero-entry
full read is vacuously complete for the configured branches, with no entry bytes
read. `complete_configured_read` is false for metadata/sample modes.

`dictionary_branches` identifies potentially dictionary-dependent branches. Their
presence/type metadata is recorded independently of reading. A null dictionary
entry means that configured branch is absent; false means no usable dictionary
was found. Absence is only fatal if required by branch/content policy. Every
present object-class branch also receives dictionary inspection. Configured reads
requiring unavailable dictionaries are UNKNOWN with dictionary_unavailable;
no silent object removal, scalar substitution or file-corruption claim occurs.
Main examples explicitly flag OSSF4L_info. Metadata-only PASS does not prove that
this object can be read. Its scalars do not replace all object indices, as recorded
in the Batch 16C audit. No dictionary is compiled/generated or repaired here.

## Worker evidence protocol

The controller strictly parses worker JSON: duplicate keys, nonfinite numbers,
unknown fields, incorrect types and incomplete or contradictory evidence produce
`worker_protocol` UNKNOWN (CLI exit 3). An empty issues list alone cannot establish
PASS. Successful responses must identify the requested tree, record open/recovery
and entry-count checks, account for required branches/types and objects (including
explicit class-match results), and agree with the requested content policy.

Metadata mode requires completed structural evidence and no entry reads. Sample
coverage must account for the deterministic requested indices for every configured
branch; full coverage must account for every configured branch and tree entry.
Allowed zero-entry full reads are vacuously complete. Partial coverage is preserved
for valid read failures or exceptions. Valid earlier failures, such as a missing
tree, do not require impossible downstream evidence: they remain FAIL. Dictionary
limitations and dependency failures remain UNKNOWN rather than protocol failures
when their staged evidence is consistent. Every dictionary-map entry is checked:
null means the branch is absent; present entries must be boolean, including entries
not listed in the profile. The worker reports dictionary_dependent_branches from
nonempty ROOT GetClassName() metadata (including object/container branches). The
controller requires those identities to be present in branch metadata and requires
the dictionary map to account for exactly those identities plus profile-listed
branches. Ordinary scalar leaf branches require no dictionary evidence. Missing
evidence or contradictory null evidence is worker_protocol UNKNOWN. This remains
a top-level ROOT metadata classification, not recursive member-dictionary proof.
Diagnostic stderr tails are retained.

These checks verify consistency of reported evidence, not independent proof that
the worker read the bytes. Split/composite deserialization and recursive dictionary
dependencies retain the existing coverage limitations.

## Results, limits and safety

Reports contain record identity, exact resolved inspected path, profile ID/version,
the complete selected profile definition and per-worker resource limits,
file size/mtime/ctime/device/inode/mode and post-inspection metadata, ROOT structure,
content coverage, issue codes, diagnostics, requested-check status and separate
checks for accessibility, limited file integrity, ROOT structure, content,
provenance and campaign acceptance. File integrity means only size/open/recovery
checks: no SHA256 or whole-file integrity/provenance proof is produced.

PASS means the requested checks completed; FAIL means a requested check conclusively
failed; UNKNOWN means incomplete inspection. Missing/nonregular/zero-byte files,
ROOT null/zombie/open refusal, recovered status, missing structures, type mismatches,
zero-policy violation and failed branch reads have distinct FAIL codes. Filesystem
access failures, missing ROOT, exceptions, crashes, timeouts, external storage,
missing dictionaries and detectable file mutation have distinct UNKNOWN codes.
Root-open refusal is not claimed to prove its underlying cause was corruption.

FAIL takes precedence over UNKNOWN when both occur; every issue remains in the
report. CLI exit codes are 0 PASS, 1 FAIL, 3 UNKNOWN and 2 invalid input/usage.
Root structure/content PASS does not verify provenance. Every output keeps
provenance status UNKNOWN with caller_declared association, campaign acceptance
not_evaluated, and the report sets acceptance_changed=false. Condor IDs are
references only; zero exit is never used to infer processing/staging success.

One new subprocess per output uses TFile READ, never UPDATE/RECREATE. Wall timeout
is 1..60 seconds (default 20), CPU limit follows it, address space 512..8192 MiB
(default 4096), core dumps disabled, sequential maximum 100 outputs. ROOT stdout
is redirected to stderr and a dedicated JSON channel is preserved; diagnostics
are included with a 4000-character tail. Captured process output is buffered, not
a streaming log service. Resource exhaustion/crash returns UNKNOWN. These limits
bound each output, not a single overall inventory deadline; callers can additionally
use an external timeout. No runtime/environment reinitialization occurs.

Stat comparison detects size/timestamp/inode changes but is not a file lock or
immutable snapshot. Same-size tampering with restored metadata can escape detection.
Do not concurrently replace inspected files or interpret reports as future byte
locks. ROOT inspection is process-isolated for robustness, not a security sandbox
for adversarial ROOT payloads. Use approved trusted local fixtures/outputs only.

JSON goes to stdout; no report-file overwrite option exists. Shell redirection can
still overwrite files, so save externally only to new approved local paths using
no-clobber protection. Tests and examples use PYTHONDONTWRITEBYTECODE=1 to avoid
import caches. ROOT fixture creation is separate explicitly authorized test work.

## Offline example

From the repository root, create a fresh temporary synthetic fixture and inventory:

```bash
export TZQ_FIXTURE_DIR=$(mktemp -d /tmp/tzq-root-validation-XXXXXX)
PYTHONDONTWRITEBYTECODE=1 python3 - <<'PY'
from array import array
import json, os
from pathlib import Path
import ROOT
root = Path(os.environ['TZQ_FIXTURE_DIR'])
f = ROOT.TFile(str(root/'tiny.root'), 'RECREATE')
t = ROOT.TTree('outputTree_00000', 'synthetic')
buffers = {'run': array('I', [1]), 'luminosityBlock': array('I', [1]),
           'event': array('Q', [1]), 'evWeight': array('d', [1.0])}
for name, value in buffers.items():
    t.Branch(name, value, name + '/' + {'run':'i','luminosityBlock':'i','event':'l','evWeight':'D'}[name])
t.Fill(); t.Write(); f.Close()
e = dict(campaign_id='synthetic', workflow='main', dataset_id='synthetic-data',
         sample_name='SyntheticData', datatype='data', chunk_id='c0', index=0,
         attempt_id='a0', path=str(root/'tiny.root'), profile_id='main-data-example')
(root/'inventory.json').write_text(json.dumps(dict(schema_version=1, kind='root_output_inventory', outputs=[e])))
PY
PYTHONDONTWRITEBYTECODE=1 python3 tools/root_output_validation.py \
  --inventory "$TZQ_FIXTURE_DIR/inventory.json" \
  --profile docs/root_validation_profiles/main-data.json \
  --local-root "$TZQ_FIXTURE_DIR"
```

The synthetic outputs are not production data or accepted campaign chunks. For
sample/full coverage, explicitly change a local profile copy's content policy;
do not infer a content policy from a filename or automatically overwrite profiles.

## Integration boundary and tests

Batch 16F can later reference a reviewed validation report as output.validation
evidence, but still requires independent processing evidence, explicit acceptance
and local size/SHA256 verification. This tool neither fills nor edits those records.
Batch 16G continues to supply independent scheduler evidence; missing/contradictory
history does not alter this report's output-origin confidence.

Tests in `tests/test_root_output_validation.py` separate real PyROOT integration
from mocked recovery/dictionary/read-error/resource-failure cases. Real integration
uses tiny temporary scalar trees/histograms and a corrupt local byte fixture,
including zero entries, missing tree/branch/object, wrong types, sample and full
reads. Missing PyROOT skips only actual ROOT integration tests; mock tests remain
available. No production datasets, ROOT macros, Condor or EOS are required.

Validation in the inspected CMSSW_13_3_3/el9_amd64_gcc12 environment (Python 3.9.14,
ROOT 6.26/11, GCC 12.3.1) used:

```bash
PYTHONDONTWRITEBYTECODE=1 timeout 60s python3 -m unittest discover -s tests -v
git diff --check
```

Initial implementation full suite: **145 passed, 0 failed, 0 skipped**: 99 existing campaign/monitor
tests plus 46 new tests (21 controller/contract tests, 14 mocked ROOT tests and
11 actual local PyROOT integration tests). The standalone synthetic main-data CLI
example returned exit 0, requested validation PASS, provenance UNKNOWN and
acceptance_changed=false. No existing ROOT outputs were altered. The initial
corrupt-file integration failure was corrected by handling PyROOT's open-time
OSError as root_open_failed; subsequent integration/full suites passed.
Whitespace checks also inspect each untracked new file against /dev/null.

Batch 16H-B remote EOS/XRootD discovery/access/checksums, broader reviewed profiles,
remote provenance and receipt integration remain deferred. Worker/staging changes,
execution-instance identities and retry control belong to later separately approved
work. No new mandatory provenance fields or existing campaign-schema changes occur.

Targeted protocol correction validation: **167 passed, 0 failed, 0 skipped**
(99 existing tests and 68 output-validation tests: 21 controller, 14 mocked ROOT,
21 protocol regressions and 12 real local ROOT integration tests). Synthetic
metadata/sample/full CLI examples each returned exit 0 and PASS, with provenance
UNKNOWN and acceptance unchanged. Protocol violations return UNKNOWN/exit 3.

Dictionary consistency correction: **173 passed, 0 failed, 0 skipped**; six new
regressions cover listed/unlisted null contradictions, valid absence/boolean
evidence, unavailable dictionaries and controller/CLI UNKNOWN exit 3. Synthetic
metadata/sample/full CLI inspections still returned PASS/exit 0 with provenance
UNKNOWN and acceptance unchanged.

Missing dictionary-evidence correction: **180 passed, 0 failed, 0 skipped**,
including real scalar/vector dictionary-identity checks. The resumed bounded
adversarial matrix passed 46 controller/CLI cases spanning dictionaries, branch
presence, objects, zero policies, read modes, indices, coverage and malformed fields.
