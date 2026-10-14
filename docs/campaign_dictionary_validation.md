# Phase B: main-campaign dictionary and output gate

`tools/campaign_dictionary_validation.py` provides an isolated READ-only gate.
`campaign_worker.py` calls preflight before processing, and the output gate after
Phase A ledger reconciliation and ordinary strict ROOT validation. Preparation
packages the helper automatically. No frozen bundle is updated.

The verified campaign request determines mode: positive integer event limit
means pilot; explicit null means unlimited. Main preparation/worker execution
reject omission of the existing max_input_events field instead of guessing unlimited. Profiles must agree. Both modes
require complete Phase A file/chain/counter evidence. Pilot requires
`pilot_outputTree_00000`, partial true, eligibility false, matching TNamed marker
and sidecar. Unlimited requires `outputTree_00000`, complete input/processed
counts, partial false, and no pilot marker, sidecar or opposite-mode tree.
Scope alone does not confer acceptance. The worker exposes unlimited technical
eligibility only after output gates pass; acceptance remains `not_evaluated`.

Preflight verifies manifest-linked library/PCM/header hashes, exact mapped
analysis-library path, compiled analyser and OSSF4LInfo dictionaries, Phase A
interface availability and readable PCM metadata keys. Metadata checks do not
trace Cling consuming the PCM or independently prove every ABI property.

The output gate reuses the existing strict ROOT worker-result validator and
profile checks. It then verifies custom branch type/category, mode provenance,
selected entries <= processed entries, and samples up to three default and
three non-default objects after at most 10,000 category reads. To preserve the
previously tested split-object reader, all branches are activated for the at
most six full-entry reads. Exact indices, values and search coverage are
reported. Missing categories and zero selected entries are coverage limitations,
not automatic corruption. Seven members must be finite and category-consistent.

The new gate protocol requires explicit structure, dictionary, PCM, artifact,
mode/provenance and read-coverage evidence. It rejects missing/contradictory
results. JSON parsing uses existing duplicate-key/nonfinite rejection. Valid
conclusive ROOT failures remain FAIL. Dictionary/dependency limitations,
timeouts, crashes and incomplete protocol evidence are UNKNOWN and block
processing/publication. Child requests/reports/logs are preserved outside bundle.
Wall timeout45s, CPU40s and address-space4GiB bounds remain local; existing process
group cleanup is used. A deliberately invalid PCM can crash ROOT before a report;
the controller records UNKNOWN instead of treating missing evidence as success.

Publication receipts are not required before publication. Existing staging
performs no-overwrite transfers, size/checksum verification and receipt-last
publication. Only after it returns does the additional local checker verify
receipt identity/scope, artifact sizes/hashes/remote checksum observations and
receipt marker consistency. It makes no additional EOS query. Checksums are not
physics acceptance or independent campaign provenance proof.

Phase B affects the main workflow only. Fakefactor's existing path is unchanged;
its dictionary-gate coverage is not established by these tests. No C++/physics,
Phase A input logic, correction, schema-contract or accepted-index change occurs.
New helper/worker bytes require a separately prepared and reviewed future bundle.

Local tests use preserved Phase A outputs, a new isolated build of the unchanged
C++ source, real ROOT negative fixtures and controller/receipt mutations. No
analyzer event loop runs during gate inspection: it uses TFile/TTree reads only.
Remote deployment, main-data coverage, full-file basket readability and Phase C
acceptance remain untested/deferred. See the persistent Phase B report for exact
counts, artifact hashes and limitations.
