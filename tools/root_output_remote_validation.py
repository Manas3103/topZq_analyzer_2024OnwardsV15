#!/usr/bin/env python3
"""Explicit read-only XRootD evidence. Never selects campaign acceptance."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import re
import selectors
import signal
import subprocess
import sys
import time
from urllib.parse import urlsplit

import root_output_validation as local

DIAGNOSTIC_LIMIT = 4000
OUTPUT_LIMIT = 1024 * 1024


class OutputLimitError(ValueError):
    def __init__(self, stdout, stderr):
        super().__init__('subprocess output exceeds supported limit')
        self.output, self.stderr = bytes(stdout), bytes(stderr)


def diagnostic(value):
    if isinstance(value, bytes): value = value.decode('utf-8', errors='replace')
    return (value or '')[-DIAGNOSTIC_LIMIT:]


def now():
    return datetime.now(timezone.utc).isoformat()


def split_locator(value, prefix=False):
    """Strict deliberately small root:// DNS-host[/port] absolute-path syntax."""
    if not isinstance(value, str) or not value.startswith('root://') or not value.isascii() or any(ord(c) < 33 or ord(c) == 127 for c in value):
        raise ValueError('exact root:// locator required')
    u = urlsplit(value)
    if u.scheme != 'root' or u.username is not None or u.password is not None or u.query or u.fragment or '?' in value or '#' in value:
        raise ValueError('credentials, queries, fragments or unsupported scheme')
    if not re.fullmatch(r'[A-Za-z0-9](?:[A-Za-z0-9.-]*[A-Za-z0-9])?(?::[1-9][0-9]{0,4})?', u.netloc):
        raise ValueError('DNS host and optional canonical port required')
    if u.port is not None and not 1 <= u.port <= 65535:
        raise ValueError('invalid port')
    if not u.path.startswith('//'):
        raise ValueError('double slash and absolute namespace required')
    namespace = u.path[1:]
    if not re.fullmatch(r'/[A-Za-z0-9._/-]+', namespace):
        raise ValueError('encoded, wildcard or ambiguous namespace')
    parts = namespace[1:].split('/')
    if prefix:
        if not namespace.endswith('/'):
            raise ValueError('approved prefix must end with slash')
        parts = parts[:-1]
    if not parts or any(p in ('', '.', '..') for p in parts):
        raise ValueError('empty or traversal namespace components')
    host = u.hostname.lower()
    port = u.port or 1094
    authority = host if port == 1094 else f'{host}:{port}'
    endpoint = f'root://{authority}'
    return dict(original=value, endpoint=endpoint, host=host, port=port,
                namespace=namespace, normalized=endpoint + '/' + namespace)


def remote_inventory(value, profiles, approved_prefixes, read_mode='metadata', allow_full=False):
    local.fields(value, 'remote inventory', ('schema_version', 'kind', 'outputs'))
    local.version(value, 'remote_root_output_inventory')
    if not isinstance(approved_prefixes, list) or not approved_prefixes:
        raise ValueError('explicit approved endpoint/namespace prefixes required')
    approvals = [split_locator(p, prefix=True) for p in approved_prefixes]
    if read_mode not in ('metadata', 'sample', 'full') or type(allow_full) is not bool or read_mode == 'full' and not allow_full:
        raise ValueError('full remote reads require --read-mode full and --allow-full-read')
    for key, p in profiles.items():
        local.profile(p)
        if key != p['profile_id']: raise ValueError('profile map key mismatch')
    outputs = value['outputs']
    if not isinstance(outputs, list) or not 1 <= len(outputs) <= 100:
        raise ValueError('remote inventory requires 1..100 records')
    attempts, locators, jobs, datasets, chunks, indices = set(), set(), set(), {}, {}, {}
    parsed = []
    for e in outputs:
        local.fields(e, 'remote output', ('campaign_id', 'workflow', 'dataset_id', 'sample_name', 'datatype', 'chunk_id', 'index', 'attempt_id', 'locator', 'profile_id'), ('job_id', 'evidence'))
        for key in ('campaign_id', 'dataset_id', 'chunk_id', 'attempt_id'):
            if not isinstance(e[key], str) or not local.ID.fullmatch(e[key]): raise ValueError(f'invalid {key}')
        if not isinstance(e['sample_name'], str) or not e['sample_name'] or type(e['index']) is not int or e['index'] < 0:
            raise ValueError('sample name/nonnegative chunk index required')
        p = profiles.get(e['profile_id'])
        if p is None or (p['workflow'], p['datatype']) != (e['workflow'], e['datatype']) or p['content']['mode'] != read_mode:
            raise ValueError('missing/mismatched profile or explicit read mode')
        aid = (e['campaign_id'], e['workflow'], e['attempt_id'])
        if aid in attempts: raise ValueError('duplicate/contradictory attempt identity')
        attempts.add(aid)
        locator = split_locator(e['locator'])
        if not any(locator['endpoint'] == a['endpoint'] and locator['namespace'].startswith(a['namespace']) for a in approvals):
            raise ValueError('unapproved endpoint/namespace')
        if locator['normalized'] in locators: raise ValueError('duplicate normalized locator')
        locators.add(locator['normalized'])
        jid = e.get('job_id')
        if jid is not None:
            if not isinstance(jid, str) or not re.fullmatch(r'(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)', jid) or jid in jobs:
                raise ValueError('invalid/duplicate job ID')
            jobs.add(jid)
        refs = e.get('evidence', [])
        if not isinstance(refs, list) or any(not isinstance(x, str) or not x.strip() for x in refs): raise ValueError('invalid evidence references')
        d = (e['campaign_id'], e['workflow'], e['dataset_id'])
        c, idx = d + (e['chunk_id'],), d + (e['index'],)
        declaration = (e['datatype'], e['sample_name'])
        if d in datasets and datasets[d] != declaration or c in chunks and chunks[c] != e['index'] or idx in indices and indices[idx] != e['chunk_id']:
            raise ValueError('contradictory dataset/chunk declaration')
        datasets[d], chunks[c], indices[idx] = declaration, e['index'], e['chunk_id']
        parsed.append(locator)
    return parsed


def run_bounded(args, timeout, input_text=None):
    """Bound capture while draining both pipes; kill the process group on exit."""
    if os.name != 'posix': raise OSError('remote process isolation requires POSIX')
    if not math.isfinite(timeout) or timeout <= 0: raise ValueError('positive finite timeout required')
    data = input_text.encode('utf-8') if input_text is not None else None
    if data is not None and len(data) > OUTPUT_LIMIT: raise ValueError('worker request exceeds supported limit')
    deadline = time.monotonic() + timeout
    proc = subprocess.Popen(args, stdin=subprocess.PIPE if input_text is not None else subprocess.DEVNULL,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            start_new_session=True, env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
    selector = selectors.DefaultSelector()
    stdout, stderr = bytearray(), bytearray()
    position = 0
    def kill_group():
        try: os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError: pass
    try:
        selector.register(proc.stdout, selectors.EVENT_READ, stdout)
        selector.register(proc.stderr, selectors.EVENT_READ, stderr)
        if data:
            os.set_blocking(proc.stdin.fileno(), False)
            selector.register(proc.stdin, selectors.EVENT_WRITE, None)
        elif proc.stdin:
            proc.stdin.close()
        while selector.get_map():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise subprocess.TimeoutExpired(args, timeout, output=bytes(stdout), stderr=bytes(stderr))
            for key, _ in selector.select(min(remaining, 0.1)):
                if key.fileobj is proc.stdin:
                    try:
                        position += os.write(proc.stdin.fileno(), data[position:position + 4096])
                    except BlockingIOError:
                        continue
                    except BrokenPipeError:
                        position = len(data)
                    if position == len(data):
                        selector.unregister(proc.stdin)
                        proc.stdin.close()
                    continue
                block = os.read(key.fileobj.fileno(), 65536)
                if not block:
                    selector.unregister(key.fileobj)
                    continue
                if len(stdout) + len(stderr) + len(block) > OUTPUT_LIMIT:
                    raise OutputLimitError(stdout, stderr)
                key.data.extend(block)
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise subprocess.TimeoutExpired(args, timeout, output=bytes(stdout), stderr=bytes(stderr))
        try:
            proc.wait(timeout=remaining)
        except subprocess.TimeoutExpired:
            raise subprocess.TimeoutExpired(args, timeout, output=bytes(stdout), stderr=bytes(stderr)) from None
        return subprocess.CompletedProcess(args, proc.returncode, stdout.decode('utf-8'), stderr.decode('utf-8'))
    finally:
        # Also kills same-group descendants after ordinary completion/cancellation.
        kill_group()
        selector.close()
        for stream in (proc.stdin, proc.stdout, proc.stderr):
            if stream: stream.close()
        try: proc.wait(timeout=1)
        except subprocess.TimeoutExpired: pass  # uninterruptible kernel I/O


def error(code, status, detail):
    return dict(code=code, status=status, detail=detail)


def budget(limit, deadline):
    return max(0.0, min(limit, deadline - time.monotonic()))


def parse_stat(text, namespace):
    values = {}
    for line in text.splitlines():
        if not line.strip(): continue
        if ':' not in line: raise ValueError('stat line lacks field separator')
        key, value = (part.strip() for part in line.split(':', 1))
        if key not in ('Path', 'Id', 'Size', 'Flags', 'MTime', 'Modtime', 'CTime', 'ATime', 'Mode', 'Owner', 'Group') or key in values or not value:
            raise ValueError('unknown/duplicate/empty stat field')
        values[key] = value
    if not {'Size', 'Flags'} <= set(values) or not re.fullmatch(r'[0-9]+', values['Size']):
        raise ValueError('stat size/flags missing or malformed')
    if 'Path' in values and values['Path'] != namespace: raise ValueError('stat path contradicts requested namespace')
    if 'Id' in values and not re.fullmatch(r'[0-9]+', values['Id']): raise ValueError('malformed stat ID')
    if 'MTime' in values and 'Modtime' in values: raise ValueError('ambiguous modification time')
    if 'Mode' in values and not re.fullmatch(r'[0-7]{3,7}', values['Mode']): raise ValueError('malformed stat mode')
    for key in ('Owner', 'Group'):
        if key in values and not re.fullmatch(r'[A-Za-z0-9_.@-]+', values[key]): raise ValueError('malformed stat owner/group')
    match = re.fullmatch(r'([0-9]+)\s*\(([^()]*)\)', values['Flags'])
    if not match: raise ValueError('unsupported stat flags format')
    flags = [v.strip() for v in match[2].split('|')] if '|' in match[2] else match[2].split()
    known = {'XBitSet', 'IsDir', 'Other', 'Offline', 'POSCPending', 'IsReadable', 'IsWriteable', 'IsWritable', 'BackUpExists'}
    if len(set(flags)) != len(flags) or not set(flags) <= known: raise ValueError('unsupported/duplicate stat flag')
    bits = dict(XBitSet=1, IsDir=2, Other=4, Offline=8, IsReadable=16, IsWriteable=32, IsWritable=32, POSCPending=64, BackUpExists=128)
    if len({bits[f] for f in flags}) != len(flags) or int(match[1]) != sum(bits[f] for f in flags):
        raise ValueError('numeric/text stat flags disagree')
    return dict(size_bytes=int(values['Size']), reported_id=values.get('Id'), flags=flags,
                flags_value=int(match[1]), reported_mtime=values.get('MTime', values.get('Modtime')),
                reported_path=values.get('Path'), reported_ctime=values.get('CTime'), reported_atime=values.get('ATime'),
                reported_mode=values.get('Mode'), reported_owner=values.get('Owner'), reported_group=values.get('Group'))


def parse_checksum(text):
    parts = text.split()
    lengths = {'adler32': 8, 'sha256': 64, 'md5': 32}
    if len(parts) != 2 or parts[0].lower() not in lengths:
        raise ValueError('unsupported checksum algorithm/format')
    algorithm, value = parts[0].lower(), parts[1].lower()
    if not re.fullmatch('[0-9a-f]{' + str(lengths[algorithm]) + '}', value):
        raise ValueError('malformed checksum value')
    return dict(algorithm=algorithm, value=value, source='server_reported', independently_verified=False)


def classify_query_error(stdout, stderr, locator=None):
    text = stdout + '\n' + stderr
    if locator is not None:
        # Requested operands can contain words such as "proxy" or "timeout";
        # classify the response, not the echoed filename.
        for operand in (locator['normalized'], locator['original'], locator['namespace'], locator['endpoint']):
            text = text.replace(operand, '')
    codes = set(re.findall(r'\[([0-9]+)\]', text))
    if codes & {'3010', '3030'} or re.search(r'authenticat|authoriz|permission denied|credential|proxy', text, re.I):
        return error('remote_authentication', 'UNKNOWN', 'authorization/authentication unavailable')
    if re.search(r'resolve|DNS|socket|network|connection|timeout|timed\s*out', text, re.I):
        return error('remote_network', 'UNKNOWN', 'network/name-resolution error')
    if codes == {'3011'}:
        return error('remote_missing', 'FAIL', 'server explicitly reports XRootD NotFound (3011)')
    return error('remote_query_error', 'UNKNOWN', 'ambiguous query failure')


def query(locator, kind, timeout, deadline):
    args = ['xrdfs', locator['endpoint']] + (['stat', locator['namespace']] if kind == 'stat' else ['query', 'checksum', locator['namespace']])
    if kind not in ('stat', 'checksum'): raise ValueError('unsupported read-only operation')
    started, monotonic_start = now(), time.monotonic()
    allotted = budget(timeout, deadline)
    result = dict(operation=kind, command=args, started_at=started, timeout_seconds=allotted,
                  returncode=None, data=None, issues=[], stdout_tail='', stderr_tail='')
    try:
        if allotted <= 0: raise TimeoutError('output/inventory deadline exhausted')
        run = run_bounded(args, allotted)
        result.update(returncode=run.returncode, stdout_tail=run.stdout[-DIAGNOSTIC_LIMIT:], stderr_tail=run.stderr[-DIAGNOSTIC_LIMIT:])
        if len(run.stdout) + len(run.stderr) > OUTPUT_LIMIT: raise ValueError('query output exceeds supported limit')
        if run.returncode:
            issue = classify_query_error(run.stdout, run.stderr, locator)
            # A checksum failure cannot itself establish the requested file's absence.
            if kind == 'checksum' and issue['status'] == 'FAIL': issue = error('checksum_unavailable', 'UNKNOWN', issue['detail'])
            result['issues'].append(issue)
        else:
            result['data'] = parse_stat(run.stdout, locator['namespace']) if kind == 'stat' else parse_checksum(run.stdout)
    except subprocess.TimeoutExpired as exc:
        result.update(stdout_tail=diagnostic(exc.output), stderr_tail=diagnostic(exc.stderr))
        result['issues'].append(error('remote_query_timeout', 'UNKNOWN', 'command wall timeout'))
    except TimeoutError as exc:
        result['issues'].append(error('validation_deadline', 'UNKNOWN', str(exc)))
    except OSError as exc:
        result['issues'].append(error('remote_dependency_or_execution', 'UNKNOWN', str(exc)))
    except OutputLimitError as exc:
        result.update(stdout_tail=diagnostic(exc.output), stderr_tail=diagnostic(exc.stderr))
        result['issues'].append(error('remote_metadata_protocol', 'UNKNOWN', str(exc)))
    except (ValueError, UnicodeError) as exc:
        result['issues'].append(error('remote_metadata_protocol', 'UNKNOWN', str(exc)))
    result.update(ended_at=now(), elapsed_seconds=time.monotonic() - monotonic_start)
    return result


def remote_worker(locator, p, timeout, memory_mb, deadline):
    allotted = budget(timeout, deadline)
    empty = dict(structure={}, content={}, issues=[], diagnostics='', started_at=now())
    started = time.monotonic()
    try:
        if allotted <= 0: raise TimeoutError('output/inventory deadline exhausted')
        request = dict(path=locator['normalized'], profile=p, memory_mb=memory_mb,
                       cpu_seconds=max(1, math.ceil(allotted)), transport='remote')
        args = [sys.executable, str(Path(local.__file__).with_name('root_output_worker.py'))]
        run = run_bounded(args, allotted, json.dumps(request))
        empty['diagnostics'] = run.stderr[-DIAGNOSTIC_LIMIT:]
        if run.returncode:
            empty['issues'] = [error('inspection_crash', 'UNKNOWN', f'worker exit {run.returncode}')]
        else:
            if len(run.stdout) + len(run.stderr) > OUTPUT_LIMIT: raise ValueError('worker output exceeds supported limit')
            r = local.strict_worker_json(run.stdout)
            local.validate_worker_result(r, p, transport='remote')
            empty.update(structure=r['structure'], content=r['content'], issues=r['issues'])
    except subprocess.TimeoutExpired as exc:
        empty['diagnostics'] = diagnostic(exc.stderr)
        empty['issues'] = [error('inspection_timeout', 'UNKNOWN', 'worker wall timeout')]
    except TimeoutError as exc:
        empty['issues'] = [error('validation_deadline', 'UNKNOWN', str(exc))]
    except OSError as exc:
        empty['issues'] = [error('inspection_execution', 'UNKNOWN', str(exc))]
    except OutputLimitError as exc:
        empty['diagnostics'] = diagnostic(exc.stderr)
        empty['issues'] = [error('worker_protocol', 'UNKNOWN', str(exc))]
    except (ValueError, TypeError, UnicodeError) as exc:
        empty['issues'] = [error('worker_protocol', 'UNKNOWN', str(exc))]
    empty.update(ended_at=now(), elapsed_seconds=time.monotonic() - started, timeout_seconds=allotted)
    return empty


def status(issues):
    return 'FAIL' if any(i['status'] == 'FAIL' for i in issues) else 'UNKNOWN' if issues else 'PASS'


def validate_remote(value, profiles, approved_prefixes, read_mode='metadata', allow_full=False,
                    require_checksum=False, command_timeout=10, worker_timeout=20,
                    output_timeout=60, inventory_timeout=None, memory_mb=4096):
    for label, limit, maximum in [('command', command_timeout, 60), ('worker', worker_timeout, 60),
                                  ('output', output_timeout, 600), ('inventory', inventory_timeout, 3600)]:
        if limit is None and label == 'inventory': continue
        if type(limit) is not int or not 1 <= limit <= maximum: raise ValueError(f'invalid {label} timeout')
    if type(memory_mb) is not int or not 512 <= memory_mb <= 8192 or type(require_checksum) is not bool:
        raise ValueError('invalid memory/checksum policy')
    locators = remote_inventory(value, profiles, approved_prefixes, read_mode, allow_full)
    started = now()
    inventory_deadline = time.monotonic() + inventory_timeout if inventory_timeout else float('inf')
    outputs = []
    for entry, locator in zip(value['outputs'], locators):
        deadline = min(time.monotonic() + output_timeout, inventory_deadline)
        p = profiles[entry['profile_id']]
        r = dict(identity=dict(entry), locator=locator, profile=dict(id=p['profile_id'], version=1, definition=p),
                 queries=[], root=None, consistency={}, issues=[], provenance=dict(status='UNKNOWN', association='caller_declared'),
                 acceptance='not_evaluated', started_at=now())
        before = query(locator, 'stat', command_timeout, deadline)
        r['queries'].append(before); r['issues'].extend(before['issues'])
        data = before['data']
        usable = data is not None
        if data:
            flags = set(data['flags'])
            if flags & {'IsDir', 'Other'}:
                r['issues'].append(error('remote_nonregular', 'FAIL', 'namespace target is not a regular file')); usable = False
            elif data['size_bytes'] == 0:
                r['issues'].append(error('remote_zero_byte', 'FAIL', 'zero-byte remote file')); usable = False
            elif flags & {'Offline', 'POSCPending'} or 'IsReadable' not in flags:
                r['issues'].append(error('remote_inaccessible', 'UNKNOWN', 'not online/readable; no recall requested')); usable = False
        if usable:
            checksum_before = query(locator, 'checksum', command_timeout, deadline) if require_checksum else None
            if checksum_before:
                r['queries'].append(checksum_before); r['issues'].extend(checksum_before['issues'])
            inspected = remote_worker(locator, p, worker_timeout, memory_mb, deadline)
            r['root'] = inspected; r['issues'].extend(inspected['issues'])
            after = query(locator, 'stat', command_timeout, deadline)
            r['queries'].append(after)
            for i in after['issues']:
                # Disappearance after initial observation is mutation/incomplete evidence.
                r['issues'].append(error('remote_changed' if i['status'] == 'FAIL' else i['code'], 'UNKNOWN', i['detail']))
            if after['data'] is not None:
                comparable = ('size_bytes', 'reported_id', 'reported_mtime', 'reported_ctime',
                              'reported_mode', 'reported_owner', 'reported_group', 'flags', 'flags_value')
                r['consistency']['metadata'] = 'consistent' if all(data[k] == after['data'][k] for k in comparable) else 'changed'
                if r['consistency']['metadata'] == 'changed': r['issues'].append(error('remote_changed', 'UNKNOWN', 'pre/post metadata differs'))
            else: r['consistency']['metadata'] = 'unresolved'
            if require_checksum:
                checksum_after = query(locator, 'checksum', command_timeout, deadline)
                r['queries'].append(checksum_after); r['issues'].extend(checksum_after['issues'])
                first, last = checksum_before['data'], checksum_after['data']
                r['consistency']['checksum'] = 'unresolved' if first is None or last is None else 'consistent' if first == last else 'changed'
                if r['consistency']['checksum'] == 'changed': r['issues'].append(error('remote_changed', 'UNKNOWN', 'pre/post checksum differs'))
        else: r['consistency']['metadata'] = 'not_rechecked'
        if not require_checksum: r['consistency']['checksum'] = 'not_requested'
        if time.monotonic() >= deadline and not any(i['code'] == 'validation_deadline' for i in r['issues']):
            r['issues'].append(error('validation_deadline', 'UNKNOWN', 'output/inventory deadline exhausted'))
        root = r['root'] or dict(structure={}, content={}, issues=[])
        s, c = root['structure'], root['content']
        root_codes = {i['code'] for i in root['issues']}
        codes = {i['code'] for i in r['issues']}
        unstable = bool(codes & {'remote_changed', 'validation_deadline'})
        structure_fail = bool(root_codes & {'missing_tree', 'invalid_entry_count', 'zero_entries_forbidden', 'missing_branch', 'branch_type_mismatch', 'missing_object', 'object_type_mismatch', 'recovered_file'})
        structure_complete = {'opened', 'entries', 'branch_types', 'objects', 'object_matches', 'dictionaries', 'dictionary_dependent_branches'} <= set(s)
        coverage_complete = c.get('complete_configured_read') is True or read_mode == 'sample' and c.get('entries_checked') == min(s.get('entries', -1), p['content']['sample_entries'])
        checksum_queries = [q for q in r['queries'] if q['operation'] == 'checksum']
        checksum_ok = len(checksum_queries) == 2 and all(q['data'] is not None for q in checksum_queries) and r['consistency'].get('checksum') == 'consistent'
        r['checks'] = dict(
            transport_access='FAIL' if codes & {'remote_missing', 'remote_nonregular', 'remote_zero_byte'} else 'PASS' if usable and s.get('opened') and not unstable and 'root_open_failed' not in root_codes else 'UNKNOWN',
            remote_metadata='FAIL' if codes & {'remote_missing', 'remote_nonregular', 'remote_zero_byte'} else 'PASS' if r['consistency']['metadata'] == 'consistent' else 'UNKNOWN',
            checksum='not_requested' if not require_checksum else 'PASS' if checksum_ok and not unstable else 'UNKNOWN',
            root_structure='FAIL' if structure_fail else 'PASS' if structure_complete and not root_codes & {'inspection_exception', 'close_exception'} and not unstable else 'UNKNOWN',
            content_readability='PASS' if coverage_complete and not root['issues'] and not unstable else 'UNKNOWN',
            provenance='UNKNOWN', campaign_acceptance='not_evaluated')
        # GetSize disagreement is diagnostic only, not an assumed integrity invariant.
        if 'root_file_size' in s:
            r['root_size_diagnostic'] = dict(root_reported=s['root_file_size'], remote_stat=data['size_bytes'], equality_enforced=False)
        r.update(status=status(r['issues']), ended_at=now())
        outputs.append(r)
    counts = dict(Counter(r['status'] for r in outputs))
    return dict(schema_version=1, kind='remote_root_output_validation_report', outputs=outputs, summary=counts,
                status=status([i for r in outputs for i in r['issues']]), started_at=started, ended_at=now(),
                approvals=approved_prefixes, limits=dict(command_timeout=command_timeout, worker_timeout=worker_timeout,
                output_timeout=output_timeout, inventory_timeout=inventory_timeout, memory_mb=memory_mb, max_outputs=100),
                acceptance_changed=False)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', required=True)
    parser.add_argument('--profile', action='append', required=True)
    parser.add_argument('--allow-prefix', action='append', required=True)
    parser.add_argument('--read-mode', choices=('metadata', 'sample', 'full'), default='metadata')
    parser.add_argument('--allow-full-read', action='store_true')
    parser.add_argument('--require-checksum', action='store_true')
    parser.add_argument('--command-timeout', type=int, default=10)
    parser.add_argument('--worker-timeout', type=int, default=20)
    parser.add_argument('--output-timeout', type=int, default=60)
    parser.add_argument('--inventory-timeout', type=int)
    parser.add_argument('--memory-mb', type=int, default=4096)
    args = parser.parse_args(argv)
    try:
        profiles = {}
        for path in args.profile:
            p = local.profile(local.load_json(path))
            if p['profile_id'] in profiles: raise ValueError('duplicate profile ID')
            profiles[p['profile_id']] = p
        report = validate_remote(local.load_json(args.inventory), profiles, args.allow_prefix, args.read_mode,
                                 args.allow_full_read, args.require_checksum, args.command_timeout,
                                 args.worker_timeout, args.output_timeout, args.inventory_timeout, args.memory_mb)
        print(json.dumps(report, indent=2, allow_nan=False))
        return {'PASS': 0, 'FAIL': 1, 'UNKNOWN': 3}[report['status']]
    except (ValueError, TypeError, OSError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr); return 2


if __name__ == '__main__': sys.exit(main())
