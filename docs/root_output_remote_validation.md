# Batch 16H-B: read-only remote ROOT output evidence

`tools/root_output_remote_validation.py` validates explicitly named XRootD outputs.
It emits JSON to stdout and leaves provenance UNKNOWN, acceptance `not_evaluated`
and `acceptance_changed=false`. It never selects accepted chunks or changes Batch
16F/16G manifests. Structural/content PASS means requested checks completed, not
campaign origin, physics correctness, scheduler success or staging success.

This is separate from [local validation](root_output_validation.md). Version-1
local inventory/profile behavior, local error classifications and campaign schemas
remain unchanged. Mounted EOS paths are unsupported; the remote controller accepts
only approved `root://` locators. Arbitrary mount aliases cannot be recognized
reliably by the local validator and remain caller responsibility.

## Inventory and approval

Version-1 `remote_root_output_inventory` requires `schema_version`, `kind`, and
1..100 explicit `outputs`. Records use the established fields:

```json
{
  "campaign_id": "synthetic", "workflow": "main", "dataset_id": "synthetic-data",
  "sample_name": "SyntheticData", "datatype": "data", "chunk_id": "c0", "index": 0,
  "attempt_id": "a0", "locator": "root://cmseos.fnal.gov//store/user/synthetic/fixture.root",
  "profile_id": "main-data-example"
}
```

Optional `job_id` and `evidence` references are caller declarations. No filename
inference or lookup establishes output provenance. Duplicate attempts, normalized
locators/jobs and contradictory dataset/chunk declarations are invalid input.
Profiles are the existing version-1 profiles, with main/FF and data/MC distinctions.

Every invocation requires one or more `--allow-prefix` values. Each value approves
an endpoint and absolute namespace prefix together; it must end in `/`. Initial
illustrative approval: `root://cmseos.fnal.gov//store/user/`. There are no implicit
approved endpoints. A prefix must be reviewed by the caller before authorization.
Host case and explicit default port 1094 normalize to the same endpoint. Original
and normalized locator, host, port and namespace are preserved. Prefix boundaries
are exact: `/store/user/` does not authorize `/store/users/`.

The deliberately restricted syntax permits ASCII DNS/IPv4 hostnames, optional
canonical numeric ports, and absolute namespaces introduced by `//`. Path segments
allow letters, digits, dot, underscore and hyphen. Local paths, mounted EOS paths,
other schemes, credentials, queries/fragments (including empty delimiters), encoded
paths, whitespace/control characters, wildcards, traversal and repeated namespace
slashes are rejected. IPv6 literals and alternative URL syntax are unsupported.
No automatic `/eos/uscms` to `/store` mapping is performed. DNS aliases, redirects,
physical replicas and differently named remote files are not proven equivalent.
Approval bounds requested locators; it is not a network firewall for server redirects.
XRootD may reconnect internally within the parent wall budget; the controller
implements no retry loop or credential renewal.

## CLI and read selection

Illustrative invocation using an intentionally synthetic, unverified remote path:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 tools/root_output_remote_validation.py \
  --inventory docs/root_remote_validation_examples/inventory.json \
  --profile docs/root_validation_profiles/main-data.json \
  --allow-prefix root://cmseos.fnal.gov//store/user/ \
  --inventory-timeout 300
```

This command performs remote access when actually run; it was not run against EOS
in Batch 16H-B. Offline tests mock the subprocess adapter and CLI calls. There is no
embedded fake-backend mode that could accidentally turn simulated evidence into
real output acceptance.

`--read-mode` defaults to `metadata`. All used profiles must match that mode;
profiles are not silently rewritten. To sample content, supply a reviewed sampled
profile and `--read-mode sample`. Full reads additionally require
`--read-mode full --allow-full-read`. Profile branch lists and zero-entry policy
remain authoritative. A full read covers configured branches, not every file object.

`--require-checksum` requests pre/post server checksum queries and makes unavailable
checksum evidence UNKNOWN. Without that flag, no checksum query runs and the
checksum dimension is `not_requested`; this does not prevent requested-check PASS.
There is no implicit hashing, copy, download or best-effort checksum-to-PASS fallback.

## Exact metadata adapter and compatibility assumptions

Only these argument arrays are issued, with no shell or user-selectable operation:

```python
["xrdfs", endpoint, "stat", namespace]
["xrdfs", endpoint, "query", "checksum", namespace]
```

No listing, recursive discovery, locate, prepare/recall, copy, deletion, rename,
retry or credential renewal is implemented. Offline/staging-pending/unreadable
flags skip ROOT inspection and produce UNKNOWN. Network access still has normal
server effects such as access logs and caches; the client makes no write request.

`xrdfs` must be installed; Python 3.9+, POSIX/Linux process controls and PyROOT are
required. ROOT is imported only by the reader child. Existing CMSSW setup is kept.

`stat` parsing supports explicit colon-separated `Size` and `Flags` fields, with
optional `Path`, decimal `Id`, one of `MTime`/`Modtime`, `CTime`, `ATime`, octal
`Mode`, `Owner` and `Group`. Reported paths must match the requested namespace.
Modification/creation time, mode and owner/group changes make pre/post evidence
inconsistent. Access time is recorded but excluded from that comparison because
reading the file can update it. Flags must have a decimal mask plus parenthesized
known whitespace-separated or pipe-separated names agreeing with the mask.
Supported bits are XBitSet=1,
IsDir=2, Other=4, Offline=8, IsReadable=16, IsWritable/IsWriteable=32,
POSCPending=64 and BackUpExists=128. These constants and error codes were inspected
in the installed XRootD 5.6.4 headers. Initial output tests were simulated; the
reliability fix also replays preserved EOS stat evidence without a new live query.
Unsupported formats, unknown/duplicate fields or contradictory masks are
UNKNOWN metadata-protocol limitations. No cross-version formatting guarantee is
claimed. Timestamps from stat remain reported strings, not converted clock proofs.

Checksum responses must be exactly algorithm plus hex digest. Supported initial
algorithms: Adler32 (8 hex digits), MD5 (32), SHA256 (64). Algorithm/value and
`source=server_reported`, `independently_verified=false` remain explicit. Adler32
is not SHA256. Unsupported algorithms/formats are UNKNOWN when requested. Matching
server checksums do not independently prove file integrity or campaign provenance;
checksum queries may incur server work and are bounded but do not cap server I/O.

Each query records its command, UTC start/end timestamps, elapsed duration, allotted
timeout, return code, parsed evidence, issues and bounded stdout/stderr tails.
Pre/post observations compare available size, reported ID/mtime and flags. Required
checksums compare algorithm and digest. Missing optional identity in both snapshots
is absence of that evidence, not an immutable-file guarantee. Incomplete queries or
observable disagreements produce UNKNOWN. Original observations remain in the report.

## Worker protocol, statuses and remote failures

The child uses `TFile.Open(..., 'READ')`. The controller enforces CP-16H-A's strict
JSON/evidence contract, explicit remote transport identity, derived dictionary
identities, required objects and metadata/sample/full coverage. Missing/contradictory
worker evidence produces `worker_protocol` UNKNOWN. Ordinary scalars retain their
existing behavior; missing dictionaries blocking configured reads remain UNKNOWN.
Recursive member dictionaries and actual OSSF4L_info reading are not established.

Remote open refusal/null/zombie and transport-ambiguous entry-read failures are
UNKNOWN, unlike local conclusive open/read failure classifications. If a requested
ROOT key exists but its object cannot be loaded, inspection is UNKNOWN rather than
a false missing-tree/object FAIL. Confirmed absent tree/branch/object, wrong type,
recovered flag and explicit zero-entry policy violations remain FAIL. Generic ROOT
exceptions remain UNKNOWN. There is no reliable exhaustive transport/corruption
classifier based on ROOT stderr alone; ambiguous cases are not declared corrupt.

ROOT `GetSize()` is diagnostic evidence, optionally null if unavailable. It is
recorded beside remote stat size without imposing an unverified equality invariant.
Metadata-only inspection does not establish tree-basket readability. Sample indices
and full ranges are preserved, including partial coverage. Secondary basket files
and TChains remain unsupported rather than followed.

XRootD server NotFound code 3011 establishes missing-file FAIL only for the initial
stat, with no other error codes or conflicting authentication/network/timeout
evidence. Echoed requested locators are removed before diagnostic classification,
so a pathname containing `proxy` or `timeout` is not itself an access error.
Authentication/authorization codes 3010/3030, proxy/credential errors, DNS/
network errors, timeouts, missing tools and ambiguous failures are UNKNOWN. A
checksum query's NotFound cannot itself establish file absence. Disappearance after
initial stat is UNKNOWN mutation/incomplete evidence. Text-only "not found" without
an identifiable server code is ambiguous. No credentials or proxy renewal is attempted.

Reports separate transport/access, metadata, requested checksum, ROOT structure,
content readability, provenance and acceptance. PASS requires no unresolved requested
checks. FAIL takes precedence over UNKNOWN while preserving every issue; thus a
conclusive missing branch can coexist with unresolved checksum evidence. CLI exits:
0 PASS, 1 FAIL, 3 UNKNOWN, 2 invalid input/usage. Missing scheduler history and Condor
exit status do not enter these classifications.

## Budgets and subprocess cleanup

Sequential maximum: 100 explicit outputs. Defaults and bounds:

| Budget | Default | Allowed |
|---|---|---|
| Metadata command wall time | 10 seconds | 1..60 |
| ROOT worker wall time | 20 seconds | 1..60 |
| Overall output deadline | 60 seconds | 1..600 |
| Optional inventory deadline | unset | 1..3600 |
| Worker address space | 4096 MiB | 512..8192 |

Every operation receives the smaller of its own timeout and remaining output/
inventory budget. Worker CPU soft/hard limits are ceiling of allotted wall time
and one second more; core dumps are disabled. Expired budgets preserve uninspected
records as UNKNOWN without launching more commands. Metadata is rechecked after
inspection when budget permits; incomplete postchecking cannot yield PASS.

Each command/worker starts a new POSIX session. Completion, timeout and cancellation
kill its process group, including same-group descendants. Cleanup waits are bounded
(up to one extra second), so deadlines are operational bounds plus cleanup overhead.
Uninterruptible kernel I/O and descendants deliberately escaping the session cannot
be controlled by this mechanism; no security-sandbox claim is made. No ROOT-level
network timeout is assumed to bound basket reads; parent deadlines remain decisive.

Diagnostics are limited to 4000 characters per stream/result. Both output pipes
are drained incrementally with one combined 1 MiB raw-byte capture limit, plus a
fixed 64 KiB read buffer. Crossing the limit terminates the process group and
produces UNKNOWN, not PASS. Cleanup does not call `communicate()` or accumulate
additional output. Worker request bytes are also limited to 1 MiB and are written
incrementally alongside reading stdout/stderr, avoiding blocked-pipe deadlocks.
This bounds retained subprocess data; it is not a total controller RSS bound.
Metadata commands have wall limits but no separate address-space limit. ROOT
retains its memory/CPU limits.

## Verification and deferred work

Tests distinguish mocked metadata/network/worker behavior from actual local ROOT
fixtures under the remote error policy and bounded local subprocess cleanup tests.
No test opens a remote file, queries EOS, submits jobs or uses production datasets.
Original CP-16H-A tests are retained unchanged. Live endpoint/plugin/authentication
compatibility, remote mutation semantics and production-scale I/O remain unverified.

Legacy production outputs may collide across campaigns/retries and lack embedded
attempt/execution provenance. Exact locators and checksums do not solve this. Batch
16F remains local-only, with its existing explicit acceptance and local SHA256 checks.
No accepted manifest, receipt, merge, retry or worker/staging change is added.
Batch 16I and live validation require separate authorization.

Historical Batch 16H-B offline validation in CMSSW_13_3_3 (Python 3.9.14,
ROOT 6.26/11):

```bash
PYTHONDONTWRITEBYTECODE=1 timeout 60s python3 -m unittest discover -s tests
```

**261 passed, 0 failed, 0 skipped**: 180 unchanged existing tests plus 81 remote
suite tests (73 mocked controller/reader tests, four process-control tests including
two actual local timeout cases, and four actual local ROOT fixture tests). Separately
executed simulated metadata/sample/full CLI examples returned PASS/exit 0 with
server-reported Adler32, provenance UNKNOWN and acceptance unchanged. A malformed
worker response returned worker_protocol UNKNOWN/exit 3. These outcomes do not
establish live XRootD compatibility. Whitespace checks cover tracked diffs and every
new file. No live EOS/Condor access or production processing occurred in that run.

Scoped reliability fixes verified on 2026-10-16:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -B -m unittest discover \
  -s tests -p 'test_root_output*.py' -v
```

**177 passed, 0 failed, 0 skipped**: 81 unchanged local-validator tests and 96
remote-validator tests. New regressions cover recorded EOS stat field formats,
pipe-separated flag masks, access-time-only changes, conflicting NotFound/network
diagnostics, echoed pathname words, streaming stdout/stderr/combined limits,
bounded simultaneous request/response handling, and structured UNKNOWN outcomes.
One saved EOS stat response was replayed locally; no fresh EOS access was used.
The subprocess tests use tiny synthetic local children; ROOT tests use temporary
local fixtures. No analyzer execution, physics change, build, production job,
acceptance operation or EOS write is part of these fixes. A checkpoint commit
does not establish live compatibility across XRootD endpoints or client versions.
