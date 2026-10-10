"""Read-only acceptance checks. No ROOT imports, directory discovery or writes."""
import hashlib
from pathlib import Path
import stat


def check_chunks(expected, inventory, local_root):
    from campaign import validate
    validate(expected)
    validate(inventory)
    dataset = validate(expected['dataset'])
    root = Path(local_root).resolve(strict=True)
    if not root.is_dir() or root == Path('/eos') or Path('/eos') in root.parents:
        raise ValueError('local-root must be an existing local directory outside /eos')
    identity = dict(campaign_id=expected['campaign_id'], dataset_id=dataset['dataset_id'], workflow=dataset['workflow'])
    issues, observations, accepted = [], [], []
    def issue(code, detail):
        issues.append(dict(code=code, detail=detail))
    if any(inventory[k] != v for k, v in identity.items()):
        issue('wrong_identity', 'inventory campaign/dataset/workflow differs from expected plan')
    chunks = {c['chunk_id']: c for c in dataset['chunks']}
    if not chunks:
        issue('empty_plan', 'no expected chunks')
    seen_attempts, paths = set(), set()
    candidates = {k: [] for k in chunks}
    for entry in inventory['attempts']:
        attempt = validate(entry['attempt'])
        cid, aid = entry['chunk_id'], attempt['attempt_id']
        key = (cid, aid)
        if key in seen_attempts:
            issue('duplicate_attempt', str(key))
        seen_attempts.add(key)
        observations.append(dict(chunk_id=cid, attempt_id=aid, status=attempt['status'], exit_code=attempt['exit_code'], accepted=entry['accepted']))
        valid = True
        def reject(code, detail):
            nonlocal valid
            valid = False
            issue(code, f'{cid}/{aid}: {detail}')
        if cid not in chunks:
            reject('unexpected_chunk', 'not in expected plan')
        elif entry['index'] != chunks[cid]['index']:
            reject('duplicate_or_wrong_index', 'index does not match expected chunk identity')
        if entry['dataset_id'] != identity['dataset_id'] or any(attempt[k] != identity[k] for k in ('campaign_id','workflow')):
            reject('wrong_identity', 'attempt campaign/dataset/workflow mismatch')
        if cid in chunks and attempt['ordered_inputs'] != chunks[cid]['ordered_inputs']:
            reject('wrong_inputs', 'ordered input list differs from submission plan')
        # Failed/rejected retries are recorded, not silently accepted or made fatal.
        if not entry['accepted']:
            continue
        if attempt['status'] != 'validated' or attempt['exit_code'] != 0:
            reject('failed_attempt', 'accepted attempt must be validated with exit 0')
        processing = attempt['runtime_effective'].get('processing', {})
        if processing.get('state') != 'verified' or processing.get('value') != 'PASS':
            reject('unverified_processing', 'verified processing PASS required')
        output = entry['output']
        if output['validation']['state'] != 'verified' or output['validation']['result'] != 'PASS':
            reject('unverified_output', 'external ROOT integrity/schema validation PASS required')
        path = Path(output['path'])
        if path.is_absolute() or '..' in path.parts or '://' in output['path']:
            reject('unsafe_path', 'output must be relative to local-root')
        else:
            try:
                path = (root / path).resolve(strict=True)
                if root not in path.parents:
                    reject('unsafe_path', 'resolved output escapes local-root')
                elif not stat.S_ISREG(path.stat().st_mode):
                    reject('invalid_file', 'not a regular file')
                else:
                    with path.open('rb') as stream:
                        metadata = __import__('os').fstat(stream.fileno())
                        if not stat.S_ISREG(metadata.st_mode):
                            reject('invalid_file', 'not a regular file')
                        elif metadata.st_size == 0:
                            reject('empty_output', 'zero-byte output')
                        else:
                            digest = hashlib.sha256()
                            for block in iter(lambda: stream.read(1024 * 1024), b''):
                                digest.update(block)
                            after = __import__('os').fstat(stream.fileno())
                            if (metadata.st_size,metadata.st_mtime_ns) != (after.st_size,after.st_mtime_ns):
                                reject('changed_output', 'file changed during checksum')
                            if metadata.st_size != output['size_bytes'] or digest.hexdigest() != output['sha256']:
                                reject('integrity_mismatch', 'size/checksum mismatch')
                            if path in paths:
                                reject('duplicate_output', 'same resolved file accepted more than once')
                            paths.add(path)
            except OSError as exc:
                reject('missing_or_unreadable_output', str(exc))
        if cid in candidates:
            candidates[cid].append(entry)
        if valid:
            accepted.append(dict(chunk_id=cid,index=entry['index'],attempt_id=aid,path=output['path'],size_bytes=output['size_bytes'],sha256=output['sha256']))
    for cid, entries in candidates.items():
        if not entries:
            issue('missing_chunk', cid + ': no accepted attempt')
        elif len(entries) != 1:
            issue('ambiguous_accepted_attempts', cid + ': multiple accepted attempts')
    accepted.sort(key=lambda a: (a['index'],a['chunk_id']))
    status = 'FAIL' if issues else 'PASS'
    result = dict(schema_version=1,kind='chunk_completeness_report',**identity,status=status,expected_count=len(chunks),accepted_count=len(accepted),issues=issues,attempts=observations)
    result['accepted_input_manifest'] = dict(schema_version=1,kind='accepted_merge_inputs',**identity,status=status,local_root=str(root),plan_source=expected['plan_source'],ordering='ascending expected chunk index',inputs=accepted if status == 'PASS' else [])
    validate(result)
    return result
