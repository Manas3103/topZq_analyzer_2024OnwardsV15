# CP-16G — Read-only campaign-aware Condor monitoring handoff

## Parent checkpoint and scope

Repository: `single_top_analyser` (`Manas3103/topZq_analyzer_2024OnwardsV15`).
Canonical checkout:
`/uscms_data/d3/msahoo/Project_tzq/CMSSW_13_3_3/src/single_top_analyser`.
Branch: `cleanup/run3-remove-legacy-run2`.

- Authoritative parent: [CP-16F](CP-16F.md), commit
  **`a59293a2c0cc22e69cdfa786b8f8469cc9f98b79`**.
- Batch 16F implementation: `b3ce2796acbdcc2c7282a68f02f57850c4fa4695`.
- Batch 16E/16E.1 implementation: `159fcbc8a53b24e956b2248daa9b8ef87fe87daf`.
- At preparation of this document, HEAD remains the CP-16F parent. Batch 16G is
  uncommitted; this document does not claim a new authoritative committed SHA.
  After a separately authorized commit, resolve the document revision with
  `git log -1 --format=%H -- docs/checkpoints/CP-16G.md` and record the exact SHA
  in the delivery report. No commit or push was performed during preparation.

Batch 16G implements observational campaign-aware monitoring of known processing
attempts using offline scheduler snapshots and an optional bounded read-only live
collector. It is a foundation for later output validation and protected retry
planning, not a resubmission system. **The monitor is not established as
production-ready. Live Condor compatibility and production-scale validation
remain unverified.**

CP-16F physics, processing, cleanup, regression and storage limitations remain in
force. This checkpoint does not expand validated 2024 C/H data coverage, establish
other-year support, or authorize execution. Phase-2 approved technical closure,
retained historical artifacts and protected physics behavior are unchanged.

## Files introduced or modified

| File | Change |
|---|---|
| `tools/campaign_monitor.py` | New monitor, strict input checks, reconciliation, JSON CLI and bounded collector. |
| `tests/test_campaign_monitor.py` | New 44-test synthetic/mocked monitoring suite. |
| `docs/campaign_monitoring.md` | New interface, identity, reconciliation and safety documentation. |
| `docs/campaign_templates/monitor/input.json` | New synthetic expected-plan/attempt mapping example. |
| `docs/campaign_templates/monitor/snapshot.json` | New synthetic idle-job snapshot. |
| `docs/campaign_metadata.md` | Append monitoring introduction/link; original 16E/16F contract text unchanged. |
| `docs/checkpoints/CP-16G.md` | This handoff and final verification record. |

No existing campaign schemas, acceptance records, production submit files, worker
scripts, `resubmit.sbng`, physics implementations or processing configs changed.
The external source-only audit remains relevant:
`/uscms_data/d3/msahoo/Project_tzq/regression_baseline/resubmission-audit-g35exydm/REPORT.md`.
Its stale-output and staging findings are not repaired by scheduler monitoring.

## Architecture and identity contracts

The independent CLI imports the existing `campaign.load_json` and `validate`
helpers. It uses the standard library plus existing jsonschema validation;
Python >=3.9 is required. It never imports ROOT or analysis configurations.
`reconcile(metadata, snapshot)` returns a report without changing its inputs.
No database, daemon, background loop, retry controller or new campaign-schema
requirement is introduced.

The version-1 `condor_monitor_input` envelope embeds existing `expected_chunks`
plans and full existing attempt manifests. Each entry supplies dataset/chunk IDs,
an optional canonical `ClusterId.ProcId`, and a nonempty mapping-evidence reference
when that job ID is declared. Campaign/workflow come from the attempt; dataset,
chunk and ordered inputs must agree with the expected plan. Attempt IDs are unique
within campaign/workflow, including across datasets/chunks. Duplicate declared
job mappings and duplicate/conflicting attempt identities are invalid inputs.

Attribute-only matching requires all of `TZQCampaignId`, `TZQWorkflow`,
`TZQDatasetId`, `TZQChunkId`, `TZQAttemptId`. A declared mapping can compensate for
missing attributes, but every supplied identity attribute must agree. No ROOT
basename, Arguments or filename heuristic establishes association. Mapping
references and explicit attributes are supplied evidence, not independent proof
of configuration execution or staging provenance.

The version-1 `condor_snapshot` envelope requires source, timezone-aware
observation time and queue/history ClassAd lists. Every ad requires nonnegative
integer ClusterId/ProcId. Available status/time/exit/hold fields are type checked;
missing optional fields remain unavailable. Original ads, source and list indices
are preserved. Unsupported envelope versions, missing required fields, duplicate
JSON keys and malformed inputs fail clearly.

## Global identity reconciliation and corrected defect

Initial implementation matched each observation independently, permitting one
scheduler job to identify different attempts in different rows without a conflict.
The targeted correction groups **all queue/history observations by job ID before
final attempt reconciliation**:

1. Form the union of known attempt candidates for each job.
2. Detect multiple candidate attempts, any row-level mapping contradiction, or
   different supplied values for an identity attribute across rows.
3. Mark every row in a conflicting group `job_identity_conflict=true`,
   `identity_conflict=true`, `resolved=false`.
4. Attach all rows to every affected known attempt via the group's candidate
   indices. Preserve raw ads and source/index, including contradictory rows that
   originally matched an unknown attempt.
5. Emit `job_identity_conflict` on each affected attempt, leave current observation
   unresolved, and count unresolved evidence so CLI exit 0 is impossible.

No queue/history, explicit/mapping, or first-row preference resolves identity
conflicts. Consistent identity observations do not receive this issue. Unknown or
unmatched identity remains unresolved rather than confirmed processing failure.
All conflicts involving no known candidate remain in unresolved observations.

## Scheduler state, exit evidence and source reconciliation

| JobStatus | State |
|---|---|
| 1 | idle |
| 2 | running |
| 3 | removed |
| 4 | completed |
| 5 | held |
| 6 | transferring_output |
| 7 | suspended |
| absent/other | unknown |

No matching observation produces unobserved, not failure. A completed current
observation with ExitBySignal true is classified signal; ExitBySignal false plus
ExitCode yields normal_zero or normal_nonzero. Otherwise exit evidence is unknown.
`missing_exit_evidence` is emitted for a selected completed observation with unknown
exit classification: completed state remains, reconciliation is unresolved.
Neither that issue nor any scheduler state triggers an automatic retry.

For an identity-consistent attempt, identical queue/history ads retain both and
use queue as current. Any whole-ad difference produces queue_history_conflict;
both observations remain and current state becomes unknown. This deliberately
includes benign extra-field differences. Duplicate observations within one source
and multiple job matches remain unresolved under the existing policy.

CLI exits: 0 no unresolved attempt/observation; 1 valid incomplete/conflicting
evidence; 2 invalid input, usage or collection failure. Exit 0 is not analysis
success, chunk completeness or output acceptance. An empty attempt list can report
zero observed attempts without establishing campaign coverage.

Reports contain identities, mapping evidence, original observations, current and
history evidence, issues and summary counts. The CLI embeds its scheduler snapshot
and live command arguments, plus monitor-input path and SHA256 of sorted JSON
content, not file bytes. `acceptance_changed=false` and
`output_acceptance=not_evaluated` explicitly separate monitoring from Batch 16F.

## Offline interface and bounded collection

Synthetic example (no real scheduler job or ROOT output):

```bash
PYTHONDONTWRITEBYTECODE=1 python3 tools/campaign_monitor.py \
  --input docs/campaign_templates/monitor/input.json \
  --snapshot docs/campaign_templates/monitor/snapshot.json
```

Optional `--live` replaces `--snapshot`. It requires 1..100 explicitly mapped IDs,
queries the local schedd through argument arrays and an exact numeric OR constraint,
and requests JSON from `condor_q` and `condor_history`. Each command has a timeout
(default 20 seconds, maximum 60); these are separate per-command bounds. History
uses `-match N+1` and `-scanlimit` (default 10000, maximum 100000). Query errors or
timeouts return exit 2 before report printing. No live collection was executed.
History scan/retention omissions cannot establish failure; attribute-only jobs
without mappings are not collected by live mode.

Reports go to stdout only, matching the existing campaign CLI convention. There
is no report-file/overwrite option, so atomic report-file replacement is not
applicable. External saving requires a new approved local path and a no-clobber
policy; shell redirection can overwrite files outside the monitor's control.

## Final static review and validation

Final review inspected the complete tracked diff and all untracked Batch 16G files,
including fixtures. The global invariant, preserved conflict evidence, affected
attempts, nonzero conflict exit, consistent state reconciliation and absence of
production/acceptance side effects passed focused inspection. **No new blocking
defect was identified.** Implementation was not edited during checkpoint preparation.

Inspected environment: CMSSW_13_3_3, el9_amd64_gcc12, Python 3.9.14,
ROOT 6.26/11 and GCC 12.3.1. Existing initialization was retained.

Exact final offline suite command:

```bash
PYTHONDONTWRITEBYTECODE=1 timeout 60s python3 -m unittest discover -s tests -v
```

Result: **99 tests passed, 0 failed**, exit 0 (36 metadata, 19 chunk acceptance,
44 monitoring). Tests use synthetic data and mocked subprocesses, never live
Condor or EOS. Eight targeted regressions cover cross-source/same-source identity
conflicts, explicit/mapping contradictions, overlapping campaign/chunk names,
unknown contradictory identity, consistent identity overlaps and state differences.
Assertions verify all affected attempts, original-ad retention and CLI exit 1.
The strengthened missing-exit test checks completed state, unknown exit evidence,
unresolved reconciliation and CLI exit 1. Existing safety tests check unchanged
input data/files and stdout-only reporting.

`git diff --check` passed. New-file whitespace was also checked using
`git diff --no-index --check /dev/null <new-file>` for each new file, including
this document. The previously recorded offline fixture CLI result was exit 0 with
one idle attempt; this checkpoint preparation reran the full suite, not that
separate fixture command. No build, analysis, ROOT processing or live query ran.

## Remaining limitations and deferred work

- **Live Condor compatibility and production-scale validation remain unverified.**
  Mocked command tests do not establish site-version behavior or actual queue health.
- Job identity is scoped to supplied local-schedd evidence. Cross-schedd aggregation
  and ID reuse require independently established provenance.
- A logical Condor job is not an execution-instance identifier. Eviction/restart
  executions and which execution produced particular bytes/logs are not tracked.
- Mapping evidence and snapshot provenance are caller supplied. No output integrity,
  configuration-runtime proof, ROOT schema or EOS receipt is verified.
- Full-ad comparison is conservative; history caps/retention can omit observations.
  Query timing is not an atomic scheduler snapshot. Unknown evidence remains unknown.
- Only explicitly listed attempts are monitored. Successful report exit does not
  establish all expected chunks were submitted or accepted.
- Batch 16F still requires independently supplied processing/output PASS evidence,
  explicit accepted-attempt choice and verified local bytes. Monitoring changes none
  of those contracts or acceptance records.

Batch **16H** output validation, **16I** worker/staging provenance (including the
execution-instance distinction), and **16J** protected retry planning/control remain
separate future work. No output verification adapter, worker routing/receipt change,
retry cap/backoff/grace policy, submission action, publication or cleanup is added.

## Safety and next-session handoff

No Condor mutation/query, EOS access/write/move/delete, production processing,
credential renewal, environment reinitialization, physics change, schema migration,
acceptance mutation, build-artifact cleanup or Git history/configuration change
occurred during checkpoint preparation. Existing legacy resubmission scripts remain
intact. Using Python without PYTHONDONTWRITEBYTECODE may create import caches; the
validated commands suppress these.

Prepared state is ready for a **separately authorized commit review**, not automatic
commit/push or operational deployment. The only new preparation change is this
checkpoint document; the existing six Batch 16G files remain uncommitted. No Batch
16H work was started. Before continuing, verify Git branch/HEAD/status, read
AGENTS.md, CP-16F, this document and docs/campaign_monitoring.md. Obtain explicit
current-task authorization before live validation, submission, processing, EOS
operations, commits or pushes. Preserve both intentional origin push destinations.
