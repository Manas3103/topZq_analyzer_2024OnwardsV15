#!/usr/bin/env python3
"""One chunk execution; preserve local diagnostics and fail closed on publication."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import sys
import time
import uuid

from campaign import load_json
from campaign_process import CommandError, run_process
from campaign_production import identifier, now, digest, canonical, validate, verify_bundle, execution_path, write_json
from campaign_staging import stage


def runtime():
    if os.environ.get('CMSSW_VERSION') != 'CMSSW_13_3_3' or os.environ.get('SCRAM_ARCH') != 'el9_amd64_gcc12':
        raise ValueError('matching initialized CMSSW runtime required')
    import ROOT
    return dict(cmssw=os.environ['CMSSW_VERSION'], architecture=os.environ['SCRAM_ARCH'],
                python=sys.version.split()[0], root=ROOT.gROOT.GetVersion())


def validate_local(output, selected, identity, job, dataset, work):
    # Reuse local Batch 16H controller and strict isolated-worker protocol unchanged.
    from root_output_validation import validate_outputs
    item = dict(campaign_id=identity['campaign_id'], workflow=identity['workflow'], dataset_id=identity['dataset_id'],
                sample_name=dataset['sample_name'], datatype=dataset['datatype'], chunk_id=identity['chunk_id'],
                index=job['index'], attempt_id=identity['attempt_id'], path=str(output), profile_id=selected['profile_id'],
                job_id=f"{identity['cluster_id']}.{identity['proc_id']}")
    return validate_outputs(dict(schema_version=1, kind='root_output_inventory', outputs=[item]),
                            {selected['profile_id']: selected}, work, timeout=30)


def execute(bundle, manifest_sha256, identities, work_root, processor=run_process,
            validator=validate_local, publisher=stage, runtime_probe=runtime, execution_id=None):
    """Mock seams for offline tests; CLI always uses real processing/validation/staging."""
    for k in ('campaign_id', 'submission_id', 'attempt_id', 'dataset_id', 'chunk_id'): identifier(identities[k])
    if identities['workflow'] not in ('main', 'fakefactor'): raise ValueError('invalid workflow')
    for k in ('cluster_id', 'proc_id'):
        if type(identities[k]) is not int or identities[k] < 0: raise ValueError('invalid scheduler identity')
    if not isinstance(manifest_sha256, str) or len(manifest_sha256) != 64 or any(c not in '0123456789abcdef' for c in manifest_sha256):
        raise ValueError('expected manifest SHA256 required')
    ex = execution_id or uuid.uuid4().hex  # Every invocation, including the same Condor job after restart.
    if len(ex) != 32 or any(c not in '0123456789abcdef' for c in ex): raise ValueError('invalid execution ID')
    root = Path(work_root).absolute()
    if root == Path('/eos') or Path('/eos') in root.parents or root.is_symlink(): raise ValueError('local scratch required')
    root.mkdir(parents=True, exist_ok=True)
    root = root.resolve()
    if Path('/eos') in root.parents: raise ValueError('local scratch required')
    work = root / ex; work.mkdir()
    identity = dict(identities, execution_id=ex)
    result = dict(identity=identity, started_at=now(), status='failed', phase='bundle',
                  processing_exit=None, validation_status=None, staging_exit=None, acceptance='not_evaluated')
    try:
        m, c = verify_bundle(bundle, manifest_sha256); b = Path(bundle).resolve()
        if (m['campaign_id'], m['submission_id']) != (identity['campaign_id'], identity['submission_id']):
            raise ValueError('invocation/submission identity mismatch')
        matches = [j for j in m['jobs'] if all(j[k] == identity[k] for k in ('workflow', 'dataset_id', 'chunk_id', 'attempt_id'))]
        if len(matches) != 1: raise ValueError('invocation does not identify exactly one prepared job')
        j = matches[0]; d = next(d for d in c['datasets'] if (d['dataset_id'], d['workflow']) == (j['dataset_id'], j['workflow']))
        profile = load_json(b / j['profile']); result['phase'] = 'runtime'; effective = runtime_probe()
        result['phase'] = 'processing'; output = work / 'output.root'; started = time.monotonic()
        module = Path(j['config']).stem
        env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', ROOT_INCLUDE_PATH=str(b) + ':' + str(b / 'src') + (':' + os.environ['ROOT_INCLUDE_PATH'] if os.environ.get('ROOT_INCLUDE_PATH') else ''),
                   LD_LIBRARY_PATH=str(b) + (':' + os.environ['LD_LIBRARY_PATH'] if os.environ.get('LD_LIBRARY_PATH') else ''))
        dictionary_preflight = None
        if identity['workflow'] == 'main':
            if 'max_input_events' not in c: raise ValueError('main gate requires explicit max_input_events=null or a positive limit')
            from campaign_dictionary_validation import gate
            result['phase'] = 'runtime'
            dictionary_preflight = gate(b, m['files'], profile, c.get('max_input_events'), work)
        result['phase'] = 'processing'; started = time.monotonic()
        command = [sys.executable, str(b / 'processnanoaod_v.py'), str(b / j['filelist']), str(output), module, '--input-ledger', str(work / 'input_validation.json')]
        if c.get('max_input_events') is not None:
            command += ['--max-input-events', str(c['max_input_events'])]
        try:
            completed = processor(command, c['processing_timeout'], cwd=b, env=env, log=work / 'processing.log')
            result['processing_exit'] = completed.returncode
            if completed.returncode: raise CommandError('nonzero_exit', 'processor failed', completed.returncode)
        except CommandError as exc:
            result['processing_exit'] = exc.returncode; raise
        result['processing_seconds'] = time.monotonic() - started
        verify_bundle(bundle, manifest_sha256)  # Runtime must not mutate frozen inputs/configuration.
        if not output.is_file() or output.is_symlink(): raise ValueError('processor did not produce a regular output')
        scope_path = Path(str(output) + '.input_scope.json')
        maximum = c.get('max_input_events')
        if maximum is not None:
            scope = load_json(scope_path)
            if scope.get('requested_max_input_events') != maximum or scope.get('partial') is not True or scope.get('full_production_eligible') is not False or scope.get('tree_prefix') != 'pilot_outputTree' or scope.get('generator_weight_bookkeeping_scope') != 'full_input_chain_when_mc' or type(scope.get('total_input_entries')) is not int or scope.get('analysis_input_entries') != min(scope['total_input_entries'], maximum):
                raise ValueError('partial input-scope evidence missing or contradictory')
        else:
            if scope_path.exists(): raise ValueError('unexpected partial provenance in unlimited execution')
            with (work / 'processing.log').open() as log:
                totals = [int(total_match.group(1)) for line in log if (total_match := re.match(r'^Total Number of Entries:\s*([0-9]+)\s*$', line))]
            if len(totals) != 1: raise ValueError('total input entry evidence missing/ambiguous')
            scope = dict(schema_version=1, kind='processing_input_scope', total_input_entries=totals[0], requested_max_input_events=None, analysis_input_entries=totals[0], generator_weight_bookkeeping_scope='full_input_chain_when_mc', partial=False, full_production_eligible=True, tree_prefix='outputTree')
        from tools.campaign_input_validation import strict_json, require_complete
        ledger = strict_json((work / 'input_validation.json').read_text())
        require_complete(ledger, (b / j['filelist']).read_text().splitlines(), maximum)
        if ledger['expected_total_entries'] != scope['total_input_entries'] or ledger['processed_entries'] != scope['analysis_input_entries']:
            raise ValueError('input ledger/scope mismatch')
        result['phase'] = 'validation'
        evidence = validator(output, profile, identity, j, d, work)
        result['validation_status'] = evidence.get('status')
        write_json(work / 'validation.json', evidence)
        if evidence.get('status') != 'PASS' or evidence.get('acceptance_changed') is not False or len(evidence.get('outputs', [])) != 1:
            raise ValueError('ROOT validation failed or incomplete')
        observed = evidence['outputs'][0]
        # Verify inventory association; validation remains caller-declared provenance.
        for k in ('campaign_id', 'workflow', 'dataset_id', 'chunk_id', 'attempt_id'):
            if observed.get('identity', {}).get(k) != identity[k]: raise ValueError('validation identity mismatch')
        if observed.get('status') != 'PASS' or observed.get('inspected_path') != str(output) or observed.get('acceptance') != 'not_evaluated':
            raise ValueError('validation output evidence inconsistent')
        if type(observed.get('structure', {}).get('entries')) is not int or not 0 <= observed['structure']['entries'] <= scope['analysis_input_entries']:
            raise ValueError('selected output entries exceed analysis input scope')
        if identity['workflow'] == 'main':
            dictionary_output = gate(b, m['files'], profile, maximum, work, scope, ledger, output)
            evidence['dictionary_checks'] = dict(preflight=dictionary_preflight, output=dictionary_output)
        evidence['input_completeness'] = ledger
        schema_fingerprint = hashlib.sha256(canonical(dict(tree=profile['tree'], branches=observed['structure']['branch_types']))).hexdigest()
        ex_record = dict(schema_version=1, kind='production_execution', **identity, started_at=result['started_at'],
                         ended_at=now(), source=c['source'], manifest_sha256=manifest_sha256,
                         campaign_sha256=m['campaign_sha256'], payload_hashes=m['files'],
                         ordered_inputs=(b / j['filelist']).read_text().splitlines(),
                         dataset_declaration={k:d[k] for k in ('das_dataset','year','datatype','era','stream','sample_name')},
                         filelist_sha256=j['filelist_sha256'], config_sha256=j['config_sha256'], profile_sha256=j['profile_sha256'],
                         processing_schema_id=d['processing_schema_id'], schema_fingerprint=schema_fingerprint,
                         input_scope=scope, runtime=effective, processing_exit=0, status='validated', root_validation=evidence,
                         acceptance='not_evaluated')
        result['input_scope'] = scope
        validate(ex_record); write_json(work / 'execution.json', ex_record)
        destination = execution_path(identity['campaign_id'], identity['submission_id'], identity['workflow'], identity['dataset_id'], identity['chunk_id'], ex)
        result['phase'] = 'staging'
        published = publisher({'output.root': output, 'processing.log': work / 'processing.log', 'execution.json': work / 'execution.json'},
                              work / 'publication.json', identity, destination,
                              dict(c['publication_policy'], endpoint=c['endpoint']), evidence)
        if published.get('receipt', {}).get('status') != 'published' or not published.get('marker'):
            raise ValueError('publisher did not verify the publication marker')
        if identity['workflow'] == 'main':
            from campaign_dictionary_validation import verify_publication
            verify_publication(published, identity, scope, {'output.root': output, 'processing.log': work / 'processing.log', 'execution.json': work / 'execution.json'}, c['endpoint'])
        result.update(status='published', staging_exit=0, publication_marker=published['marker'], destination=destination)
    except Exception as exc:
        if result['phase'] == 'validation': result['validation_status'] = getattr(exc, 'status', 'UNKNOWN')
        if result['phase'] == 'staging': result['staging_exit'] = 1
        result['error'] = dict(type=type(exc).__name__, detail=str(exc), code=getattr(exc, 'code', None),
                               stdout=getattr(exc, 'stdout', '')[-4000:], stderr=getattr(exc, 'stderr', '')[-4000:])
    result['ended_at'] = now()
    if result['status'] == 'failed':
        failure = dict(schema_version=1, kind='production_execution', **identity, manifest_sha256=manifest_sha256,
                       **{k:result[k] for k in ('started_at','ended_at','status','phase','processing_exit','validation_status','staging_exit','acceptance','error')})
        validate(failure); write_json(work / 'execution.failure.json', failure)
    write_json(work / 'result.json', result)
    return result, work


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    for key in ('bundle', 'manifest-sha256', 'campaign-id', 'submission-id', 'workflow', 'dataset-id', 'chunk-id', 'attempt-id', 'work-root'):
        p.add_argument('--' + key, required=True)
    p.add_argument('--execution-id')
    p.add_argument('--cluster-id', type=int, required=True); p.add_argument('--proc-id', type=int, required=True)
    a = p.parse_args(argv)
    try:
        identity = {k: getattr(a, k) for k in ('campaign_id', 'submission_id', 'workflow', 'dataset_id', 'chunk_id', 'attempt_id', 'cluster_id', 'proc_id')}
        result, work = execute(a.bundle, a.manifest_sha256, identity, a.work_root, execution_id=a.execution_id)
        print(json.dumps(dict(result=result, diagnostics=str(work))))
        return 0 if result['status'] == 'published' else 1
    except (ValueError, OSError) as exc: print(str(exc), file=sys.stderr); return 2


if __name__ == '__main__': raise SystemExit(main())
