# Phase C: manual technical acceptance index

This tool does not submit jobs, inspect ROOT content, copy ROOT files, merge,
retry, clean EOS, or grant physics acceptance. It validates frozen assignment
and saved Phase A/B/publication evidence, then independently hashes explicitly
provided local artifacts. Live EOS state remains NOT TESTED in this phase.

## Approved policy and activation boundary

Acceptance and supersession require an explicit user-authorized decision,
operator identity resolved from OS UID, timestamp, reason and authorization
reference. The reference is an audit record, not a cryptographic proof of user
approval. Access controls and a trusted coordinator remain necessary. No newest
execution selection or automatic supersession/deletion occurs.

Prospective store: `/uscms_data/d3/msahoo/Project_tzq/production_indexes/2024/`.
Persistent initialization/publication is deliberately blocked. Only owned,
non-group/world-writable, symlink-free temporary stores under `/tmp` are writable;
all indices/manifests carry `test_only=true`. This does not establish the
prospective filesystem's locking/rename/fsync behavior. One coordinator host
and UID are recorded; writes on another host/UID are rejected. Locks are advisory
and protect cooperating writers, not hostile processes with the same permissions.

## Explicit completeness scope

Use `scope(campaign, workflow, selector, scope_id, datasets=None,
campaign_record=...)`. The original campaign JSON bytes/hash are retained by
reference and verified; main campaign must explicitly request unlimited mode.
Selectors `mc`, `data`, `full` and `explicit` select declared datasets and all
their declared chunks. `explicit` requires dataset IDs. Expected chunks are
rebuilt from the frozen campaign; edited/missing/duplicate declarations fail.

COMPLETE means exactly all chunks in that scope, not all known 2024 data.
`campaign_complete_scope` means all **main-workflow** chunks declared by that
campaign are covered. It cannot establish that an externally incomplete campaign
contains every intended 2024 dataset. MC-only/data-only subset completeness never
claims whole-campaign coverage. Fakefactor acceptance is not supported until its
Phase A/B requirements are validated.

## Candidate contract

Explicit candidate JSON fields:

- `execution`, `publication`, `result`, `manifest`, `profile`, `attempt`: local
  paths to immutable metadata; no recursive discovery.
- `local_artifacts`: `output.root`, `processing.log`, `execution.json`,
  `publication.json`, referencing existing files. No ROOT copies are created.

Checks bind scope/campaign, submission, deterministic attempt, chunk/index,
ordered-input hash, configuration/profile/schema/source/payload hashes,
Phase A file ledger and independent Snapshot counters, Phase B dictionary/PCM,
mode/provenance/schema/read-coverage reports, execution success and publication
receipt/marker. Partial, missing/inconsistent, failed or unresolved evidence is
rejected. ROOT-required branches/types come from the frozen profile, not a new
universal schema guess. Independent local size/SHA256/Adler32 observations must
agree with receipt artifact/remote-checksum evidence using the same algorithm;
Adler32 is not substituted for SHA256. Local agreement does not prove current
remote presence. Metadata/artifact mutation is checked; unknown remote state is
explicit in temporary evidence. Production activation will require a separately
authorized bounded remote-observation policy/adapter, not a silent downgrade.

Historical successful outputs lacking Phase A/B evidence remain ineligible for
this strict contract. Tests compose synthetic metadata from saved validation
reports and reference a preserved ROOT output without copying it; these are not
new production executions or retroactive acceptance of historical campaigns.

## Revisions and concurrency

Layout: `STORE.json`, stable `LOCK`, `revisions/r-<content-hash>.json`, `HEAD.json`.
Each immutable revision embeds complete selection/decision history and scope.
Every dataset/chunk has at most one accepted identity. Multiple successful
candidates require manual choice. A replacement needs `supersede` and the exact
prior execution ID. Previous revisions and files are retained.

Updater verifies candidate, takes bounded `flock`, checks expected parent and
coordinator, rechecks candidate, writes/fsyncs an exclusive revision via local
hardlink publication, then fsyncs and atomically replaces HEAD on the same
filesystem. Parent revision IDs are content hashes; stale writes fail instead of
automatically merging or retrying. Concurrent tests cover same/different chunks.
Readers pin a revision/hash and verify content and history. Deterministic
regeneration holds for the same frozen scope, evidence observations and recorded
decisions; a new decision/observation timestamp intentionally changes identity.

An interruption before HEAD leaves an orphan revision with the old HEAD intact.
An error after replacement may mean commit already occurred: reread HEAD; never
blindly retry. Tests exercise those boundaries and lock timeout. Actual power-loss
and cross-host/shared-filesystem durability remain unverified. No rollback,
automatic orphan cleanup or EOS rename/CAS claim is made.

## CLI and downstream contract

Examples use only a temporary fixture store and explicit metadata files:

```bash
python3 tools/campaign_acceptance.py init --store /tmp/my-test-index --scope /tmp/scope.json
python3 tools/campaign_acceptance.py decide --store /tmp/my-test-index \
  --candidate /tmp/candidate.json --expected-parent NONE --action accept \
  --reason 'User-reviewed technical validation' --authorization-reference 'approved-record-reference' \
  --authorize-user-decision
python3 tools/campaign_acceptance.py validate --store /tmp/my-test-index \
  --revision /tmp/my-test-index/revisions/r-HASH.json
python3 tools/campaign_acceptance.py manifest --store /tmp/my-test-index \
  --revision /tmp/my-test-index/revisions/r-HASH.json
python3 tools/campaign_acceptance.py validate-manifest --store /tmp/my-test-index \
  --revision /tmp/my-test-index/revisions/r-HASH.json --manifest /tmp/plot-inputs.json
```

Commands print JSON to stdout, exit0 on valid operation and1 on failure; no report
file is silently overwritten. Manifest generation requires COMPLETE. Validation
rechecks metadata/artifacts and requires the selected revision to be current HEAD;
superseded/non-authoritative revisions fail current-production validation. Older
revisions remain historical records, not automatically eligible plot inputs.
Plot-manifest comparison detects edits, duplicate/missing inputs and stale pins.

Schemas: `accepted_scope.schema.json`, `accepted_revision.schema.json`,
`accepted_plot_inputs.schema.json`. Python validation additionally enforces
semantic links, completeness, hashes and decision replay. Technical acceptance
is `ACCEPTED`; physics acceptance always `NOT_EVALUATED`. Dataset-wide generator
normalization and data-stream deduplication are separate later-stage requirements.

Plotting code is unchanged. Future adapters must validate a current COMPLETE
revision pin, reject test-only/unresolved manifests for production, and use only
its explicit ROOT locators. Existing scripts accepting arbitrary files cannot be
claimed protected by this new index alone. No persistent index exists yet.

Representative sparse campaigns require `selector=explicit`; `mc`, `data` and
`full` selectors are refused for them. Their `campaign_complete_scope` is always
false, even when every explicitly approved representative chunk is present.
COMPLETE may describe that revision-pinned explicit subset only. The frozen
campaign records `representative_subset.full_2024_complete=false` and its exact
approval document/hash, so this cannot establish completeness of full 2024.
