#!/usr/bin/env python3
"""Manifest-last publication. No acceptance, rename, overwrite, retry or cleanup."""
import hashlib
import json
import uuid
from pathlib import Path
import re
import time
import zlib

from campaign_process import CommandError, run_process
from campaign_production import endpoint, now, digest, validate, write_json, PREFIX, execution_path


class StagingError(RuntimeError):
    pass


def checksums(path, deadline=None):
    sha, adler, size = hashlib.sha256(), 1, 0
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            if deadline is not None and time.monotonic() >= deadline: raise StagingError('local checksum deadline exhausted')
            size += len(block); sha.update(block); adler = zlib.adler32(block, adler)
    return dict(size_bytes=size, sha256=sha.hexdigest(), adler32=f'{adler & 0xffffffff:08x}')


def stage(files, receipt_path, identity, destination, policy, validation, runner=run_process):
    """Require caller-verified no-clobber capability. Observe every uploaded artifact."""
    ep = endpoint(policy['endpoint'])
    expected = execution_path(identity['campaign_id'], identity['submission_id'], identity['workflow'], identity['dataset_id'], identity['chunk_id'], identity['execution_id'])
    if destination != expected: raise StagingError('destination/identity mismatch')
    if not destination.startswith(PREFIX) or any(x in ('', '.', '..') for x in destination[1:].split('/')) or not re.fullmatch(r'/[A-Za-z0-9._/-]+', destination):
        raise StagingError('unsafe remote destination')
    if policy['strategy'] != 'manifest_last' or not policy['no_clobber_confirmed'] or policy['checksum_algorithm'] not in ('adler32', 'sha256'):
        raise StagingError('endpoint publication capability not explicitly confirmed')
    if validation.get('status') != 'PASS' or validation.get('acceptance_changed') is not False:
        raise StagingError('local ROOT validation did not pass')
    if set(files) != {'output.root', 'processing.log', 'execution.json'}:
        raise StagingError('exact publication artifact set required')
    if len(validation.get('outputs', [])) != 1: raise StagingError('one validated output required')
    observed = validation['outputs'][0]
    root_path = Path(files['output.root'])
    if observed.get('status') != 'PASS' or observed.get('inspected_path') != str(root_path) or observed.get('acceptance') != 'not_evaluated':
        raise StagingError('validation path/status mismatch')
    for key in ('campaign_id', 'workflow', 'dataset_id', 'chunk_id', 'attempt_id'):
        if observed.get('identity', {}).get(key) != identity[key]: raise StagingError('validation identity mismatch')
    current = root_path.stat()
    expected_file = observed.get('file', {})
    for key, actual in [('size_bytes', current.st_size), ('mtime_ns', current.st_mtime_ns), ('ctime_ns', current.st_ctime_ns), ('inode', current.st_ino), ('device', current.st_dev)]:
        if expected_file.get(key) != actual: raise StagingError('output changed since ROOT validation')
    from campaign import load_json
    execution = validate(load_json(files['execution.json']))
    if execution.get('status') != 'validated': raise StagingError('execution evidence not validated')
    for key, value in identity.items():
        if execution.get(key) != value: raise StagingError('execution/publication identity mismatch')
    deadline = time.monotonic() + policy['overall_timeout']; evidence = []
    trace = Path(receipt_path).with_name('staging-' + uuid.uuid4().hex + '.json')
    write_json(trace, evidence)
    def command(args):
        remaining = min(policy['command_timeout'], deadline - time.monotonic())
        if remaining <= 0: raise StagingError('staging deadline exhausted')
        record = dict(command=args, started_at=now(), timeout_seconds=remaining)
        evidence.append(record)
        try:
            result = runner(args, remaining)
            if result.returncode: raise CommandError('nonzero_exit', 'nonzero endpoint result', result.returncode, result.stdout, result.stderr)
            record.update(returncode=0, stdout=result.stdout[-4000:], stderr=result.stderr[-4000:], ended_at=now())
            return result.stdout
        except CommandError as exc:
            record.update(returncode=exc.returncode, code=exc.code, stdout=exc.stdout[-4000:], stderr=exc.stderr[-4000:], ended_at=now())
            raise
        finally:
            record.setdefault('ended_at', now())
            trace.write_text(json.dumps(evidence, indent=2, allow_nan=False) + '\n')
    # Nonzero stat alone does not establish absence. Require sole explicit NotFound.
    try:
        command(['xrdfs', ep, 'stat', destination])
    except CommandError as exc:
        text = exc.stdout + '\n' + exc.stderr
        # XRootD includes the requested path in its diagnostic. Do not mistake
        # identity/path tokens (for example a submission ID containing
        # "timeout") for transport evidence; classify only the response text.
        diagnostic = text.replace(destination, '').replace(ep, '').replace('\x00', ' ')
        codes = set(re.findall(r'\[([0-9]+)\]', diagnostic))
        if exc.code != 'nonzero_exit' or codes != {'3011'} or re.search(r'network|connection|socket|resolve|auth|proxy|permission|timeout', diagnostic, re.I):
            raise StagingError('destination absence not established') from exc
    else: raise StagingError('remote execution destination already exists')
    command(['xrdfs', ep, 'mkdir', '-p', destination])
    artifacts = {}
    def upload(name, path):
        path = Path(path)
        if path.is_symlink() or not path.is_file(): raise StagingError('nonregular publication artifact')
        before = path.stat(); local = checksums(path, deadline)
        locator = ep + '/' + destination + '/' + name
        command(['xrdcp', str(path), locator])  # Deliberately no -f, --force or overwrite.
        stat = command(['xrdfs', ep, 'stat', destination + '/' + name])
        sizes = re.findall(r'^Size:\s*([0-9]+)\s*$', stat, re.M)
        paths = re.findall(r'^Path:\s*(\S+)\s*$', stat, re.M)
        flags = re.findall(r'^Flags:\s*([0-9]+)\s*\(([^()]*)\)\s*$', stat, re.M)
        if len(sizes) != 1 or int(sizes[0]) != local['size_bytes'] or len(flags) != 1:
            raise StagingError('remote size/flags verification failed')
        if paths and (len(paths) != 1 or paths[0] != destination + '/' + name): raise StagingError('remote path contradiction')
        mask = int(flags[0][0]); raw_names = flags[0][1].split(); names = set(raw_names)
        bits = dict(XBitSet=1, IsDir=2, Other=4, Offline=8, IsReadable=16, IsWriteable=32, IsWritable=32, POSCPending=64, BackUpExists=128)
        if len(raw_names) != len(names) or not names <= set(bits) or len({bits[n] for n in names}) != len(names) or sum(bits[n] for n in names) != mask:
            raise StagingError('remote flags inconsistent or unsupported')
        if mask & (2 | 4 | 8 | 64) or not mask & 16 or not 'IsReadable' in names or names & {'IsDir', 'Other', 'Offline', 'POSCPending'}:
            raise StagingError('remote artifact not online regular/readable')
        text = command(['xrdfs', ep, 'query', 'checksum', destination + '/' + name]).split()
        algorithm = policy['checksum_algorithm']; length = 8 if algorithm == 'adler32' else 64
        if len(text) != 2 or text[0].lower() != algorithm or not re.fullmatch('[0-9a-fA-F]{' + str(length) + '}', text[1]) or text[1].lower() != local[algorithm]:
            raise StagingError('remote checksum unavailable or inconsistent')
        after = path.stat()
        if (before.st_size, before.st_mtime_ns, before.st_ctime_ns) != (after.st_size, after.st_mtime_ns, after.st_ctime_ns) or checksums(path, deadline) != local:
            raise StagingError('local artifact changed during staging')
        return dict(locator=locator, **local, remote_checksum=dict(algorithm=algorithm, value=text[1].lower()))
    for name, path in files.items(): artifacts[name] = upload(name, path)
    receipt = dict(schema_version=1, kind='production_publication', **identity, created_at=now(),
                   status='published', acceptance='not_evaluated', destination=destination,
                   strategy='manifest_last', artifacts=artifacts, root_validation=validation,
                   checksum_algorithm=policy['checksum_algorithm'], observations=list(evidence))
    if 'input_scope' in execution: receipt['input_scope'] = execution['input_scope']
    validate(receipt); write_json(receipt_path, receipt)
    # This marker is published last. A failed/partial marker must never be trusted.
    marker = upload('publication.json', receipt_path)
    if time.monotonic() >= deadline: raise StagingError('publication verification deadline exhausted')
    return dict(receipt=receipt, marker=marker, observations=evidence)
