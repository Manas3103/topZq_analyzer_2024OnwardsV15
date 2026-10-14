"""Opt-in campaign input ledger. No analysis imports in the controller."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import time
from urllib.parse import urlsplit


class InputError(ValueError):
    pass


def strict_json(text):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result: raise InputError('duplicate JSON key')
            result[key] = value
        return result
    def constant(value): raise InputError('nonfinite JSON value')
    return json.loads(text, object_pairs_hook=pairs, parse_constant=constant)


def save(path, value):
    path = Path(path)
    fd, tmp = tempfile.mkstemp(prefix='.input-ledger-', dir=path.parent)
    try:
        with os.fdopen(fd, 'w') as stream:
            json.dump(value, stream, allow_nan=False, indent=2)
            stream.flush(); os.fsync(stream.fileno())
        os.link(tmp, path)  # Atomic visibility, refuse existing ledger.
    finally:
        os.unlink(tmp)


def identity(locator):
    if not isinstance(locator, str) or not locator or any(c.isspace() or ord(c)<32 for c in locator) or any(c in locator for c in '*?[]#'):
        raise InputError('unsafe input locator')
    u = urlsplit(locator)
    if u.scheme:
        if u.scheme != 'root' or not u.hostname or u.username or u.password or u.query or u.fragment:
            raise InputError('unsupported input locator')
        path = '/' + u.path.lstrip('/')
        if not path.startswith('/store/'): raise InputError('explicit /store input required')
    else:
        path = str(Path(locator).resolve())
    if '..' in Path(path).parts: raise InputError('input traversal')
    return path


def inspect(locator, tree):
    # Invoked only in an isolated child by the controller, never on module import.
    import ROOT
    f = ROOT.TFile.Open(locator, 'READ')
    if not f or f.IsZombie(): raise InputError('ROOT open refused or invalid file')
    try:
        if f.TestBit(ROOT.TFile.kRecovered): raise InputError('recovered input')
        t = f.Get(tree)
        if not t or not t.InheritsFrom('TTree'): raise InputError('missing expected input tree')
        n = int(t.GetEntries())
        if n < 0: raise InputError('invalid entry count')
        return dict(status='PASS', tree=tree, entries=n, uuid=str(f.GetUUID().AsString()))
    finally:
        f.Close()


def subprocess_inspect(locator, tree, timeout):
    from .campaign_process import run_process
    result = run_process([sys.executable, str(Path(__file__).resolve()), '--inspect', locator, '--tree', tree], timeout)
    return strict_json(result.stdout)


def validate_inputs(inputs, tree, inspector=subprocess_inspect, overall_timeout=600, per_file_timeout=30):
    if not inputs or not isinstance(tree, str) or not tree: raise InputError('explicit nonempty input list/tree required')
    keys = [identity(x) for x in inputs]
    if len(keys) != len(set(keys)): raise InputError('duplicate assigned input')
    ledger = dict(schema_version=1, kind='campaign_input_ledger', status='UNKNOWN', completion='UNKNOWN', tree=tree,
                  ordered_inputs=list(inputs), files=[], expected_total_entries=0)
    deadline = time.monotonic() + overall_timeout
    for index, (locator, key) in enumerate(zip(inputs, keys)):
        record = dict(index=index, locator=locator, identity=key, status='UNKNOWN')
        ledger['files'].append(record)
        try:
            timeout = min(per_file_timeout, deadline-time.monotonic())
            if timeout <= 0: raise InputError('input validation deadline')
            result = inspector(locator, tree, timeout)
            if not isinstance(result, dict) or set(result) != {'status','tree','entries','uuid'} or result['status'] != 'PASS' or result['tree'] != tree or type(result['entries']) is not int or result['entries'] < 0 or not isinstance(result['uuid'],str) or not result['uuid']:
                raise InputError('input worker protocol')
            record.update(result, empty=result['entries']==0)
            ledger['expected_total_entries'] += result['entries']
        except Exception as exc:
            record['issue'] = str(exc)
            if not urlsplit(locator).scheme and not Path(locator).exists():
                record.update(status='FAIL',issue='missing_input')
            ledger['issue']='required_input_unverified'
            return ledger
    ledger['status']='PASS'
    if ledger['expected_total_entries']==0:
        ledger.update(status='UNKNOWN', issue='unsupported_empty_chunk')
    return ledger


def checked_chain(ROOT, ledger):
    if ledger['status'] != 'PASS': raise InputError(ledger.get('issue','input validation incomplete'))
    chain = ROOT.TChain(ledger['tree']); positive = []
    for record in ledger['files']:
        if record['entries']==0:
            record['chain_participation']='validated_empty'
            continue
        if chain.AddFile(record['locator'], 0, ledger['tree']) != 1: raise InputError('chain_add_failed')
        record['chain_participation']='added'; positive.append(record)
    elements = list(chain.GetListOfFiles())
    if len(elements)!=len(positive): raise InputError('chain_file_count_mismatch')
    for element, record in zip(elements,positive):
        if element.GetTitle()!=record['locator'] or element.GetName()!=ledger['tree'] or int(element.GetEntries())!=record['entries']:
            raise InputError('chain_element_mismatch')
    if int(chain.GetEntries())!=ledger['expected_total_entries']: raise InputError('chain_total_mismatch')
    ledger['chain_total_entries']=int(chain.GetEntries())
    return chain


def complete(ledger, processed, snapshot_counts, maximum):
    if not isinstance(snapshot_counts, list): raise InputError('snapshot coverage must be a list')
    expected=ledger['expected_total_entries'] if maximum is None else min(ledger['expected_total_entries'],maximum)
    if type(processed) is not int or processed!=expected or not snapshot_counts or any(type(n) is not int or n!=expected for n in snapshot_counts):
        raise InputError('processing_completion_unverified')
    ledger.update(completion='PASS', processed_entries=processed, snapshot_processed_entries=snapshot_counts, requested_max_input_events=maximum)


def require_complete(ledger, inputs, maximum):
    if not isinstance(ledger, dict) or ledger.get('schema_version') != 1 or ledger.get('kind') != 'campaign_input_ledger' or ledger.get('status') != 'PASS' or ledger.get('completion') != 'PASS' or ledger.get('ordered_inputs') != inputs or ledger.get('requested_max_input_events') != maximum or ledger.get('tree') != 'Events':
        raise InputError('input ledger identity/completion mismatch')
    if not isinstance(ledger.get('files'), list) or len(ledger['files']) != len(inputs):
        raise InputError('incomplete file evidence')
    total=0
    for index, (record, locator) in enumerate(zip(ledger['files'], inputs)):
        if not isinstance(record,dict) or type(record.get('index')) is not int or record.get('index')!=index or record.get('locator')!=locator or record.get('identity')!=identity(locator) or record.get('tree')!=ledger.get('tree') or record.get('status')!='PASS' or type(record.get('entries')) is not int or record['entries']<0 or record.get('empty') is not (record['entries']==0) or record.get('chain_participation')!=('validated_empty' if record['entries']==0 else 'added'):
            raise InputError('contradictory per-file evidence')
        total+=record['entries']
    if total<=0 or type(ledger.get('expected_total_entries')) is not int or ledger['expected_total_entries']!=total or type(ledger.get('chain_total_entries')) is not int or ledger['chain_total_entries']!=total:
        raise InputError('input total mismatch')
    complete(ledger,ledger.get('processed_entries'),ledger.get('snapshot_processed_entries'),maximum)


if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--inspect',required=True);parser.add_argument('--tree',required=True);args=parser.parse_args()
    import resource
    resource.setrlimit(resource.RLIMIT_CPU,(30,30));resource.setrlimit(resource.RLIMIT_AS,(4*1024**3,4*1024**3))
    print(json.dumps(inspect(args.inspect,args.tree),allow_nan=False))
