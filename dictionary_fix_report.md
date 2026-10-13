# OSSF4LInfo ROOT dictionary fix report

Result: PASS for dictionary registration and sampled backward-compatible reading of both existing production outputs.
Branch: fix/ossf4linfo-root-dictionary
HEAD: 6dd76337e367887248ea51cf50b9bcf5e5500fe2
No commit, staging, job submission, EOS access or production-bundle creation performed. Existing uncommitted Batch16H-B/16I work carried onto this branch unchanged.

## Before status and change

src/utility.h:238 defines OSSF4LInfo as a plain struct with seven int/double members. NanoAODAnalyzerrdframe.h includes utility.h, so rootcling sees the type transitively. Existing Linkdef selected analyzer/vector classes but omitted this stored object. Before readers reproduced HasDictionary=false and `no dictionary for class OSSF4LInfo is available`; BaseAnalyser/NanoAODAnalyzerrdframe dictionaries were available.

Single implementation change to src/Linkdef.h:
```cpp
#pragma link C++ class OSSF4LInfo+;
```
Struct layout, member types, BaseAnalyser logic, event weights, selections, processing configuration and corrections unchanged. Generated source now contains OSSF4LInfo_Dictionary and TGenericClassInfo registration. No ClassDef, ClassImp or custom Streamer was introduced.

## Build

Initialized environment retained: CMSSW_13_3_3/el9_amd64_gcc12, ROOT6.26/11, GCC12.3.1, Python3.9.14. Existing generated artifacts backed up under .dictionary_fix_validation/before before the authorized rebuild.

Exact command:
```bash
timeout --signal=TERM --kill-after=10s 600s make -j2 libnanoadrdframe.so > .dictionary_fix_validation/build.log 2>&1
```
Exit0. Make regenerated rootdict_headers.d, rootdict.C and rootdict_rdict.pcm, compiled rootdict.o/rootdict.d, then relinked libnanoadrdframe.so. Analyzer/helper objects were not recompiled; no make clean or standalone executable build. root-level PCM lookup remains the internal link to build/dict/rootdict_rdict.pcm. Generated files were produced by build tools, not manually edited.

Underlying dictionary command from Makefile:
```make
rootcling -I$(CORRECTION_INCDIR) -I$(SRCDIR) $(DICT_SOURCE) $(DICT_HEADERS)
```
DICT_HEADERS: src/NanoAODAnalyzerrdframe.h,src/BaseAnalyser.h,src/FakeFactorAnalyser.h,src/Linkdef.h.

## Initial existing-output runtime validation

Four bounded fresh Python processes (two files x old/new libraries), all exit0. Old library/PCM were retained under .dictionary_fix_validation/before; new library used the repository's matching generated PCM. Output files opened READ only. Runtime commands, durations and stdout/stderr retained in .dictionary_fix_validation/runtime-commands.json and output*-before/after files.

Reader locates three default and three non-default OSSF4L_category entries, then reads the entire entry at those indices and all seven object members. It compares object category/mass against corresponding scalar values where meaningful. No processor/analysis graph was run; no output ROOT files were generated or modified.

| Output | Selected entries | Default indices | Non-default indices | Scalar branches compared | Result |
|---|---:|---|---|---:|---|
|output0|61601|[0, 1, 2]|[143, 161, 187]|569|PASS|
|output1|73309|[0, 1, 2]|[20, 29, 233]|569|PASS|

After loading new library: OSSF4LInfo.HasDictionary=true; analyzer dictionaries remain true; OSSF4L_info remains class OSSF4LInfo; seven-member on-file streamer layout unchanged. Both after stderr files are empty: neither `The target class dictionary is not present` nor the observed `no dictionary for class OSSF4LInfo` warning occurred.

Each file compares six sampled entries across569 scalar branches (same types and reader-value encodings) and identical OSSF4LInfo member values before/after. Non-default samples have category1 and meaningful mass/index values. No category2-specific test or full branch-read sweep is claimed. Integer scalar leaf values were compared through ROOT's double-valued GetValue accessor; this is not a claim of raw per-leaf byte comparison for arbitrary64-bit integers.

## Physics and compatibility assessment

Existing production files have exactly the same SHA256 before/after, which preserves every stored scalar byte. Source fingerprints confirm BaseAnalyser.cpp and utility.h unchanged; only Linkdef.h differs among all pre-existing source files. The sampled old/new readers agree, including nontrivial objects. This establishes backward read compatibility for these two files and samples, not new-production physics equivalence or complete-file deserialization proof. No new NanoAOD processing was run during the initial fix verification; the subsequent authorized regression is recorded below.

The successful production-validation bundle still passes its original manifest verification and was not changed. Its old library/PCM intentionally remain frozen; this report does not retroactively replace them. EOS outputs and prior pilot records untouched. Any future bundle using the new dictionary needs separately approved preparation and new payload hashes. No such bundle created.

## Evidence and current state

- .dictionary_fix_validation/build.log
- .dictionary_fix_validation/runtime-commands.json
- .dictionary_fix_validation/comparison-results.json
- .dictionary_fix_validation/source-hashes-before.json
- .dictionary_fix_validation/output-hashes-before.json
- .dictionary_fix_validation/output*-before/after.json,stdout,stderr

Generated artifact SHA256 after rebuild:
- libnanoadrdframe.so: `046238bf82e0fc8b56d4fc3c3ce5eee12196810de2171ad4a4541a77fef47ad2`
- build/dict/rootdict.C: `dd2f16cd3eed0a8cfc13d45b65a18fa4ff81a3b06d91c5629f4e4bd75b7fe233`
- build/dict/rootdict_rdict.pcm: `bbbfe852ad50c1691e3ce8971998754638c57b0fe6f96b132d26cc8fc2067311`
- build/obj/rootdict.o: `150f6e604eb0e8f9e530a9b0c4dc72274204c849addff9e6c9bdc5a77216e038`

Production output SHA256 (unchanged):
- /uscms_data/d3/msahoo/Project_tzq/production_records/2024/production-validation-main-tzq-2024-1de3bd3adb6d437a9bf9cdf779ffc676/submission-retry-08f12d8dacef41918cdf9a40a9e7708e/campaign_executions/d654a28cbaf04785b949d7526b7f2468/output.root: `4c52a396d1f811d63a1bd8169fe0f7f16dcb211c05868965c85fb3614f3e66c5`
- /uscms_data/d3/msahoo/Project_tzq/production_records/2024/production-validation-main-tzq-2024-1de3bd3adb6d437a9bf9cdf779ffc676/submission-retry-08f12d8dacef41918cdf9a40a9e7708e/campaign_executions/fa3842004865455996fce345935f922e/output.root: `afb81d6c0ee00eff60bbd5ffc16fed141322ef05b1948bade2b4099e54ce6676`

`git diff --check` passed. No files staged or committed. New report and local validation evidence are untracked. Existing unrelated changes remain preserved.

## Controlled pre-commit physics regression (subsequent verification)

Result: PASS, limited to one trustworthy local2024 MC input and the unchanged main-analysis configuration. Nothing staged or committed; no production bundle created.

Old library: .dictionary_fix_validation/before/libnanoadrdframe.so SHA2560daac3a2e46853290cba4876201c780afbededb364c373fde9bf0a1a2ccfb998 with backed-up matching PCM.
New library: repository libnanoadrdframe.so SHA256046238bf82e0fc8b56d4fc3c3ce5eee12196810de2171ad4a4541a77fef47ad2 with matching new PCM.
Input: regression_baseline/inputs/43318103-fc71-48c7-8d99-4915164b3b87.root;141000 entries; SHA256 checked against production-48d4a5b-wigcmx95/manifest.json.
Both runs used identical jobconfiganalysis_2024.py/correction/helper bytes, original three-positional CLI and no input-event limit. No physics prescriptions selected or changed. Only source Linkdef and library/PCM differ in the staged trees. Each processor was bounded180s; exact commands are in .dictionary_fix_validation/physics_regression/run.sh, and library mapping assertions in logs/*-loading.json.

Bubblewrap mounted the host filesystem read-only, regression evidence writable at /mnt, private/tmp and disabled networking. A local DAS shim exits1 to prevent the processor's preliminary network lookup. Actual processing used the approved local fixture only. No rebuild performed in this task. No existing build artifacts overwritten. The preceding dictionary rebuild was a targeted in-repository rebuild; it should not be described as an isolated rebuild. This subsequent event regression was isolated.

Build evidence review confirms the intended transitive utility.h, Linkdef, analyzer headers, ROOT6.26/11 and CMSSW gcc12 compilation flags. Artifact SHA256 values still match the preceding report. rootdict source contains explicit OSSF4LInfo registration. Source fingerprints preserve all pre-existing changes other than the intended Linkdef addition.

Full compare_production.py result:
- Tree outputTree_00000 and6411 selected entries match.
-648 top-level /655 recursive branches: schemas/order/types match.
-All stored scalar/vector/object values compared across every selected entry, exact canonical array buffers: zero numerical differences.
-No missing/extra ROOT keys;0 histograms present in this main configuration.
-Full comparison exit0; both processors exit0.

Additional before/after object-read comparison: dictionary false -> true; three default entries0,1,2 and three non-default entries196,251,364; all seven OSSF4LInfo members match. All569 scalar branches match at those samples. After-reader stderr empty. Category2-specific coverage still not established; no multi-file/data/fakefactor/new-worker regression claimed.

Evidence: physics_regression/logs/full-comparison.json, full-comparison.log, before/after.log, before/after-loading.json and before/after-object-read.json/stdout/stderr. Existing production ROOT-file compatibility tests remain valid, and their file hashes were preserved. This main-fixture comparison strengthens regression evidence but is not independent physics correctness.

## Packaging and proposed commit scope

No packaging/schema/controller change is required: campaign_production.py already packages libnanoadrdframe.so, matching rootdict_rdict.pcm, Linkdef and mandatory/transitive headers including utility.h. New library/PCM/Linkdef hashes must be frozen together in a separately approved NEW campaign/bundle; do not mutate successful bundles or reuse their processing hash maps. Relocation of the NEW dictionary on a worker remains untested. Existing successful bundle hashes remain unchanged.

Files proposed for a dictionary-only commit: src/Linkdef.h and dictionary_fix_report.md ONLY. Exclude .dictionary_fix_validation/ (build backups, generated ROOT outputs/logs/scratch), all generated build artifacts, campaign_executions/, and pre-existing Batch16H-B/16I files. No git add/commit performed.
Proposed commit message: fix: generate ROOT dictionary for OSSF4LInfo

Current branch fix/ossf4linfo-root-dictionary, HEAD6dd76337e367887248ea51cf50b9bcf5e5500fe2. Staged set empty. Ready for explicit dictionary-only commit approval and subsequent separately authorized new validation-campaign preparation, with live worker portability still to validate.
