#!/usr/bin/env python3
"""Read-only scheduler monitoring; reports to stdout only."""
import argparse
from collections import Counter
import datetime
import hashlib
import json
import re
import subprocess
import sys

from campaign import load_json, validate

IDENTITY = ('campaign_id', 'workflow', 'dataset_id', 'chunk_id', 'attempt_id')
ATTRS = ('TZQCampaignId', 'TZQWorkflow', 'TZQDatasetId', 'TZQChunkId', 'TZQAttemptId')
STATES = {1: 'idle', 2: 'running', 3: 'removed', 4: 'completed', 5: 'held',
          6: 'transferring_output', 7: 'suspended'}


def envelope(value, kind, fields):
    if not isinstance(value, dict) or set(value) != set(fields) | {'schema_version', 'kind'}:
        raise ValueError(f'{kind}: required fields {fields}; unknown fields rejected')
    if type(value['schema_version']) is not int or value['schema_version'] != 1 or value['kind'] != kind:
        raise ValueError(f'unsupported {kind} schema/version')


def job_id(value):
    if not isinstance(value, str) or not re.fullmatch(r'(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)', value):
        raise ValueError('job_id must be canonical ClusterId.ProcId')
    return value


def registry(value):
    envelope(value, 'condor_monitor_input', ['expected', 'attempts'])
    if not isinstance(value['expected'], list) or not isinstance(value['attempts'], list):
        raise ValueError('expected and attempts must be lists')
    plans = {}
    for plan in value['expected']:
        validate(plan)
        if plan['kind'] != 'expected_chunks':
            raise ValueError('expected_chunks manifest required')
        d = validate(plan['dataset'])
        key = (plan['campaign_id'], d['workflow'], d['dataset_id'])
        if key in plans:
            raise ValueError('duplicate expected dataset identity')
        plans[key] = d
    result, seen, jobs = [], set(), set()
    for entry in value['attempts']:
        if not isinstance(entry, dict) or set(entry) != {'dataset_id', 'chunk_id', 'attempt', 'job_id', 'mapping_evidence'}:
            raise ValueError('attempt entry requires dataset_id, chunk_id, attempt, job_id, mapping_evidence')
        a = validate(entry['attempt'])
        if a['kind'] != 'attempt':
            raise ValueError('attempt manifest required')
        identity = dict(campaign_id=a['campaign_id'], workflow=a['workflow'],
                        dataset_id=entry['dataset_id'], chunk_id=entry['chunk_id'], attempt_id=a['attempt_id'])
        if any(not isinstance(v, str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*', v) for v in identity.values()):
            raise ValueError('invalid identity')
        key = tuple(identity[k] for k in IDENTITY)
        # Attempt IDs are unique within campaign/workflow, including across chunks.
        aid = (a['campaign_id'], a['workflow'], a['attempt_id'])
        if aid in seen:
            raise ValueError('duplicate or conflicting attempt identity')
        seen.add(aid)
        d = plans.get(key[:3])
        chunks = {} if d is None else {c['chunk_id']: c for c in d['chunks']}
        if entry['chunk_id'] not in chunks or a['ordered_inputs'] != chunks[entry['chunk_id']]['ordered_inputs']:
            raise ValueError('attempt not linked to expected dataset/chunk/ordered inputs')
        jid, evidence = entry['job_id'], entry['mapping_evidence']
        if jid is not None:
            job_id(jid)
            if jid in jobs or not isinstance(evidence, str) or not evidence.strip():
                raise ValueError('duplicate job mapping or missing mapping evidence')
            jobs.add(jid)
        elif evidence is not None:
            raise ValueError('mapping evidence without job_id')
        result.append(dict(identity=identity, job_id=jid, mapping_evidence=evidence))
    return result


def snapshot(value):
    envelope(value, 'condor_snapshot', ['source', 'observed_at', 'queue', 'history'])
    if not isinstance(value['source'], str) or not value['source'].strip():
        raise ValueError('snapshot source required')
    try:
        stamp = datetime.datetime.fromisoformat(value['observed_at'].replace('Z', '+00:00'))
        if stamp.tzinfo is None:
            raise ValueError('timezone required')
    except (AttributeError, TypeError, ValueError) as exc:
        raise ValueError('observed_at requires timezone-aware timestamp') from exc
    for origin in ('queue', 'history'):
        if not isinstance(value[origin], list):
            raise ValueError('queue/history must be ClassAd lists')
        for ad in value[origin]:
            if not isinstance(ad, dict):
                raise ValueError('ClassAd must be an object')
            for field in ('ClusterId', 'ProcId'):
                if type(ad.get(field)) is not int or ad[field] < 0:
                    raise ValueError(f'invalid or missing {field}')
            for field in ('JobStatus', 'EnteredCurrentStatus', 'QDate', 'CompletionDate', 'ExitCode', 'ExitSignal', 'HoldReasonCode'):
                if field in ad and (type(ad[field]) is not int or ad[field] < 0):
                    raise ValueError(f'invalid {field}')
            if 'ExitBySignal' in ad and type(ad['ExitBySignal']) is not bool:
                raise ValueError('invalid ExitBySignal')
            for field in ATTRS + ('HoldReason',):
                if field in ad and (not isinstance(ad[field], str) or not ad[field]):
                    raise ValueError(f'invalid {field}')
    return value


def exit_state(ad):
    if ad.get('JobStatus') != 4:
        return 'not_completed'
    if ad.get('ExitBySignal') is True:
        return 'signal'
    if ad.get('ExitBySignal') is False and 'ExitCode' in ad:
        return 'normal_zero' if ad['ExitCode'] == 0 else 'normal_nonzero'
    return 'unknown'


def reconcile(metadata, evidence):
    known = registry(metadata)
    snapshot(evidence)
    observations, unresolved = [], []
    for origin in ('queue', 'history'):
        for index, ad in enumerate(evidence[origin]):
            jid = f"{ad['ClusterId']}.{ad['ProcId']}"
            supplied = {k: ad[attr] for k, attr in zip(IDENTITY, ATTRS) if attr in ad}
            mapped = [i for i, a in enumerate(known) if a['job_id'] == jid]
            explicit = [i for i, a in enumerate(known) if supplied == a['identity']] if len(supplied) == len(IDENTITY) else []
            candidates = sorted(set(mapped + explicit))
            conflict = len(candidates) > 1 or any(
                any(known[i]['identity'][k] != v for k, v in supplied.items()) or
                known[i]['job_id'] not in (None, jid) for i in candidates)
            resolved = len(candidates) == 1 and not conflict
            item = dict(job_id=jid, origin=origin, index=index, ad=ad,
                        candidates=candidates, resolved=resolved,
                        matching='explicit_attributes' if explicit else 'declared_mapping' if mapped else 'unmatched',
                        identity_conflict=conflict)
            observations.append(item)
    # Reconcile the logical job globally before confirming any attempt match.
    by_job = {}
    for observation in observations:
        by_job.setdefault(observation['job_id'], []).append(observation)
    for group in by_job.values():
        candidates = sorted({i for o in group for i in o['candidates']})
        conflict = (len(candidates) > 1 or any(o['identity_conflict'] for o in group) or
                    any(len({o['ad'][attr] for o in group if attr in o['ad']}) > 1
                        for attr in ATTRS))
        for observation in group:
            observation['job_identity_conflict'] = conflict
            if conflict:
                observation['resolved'] = False
                observation['identity_conflict'] = True
                # Every affected attempt retains all rows, including the contradictory
                # row that originally matched another attempt (or no known attempt).
                observation['candidates'] = candidates
    unresolved = [o for o in observations if not o['resolved']]
    attempts = []
    for i, a in enumerate(known):
        matches = [o for o in observations if i in o['candidates']]
        issues = []
        if any(o['job_identity_conflict'] for o in matches):
            issues.append('job_identity_conflict')
        if any(not o['resolved'] for o in matches):
            issues.append('identity_conflict')
        ids = {o['job_id'] for o in matches}
        if len(ids) > 1:
            issues.append('multiple_jobs')
        queue = [o for o in matches if o['origin'] == 'queue']
        history = [o for o in matches if o['origin'] == 'history']
        if len(queue) > 1 or len(history) > 1:
            issues.append('duplicate_observations')
        if queue and history:
            # Only identical ads are consistent overlaps; preserve both either way.
            if queue[0]['ad'] != history[0]['ad']:
                issues.append('queue_history_conflict')
        current = (queue or history or [None])[0] if not issues else None
        state = STATES.get(current['ad'].get('JobStatus'), 'unknown') if current else 'unobserved' if not matches else 'unknown'
        if state in ('unknown', 'unobserved'):
            issues.append('missing_or_unknown_scheduler_state')
        if current and state == 'completed' and exit_state(current['ad']) == 'unknown':
            issues.append('missing_exit_evidence')
        attempts.append(dict(**a, associated_job_id=next(iter(ids)) if len(ids) == 1 and not any(o['identity_conflict'] for o in matches) else a['job_id'],
                             state=state, exit=exit_state(current['ad']) if current else 'unknown',
                             current=current, history=history, observations=matches, issues=issues,
                             reconciliation='unresolved' if issues else 'observed'))
    return dict(schema_version=1, kind='condor_monitor_report', observed_at=evidence['observed_at'],
                source=evidence['source'], attempts=attempts, unresolved_observations=unresolved,
                summary=dict(attempts=len(attempts), states=dict(Counter(a['state'] for a in attempts)),
                             unresolved_attempts=sum(bool(a['issues']) for a in attempts),
                             unresolved_observations=len(unresolved)),
                acceptance_changed=False, output_acceptance='not_evaluated')


def collect(metadata, timeout=20, scanlimit=10000):
    known = registry(metadata)
    ids = sorted({a['job_id'] for a in known if a['job_id'] is not None})
    if not ids or len(ids) > 100 or not 0 < timeout <= 60 or not 0 < scanlimit <= 100000:
        raise ValueError('live collection requires 1..100 explicit jobs, timeout <=60, scanlimit <=100000')
    constraint = ' || '.join(f'(ClusterId == {j.split(".")[0]} && ProcId == {j.split(".")[1]})' for j in ids)
    result = dict(schema_version=1, kind='condor_snapshot', source='local schedd explicit job query',
                  observed_at=datetime.datetime.now(datetime.timezone.utc).isoformat())
    commands = {}
    for origin, command in [('queue', 'condor_q'), ('history', 'condor_history')]:
        args = [command, '-json', '-constraint', constraint]
        if origin == 'history':
            args += ['-match', str(len(ids) + 1), '-scanlimit', str(scanlimit)]
        commands[origin] = args
        text = subprocess.check_output(args, text=True, timeout=timeout)
        # Use the same strict JSON reader, without writing scheduler evidence.
        def pairs(items):
            d = {}
            for k, v in items:
                if k in d:
                    raise ValueError('duplicate JSON key in scheduler response')
                d[k] = v
            return d
        result[origin] = json.loads(text, object_pairs_hook=pairs,
                                    parse_constant=lambda x: (_ for _ in ()).throw(ValueError(x)))
    snapshot(result)
    return result, commands


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', required=True, help='versioned monitor input with expected plans and attempts')
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--snapshot', help='offline versioned scheduler snapshot')
    mode.add_argument('--live', action='store_true', help='bounded local scheduler queries for explicitly mapped jobs')
    parser.add_argument('--timeout', type=int, default=20)
    parser.add_argument('--scanlimit', type=int, default=10000)
    args = parser.parse_args(argv)
    try:
        metadata = load_json(args.input)
        commands = None
        if args.live:
            evidence, commands = collect(metadata, args.timeout, args.scanlimit)
        else:
            evidence = load_json(args.snapshot)
        report = reconcile(metadata, evidence)
        report['inputs'] = {'monitor_input': {'path': args.input, 'sha256': hashlib.sha256(json.dumps(metadata, sort_keys=True).encode()).hexdigest()},
                            'snapshot': evidence, 'commands': commands}
        print(json.dumps(report, indent=2, allow_nan=False))
        return 1 if report['summary']['unresolved_attempts'] or report['summary']['unresolved_observations'] else 0
    except (ValueError, OSError, subprocess.SubprocessError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
