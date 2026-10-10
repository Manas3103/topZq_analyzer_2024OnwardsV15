# Batch 16G: read-only Condor monitoring

`tools/campaign_monitor.py` uses the existing campaign validator and Python standard
library. It does not import ROOT/configuration modules, inspect outputs, submit,
release, remove or retry jobs, write files, or modify acceptance. Existing production
and resubmission scripts remain intact. Python 3.9+ and existing jsonschema support
are required. The new envelopes are independently versioned; existing campaign
schema contracts are unchanged.

Offline example (synthetic identity, not a production job):

```bash
PYTHONDONTWRITEBYTECODE=1 python3 tools/campaign_monitor.py \
  --input docs/campaign_templates/monitor/input.json \
  --snapshot docs/campaign_templates/monitor/snapshot.json
```

Python interface: `reconcile(metadata, snapshot)` returns a JSON-compatible report
without mutating inputs. CLI exit codes: 0 all attempt states observed without
reconciliation issues; 1 valid but incomplete/conflicting evidence; 2 invalid input,
failed collection or usage. Exit 0 is not analysis success or output acceptance.
A completed job with unavailable exit diagnostics has `exit=unknown` and an
unresolved `missing_exit_evidence` issue; its completed scheduler state is retained.

## Input identity and evidence

`condor_monitor_input` requires integer `schema_version=1`, `expected` and `attempts`
lists. Each expected item is a complete existing `expected_chunks` manifest, including
its dataset and submission-plan path/hash. Each attempt entry requires:

- `dataset_id`, `chunk_id`, and a complete existing `attempt` manifest.
- `job_id`: canonical `ClusterId.ProcId`, or null.
- `mapping_evidence`: nonempty submission/receipt evidence reference for a mapped
  job, or null when no job is mapped.

Dataset, chunk, workflow, campaign and ordered inputs must agree with the expected
plan. Attempt IDs are unique within campaign/workflow, including across datasets.
A scheduler job cannot be mapped to multiple attempts. Retries of a chunk have
separate attempt IDs. Input duplicates/conflicts are invalid, not auto-repaired.
Mapping evidence is a caller declaration, not independently certified provenance.

Explicit scheduler attributes use `TZQCampaignId`, `TZQWorkflow`, `TZQDatasetId`,
`TZQChunkId`, `TZQAttemptId`. All five are required for attribute-only matching.
An explicit mapping can supply missing attributes, but every supplied attribute
must agree; mapping and complete attributes cannot point to different attempts.
No association uses output basenames, Arguments, timestamps or filename heuristics.
Legacy submit files do not yet populate these attributes. Their log convention is
`logs/ClusterId.Process.out/.err` and `logs/ClusterId.log`; this batch does not parse
these logs. Mapping a legacy job requires separately reviewed submission evidence.
Cluster/proc identity is scoped to the supplied local schedd evidence; cross-schedd
aggregation and scheduler-ID reuse require independent provenance review.

`condor_snapshot` requires integer `schema_version=1`, a nonempty `source`, timezone
aware `observed_at`, and `queue`/`history` lists of JSON ClassAds. Empty lists mean no
observations returned, not failed jobs. Every ad requires nonnegative integer
ClusterId/ProcId. JobStatus may be absent, producing unknown. Available status/time,
exit and hold fields are type checked. ExitBySignal must be boolean. All original
ads (including extra provenance fields) are retained. Envelope unknown fields,
unsupported versions, malformed JSON and duplicate JSON keys are rejected.

## Classification and reconciliation

| JobStatus | State |
|---|---|
| 1 | idle |
| 2 | running |
| 3 | removed |
| 4 | completed |
| 5 | held |
| 6 | transferring_output |
| 7 | suspended |
| missing/other | unknown |

No matching observation yields unobserved, never inferred failure. Completed jobs
are classified as signal termination when ExitBySignal is true; normal_zero or
normal_nonzero requires ExitBySignal false and an ExitCode. Otherwise exit is
unknown. Removed/held jobs are not automatically retry candidates. Evictions or
restarts cannot be reconstructed from a single snapshot; a running observation
remains running, without inventing terminal failure.

For each attempt, report current, history and all matching observations, job IDs,
matching source, mapping evidence and issues. Before final attempt associations,
all rows are grouped by ClusterId.ProcId across both sources. A job identifying
different known attempts, contradicting a declared mapping, or carrying conflicting
values for a supplied identity attribute has `job_identity_conflict=true` on every
row. Every affected known attempt receives `job_identity_conflict`, unresolved
reconciliation and all original rows for that job; observation `candidates` lists
the affected attempt indices. Raw ads and their source/index remain intact, even
when a conflicting row identifies an unknown attempt. No source or matching method
wins an identity conflict, and CLI exit 0 is prevented. Consistent identities do
not trigger this issue; existing duplicate/state reconciliation still applies.
Queue takes precedence only when
there is no contradictory history. Identical queue/history ads retain both and use
queue as current; any differing ads produce queue_history_conflict and unknown
current state. This conservative policy intentionally flags even extra-field
changes for review. Duplicate ads within either source and multiple job matches
remain unresolved; no last-row/newest-attempt selection occurs. Original list
indices preserve evidence order. Unmatched/identity-conflicting ads appear in
unresolved_observations. Summary counts cover attempt states and unresolved cases.

Reports embed the scheduler snapshot and commands (live mode) plus the monitor
input path and SHA256 of canonical sorted JSON content (not file bytes). They
record `acceptance_changed=false`, `output_acceptance=not_evaluated`. Attempt
statuses in source manifests are never rewritten to scheduler states.

## Bounded live collection and report safety

Optional `--live` replaces `--snapshot`. It queries only 1..100 explicitly mapped
job IDs on the local schedd using argument arrays, `-json` and an exact numeric
constraint. Each command has `--timeout` (default 20 seconds, maximum 60).
History has `-match N+1` and `--scanlimit` (default 10000, maximum 100000).
A scan cap or history retention may omit an old job: absence remains unobserved.
Jobs known only by attributes are not queried in live mode; use reviewed snapshots
or explicit mappings. Query errors/timeouts return exit 2 with no report, so they
cannot masquerade as an empty successful observation. No live query was required
by unit tests; site-version compatibility remains unverified.

Read-only command options follow the official
[condor_q documentation](https://htcondor.readthedocs.io/en/main/man-pages/condor_q.html)
and [condor_history documentation](https://htcondor.readthedocs.io/en/24.x/man-pages/condor_history.html).

Reports go to stdout, matching Batch 16E/16F. There is deliberately no output-file
or overwrite option, so the monitor cannot overwrite source manifests, acceptance
records, snapshots or existing reports. Atomic file replacement is consequently
not applicable. If saving stdout externally, choose a new approved local path and
use shell no-clobber (`set -C`); ordinary `>` redirects can overwrite files outside
the monitor's control. Do not redirect to EOS without explicit authorization.

## Validation boundaries and later batches

Synthetic tests exercise states, exits, absent history, retries, campaign identity,
partial/conflicting attributes, duplicate evidence, malformed/versioned inputs,
stdout-only file safety and input immutability. Mocked subprocess tests verify
bounded query construction and timeout propagation; they do not verify live
scheduler compatibility, real queue health, ROOT integrity or EOS receipts.

Validation command: `PYTHONDONTWRITEBYTECODE=1 timeout 60s python3 -m unittest
discover -s tests -v`. After the targeted global-identity correction, Batch 16G
result: **99 passed, 0 failed** (36 metadata, 19 chunk acceptance, 44 monitoring).
Eight added tests cover cross-source/same-source identity conflicts, mappings,
campaign overlap, unknown conflicting identity and consistent identity/state cases;
conflict tests assert evidence retention and CLI exit 1. The missing-exit test now
asserts completed state, unknown exit, unresolved reconciliation and CLI exit 1.
The previously tested documented offline CLI example also
returned exit 0 with one idle attempt; no live scheduler or EOS access occurred.

Batch 16F acceptance remains independent: an explicitly accepted, validated attempt
with processing/output PASS and verified local bytes is still required. Scheduler
completion/exit zero cannot supply those validations. Batch 16H output validation,
16I worker/staging provenance and 16J protected retry planning/control are deferred.
No retry decisions, grace-period prescriptions, output publication, new mandatory
physics provenance fields, campaign migrations or staging changes are introduced.
