# Campaign metadata foundation (Batch 16E)

This CLI plans metadata only. It never creates campaign directories, imports
ROOT or configuration modules, contacts DAS/EOS, submits jobs, or changes the
shell environment. All results go to stdout. Saving stdout is a separate user
operation; do not redirect to EOS without explicit authorization.

Requires Python 3.9+ and `jsonschema` with Draft 2020-12 support (available in the
inspected CMSSW environment). No automatic package installation. Schema references
resolve locally; validation does not fetch remote schemas.

```bash
python3 tools/campaign.py --help
python3 tools/campaign.py config jobconfiganalysis_2024.py
python3 tools/campaign.py plan --id 2024-CH-example --eras C,H
python3 tools/campaign.py validate docs/campaign_templates/campaign.json
python3 tools/campaign.py summary docs/campaign_templates/campaign.json
python3 tools/campaign.py tree docs/campaign_templates/campaign.json
PYTHONDONTWRITEBYTECODE=1 timeout 60s python3 -m unittest discover -s tests -v
```

A caller may save a draft to an approved local path, then validate and preview it.
Use repeated `--dataset <dataset.json>` to attach explicitly reviewed sample
manifests. Dataset templates are synthetic placeholders, not production samples.
No dataset-list parser or automatic sample discovery is implemented: preserve the
list path/hash in `sample_list`, exact dataset version/path, datatype, era, ordered
inputs and chunk indices. No automatic DAS lookup or default sample selection.
Optional `--plot-commit <full-40-character-SHA>` is declared, not remotely verified.

## Files and contracts

Four schemas under `schemas/campaign/` and matching examples under
`docs/campaign_templates/` cover campaign, dataset, attempt and stage/artifact.
Examples are structurally valid drafts. `UNRESOLVED`, zero hashes and example URLs
are placeholders and must be replaced before operational use. Schema validity is
not completion, provenance verification, physics approval, or execution permission.

Evidence records contain `state`, `value`, `evidence`, `reason`:

- `declared`: literal configuration or user statement; not proof of runtime use.
- `verified`: caller supplies evidence; the metadata validator does not execute it.
- `unknown` / `unavailable`: null value and a required explanation.

`plan` captures current full Git SHA, remote URL, branch and dirty-path listing;
new tooling itself therefore makes the draft dirty. Dirty paths are not a stored
patch/archive: reproducible dirty campaigns require separately captured changes.
No source snapshot or backup is created. Configuration dictionaries are parsed with
`ast.literal_eval`, not imported. Nonliteral and repeated assignments are rejected.
Configuration checks cover year, mode and tree names, not physics correctness,
complete correction-interface compatibility or payload validity.

Config snapshots include all declared keys/flags and file hashes. Payload identities
record local content hashes where available, without opening ROOT files. Relative
payload paths resolve against this repository; a different runtime CWD needs its
own effective-path evidence. Source references cover selection, trigger, FF and
Linkdef implementation hashes. Actual HLT expressions, seeds/threading, applied
corrections, dictionaries and mapped libraries must be recorded after separately
authorized processing. Shell environment values are declared, not verified;
missing ROOT/correctionlib versions stay unknown. No runtime commands are executed.

The intentionally simplified histogram formula is documented as declared from the
current recipe; luminosity, units, cross sections and denominator coverage remain
unknown until supplied with provenance. Its recipe/wrapper hashes are recorded.
Main and FF normalization records remain independent; FF measurement weights must
be recorded separately from generic histogram normalization. No weights change.

Stage `artifacts[].metadata` evidence records can carry size/checksum, tree/schema,
entries/order, weights/cutflows, histogram axes/bins/flow/errors/integrals, duplicate
counts, excluded columns, paths and timestamps. Attempt records hold ordered inputs,
command and exit status; stage records link parents. Status and approval are separate
labels, not a validated state-transition engine. Unknown metadata is permitted for
planning. Missing production coverage must never be interpreted as a PASS.

Validation rejects unknown fields, malformed IDs/timestamps, duplicate JSON keys,
duplicate dataset/chunk/input identities, inconsistent chunk ordering and data eras
outside scope. Adding eras only declares scope; it does not validate that era's physics.

## Integration boundaries and next stages

Existing production/merge/dedup/histogram scripts are untouched. Current EOS names
can be referenced in metadata; nothing is migrated, copied or backed up. Planning
previews separate main/FF attempts and stages without asserting EOS publication
semantics. Safe merge execution, first-wins dedup validation, complete runtime capture,
accepted-retry selection, immutable publishing, Condor routing, histogram/plot reports
and archival tooling require independent implementation and authorization.

Historical ROOT reflection limitations and the conditional OSSF4L_info exclusion
remain unchanged. Preserve required scalar/derived branches and validate duplicate
content before approving outputs; this metadata foundation cannot prove either.

## External generator-weight calculations (Batch 16E.1)

Each dataset now requires `generator_weights`: `sumGenWeight`, `source_script`,
`input_dataset`, `input_filelist`, `calculation_report`, `coverage`, and
`provenance`. Values may remain explicitly unknown. Script/filelist/report
identities carry paths and SHA256; dataset identity must match the exact sample.
A verified sum requires verified linked identities, checksums, coverage and
provenance. This checks metadata consistency, not the truth of external evidence:
the CLI never executes the calculation or reads its input ROOT files. Coverage
should record expected/read/skipped files, ordered-list hash, Runs versus Events
branch, precision and calculation command/version. Signed/zero generator sums are
recordable; no normalization prescription is chosen here.

Dataset normalization has explicit formula, luminosity/value/unit, cross section/
unit and data policy fields. Unknown units stay unknown. To use externally computed
values, edit a reviewed local dataset manifest, then supply it with `plan --dataset`.
No text-table ingestion or automatic substitution into production weights occurs.

## Chunk retention metadata

Dataset and stage manifests require `chunk_retention`, containing accepted
(dataset, chunk, attempt, artifact path/checksum) identities, merge and data-dedup
validation evidence, retention start, suggested period (default template: 7 days),
cleanup eligibility and timestamped audit trail. The period starts at the explicitly
recorded retention-start time, not file mtime. It is a suggestion, not an automatic
expiration. Repeated accepted attempts for the same chunk are rejected; dataset
records must reference their defined chunks.

`deletion_enabled` is constrained to **false**. There are no delete/cleanup commands.
`eligible_for_review` requires verified PASS evidence, accepted chunks, a verified
start, elapsed suggested period and an audit entry. Dataset MC does not require
data dedup validation; stage-level eligibility conservatively requires both checks
because it has no sample datatype. Eligibility does not authorize deletion or prove
recoverability, approval, backup completeness or artifact integrity. Historical
external actions can be documented in the audit trail; the tool never performs them.

Planning hashes only payloads resolved inside the repository; external paths and
symlinks outside it are recorded uninspected. Payload references are not dereferenced
by manifest validation. Git inspection has a bounded timeout. No network lookup,
EOS access or environment reinitialization is part of these commands.

## Read-only chunk acceptance (Batch 16F)

`chunks` uses an explicit expected plan and inventory, not filename discovery:

```bash
python3 tools/campaign.py validate /tmp/expected.json
python3 tools/campaign.py chunks --expected /tmp/expected.json \
  --inventory /tmp/inventory.json --local-root /tmp/local-chunks --format json
# Repeat with --format summary or --format accepted; all output goes to stdout.
```

Four additional schemas define `expected_chunks`, `chunk_inventory`,
`chunk_completeness_report`, and `accepted_merge_inputs`. Existing commands and
manifests remain compatible. Expected plans embed the existing dataset manifest
and carry a `campaign_id` and production submission/filelist `plan_source`
(path/SHA256). Chunk IDs, indices and ordered inputs come from that reviewed plan,
not inferred ROOT filenames. Duplicate expected indices are invalid usage.

Inventory carries campaign, dataset and workflow identities and `attempts` entries:
`chunk_id`, `index`, `dataset_id`, `accepted`, the existing full `attempt` manifest,
and `output` (`path`, `size_bytes`, `sha256`, `validation`). Output validation has
`state`, `result`, `evidence`; acceptance requires verified PASS. The attempt must
be validated, exit 0, and have verified processing PASS with evidence. Exactly one
accepted attempt is required per expected chunk. Rejected/failed retries remain
in report observations and do not block a valid accepted retry. Unexpected chunk
identities, wrong input ordering or mismatched campaign/sample/workflow block PASS.
The caller explicitly selects accepted attempts; there is no newest-file policy.

Outputs must be relative paths inside an explicit existing local root. Absolute,
traversal, network URLs, resolved symlink escapes and `/eos` roots are rejected.
Accepted files must be regular, nonempty and match declared size/SHA256. Files are
hashed in 1 MiB blocks; no ROOT is imported. ROOT integrity, schema, tree and cutflow
validation must be performed separately and linked as evidence. Nonzero file size
and a checksum alone cannot prove successful processing. Arbitrary mounts cannot
be identified reliably; supply only approved local storage, not an EOS mount alias.

Exit codes: 0 PASS, 1 completeness/acceptance FAIL, 2 malformed input or usage.
Reports list issues and attempt histories, counts, identities and an embedded
accepted input manifest. On FAIL the merge input list is empty. On PASS it is
ordered by ascending expected chunk index (gaps are allowed if explicitly planned).
Example summary: `PASS: 2/2 chunks accepted`. Example accepted input entry:

```json
{"chunk_id":"c0","index":0,"attempt_id":"a0","path":"0.root",
 "size_bytes":100,"sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}
```

This example is synthetic, not a real ROOT artifact. Reports bind the local root
and submission-plan identity, but do not independently certify the supplied plan
or evidence. No directory scan is performed: unexpected chunks can be identified
only if included in the inventory. Hashing cost scales with accepted file size;
use an external timeout for large local inputs. Files can change after inspection,
so a future merger must recheck checksums immediately before use. No merge, output
publication, retention cleanup, deletion or approval action is implemented.

## Read-only scheduler monitoring (Batch 16G)

The independent `tools/campaign_monitor.py` interface links existing expected-chunk
and attempt manifests to explicit Condor identity evidence. It emits reports to
stdout and leaves acceptance records and production scripts unchanged. See
[campaign_monitoring.md](campaign_monitoring.md) for offline fixtures, mapping
requirements, query bounds, conservative reconciliation and deferred work.
