# Phase A: strict campaign input completeness

New campaign workers pass `--input-ledger <execution-dir>/input_validation.json`
to the existing three-positional production processor. Omitting the option
preserves the legacy input path. No production bundles were regenerated.

The strict path requires an explicit ordered filelist, rejects duplicate
normalized identities and configuration truncation, and sequentially opens
each assigned file in a bounded ROOT READ child. Each result must identify the
requested input tree, a nonnegative integral entry count and a file UUID.
Malformed protocol, missing/dependency/network/ROOT failures and timeouts block
processing. Remote open failure is unresolved rather than proof of corruption.
The file ledger preserves the original order and individual empty files.

Per-file wall timeout is 30 seconds; inventory budget is 600 seconds. Children
have 30-second CPU and 4-GiB address-space limits; the process runner bounds
stdout/stderr and kills process groups. ROOT reads only tree metadata, not all
input baskets. The subsequent eager chain opens are bounded by the existing
processor timeout, not an additional per-open timer.

Positive-entry files are added with eager `AddFile(..., 0, tree)`. Element
locators, tree names, entry counts, order and aggregate totals must agree.
Validated zero-entry files stay in the ledger as `validated_empty` and are
explicitly omitted from the chain; they contribute no events. An entirely
empty chunk stops before analyzer construction with `unsupported_empty_chunk`,
completion UNKNOWN, and no publication. Normalization is not fabricated.

Completion verification is opt-in. A fresh unfiltered Count (or the same pilot
input Range) is booked immediately before each synchronous Snapshot. After
Snapshot, `IsReady()` must be true and the loop-manager run count must have
increased by exactly one. Only then is the result accessed; another run-count
check guards against triggering an additional loop. The getter refuses evidence
until run() finishes and at least one Snapshot is verified. Counter booking is
late so setup counts and full-input genWeight scans cannot satisfy it.
Selections, corrections, generator bookkeeping and output branches are unchanged.
Each Snapshot's processed count must match the validated assigned total, or
min(total, pilot limit). Output selected entries remain a separate ROOT check.

The worker revalidates per-file evidence, chain totals and completion counts
before output validation/staging. The ledger is embedded under
`root_validation.input_completeness` in execution/publication evidence;
acceptance remains `not_evaluated`. Atomic local ledger creation refuses an
existing destination. Failure diagnostics stay in the execution directory/logs.

## Verification boundaries

The focused tests are deterministic protocol, mock ROOT/chain, worker failure
and static C++ guard tests. No actual ROOT file was inspected and no analyzer
was compiled/executed during Phase A. An authorized isolated build and runtime
regression must establish Count/Snapshot sharing, pilot Range behavior,
zero-entry chain behavior, exception propagation and unchanged physics values.
The current generated library lacks the new interface and must not be packaged
as compatible until that validation passes. No generated artifacts changed.

This is metadata and execution-count evidence, not a complete basket scan.
Same-size/count input mutation, aliases not identified by normalized namespace,
and ROOT errors not propagating as exceptions require further runtime testing.
Per-file UUID is recorded but is not independent provenance verification.
Existing generic production-test mocks which return success without a ledger
need updated strict-mode evidence before broader suite validation; the full
suite was not run under this focused-test authorization.
Dictionary gates, accepted-output index, frozen bundles and old records are unchanged.
