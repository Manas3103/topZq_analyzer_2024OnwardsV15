#!/usr/bin/env python3
"""Prepare frozen 2024 production bundles. No scheduler or remote operations."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import shutil
import stat
import tempfile
from urllib.parse import urlsplit

from campaign import load_json, read_config

ROOT = Path(__file__).resolve().parents[1]
ID = re.compile(r'^[A-Za-z0-9][A-Za-z0-9._-]*$')
PREFIX = '/store/user/msahoo/tzq/2024/campaigns/'
KINDS = ('campaign', 'dataset', 'chunk', 'submission', 'attempt', 'execution', 'publication')


def now():
    return datetime.now(timezone.utc).isoformat()


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''): h.update(block)
    return h.hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False).encode()


def identifier(value):
    if not isinstance(value, str) or not ID.fullmatch(value) or value in ('.', '..'):
        raise ValueError('unsafe identity')
    return value


def relative(value):
    if not isinstance(value, str) or not re.fullmatch(r'[A-Za-z0-9._/-]+', value):
        raise ValueError('unsafe bundle path')
    if value.startswith('/') or any(p in ('', '.', '..') for p in value.split('/')):
        raise ValueError('unsafe bundle path')
    return value


def endpoint(value):
    if not isinstance(value, str) or not re.fullmatch(r'root://[A-Za-z0-9][A-Za-z0-9.-]*(?::[1-9][0-9]{0,4})?', value):
        raise ValueError('exact root endpoint required')
    u = urlsplit(value)
    if u.port is not None and u.port > 65535: raise ValueError('invalid endpoint port')
    return value


def nano_input(value, datatype):
    if not isinstance(value, str) or not value.isascii() or re.search(r'[\s%*?#[\]\\]', value):
        raise ValueError('exact original NanoAOD input required')
    u = urlsplit(value)
    if u.scheme != 'root' or u.username or u.password or u.query or u.fragment:
        raise ValueError('explicit root:// NanoAOD URL required')
    endpoint('root://' + u.netloc)
    ns = u.path[1:] if u.path.startswith('//') else u.path
    required = '/store/data/' if datatype == 'data' else '/store/mc/'
    if not ns.startswith(required) or not ns.endswith('.root') or any(x in ('', '.', '..') for x in ns[1:].split('/')):
        raise ValueError('original /store/data or /store/mc ROOT input required')
    return 'root://' + u.netloc.lower() + '/' + ns


def validate(value, *, _sparse=False):
    from jsonschema import Draft202012Validator, FormatChecker
    if not isinstance(value, dict) or value.get('kind') not in ['production_' + k for k in KINDS]:
        raise ValueError('unknown production metadata kind')
    kind = value['kind'][len('production_'):]
    schema = load_json(ROOT / 'schemas' / 'production' / (kind + '.schema.json'))
    errors = list(Draft202012Validator(schema, format_checker=FormatChecker()).iter_errors(value))
    if errors: raise ValueError('; '.join(e.message for e in errors))
    if kind == 'campaign':
        endpoint(value['endpoint'])
        subset = value.get('representative_subset')
        if subset is not None:
            if subset['full_2024_complete'] is not False:
                raise ValueError('representative subset cannot be full 2024 complete')
            approved = {}
            for row in subset['assignments']:
                key = (row['workflow'], row['dataset_id'], row['chunk_id'])
                if key in approved: raise ValueError('duplicate approved assignment')
                approved[key] = row
            declared = {}
            for d in value['datasets']:
                for ch in d['chunks']:
                    row = dict(dataset_id=d['dataset_id'], sample_name=d['sample_name'], das_dataset=d['das_dataset'], workflow=d['workflow'], chunk_id=ch['chunk_id'], index=ch['index'], filelist_sha256=hashlib.sha256(('\n'.join(ch['ordered_inputs']) + '\n').encode()).hexdigest())
                    key = (d['workflow'], d['dataset_id'], ch['chunk_id'])
                    if key in declared: raise ValueError('duplicate declared assignment')
                    declared[key] = row
            if approved != declared: raise ValueError('declared assignments differ from explicit approval')
        seen_d, seen_inputs = set(), set()
        for d in value['datasets']:
            validate(d, _sparse=subset is not None)
            key = (d['workflow'], d['dataset_id'])
            if key in seen_d: raise ValueError('duplicate dataset identity')
            seen_d.add(key)
            for c in d['chunks']:
                for x in c['ordered_inputs']:
                    normalized = nano_input(x, d['datatype'])
                    key = (d['workflow'], urlsplit(normalized).path)
                    if key in seen_inputs: raise ValueError('duplicate or ambiguous input coverage')
                    seen_inputs.add(key)
    elif kind == 'dataset':
        if (value['datatype'] == 'data') != (value['era'] is not None and value['stream'] is not None):
            raise ValueError('data requires era/stream; MC requires null era/stream')
        if value['datatype'] == 'mc' and (value['era'] is not None or value['stream'] is not None):
            raise ValueError('MC era/stream must be null')
        dataset_name, processing, tier = value['das_dataset'][1:].split('/')
        if value['datatype'] == 'data':
            if tier != 'NANOAOD' or dataset_name != value['stream'] or not processing.startswith('Run2024' + value['era'] + '-'):
                raise ValueError('DAS stream/era/type declaration mismatch')
        elif tier != 'NANOAODSIM' or '2024' not in processing:
            raise ValueError('2024 MC DAS declaration required')
        for key in ('chunk_id', 'index'):
            vals = [c[key] for c in value['chunks']]
            if len(vals) != len(set(vals)): raise ValueError('duplicate chunk identity/index')
        if not _sparse and sorted(c['index'] for c in value['chunks']) != list(range(len(value['chunks']))):
            raise ValueError('chunk indices must be contiguous from zero')
        for c in value['chunks']:
            validate(c)
            for x in c['ordered_inputs']: nano_input(x, value['datatype'])
        relative(value['config']); relative(value['profile'])
    elif kind == 'chunk':
        identifier(value['chunk_id'])
    if kind in ('execution', 'publication') and 'input_scope' in value:
        scope = value['input_scope']; maximum = scope['requested_max_input_events']; limited = maximum is not None
        if scope['partial'] != limited or scope['full_production_eligible'] == limited or scope['tree_prefix'] != ('pilot_outputTree' if limited else 'outputTree') or scope['analysis_input_entries'] != (min(scope['total_input_entries'], maximum) if limited else scope['total_input_entries']):
            raise ValueError('inconsistent input-scope/eligibility metadata')
    return value


def write_json(path, value):
    with Path(path).open('x') as f: json.dump(value, f, indent=2, allow_nan=False); f.write('\n')


def execution_path(campaign, submission, workflow, dataset, chunk, execution):
    if workflow not in ('main', 'fakefactor'): raise ValueError('invalid workflow')
    values = [identifier(x) for x in (campaign, submission, dataset, chunk, execution)]
    ca, su, ds, ch, ex = values
    return f'{PREFIX}{ca}/{workflow}/attempts/{su}/datasets/{ds}/chunks/{ch}/executions/{ex}'


def attempt_id(campaign_id, submission_id, workflow, dataset_id, chunk_id):
    return 'a-' + hashlib.sha256(canonical([campaign_id, submission_id, workflow, dataset_id, chunk_id])).hexdigest()


def prepare(campaign, submission_id, source_root, destination, payload, *, approved_assignments=None):
    """Caller supplies all runtime payload files. Refuse existing bundle directories."""
    validate(campaign); identifier(submission_id)
    subset = campaign.get('representative_subset')
    if (subset is not None) != (approved_assignments is not None):
        raise ValueError('sparse mode requires an explicit approved-assignments file')
    approval_bytes = None
    if subset is not None:
        approval_path = Path(approved_assignments)
        if approval_path.is_symlink() or not approval_path.is_file():
            raise ValueError('regular approved-assignments file required')
        approval_bytes = approval_path.read_bytes()
        if hashlib.sha256(approval_bytes).hexdigest() != subset['approval_sha256'] or load_json(approval_path) != {k:v for k,v in subset.items() if k != 'approval_sha256'}:
            raise ValueError('approved-assignments contents/hash mismatch')
    source_root = Path(source_root).resolve(strict=True)
    destination = Path(destination).absolute()
    if source_root == Path('/eos') or Path('/eos') in source_root.parents or destination == Path('/eos') or Path('/eos') in destination.resolve().parents:
        raise ValueError('preparation requires local paths outside mounted EOS')
    if destination.exists() or destination.is_symlink(): raise ValueError('bundle destination already exists')
    if not destination.parent.is_dir(): raise ValueError('bundle parent must exist')
    if not isinstance(payload, list) or any(not isinstance(x, str) for x in payload) or len(payload) != len(set(payload)): raise ValueError('duplicate payload path')
    paths = {relative(x) for x in payload}
    paths.update(['processnanoaod_v.py', 'libnanoadrdframe.so', 'rootdict_rdict.pcm'])
    headers = ['src/NanoAODAnalyzerrdframe.h', 'src/BaseAnalyser.h', 'src/FakeFactorAnalyser.h', 'src/Linkdef.h', 'src/utility.h', 'src/RNodeTree.h']
    visited = set()
    while headers:
        header = headers.pop()
        if header in visited: continue
        visited.add(header); paths.add(header)
        f = source_root / header
        if not f.is_file() or f.is_symlink(): raise ValueError('required local header missing/unsafe: ' + header)
        for inc in re.findall(r'^\s*#\s*include\s*"([^"]+)"', f.read_text(), re.M):
            for candidate in (f.parent / inc, source_root / inc, source_root / 'src' / inc):
                if candidate.is_file():
                    resolved = candidate.resolve()
                    if source_root not in resolved.parents or candidate.is_symlink(): raise ValueError('header dependency escapes source')
                    headers.append(str(candidate.relative_to(source_root))); break
    for d in campaign['datasets']: paths.update([d['config'], d['profile']])
    for d in campaign['datasets']:
        info = read_config(source_root / d['config'])
        c, flags = info['config'], info['procflags']
        expected_class = 'FakeFactorAnalyser' if d['workflow'] == 'fakefactor' else 'BaseAnalyser'
        if c.get('analysertype', 'BaseAnalyser') != expected_class or c['intreename'] != 'Events' or c['outtreename'] != 'outputTree':
            raise ValueError('workflow or tree/config mismatch')
        if c['datatype'] not in (-1, 1 if d['datatype'] == 'data' else 0): raise ValueError('datatype/config mismatch')
        limit = flags.get('nrootfiles')
        if type(limit) is not int or limit < 0 or limit and any(len(ch['ordered_inputs']) > limit for ch in d['chunks']):
            raise ValueError('config would truncate a frozen chunk')
        # Copy literal relative correction-file dependencies without changing configuration.
        for val in c.values():
            if isinstance(val, str) and '/' in val and val.endswith(('.json', '.root', '.txt')):
                paths.add(relative(val))
        from root_output_validation import profile
        p = profile(load_json(source_root / d['profile']))
        if campaign.get('max_input_events') is not None and p['required_objects'].get('TZQProcessingProvenance') != 'TNamed':
            raise ValueError('limited profile must require pilot provenance TNamed')
        if (p['workflow'], p['datatype']) != (d['workflow'], d['datatype']) or p['tree'] != ('pilot_outputTree_00000' if campaign.get('max_input_events') is not None else 'outputTree_00000'):
            raise ValueError('validation profile/config mismatch')
    # Runtime helpers and their schemas are frozen alongside caller payload.
    if any(d['workflow'] == 'main' for d in campaign['datasets']) and 'max_input_events' not in campaign:
        raise ValueError('main gate requires explicit max_input_events=null or a positive limit')
    helpers = ['tools/campaign_production.py', 'tools/campaign_worker.py', 'tools/campaign_staging.py',
               'tools/campaign_process.py', 'tools/campaign_input_validation.py', 'tools/campaign_dictionary_validation.py', 'tools/campaign_event_limit.py', 'tools/campaign_catalog.py', 'tools/campaign.py', 'tools/root_output_validation.py', 'tools/root_output_worker.py']
    internal = ['runjob_campaign_2024.sh', 'schemas/production/catalog_registration.schema.json'] + helpers + ['schemas/production/' + k + '.schema.json' for k in KINDS]
    if paths & set(internal): raise ValueError('runtime helper supplied twice')
    if any(x in ('manifest.json', 'campaign.json', 'representative_assignments.json') or x.startswith(('filelists/', 'queues/', 'attempts/')) for x in paths):
        raise ValueError('reserved bundle path')
    all_sources = {x: source_root / x for x in paths}
    all_sources.update({x: ROOT / x for x in internal})
    for x, f in list(all_sources.items()):
        expected_root = ROOT if x in internal else source_root
        if x == 'rootdict_rdict.pcm' and f.is_symlink():
            resolved = f.resolve(strict=True)
            if resolved != source_root / 'build/dict/rootdict_rdict.pcm' or not resolved.is_file():
                raise ValueError('PCM symlink target is not approved generated PCM')
            all_sources[x] = f = resolved
        if x == 'rootdict_rdict.pcm' and f.is_file() and not f.stat().st_size: raise ValueError('empty PCM payload')
        if f.is_symlink() or not f.is_file() or expected_root not in f.resolve().parents:
            raise ValueError('missing/nonregular/escaping payload: ' + x)
    frozen_processing = {x:digest(f) for x,f in sorted(all_sources.items())}
    if campaign.get('processing_files') is not None and campaign['processing_files'] != frozen_processing:
        raise ValueError('immutable campaign processing payload changed; new campaign required')
    campaign = json.loads(canonical(campaign))
    campaign['processing_files'] = frozen_processing
    tmp = Path(tempfile.mkdtemp(prefix='.production-preparation-', dir=destination.parent))
    try:
        hashes = {}
        if approval_bytes is not None:
            (tmp / 'representative_assignments.json').write_bytes(approval_bytes)
            hashes['representative_assignments.json'] = hashlib.sha256(approval_bytes).hexdigest()
        for x, f in sorted(all_sources.items()):
            target = tmp / x; target.parent.mkdir(parents=True, exist_ok=True)
            before = digest(f)
            if before != frozen_processing[x]: raise ValueError('payload changed after identity freeze')
            shutil.copy2(f, target)
            target.chmod(stat.S_IMODE(f.stat().st_mode) & 0o777)
            if before != digest(target) or before != digest(f): raise ValueError('payload changed while copying')
            hashes[x] = before
        write_json(tmp / 'campaign.json', campaign); hashes['campaign.json'] = digest(tmp / 'campaign.json')
        jobs = []
        for d in sorted(campaign['datasets'], key=lambda v: (v['workflow'], v['dataset_id'])):
            for ch in sorted(d['chunks'], key=lambda v: v['index']):
                path = f"filelists/{d['workflow']}/{d['dataset_id']}/{ch['chunk_id']}.txt"
                target = tmp / path; target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text('\n'.join(ch['ordered_inputs']) + '\n'); hashes[path] = digest(target)
                jobs.append(dict(workflow=d['workflow'], dataset_id=d['dataset_id'], chunk_id=ch['chunk_id'],
                                 attempt_id=attempt_id(campaign['campaign_id'], submission_id, d['workflow'], d['dataset_id'], ch['chunk_id']), index=ch['index'], filelist=path,
                                 filelist_sha256=hashes[path], config=d['config'], config_sha256=hashes[d['config']],
                                 profile=d['profile'], profile_sha256=hashes[d['profile']]))
        for j in jobs:
            attempt = dict(schema_version=1, kind='production_attempt', campaign_id=campaign['campaign_id'], submission_id=submission_id, workflow=j['workflow'], dataset_id=j['dataset_id'], chunk_id=j['chunk_id'], attempt_id=j['attempt_id'], filelist_sha256=j['filelist_sha256'], config_sha256=j['config_sha256'], created_at=campaign['created_at'])
            validate(attempt)
            path = f"attempts/{j['workflow']}/{j['attempt_id']}.json"
            (tmp / path).parent.mkdir(parents=True, exist_ok=True)
            write_json(tmp / path, attempt); hashes[path] = digest(tmp / path)
        for workflow in ('main', 'fakefactor'):
            lines = [' '.join(j[k] for k in ('dataset_id', 'chunk_id', 'attempt_id')) for j in jobs if j['workflow'] == workflow]
            path = f'queues/{workflow}.txt'; target = tmp / path; target.parent.mkdir(exist_ok=True)
            target.write_text('\n'.join(lines) + ('\n' if lines else '')); hashes[path] = digest(target)
        m = dict(schema_version=1, kind='production_submission', campaign_id=campaign['campaign_id'],
                 submission_id=submission_id, created_at=campaign['created_at'], campaign_sha256=hashes['campaign.json'], source=campaign['source'],
                 files=hashes, jobs=jobs)
        validate(m); write_json(tmp / 'manifest.json', m)
        verify_bundle(tmp, digest(tmp / 'manifest.json'))
        # mkdir claims the final name exclusively; failures retain a non-executable partial bundle.
        destination.mkdir()
        for f in tmp.iterdir(): shutil.move(str(f), str(destination / f.name))
        return m
    finally:
        shutil.rmtree(tmp)


def verify_bundle(bundle, expected_manifest_sha256):
    b = Path(bundle).resolve(strict=True)
    if digest(b / 'manifest.json') != expected_manifest_sha256: raise ValueError('manifest hash mismatch')
    m = validate(load_json(b / 'manifest.json'))
    for path, expected in m['files'].items():
        relative(path); f = b / path
        if f.is_symlink() or not f.is_file() or b not in f.resolve().parents or digest(f) != expected:
            raise ValueError('bundle integrity mismatch: ' + path)
    if not (b / 'runjob_campaign_2024.sh').stat().st_mode & stat.S_IXUSR:
        raise ValueError('bundled wrapper is not executable')
    c = validate(load_json(b / 'campaign.json'))
    subset = c.get('representative_subset')
    if subset is not None:
        approval = b / 'representative_assignments.json'
        if m['files'].get('representative_assignments.json') != subset['approval_sha256'] or digest(approval) != subset['approval_sha256'] or load_json(approval) != {k:v for k,v in subset.items() if k != 'approval_sha256'}:
            raise ValueError('frozen representative approval mismatch')
    elif 'representative_assignments.json' in m['files']:
        raise ValueError('representative approval without sparse mode')
    if digest(b / 'campaign.json') != m['campaign_sha256'] or c['campaign_id'] != m['campaign_id'] or c['source'] != m['source']:
        raise ValueError('campaign/submission mismatch')
    if any(m['files'].get(x) != sha for x,sha in c.get('processing_files', {}).items()) or not c.get('processing_files'):
        raise ValueError('immutable campaign processing hashes inconsistent')
    expected = {(d['workflow'], d['dataset_id'], ch['chunk_id'], attempt_id(c['campaign_id'], m['submission_id'], d['workflow'], d['dataset_id'], ch['chunk_id'])): (d, ch)
                for d in c['datasets'] for ch in d['chunks']}
    if len(m['jobs']) != len(expected): raise ValueError('job count differs from campaign')
    seen = set()
    for j in m['jobs']:
        key = tuple(j[k] for k in ('workflow', 'dataset_id', 'chunk_id', 'attempt_id'))
        if key not in expected or key in seen: raise ValueError('job identity differs from campaign')
        seen.add(key); d, ch = expected[key]
        if j['index'] != ch['index'] or j['config'] != d['config'] or j['profile'] != d['profile']:
            raise ValueError('job declaration differs from campaign')
        for key, hash_key in [('filelist', 'filelist_sha256'), ('config', 'config_sha256'), ('profile', 'profile_sha256')]:
            if m['files'].get(j[key]) != j[hash_key]: raise ValueError('job hash mismatch')
        expected_filelist = f"filelists/{d['workflow']}/{d['dataset_id']}/{ch['chunk_id']}.txt"
        if j['filelist'] != expected_filelist: raise ValueError('unexpected filelist association')
        attempt_file = b / f"attempts/{j['workflow']}/{j['attempt_id']}.json"
        attempt = validate(load_json(attempt_file))
        for field in ('campaign_id', 'submission_id'):
            if attempt[field] != m[field]: raise ValueError('attempt/submission mismatch')
        for field in ('workflow','dataset_id','chunk_id','attempt_id','filelist_sha256','config_sha256'):
            if attempt[field] != j[field]: raise ValueError('attempt/job mismatch')
        if (b / j['filelist']).read_text().splitlines() != ch['ordered_inputs']:
            raise ValueError('filelist differs from original inputs')
    return m, c


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--campaign', required=True); p.add_argument('--submission-id', required=True)
    p.add_argument('--source-root', required=True); p.add_argument('--destination', required=True)
    p.add_argument('--payload', action='append', default=[])
    p.add_argument('--approved-assignments', help='explicit approval file for representative sparse mode only')
    a = p.parse_args(argv)
    try:
        prepare(load_json(a.campaign), a.submission_id, a.source_root, a.destination, a.payload, approved_assignments=a.approved_assignments)
        print(json.dumps(dict(bundle=str(Path(a.destination).absolute()), manifest_sha256=digest(Path(a.destination) / 'manifest.json'))))
        return 0
    except (ValueError, OSError) as e: print(str(e), file=__import__('sys').stderr); return 2


if __name__ == '__main__': raise SystemExit(main())
