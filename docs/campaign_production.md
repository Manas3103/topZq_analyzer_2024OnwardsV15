# Batch 16I — Fresh 2024 campaign production

This is new infrastructure alongside the unchanged legacy wrapper/submit files.
It prepares bundles locally; preparation never calls DAS, Condor or EOS. Actual
worker execution invokes the authoritative `processnanoaod_v.py` and can write
EOS only when separately authorized. No live deployment is implied by offline
success. The old-output audits remain deferred in
`checkpoints/CP-2024-DEFERRED-VALIDATION.md`.

## Identities and versioned contracts

Seven independent version-1 contracts live under `schemas/production/`; illustrative
JSON examples are under `campaign_templates/production/`. They do not change
Batch 16E/F/G schemas or acceptance. JSON-schema validation uses the repository's
existing `jsonschema` dependency; no dependency is installed automatically.

- Campaign: 2024, explicit source identity, endpoint, independent datasets/configs,
  validation profiles, processing timeout, and publication policy. Preparation
  freezes `processing_files` hashes into `campaign.json`. Reusing that frozen
  campaign with changed payloads is rejected. A human campaign ID alone is not a
  checksum; callers must use a new campaign for changed processing configuration.
- Dataset: exact DAS version, sample, datatype, year, workflow, data era/stream
  (null for MC), configuration/profile paths and processing-schema label.
- Chunk: contiguous numeric index, safe ID and explicitly ordered original
  NanoAOD URLs. Only `root://` /store/data or /store/mc ROOT files are permitted;
  mounted EOS, output namespaces, discovery/globs and mixed type declarations
  are rejected. URL spelling does not independently prove the file's origin.
- Submission: frozen campaign hash, file/config/payload hashes and exact jobs.
- Attempt: deterministically derived from campaign/submission/workflow/dataset/
  chunk, with a full SHA256 suffix. Distinct chunks never share one submission
  ID as their attempt ID. New submissions get new logical attempt identities.
- Execution: a UUID on each wrapper invocation, even for a restarted identical
  ClusterId.ProcId. Records runtime, processing status, actual schema fingerprint,
  input declarations and hashes, plus local ROOT evidence. Configured schema IDs
  are caller labels; observed branch-type fingerprints are separate evidence.
- Publication: verified artifact locators, local SHA256/Adler32, algorithm-specific
  remote checksum, command observations and local validation evidence.

Execution/publication acceptance is always `not_evaluated`. Nothing updates
accepted manifests or chooses a winning attempt. Existing Batch 16F stays local-only.
Source commit/source_sha256 are caller declarations; actual transferred file
hashes are checked independently. This is integrity checking, not signed code
or an adversarial worker sandbox. Freeze and retain the campaign/bundle locally
before submission; remote metadata catalogs are not installed automatically. Local registration is now available as described below.

## Local preparation

Provide an explicitly reviewed campaign JSON, original NanoAOD URLs and runtime
payload paths. No original dataset list is silently omitted: the retained
`DYto2Tau-2Jets_M2L10-50` appears in downstream preferences, and a real production
campaign must declare its exact original dataset/chunks. No old merged input is
used as a fresh production source.

```bash
python3 tools/campaign_production.py \
  --campaign /approved/fresh-campaign.json --submission-id pilot-001 \
  --source-root /approved/source-checkout --destination /new/local/bundle \
  --payload src/GenParticleHelper.h
```

The example payload options are illustrative, NOT a complete production bundle.
The caller must explicitly include matching PCM/headers and all runtime resources
required by its built library. Processor/library/config/profile, the active root-level PCM bytes, mandatory framework headers and discovered local quoted-include dependencies, and literal relative correction files are frozen automatically. Only the known rootdict_rdict.pcm link to build/dict/rootdict_rdict.pcm is dereferenced; other symlink payloads are rejected. Executable/read permission bits are preserved, with special permission bits stripped. Dictionary portability still requires a live worker test. No build is performed.
Framework include/library lookup uses bundle root/src and bundle root respectively.
Preparation validates literal 2024 configuration/datatype/analyser/tree contracts
and refuses `nrootfiles` truncation. The existing processor's DAS check for a
filelist is unchanged; DAS availability remains a runtime requirement/limitation.

Preparation refuses existing destinations and symlink/escaping payload files.
Identical declarations/timestamps/payloads produce identical manifests/filelists.
If copying/finalization fails, any partial claimed destination is retained and
cannot be reused implicitly. A local bundle hash does not discover conflicting
campaign IDs in another host/catalog: register the frozen campaign before pilot.

## Condor integration

`job_campaign_main.submit` and `job_campaign_fakefactor.submit` are new. All
existing submit files are unchanged. Specify these macros explicitly:
`bundle_path`, `bundle_name` (basename of transferred directory), `manifest_sha256`,
`campaign_id`, `submission_id`, `proxy_path` (existing approved proxy), and `log_dir` (new local directory).
For example, AFTER separate submission approval:

```bash
condor_submit job_campaign_main.submit \
  -append 'bundle_path=/approved/bundle' -append 'bundle_name=bundle' \
  -append 'manifest_sha256=<printed-preparation-hash>' \
  -append 'campaign_id=<reviewed-id>' -append 'submission_id=pilot-001' \
  -append 'proxy_path=/approved/existing-proxy' \
  -append 'log_dir=/new/local/condor-logs'
```

No tool here executes that command. Transfer input is the explicit prepared bundle,
not `.`. Queue rows contain dataset/chunk/attempt identities. Existing monitor
ClassAds TZQCampaignId/TZQWorkflow/TZQDatasetId/TZQChunkId/TZQAttemptId are retained;
TZQSubmissionId is additional. ClusterId/ProcId are passed explicitly. Diagnostics
are returned from `campaign_executions` on exit; no blanket worker cleanup occurs.

The new shell wrapper initializes a matching CMSSW_13_3_3 gcc12 runtime only on
an actual Condor worker. Interactive execution requires matching initialized
runtime. It never changes the user's current shell. A bootstrap UUID record is
created before environment setup, so setup failures retain an execution identity;
Condor stdout/stderr contain setup failure diagnostics. The Python worker verifies
manifest and every payload/filelist hash before and after processing.

## Validation, staging and visibility

The worker calls the unchanged local ROOT-validation interface, with its strict
isolated reader protocol. Each dataset explicitly chooses a profile: no universal
physics branch list is invented. Zero entries follow that profile. Missing ROOT,
timeout, dictionary-limited required content or invalid output cannot be published.
Metadata-only PASS proves only the requested structure/opening checks; it does
not prove full content readability. The worker never changes event weights.

Execution destinations are exactly:

```text
/store/user/msahoo/tzq/2024/campaigns/<campaign>/
  <main|fakefactor>/attempts/<submission>/datasets/<dataset>/chunks/<chunk>/
    executions/<uuid>/output.root
                      processing.log
                      execution.json
                      publication.json
```

Stages/revisions/plots/validation paths remain reserved for later tools; Batch 16I
neither merges outputs nor implements stage processing or acceptance selection.

Staging first requires confirmed absence (sole explicit NotFound); auth/network/
timeout/ambiguous failures stop. It creates the execution directory and uploads
without force options. Existing destinations are refused. Endpoint policy must
explicitly set `no_clobber_confirmed=true` only after a separately authorized
capability test. A stat-then-copy check is NOT an exclusive distributed lock.
Default xrdcp/server no-overwrite behavior and concurrency must be proven live.

Every artifact is checked for online/readable regular-file flags, size and a
required matching server checksum. Adler32 is never treated as SHA256. Local
SHA256 is recorded separately; server checksums do not prove campaign provenance.
Local output metadata is bound to validation and mutation during transfer stops
publication. Execution identity must match the exact destination. The receipt
is uploaded LAST and itself verified. Any error yields worker failure and keeps
local diagnostics; partial remote files are never cleaned automatically.

There is no rename or multi-file atomicity assumption. Consumers must require a
complete valid receipt, matching identities and verified referenced artifacts;
file existence or a partially uploaded receipt is insufficient. A failed final
receipt verification may leave remote files, even a readable marker, and must
remain unresolved against worker result/independent observations. No consumer
acceptance selector is provided in this batch. `execution.json` is the immutable
validated-processing snapshot; `result.json` separately records later staging
success/failure; `execution.failure.json` is a versioned failure record with unavailable results explicit, and is never published as success. `publication.json` can exist locally before a failed upload and
is not itself proof of remote success.

All subprocesses use argument arrays, no shell, POSIX sessions and group cleanup.
Capture is bounded during streaming (256 KiB); processing logs have a 128 MiB cap.
Processing has campaign wall limit (1..86400 s), ROOT validation 30 s, staging
per-command (1..3600 s) and overall (1..14400 s) budgets. Cleanup adds up to two
seconds. Escaped sessions/uninterruptible kernel operations remain limitations;
XRootD internal reconnects are not controller retries. No credential renewal.

## Downstream decisions and offline gates

`campaign_templates/production/downstream_preferences.json` keeps normalization
outside processing: 574.2 tth_non2b cross section with unchanged denominator,
12 fb^-1 for current LIMITED normalized analysis, and four temporary exclusions.
The authoritative histogram JSON and conflicting historical JSON are unchanged.
Historical data recipe is five streams combined per era then deduplicated, with
validation deferred. No histogram/dedup work runs here. The central mapping
preserves known active memberships; ttwz is explicitly unassigned rather than
silently classified or omitted. Its assignment requires approval.

Tests use synthetic temporary bundles, mocked processing, in-memory endpoints,
local subprocesses and tiny local ROOT fixtures. Never live EOS, Condor or actual
NanoAOD processing. Run:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests -p test_campaign_production.py
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests
bash -n runjob_campaign_2024.sh
git diff --check
```

Live worker packaging, proxy delegation, endpoint no-clobber semantics, checksum
support and publication visibility remain UNVERIFIED. Offline tests do not make
this production-ready. A reviewed minimal pilot needs separate build/processing/
submission/EOS-write authorization. Batch 16J retry scheduling and accepted-output
selection are deliberately not implemented. The six Batch 16H-B files and the
deferred-validation checkpoint remain unchanged.

## Recorded offline validation

Final implementation run in the initialized CMSSW_13_3_3 / Python 3.9.14 /
ROOT 6.26/11 environment:

- Batch 16I suite: 67 passed, 0 failed, 0 skipped (9.225 seconds).
- Complete suite: 328 passed, 0 failed, 0 skipped (38.569 seconds).
- Three new integration cases use tiny local ROOT files; one passes the real
  local validator and staging helper with an in-memory mocked endpoint. Other
  execution fixtures never invoke the real NanoAOD processor.
- Preparation CLI and shell-wrapper failure path tested offline.
- `bash -n runjob_campaign_2024.sh`, `git diff --check`, and whitespace checks
  of all 25 new files passed. Seven example contracts validate.
- All six pre-existing Batch 16H-B file SHA256 values were unchanged.

Failed staging observations are retained in new local `staging-<uuid>.json`
files, including command outcomes and bounded diagnostics. No real EOS access,
write, Condor submission, production event processing, build, commit or push was
performed. Metadata-only checks do not establish production-scale or physics
validation.

## Batch 16I-Fix: optional pilot scope and local registration

`processnanoaod_v.py INPUT OUTPUT CONFIG [--max-input-events N]` preserves the
existing three positional arguments. Omission leaves the original analysis graph,
output tree names/objects and full-chain generator bookkeeping unchanged.
For an explicit positive N, the shared framework sets `_rlm.Range(0,N)` before
setParams/corrections/objects/selections and books a preselection input count.
`_rd` remains unrestricted; both analysers' `_rd.Sum("genWeight")` calls remain
unchanged. Consequently limited MC pilots STILL read the full input for generator
bookkeeping; N is not a limit on all I/O. Implicit MT is rejected for limited mode
rather than silently changing runtime settings. Limited analyser instances cannot
be reused through setTree. No physics formula or selection was changed.

A campaign may declare `max_input_events`; null/omission means unlimited. Limited
profiles must target `pilot_outputTree_00000` and require
`TZQProcessingProvenance` as a TNamed. The limit is passed from the frozen campaign
to the processor, not as an untracked worker override. Limited files contain a
JSON TNamed and exclusive local input-scope sidecar, with total entries, requested
N, actual preselection analysis entries, full-chain MC-bookkeeping scope, partial
status and `full_production_eligible=false`. Even N >= total stays pilot-only.
Execution and publication records propagate this scope. Unlimited worker counts
come from the existing processor total-entry diagnostic; missing/ambiguous evidence
fails instead of fabricating counts. Selected entries cannot exceed input scope.

The pilot tree prefix deliberately prevents standard main-tree histogram/validation
profiles from consuming a limited file as a complete chunk. Complete-chunk consumers
must additionally call `campaign_event_limit.require_complete` on validated scope;
it never grants acceptance. Publication remains separate from acceptance. Generic
manual ROOT tools can bypass metadata and combine objects; they are not made safe
or authorized by this feature. No retry/accepted-output selector is implemented.
A future 16J selector must enforce this eligibility contract. The authoritative
histogram JSON and all Batch 16F/G acceptance code remain unchanged.

The changed C++ interface requires a matching library/dictionary/PCM rebuild before
any limited pilot. No build or actual analyser execution was performed in this fix.
Tests of processor wiring use mocks; tests of Range/bookkeeping use tiny synthetic
ROOT graphs. They do not prove compiled analyser equivalence or dictionary portability.
An authorized unlimited regression against the preserved baseline remains required.

Local registration uses a trusted local POSIX directory, exclusive no-follow file
creation, fsync, exact byte identity and immutable submission linkage:

```bash
python3 tools/campaign_catalog.py --bundle /approved/bundle \
  --manifest-sha256 <preparation-hash> --local-catalog /existing/local/catalog
```

A new campaign gets campaign.json, <submission>.manifest.json and
<submission>.registration.json. Identical content is idempotent; different or
incomplete content, symlinks and remote/mounted-EOS paths are rejected without
overwrite. Interrupted registration can leave incomplete records and is blocked
until separately reviewed; no automatic repair or cleanup. The local receipt
explicitly says remote_registered=false and records pilot eligibility. Its contract
is schemas/production/catalog_registration.schema.json. No remote registration is
implemented: endpoint exclusive-create/no-clobber semantics remain unverified.
This local catalog is not an EOS registration, production approval or acceptance.

The six Batch 16H-B files, legacy wrapper/submit files, deferred-validation
checkpoint and histogram JSON remain preserved. No EOS capability test, submission,
production processing, generated dictionary edit or Git history change occurred.

Final Batch 16I-Fix offline results: 95 production tests passed (67 existing plus
28 new fix regressions), 0 failed, 0 skipped; 356 complete repository tests passed,
0 failed, 0 skipped (39.083 seconds). Production schemas/seven examples, shell
syntax and diff/new-file whitespace checks passed. Mocks cover both analyser
interfaces; synthetic ROOT verifies Range counts alongside full-chain sums and
pilot marker/default-profile rejection. No C++ build or compiled-analyser event
regression was executed. Existing invalid-escape DeprecationWarnings surfaced
while AST-parsing historical processor literals; no related logic was changed.

### Explicit representative sparse subsets

Full production still requires contiguous chunk indices from zero. An optional
`representative_subset` campaign declaration permits original sparse indices only
for `purpose=representative_validation` or `test`, with `mode=sparse_subset` and
`full_2024_complete=false`. It records the approval reference, source-inventory
SHA256, approved dataset/sample/DAS/workflow/chunk/index assignments and each
original ordered filelist SHA256. Declared assignments must match exactly; no
renumbering, discovery or missing-chunk synthesis occurs.

Preparation additionally requires `--approved-assignments PATH` (Python keyword
`approved_assignments`). This explicit JSON document contains the declaration
without `approval_sha256`; the campaign records its exact byte hash. The unchanged
approval bytes are frozen as `representative_assignments.json` in the manifest
and rechecked during worker bundle verification. A self-declared approval reference
is an audit record, not a cryptographic authorization: the trusted caller must
supply the user-approved document. Existing default campaigns require no new field
or argument. An approval file without sparse mode is rejected.

The worker's chunk IDs, indices, attempt derivation and processing semantics do not
change. Phase C permits only an explicit subset scope for these campaigns and
keeps `campaign_complete_scope=false`. COMPLETE can describe only that explicitly
selected representative scope, never full 2024. Persistent acceptance remains
inactive and test-only; this option does not select or accept executions.
