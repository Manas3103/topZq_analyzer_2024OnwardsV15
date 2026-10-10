# Repository instructions

## Project scope and entry points

- This is a CMS tZq analysis framework based on ROOT RDataFrame and NanoAOD.
- Active development targets CMS Run 3: 2022, 2023, 2024, and 2025. Current development is centered on 2024 NanoAODv15. This scope does not establish that every target year is already implemented.
- `processnanoaod_v.py` is the authoritative production entry point.
- `nanoaodrdataframe` and `fakefactorframe` are only for small-file testing/debugging, not normal production processing.
- Check year/era-specific NanoAOD schemas, triggers, correction payloads, calibrations, and working points. Do not assume a 2024 implementation applies to another Run-3 year.
- Read `docs/codex_repository_analysis.md` for architecture context. Its findings are investigation notes, not automatically approved bugs or fixes. Verify relevant current code before relying on the notes.

## Environment and execution authorization

- The CMSSW environment is normally already initialized by the user's shell on CMSLPC. Do not reconfigure it automatically.
- Before an authorized build or execution, inspect relevant environment values such as `CMSSW_BASE`, `CMSSW_VERSION`, ROOT, compiler, and Python versions. Report missing or inconsistent setup rather than silently replacing it.
- Do not automatically run `cmsenv`, `scram runtime`, source setup scripts, change CMSSW releases, or change `SCRAM_ARCH`.
- Read-only investigation is allowed. A request to change code does not implicitly authorize compilation or execution.
- Never compile, run `make` or another build command, execute analysis code or ROOT macros, run a processor or analysis executable, process even a small ROOT test file, submit/resubmit Condor jobs, or write to EOS unless the user explicitly authorizes that execution in the current task.
- This restriction includes histogram production, normalization, fake-factor processing, and event-processing validation helpers. Do not invoke wrappers or import analysis modules to bypass it.
- RDataFrame setup is not necessarily read-only: dereferencing `Count`, `Sum`, histogram results, or performing `Snapshot` can trigger event processing before `run()`.
- Credential renewal, production staging, large dataset processing, destructive cleanup, and removal of production outputs require explicit authorization in the current task.

## Scope of changes

- Inspect Git status before editing and preserve unrelated user changes.
- Investigation, explanation, review, and diagnosis default to read-only work.
- Keep changes narrowly scoped to the request. Do not automatically fix unrelated issues, stale comments, or inconsistencies; report them separately.
- Avoid unrelated refactoring, formatting, renaming, modernization, and cleanup.
- Before nontrivial edits, trace the active execution path, callers, RDataFrame dependencies, produced columns, stored branches, and downstream consumers.
- Determine whether a workflow is active before changing it. Do not synchronize inactive alternatives merely because they appear outdated.

## Protected physics behavior

- Physics selections, corrections, weights, normalization, reconstruction, fake-factor definitions, and systematics are protected behavior. Change them only when the user's request explicitly requires it.
- Protection includes object IDs and baseline/fakeable/tight selections; overlap cleaning; trigger combinations and prescales; golden JSON handling; JEC/JER and MET propagation; lepton momentum corrections; tagging discriminators, working points, efficiencies and scale factors; pileup and generator weights; OSSF/Z/W/top reconstruction; signal/control regions; FF cone-pT, measurement regions and numerator/denominator weights; and systematic propagation.
- Identify any possible physics consequence of a requested software change. Preserve existing behavior at an unresolved decision boundary and ask the user before choosing a physics prescription.
- Do not guess physics or normalization conventions. Comments, old implementations, existing binaries, and existing ROOT outputs do not establish correctness.
- Preserve deterministic smearing and its seed behavior unless a change is explicitly requested.

## RDataFrame and interface contracts

- Trace `Define`, `Redefine`, `Filter`, and all dependent columns before changing expressions. Distinguish original input columns from columns defined later in the graph.
- Preserve alignment of all corresponding object vectors through masking, sorting, merging, and overlap removal. Track sorting permutations and avoid applying them twice.
- Search downstream uses before renaming/removing columns or changing their types. A column may be consumed outside its source file.
- Preserve contracts among C++ column names, `addVartoStore` regexes, `outputTree_*` names and stored branches, histogram JSON definitions, and downstream histogram/normalization scripts.
- Cut indices affect RNodeTree parentage and output tree names. Do not change their structure or snapshot behavior as an incidental refactor.
- Keep Python configuration keys, the positional `setupCorrections()` interface, analyser class names, and ROOT `Linkdef.h` declarations consistent when an authorized change touches those interfaces.
- The production processor and standalone debugging mains use different configuration paths. Do not treat a hard-coded executable example as the production configuration.

## Data, MC, corrections, and normalization

- Preserve data/MC separation, including truth columns, smearing, generator/pileup weights, scale factors, certification, and data-specific corrections.
- Do not combine different data/MC types, years, eras, NanoAOD versions, or calibration campaigns without an explicit request and an established processing policy.
- Treat correction payloads and required calibration/efficiency files, including BTag ROOT inputs, as analysis inputs. Do not delete or replace them as disposable generated output.
- Do not choose correction tags, payload eras, or working points based only on names or comments.
- For an authorized weighting/normalization change, establish the data/MC type, cross-section and luminosity units, generator-weight convention, correction terms, and denominator scope.
- Distinguish chunk-level from dataset-wide sum-of-weights denominators. Do not silently substitute one for the other.
- Trace both tree-production and downstream histogram weights; do not assume they use the same formula or automatically make them agree.
- Explicit variation columns do not prove that shifted selections and reconstruction are propagated. Trace the actual consumers before changing systematics.

## Run-2 legacy code

- Run-3 development does not need to maintain or synchronize Run-2 implementations unless the user explicitly requests Run-2 support.
- Do not automatically remove Run-2 code or propagate Run-3 changes into it for consistency.
- Before Run-2 cleanup, classify material as standalone legacy files, Run-2 logic embedded in active `src/*`, shared code, or ambiguous code.
- Trace dependencies and possible effects on the active Run-3 path. Report proposed modifications/removals and their effects, then wait for user permission before editing or deleting them.
- An old year, trigger, calibration, working point, or comment is not sufficient evidence that code is unused. A cleanup branch does not itself authorize removal.

## Build artifacts, validation, and production

- Do not manually edit generated ROOT dictionaries, object files, executables, shared libraries, PCM files, caches, logs, or analysis ROOT outputs unless explicitly requested.
- Active ROOT dictionary source is generated under `build/dict/`. Historical `src/rootdict.C` is ignored and no longer tracked; existing local copies may remain. Do not regenerate dictionaries or run `make clean` without explicit authorization; cleanup can remove build products and other artifacts.
- When validation execution is explicitly authorized, use the smallest useful bounded validation and report exact commands and results. Compilation alone does not establish physics validity.
- When execution is not authorized, use read-only inspection and explain what runtime/build validation remains unperformed.
- Do not modify active production queues, stage files to EOS, renew credentials, submit jobs, or remove outputs as a side effect of development.
- Do not infer production success solely from process exit status when EOS staging or other external outputs matter. Inspect wrapper behavior before any authorized use; wrappers may create environments, overwrite outputs, or delete scratch contents.

## Git policy

- Do not modify Git remotes or remote configuration. The existing `origin` push configuration, including GitHub and CERN GitLab destinations, is intentional.
- Do not create/switch branches, stage files, commit, push, merge, rebase, reset, amend, force-push, or rewrite history unless explicitly requested.
- When a commit is requested, inspect the working tree and include only intended changes. Show the intended files and summarize the proposed commit before committing.
- Do not include unrelated changes, generated artifacts, ROOT outputs, or logs in a commit.

## Communication

- For nontrivial changes, explain current behavior, relevant files/functions, intended modifications, possible physics effects, and proposed validation.
- Report additional suspected issues separately without expanding the task. Label uncertainties and ask at unresolved physics or normalization decisions.
- At completion, summarize changes, inspection/validation performed, remaining limitations, and Git status. Do not claim unperformed builds or runtime checks.
