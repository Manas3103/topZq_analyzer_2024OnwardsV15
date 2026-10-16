#!/usr/bin/env python3
"""Offline local ROOT evidence; never changes campaign acceptance."""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import sys
from campaign import load_json

ID = re.compile(r'^[A-Za-z0-9][A-Za-z0-9._-]*$')


def fields(value, kind, required, optional=()):
    if not isinstance(value, dict) or not set(required) <= set(value) or set(value) - set(required) - set(optional):
        raise ValueError(f'{kind}: missing or unknown fields')


def version(value, kind):
    if type(value.get('schema_version')) is not int or value['schema_version'] != 1 or value.get('kind') != kind:
        raise ValueError(f'unsupported {kind} version/kind')


def names(values):
    if not isinstance(values, list) or any(not isinstance(x, str) or not x or '/' in x or '*' in x for x in values) or len(set(values)) != len(values):
        raise ValueError('names must be unique nonempty exact names')


def profile(value):
    fields(value, 'profile', ('schema_version', 'kind', 'profile_id', 'workflow', 'datatype', 'tree', 'required_branches', 'branch_types', 'required_objects', 'allow_zero_entries', 'content', 'dictionary_branches'))
    version(value, 'root_validation_profile')
    if not isinstance(value['profile_id'], str) or not ID.fullmatch(value['profile_id']):
        raise ValueError('invalid profile ID')
    if value['workflow'] not in ('main', 'fakefactor') or value['datatype'] not in ('data', 'mc'):
        raise ValueError('invalid workflow/datatype')
    names([value['tree']]); names(value['required_branches']); names(value['dictionary_branches'])
    if type(value['allow_zero_entries']) is not bool:
        raise ValueError('allow_zero_entries must be boolean')
    for key in ('branch_types', 'required_objects'):
        if not isinstance(value[key], dict):
            raise ValueError(f'{key} must be object')
        names(list(value[key]))
        if any(not isinstance(v, str) or not v for v in value[key].values()):
            raise ValueError('type/class names must be nonempty strings')
    if not set(value['branch_types']) <= set(value['required_branches']):
        raise ValueError('type expectations must refer to required branches')
    fields(value['content'], 'content', ('mode', 'branches', 'sample_entries'))
    content = value['content']; names(content['branches'])
    if content['mode'] not in ('metadata', 'sample', 'full') or type(content['sample_entries']) is not int or not 1 <= content['sample_entries'] <= 10000:
        raise ValueError('invalid content policy')
    if content['mode'] != 'metadata' and not content['branches']:
        raise ValueError('content reading requires explicit branches')
    return value


def safe_path(value, root):
    if not isinstance(value, str) or not value or '://' in value or ':' in value or not Path(value).is_absolute() or '..' in Path(value).parts:
        raise ValueError('exact absolute local paths required; URLs/traversal rejected')
    path = Path(value)
    if path == Path('/eos') or Path('/eos') in path.parents:
        raise ValueError('mounted EOS paths rejected')
    resolved = path.resolve()
    if Path('/eos') in path.parents or Path('/eos') in resolved.parents or resolved == Path('/eos') or root not in resolved.parents:
        raise ValueError('EOS or path outside approved local root')
    return resolved


def inventory(value, profiles, root):
    fields(value, 'inventory', ('schema_version', 'kind', 'outputs'))
    version(value, 'root_output_inventory')
    if not isinstance(value['outputs'], list) or not value['outputs'] or len(value['outputs']) > 100:
        raise ValueError('inventory requires 1..100 outputs')
    identities, attempts, jobs, paths = set(), {}, {}, set()
    for entry in value['outputs']:
        fields(entry, 'output', ('campaign_id', 'workflow', 'dataset_id', 'sample_name', 'datatype', 'chunk_id', 'index', 'attempt_id', 'path', 'profile_id'), ('job_id', 'evidence'))
        for key in ('campaign_id', 'dataset_id', 'chunk_id', 'attempt_id'):
            if not isinstance(entry[key], str) or not ID.fullmatch(entry[key]):
                raise ValueError(f'invalid {key}')
        if not isinstance(entry['sample_name'], str) or not entry['sample_name'] or type(entry['index']) is not int or entry['index'] < 0:
            raise ValueError('sample name and nonnegative integer index required')
        p = profiles.get(entry['profile_id'])
        if p is None or (p['workflow'], p['datatype']) != (entry['workflow'], entry['datatype']):
            raise ValueError('missing or mismatched workflow/datatype profile')
        key = tuple(entry[k] for k in ('campaign_id', 'workflow', 'dataset_id', 'chunk_id', 'attempt_id'))
        aid = (entry['campaign_id'], entry['workflow'], entry['attempt_id'])
        if key in identities or aid in attempts:
            raise ValueError('duplicate or contradictory output/attempt identity')
        identities.add(key); attempts[aid] = key
        path = safe_path(entry['path'], root)
        if path in paths:
            raise ValueError('same resolved output assigned more than once')
        paths.add(path)
        jid = entry.get('job_id')
        if jid is not None:
            if not isinstance(jid, str) or not re.fullmatch(r'(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)', jid) or jid in jobs:
                raise ValueError('invalid/duplicate job identity')
            jobs[jid] = key
        evidence = entry.get('evidence', [])
        if not isinstance(evidence, list) or any(not isinstance(x, str) or not x.strip() for x in evidence):
            raise ValueError('evidence references must be nonempty strings')
    # Dataset and chunk declarations must agree across retries.
    datasets, chunks, indices = {}, {}, {}
    for e in value['outputs']:
        d = (e['campaign_id'], e['workflow'], e['dataset_id'])
        c = d + (e['chunk_id'],); idx = d + (e['index'],)
        declaration = (e['datatype'], e['sample_name'])
        if d in datasets and datasets[d] != declaration or c in chunks and chunks[c] != e['index'] or idx in indices and indices[idx] != e['chunk_id']:
            raise ValueError('contradictory dataset/chunk declaration')
        datasets[d] = declaration; chunks[c] = e['index']; indices[idx] = e['chunk_id']


def issue(result, code, status, detail):
    result['issues'].append(dict(code=code, status=status, detail=detail))


def validate_worker_result(r, p, transport='local'):
    """Validate staged evidence, not the truth of the worker's byte reads."""
    def require(condition):
        if not condition: raise ValueError('incomplete or contradictory worker evidence')
    require(transport in ('local', 'remote'))
    fields(r, 'worker', ('issues', 'structure', 'content') + (('transport',) if transport == 'remote' else ()))
    if transport == 'remote': require(r['transport'] == 'remote')
    require(isinstance(r['issues'], list) and isinstance(r['structure'], dict) and isinstance(r['content'], dict))
    allowed = {
        'root_dependency_unavailable': 'UNKNOWN', 'inspection_exception': 'UNKNOWN',
        'close_exception': 'UNKNOWN', 'external_storage': 'UNKNOWN', 'dictionary_unavailable': 'UNKNOWN',
        'root_open_failed': 'FAIL', 'recovered_file': 'FAIL', 'missing_tree': 'FAIL',
        'invalid_entry_count': 'FAIL', 'zero_entries_forbidden': 'FAIL', 'missing_branch': 'FAIL',
        'branch_type_mismatch': 'FAIL', 'missing_object': 'FAIL', 'object_type_mismatch': 'FAIL',
        'missing_read_branch': 'FAIL', 'content_unreadable': 'FAIL'}
    if transport == 'remote':
        allowed.update(root_open_failed='UNKNOWN', content_unreadable='UNKNOWN')
    for i in r['issues']:
        fields(i, 'issue', ('code', 'status', 'detail'))
        require(isinstance(i['code'], str) and i['code'] in allowed and
                i['status'] == allowed[i['code']] and isinstance(i['detail'], str))
    codes = {i['code'] for i in r['issues']}
    s, c = r['structure'], r['content']
    require(not set(s) - ({'opened', 'tree', 'recovered', 'entries', 'branch_types', 'objects', 'object_matches', 'dictionaries', 'dictionary_dependent_branches'} | ({'root_file_size'} if transport == 'remote' else set())))
    if 'root_file_size' in s:
        require(s.get('opened') is True and (s['root_file_size'] is None or type(s['root_file_size']) is int and s['root_file_size'] >= 0))
    if 'opened' in s: require(s['opened'] is True)
    if 'tree' in s: require(s['tree'] == p['tree'])
    if 'recovered' in s: require(type(s['recovered']) is bool and s['recovered'] == ('recovered_file' in codes))
    if 'entries' in s: require(type(s['entries']) is int)
    if 'dictionary_dependent_branches' in s: names(s['dictionary_dependent_branches'])
    for key in ('branch_types', 'objects', 'object_matches', 'dictionaries'):
        if key in s:
            require(isinstance(s[key], dict) and all(isinstance(k, str) and k for k in s[key]))
            require(all((v is None or isinstance(v, str)) if key in ('branch_types', 'objects') else
                        (type(v) is bool or key == 'dictionaries' and v is None) for v in s[key].values()))
    if 'root_dependency_unavailable' in codes:
        require(codes == {'root_dependency_unavailable'} and not s and not c); return
    fields(c, 'content', ('mode', 'requested_branches', 'checked_branches', 'checked_entries', 'entry_range', 'complete_configured_read'), ('coverage', 'entries_checked'))
    require(c['mode'] == p['content']['mode'] and c['requested_branches'] == p['content']['branches'])
    names(c['checked_branches'])
    require(set(c['checked_branches']) <= set(p['content']['branches']) and type(c['complete_configured_read']) is bool)
    require(isinstance(c['checked_entries'], list) and all(type(x) is int and x >= 0 for x in c['checked_entries']))
    def interval(v):
        return v is None or isinstance(v, list) and len(v) == 2 and all(type(x) is int for x in v) and v[0] == 0 and v[1] >= 0
    require(interval(c['entry_range']))
    if 'entries_checked' in c: require(type(c['entries_checked']) is int and c['entries_checked'] >= 0)
    if 'entries' not in s:
        require(not c['checked_branches'] and not c['checked_entries'] and c['entry_range'] is None and
                not c['complete_configured_read'] and 'coverage' not in c and 'entries_checked' not in c)
    interrupted = bool(codes & {'inspection_exception', 'close_exception'})
    def need(keys): require(set(keys) <= set(s) or interrupted)
    if 'root_open_failed' in codes:
        require(not s and not c['checked_branches'] and not c['complete_configured_read']); return
    need(('opened', 'tree', 'recovered'))
    if 'missing_tree' in codes:
        require(all(i['detail'] == p['tree'] for i in r['issues'] if i['code'] == 'missing_tree'))
        require('entries' not in s and not c['checked_branches'] and not c['complete_configured_read']); return
    if 'entries' not in s:
        require(interrupted or 'external_storage' in codes)
        require(not c['checked_branches'] and not c['complete_configured_read']); return
    n = s['entries']
    require((n < 0) == ('invalid_entry_count' in codes))
    if n < 0: require(not c['complete_configured_read']); return
    require(('zero_entries_forbidden' in codes) == (n == 0 and not p['allow_zero_entries']))
    need(('branch_types', 'objects', 'object_matches', 'dictionaries', 'dictionary_dependent_branches'))
    if not {'branch_types', 'objects', 'object_matches', 'dictionaries', 'dictionary_dependent_branches'} <= set(s):
        require(not c['complete_configured_read']); return
    types, objects, matches, dictionaries = (s[k] for k in ('branch_types', 'objects', 'object_matches', 'dictionaries'))
    require(set(objects) == set(p['required_objects']) and set(matches) == set(objects))
    dependent = set(s['dictionary_dependent_branches'])
    require(dependent <= set(types))
    require(set(dictionaries) == set(p['dictionary_branches']) | dependent)
    # Worker uses null only for absent branches, including unlisted entries.
    for name, available in dictionaries.items():
        require((available is None) == (name not in types))
    def details(code): return {i['detail'] for i in r['issues'] if i['code'] == code}
    require(details('missing_branch') == set(p['required_branches']) - set(types))
    expected_types = {f'{name}: {types[name]} != {expected}' for name, expected in p['branch_types'].items() if name in types and types[name] != expected}
    require(details('branch_type_mismatch') == expected_types)
    require(details('missing_object') == {name for name, value in objects.items() if value is None})
    require(details('object_type_mismatch') == {name for name in objects if objects[name] is not None and not matches[name]})
    require(all(not matches[name] for name in objects if objects[name] is None))
    if c['mode'] == 'metadata':
        require(not c['checked_branches'] and not c['checked_entries'] and c['entry_range'] is None and
                not c['complete_configured_read'] and 'coverage' not in c and 'entries_checked' not in c)
        require(not codes & {'dictionary_unavailable', 'missing_read_branch', 'content_unreadable'}); return
    requested = p['content']['branches']
    if 'external_storage' not in codes:
        require(details('missing_read_branch') == set(requested) - set(types))
        require(details('dictionary_unavailable') == {name for name in requested if name in types and name in dictionaries and dictionaries[name] is False})
    blocked = bool(codes & {'external_storage', 'missing_read_branch', 'dictionary_unavailable'})
    if blocked:
        require(not c['checked_branches'] and not c['checked_entries'] and c['entry_range'] is None and not c['complete_configured_read'] and 'coverage' not in c); return
    count = min(n, p['content']['sample_entries'])
    indices = sorted({i * (n - 1) // (count - 1) for i in range(count)}) if count > 1 else ([0] if count else [])
    require('coverage' in c or interrupted)
    if 'coverage' not in c: require(not c['complete_configured_read']); return
    require(isinstance(c['coverage'], dict) and set(c['coverage']) == set(requested))
    done = c.get('entries_checked', 0)
    require(done <= (n if c['mode'] == 'full' else count))
    partial = 'inspection_exception' in codes or 'content_unreadable' in codes
    read_branches = set()
    for name, coverage in c['coverage'].items():
        fields(coverage, 'coverage', ('entries', 'entry_range'))
        require(isinstance(coverage['entries'], list) and all(type(x) is int for x in coverage['entries']) and interval(coverage['entry_range']))
        if c['mode'] == 'sample':
            length = len(coverage['entries'])
            require(coverage['entry_range'] is None and coverage['entries'] == indices[:length] and done <= length <= done + int(partial))
        else:
            require(not coverage['entries'])
            length = coverage['entry_range'][1] if coverage['entry_range'] is not None else 0
            require(done <= length <= done + int(partial) and length <= n)
        if length: read_branches.add(name)
    require(set(c['checked_branches']) == read_branches)
    require(c['checked_entries'] == (indices[:done] if c['mode'] == 'sample' else []))
    require(c['entry_range'] == ([0, done] if c['mode'] == 'full' and done else None))
    require(c['complete_configured_read'] == (c['mode'] == 'full' and not partial))
    if not partial: require(done == (n if c['mode'] == 'full' else count) and 'entries_checked' in c)


def strict_worker_json(text):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result: raise ValueError('duplicate worker JSON key')
            result[key] = value
        return result
    def invalid(value): raise ValueError('nonfinite worker JSON number')
    # The protocol contains no floating-point evidence.
    return json.loads(text, object_pairs_hook=pairs, parse_constant=invalid, parse_float=invalid)


def worker(path, selected, timeout, memory_mb):
    request = dict(path=str(path), profile=selected, memory_mb=memory_mb, cpu_seconds=timeout)
    try:
        run = subprocess.run([sys.executable, str(Path(__file__).with_name('root_output_worker.py'))],
                             input=json.dumps(request), text=True, capture_output=True, timeout=timeout,
                             env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
    except subprocess.TimeoutExpired:
        return dict(issues=[dict(code='inspection_timeout', status='UNKNOWN', detail='wall timeout')], structure={}, content={}, diagnostics='')
    if run.returncode:
        return dict(issues=[dict(code='inspection_crash', status='UNKNOWN', detail=f'worker exit {run.returncode}')], structure={}, content={}, diagnostics=run.stderr[-4000:])
    try:
        result = strict_worker_json(run.stdout)
        validate_worker_result(result, selected)
        result['diagnostics'] = run.stderr[-4000:]
        return result
    except (ValueError, TypeError):
        return dict(issues=[dict(code='worker_protocol', status='UNKNOWN', detail='invalid worker JSON')], structure={}, content={}, diagnostics=run.stderr[-4000:])


def metadata(path):
    s = path.stat()
    return dict(size_bytes=s.st_size, mtime_ns=s.st_mtime_ns, ctime_ns=s.st_ctime_ns, device=s.st_dev, inode=s.st_ino, mode=s.st_mode)


def validate_outputs(value, profiles, local_root, timeout=20, memory_mb=4096):
    if type(timeout) is not int or not 1 <= timeout <= 60 or type(memory_mb) is not int or not 512 <= memory_mb <= 8192:
        raise ValueError('timeout 1..60 seconds, memory 512..8192 MiB required')
    if '://' in str(local_root) or Path('/eos') in Path(local_root).parents or Path(local_root) == Path('/eos'):
        raise ValueError('EOS/network local root rejected')
    root = Path(local_root).resolve(strict=True)
    if not root.is_dir() or root == Path('/eos') or Path('/eos') in root.parents:
        raise ValueError('approved existing local root required; EOS rejected')
    for key, p in profiles.items():
        profile(p)
        if key != p['profile_id']: raise ValueError('profile map key differs from profile ID')
    inventory(value, profiles, root)
    results = []
    for entry in value['outputs']:
        p = profiles[entry['profile_id']]; path = safe_path(entry['path'], root)
        r = dict(identity=dict(entry), inspected_path=str(path), profile=dict(id=p['profile_id'], version=1, definition=p),
                 file={}, structure={}, content={}, issues=[], provenance=dict(status='UNKNOWN', association='caller_declared'),
                 acceptance='not_evaluated', diagnostics='')
        try:
            before = metadata(path); r['file'] = dict(before)
            if not stat.S_ISREG(before['mode']): issue(r, 'nonregular_file', 'FAIL', 'not a regular file')
            elif not before['size_bytes']: issue(r, 'zero_byte_file', 'FAIL', 'empty file')
            else:
                # Check filesystem readability separately from ROOT dependency/inspection.
                with path.open('rb') as stream: stream.read(1)
                inspected = worker(path, p, timeout, memory_mb)
                r.update(structure=inspected['structure'], content=inspected['content'], diagnostics=inspected.get('diagnostics', ''))
                r['issues'].extend(inspected['issues'])
                after = metadata(path); r['file']['after'] = after
                if before != after: issue(r, 'file_changed', 'UNKNOWN', 'metadata changed during inspection')
        except FileNotFoundError:
            issue(r, 'file_changed' if r['file'] else 'missing_file', 'UNKNOWN' if r['file'] else 'FAIL', 'file absent')
        except OSError as exc:
            issue(r, 'filesystem_access', 'UNKNOWN', str(exc))
        statuses = {i['status'] for i in r['issues']}
        r['status'] = 'FAIL' if 'FAIL' in statuses else 'UNKNOWN' if statuses else 'PASS'
        codes = {i['code'] for i in r['issues']}
        accessible = bool(r['file']) and not codes & {'nonregular_file', 'zero_byte_file', 'filesystem_access', 'file_changed'}
        structural = {'missing_tree', 'invalid_entry_count', 'zero_entries_forbidden', 'missing_branch', 'branch_type_mismatch', 'missing_object', 'object_type_mismatch'}
        content_complete = r['content'].get('complete_configured_read') or (
            p['content']['mode'] == 'sample' and r['content'].get('entries_checked') == min(r['structure'].get('entries', -1), p['content']['sample_entries']))
        r['checks'] = dict(
            accessibility='PASS' if accessible else 'FAIL' if codes & {'missing_file', 'nonregular_file', 'zero_byte_file'} else 'UNKNOWN',
            file_integrity=dict(status='FAIL' if codes & {'root_open_failed', 'recovered_file', 'zero_byte_file'} else 'PASS' if r['structure'].get('opened') and 'file_changed' not in codes else 'UNKNOWN', scope='size/open/recovery checks only; no checksum or whole-file integrity proof'),
            root_structure='FAIL' if codes & structural else 'PASS' if 'entries' in r['structure'] and not codes & {'inspection_exception', 'close_exception', 'file_changed'} else 'UNKNOWN',
            content_readability='FAIL' if 'content_unreadable' in codes else 'PASS' if content_complete and not codes & {'inspection_exception', 'close_exception', 'file_changed'} else 'UNKNOWN',
            provenance='UNKNOWN', campaign_acceptance='not_evaluated')
        results.append(r)
    counts = dict(Counter(r['status'] for r in results))
    return dict(schema_version=1, kind='root_output_validation_report', outputs=results, summary=counts,
                limits=dict(timeout_seconds_per_output=timeout, memory_mib_per_worker=memory_mb, max_outputs=100),
                acceptance_changed=False, status='FAIL' if counts.get('FAIL') else 'UNKNOWN' if counts.get('UNKNOWN') else 'PASS')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', required=True); parser.add_argument('--profile', action='append', required=True)
    parser.add_argument('--local-root', required=True); parser.add_argument('--timeout', type=int, default=20)
    parser.add_argument('--memory-mb', type=int, default=4096)
    args = parser.parse_args(argv)
    try:
        profiles = {}
        for path in args.profile:
            p = profile(load_json(path))
            if p['profile_id'] in profiles: raise ValueError('duplicate profile ID')
            profiles[p['profile_id']] = p
        result = validate_outputs(load_json(args.inventory), profiles, args.local_root, args.timeout, args.memory_mb)
        print(json.dumps(result, indent=2, allow_nan=False))
        return {'PASS': 0, 'FAIL': 1, 'UNKNOWN': 3}[result['status']]
    except (ValueError, OSError, TypeError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr); return 2


if __name__ == '__main__': sys.exit(main())
