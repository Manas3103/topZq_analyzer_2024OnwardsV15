# CP-2024-DEFERRED-VALIDATION — Fresh production transition

Date: 2026-10-12 (America/Chicago).
Branch: `cleanup/run3-remove-legacy-run2`.
Committed baseline: CP-16H-A, `6dd76337e367887248ea51cf50b9bcf5e5500fe2`.
Documentation-only checkpoint; not a production or acceptance PASS.

## Direction and preserved baseline

The user now targets fresh full 2024 production from original NanoAOD inputs.
The old merged-file full-histogram plan is superseded as the primary task.
The successful downstream baseline is the isolated three-plot diagnostic:
`/tmp/tzq-2024-three-plots-ezc917kx/`, with exactly
`ThreeLRegion_leadingLepton_pt`, `WZ_Region_zMass`, and
`SignalRegion_tzq_topMass`, H data, tZq, WZ and ZZ.
Its 12.75 fb^-1 normalization and plot label were approved for that diagnostic
only. Its MC cross sections and generator-weight sums remain provisional.
Temporary artifacts are not a durable campaign acceptance record.

## Explicitly deferred old-output audits

| Audit | State | Meaning |
|---|---|---|
| Old EOS inventory and chunk completeness | DEFERRED | No complete accepted-chunk inventory established |
| Old merged-file completeness and parent membership | DEFERRED | No verified ordered merge-parent association established |
| Old data deduplication correctness | DEFERRED | No verified uniqueness/duplicate-content/retention result established |

None is PASSED. Do not restart these audits without user direction.
Readability, existence and successful diagnostic plotting do not establish
these deferred properties. These are not automatic prerequisites for a fresh
raw-NanoAOD pilot, unless a finding demonstrates a relevant active-code defect.

## Unresolved observations and configuration defects

- Exact old input `/store/user/msahoo/2024_merged_rootfile/DYto2Tau-2Jets_M2L10-50.root`
  opened and reported `outputTree_00000` with four entries and zero branches
  during bounded metadata inspection. It cannot supply the main histogram
  recipe; cause unknown. No production-code cause or replacement established.
- The other 40 intended old merged inputs lack four authoritative histogram
  definitions: `leadingLepton_pt`, `subleadingLepton_pt`,
  `trailingLepton_pt`, `top_mass`. Histogrammer warns and skips them.
  Applicability to a fresh production schema remains to be reviewed; no branch
  aliases, physics definitions or configuration changes are approved here.
- `tth_non2b` cross section conflicts: active `2024_sample.txt` is 574.2;
  `samples_2024.json` is 372500.0. Neither selected or independently verified.
- Full-run 2024 luminosity is UNAPPROVED. Diagnostic 12.75 is not full-run approval.
- Active histogram table includes separate C/H data; external active plot mapping
  selects H only and expects `Era_H_merged_nodup_hist.root`, rather than generated
  `Merged_H_era_data_nodup_py_hist.root`. No combined-era policy approved.
- External mapping uses split skimmed WZ/ZZ/ttbar/WG filenames, while the active
  histogram table uses merged-sample basenames. DY names also differ. `ttwz`
  appears in the histogram sample table but not the external active grouping.
  No sample substitution or silent omission approved.

These unresolved settings are separate from the deferred audits. They must be
resolved for their relevant downstream uses, not marked validated by deferral.

## Evidence and restrictions

Active histogram workflow: `normalized_hist/Python_rdf_norm/`.
Authoritative recipe: `hist_config_with_labels.json` there.
External grouping/plotting: `/uscms_data/d3/msahoo/Project_tzq/merge_project/`.
`normalization_with_C/` is outdated and excluded from current recommendations.
Preparation evidence: `/tmp/tzq-2024-full-preparation-ap5ov35j/`; temporary,
not accepted provenance. Bounded exact-path inspection observed 41/41 old
inputs existing/opening with the named tree; no full content certification.

Campaign design reference: CP-16F, proposed namespace
`/store/user/msahoo/tzq/2024/campaigns/`.
Batch 16I design and launch approvals remain separate. No Batch 16I/16J
implementation, EOS writes or submission authorized by this checkpoint.
The six uncommitted Batch 16H-B files are preserved.

## Checkpoint creation validation

Created this Markdown only; no physics, production, schema or acceptance change.
No tests, build, processing, EOS or Condor operation performed for its creation.
Not staged, committed or pushed. Git tracking requires separate authorization.
