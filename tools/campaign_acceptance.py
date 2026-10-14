#!/usr/bin/env python3
"""Manual technical acceptance. Temporary POSIX stores only; no EOS/scheduler writes."""
import argparse
from contextlib import contextmanager
from datetime import datetime, timezone
import fcntl
import pwd
import hashlib
import json
import os
from pathlib import Path
import socket
import tempfile
import time
sys_path = str(Path(__file__).resolve().parent)
import sys
sys.path.insert(0, sys_path)
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from campaign import load_json
from campaign_production import canonical, digest, identifier, attempt_id, execution_path, validate as production_validate
from campaign_input_validation import require_complete
from campaign_dictionary_validation import context as mode_context, validate_report
from campaign_staging import checksums

PROSPECTIVE_STORE = '/uscms_data/d3/msahoo/Project_tzq/production_indexes/2024/'


class AcceptanceError(ValueError):
    pass


def stamp():
    return datetime.now(timezone.utc).isoformat()


def object_hash(value):
    return hashlib.sha256(canonical(value)).hexdigest()


def key(value):
    return value['dataset_id'], value['chunk_id']


def scope(campaign, workflow, selector, scope_id, datasets=None, campaign_record=None):
    production_validate(campaign)
    if campaign_record is None or load_json(campaign_record)!=campaign:
        raise AcceptanceError('exact campaign record required')
    identifier(scope_id)
    if workflow != 'main' or selector not in ('mc', 'data', 'full', 'explicit'):
        raise AcceptanceError('only Phase A/B-validated main scopes are supported')
    if campaign.get('max_input_events', 'missing') is not None:
        raise AcceptanceError('explicit unlimited campaign required')
    if campaign.get('representative_subset') is not None and selector != 'explicit':
        raise AcceptanceError('representative subset requires explicit scope; never full 2024')
    declared = [d for d in campaign['datasets'] if d['workflow'] == workflow]
    if selector == 'explicit':
        if not datasets or len(datasets) != len(set(datasets)) or not set(datasets) <= {d['dataset_id'] for d in declared}:
            raise AcceptanceError('invalid explicit dataset scope')
        selected = [d for d in declared if d['dataset_id'] in datasets]
    else:
        if datasets is not None: raise AcceptanceError('dataset overrides require explicit selector')
        selected = [d for d in declared if selector == 'full' or d['datatype'] == selector]
    expected = []
    for d in selected:
        for chunk in d['chunks']:
            expected.append(dict(dataset_id=d['dataset_id'], datatype=d['datatype'], chunk_id=chunk['chunk_id'], index=chunk['index'],
                                 ordered_inputs=chunk['ordered_inputs'], config=d['config'], profile=d['profile'], processing_schema_id=d['processing_schema_id']))
    expected.sort(key=lambda v: (v['dataset_id'], v['index'], v['chunk_id']))
    if not expected: raise AcceptanceError('empty scope')
    return dict(schema_version=1, kind='accepted_output_scope', scope_id=scope_id, selector=selector,
                campaign=campaign, campaign_record=str(Path(campaign_record).resolve()), campaign_sha256=digest(campaign_record), workflow=workflow, expected=expected,
                campaign_complete_scope=campaign.get('representative_subset') is None and len(expected) == sum(len(d['chunks']) for d in declared))


def validate_scope(value):
    if not isinstance(value, dict) or set(value) != {'schema_version','kind','scope_id','selector','campaign','campaign_record','campaign_sha256','workflow','expected','campaign_complete_scope'} or type(value['schema_version']) is not int or value['schema_version'] != 1 or value['kind'] != 'accepted_output_scope':
        raise AcceptanceError('unsupported/invalid scope')
    ds = sorted({v['dataset_id'] for v in value['expected']}) if value['selector'] == 'explicit' else None
    rebuilt = scope(value['campaign'], value['workflow'], value['selector'], value['scope_id'], ds, value['campaign_record'])
    if value != rebuilt: raise AcceptanceError('scope differs from declared campaign')
    return value


def verify_candidate(spec, candidate):
    """Explicit metadata and locally held artifact observations; no ROOT copies/queries."""
    validate_scope(spec)
    if not isinstance(candidate, dict) or set(candidate) != {'execution','publication','result','manifest','profile','attempt','local_artifacts'}:
        raise AcceptanceError('invalid candidate input')
    paths = {n: Path(candidate[n]) for n in ('execution','publication','result','manifest','profile','attempt')}
    initial_hashes={n:digest(p) for n,p in paths.items()}
    objects = {n: load_json(p) for n,p in paths.items()}
    ex,pub,result,m,p = (objects[n] for n in ('execution','publication','result','manifest','profile'))
    production_validate(ex); production_validate(pub); production_validate(m); production_validate(objects['attempt'])
    attempt=objects['attempt']
    ca=spec['campaign']; identity={n:ex[n] for n in ('campaign_id','submission_id','workflow','dataset_id','chunk_id','attempt_id','execution_id','cluster_id','proc_id')}
    expected = [v for v in spec['expected'] if key(v)==key(ex)]
    if len(expected)!=1 or ex['campaign_id']!=ca['campaign_id'] or ex['workflow']!=spec['workflow']:
        raise AcceptanceError('execution outside scope')
    expected=expected[0]
    attempt_path='attempts/'+identity['workflow']+'/'+identity['attempt_id']+'.json'
    if m['files'].get(attempt_path)!=digest(paths['attempt']) or any(attempt[n]!=identity[n] for n in ('campaign_id','submission_id','workflow','dataset_id','chunk_id','attempt_id')) or attempt['filelist_sha256']!=ex['filelist_sha256'] or attempt['config_sha256']!=ex['config_sha256']:
        raise AcceptanceError('attempt metadata mismatch')
    if identity['attempt_id']!=attempt_id(identity['campaign_id'],identity['submission_id'],identity['workflow'],identity['dataset_id'],identity['chunk_id']):
        raise AcceptanceError('invalid attempt linkage')
    if m['campaign_id']!=identity['campaign_id'] or m['submission_id']!=identity['submission_id'] or m['campaign_sha256']!=spec['campaign_sha256'] or ex['campaign_sha256']!=spec['campaign_sha256'] or ex['manifest_sha256']!=digest(paths['manifest']) or ex['payload_hashes']!=m['files'] or ex['source']!=ca['source'] or m['source']!=ca['source']:
        raise AcceptanceError('manifest/campaign/source mismatch')
    if m['files'].get('campaign.json')!=spec['campaign_sha256']:raise AcceptanceError('campaign file hash mismatch')
    if any(m['files'].get(n)!=h for n,h in ca['processing_files'].items()):
        raise AcceptanceError('processing payload differs from frozen campaign')
    jobs=[j for j in m['jobs'] if all(j[n]==identity[n] for n in ('workflow','dataset_id','chunk_id','attempt_id'))]
    if len(jobs)!=1: raise AcceptanceError('ambiguous prepared job')
    job=jobs[0]
    inputs=expected['ordered_inputs']; file_hash=hashlib.sha256(('\n'.join(inputs)+'\n').encode()).hexdigest()
    if ex['ordered_inputs']!=inputs or ex['filelist_sha256']!=file_hash or job['filelist_sha256']!=file_hash or m['files'].get(job['filelist'])!=file_hash or job['index']!=expected['index'] or ex['processing_schema_id']!=expected['processing_schema_id'] or job['config']!=expected['config'] or job['profile']!=expected['profile'] or ex['config_sha256']!=m['files'].get(expected['config']) or job['config_sha256']!=ex['config_sha256'] or ex['profile_sha256']!=digest(paths['profile']) or job['profile_sha256']!=ex['profile_sha256'] or m['files'].get(expected['profile'])!=digest(paths['profile']):
        raise AcceptanceError('input/config/profile linkage mismatch')
    if p['workflow']!=spec['workflow'] or p['datatype']!=expected['datatype'] or p['tree']!='outputTree_00000':raise AcceptanceError('profile workflow/datatype mismatch')
    declaration=next(d for d in ca['datasets'] if d['workflow']==spec['workflow'] and d['dataset_id']==identity['dataset_id'])
    if ex['dataset_declaration']!={n:declaration[n] for n in ('das_dataset','year','datatype','era','stream','sample_name')}:
        raise AcceptanceError('dataset declaration mismatch')
    if ex['status']!='validated' or ex['processing_exit']!=0 or ex['acceptance']!='not_evaluated' or result.get('status')!='published' or type(result.get('processing_exit')) is not int or result.get('processing_exit')!=0 or result.get('validation_status')!='PASS' or type(result.get('staging_exit')) is not int or result.get('staging_exit')!=0 or result.get('identity')!=identity or result.get('input_scope')!=ex['input_scope']:
        raise AcceptanceError('unsuccessful execution')
    evidence=ex['root_validation']; ledger=evidence.get('input_completeness'); scope_value=ex['input_scope']
    if not isinstance(ledger,dict):raise AcceptanceError('missing Phase A ledger')
    require_complete(ledger,inputs,None);mode_context(p,None,scope_value,ledger)
    if evidence.get('status')!='PASS' or evidence.get('acceptance_changed') is not False or len(evidence.get('outputs',[]))!=1:
        raise AcceptanceError('ROOT validation incomplete')
    observed=evidence['outputs'][0]
    if observed.get('profile')!={'profile_id':p['profile_id'],'schema_version':p['schema_version']}:raise AcceptanceError('observed profile mismatch')
    if observed.get('status')!='PASS' or observed.get('acceptance')!='not_evaluated' or any(observed['identity'].get(n)!=identity[n] for n in ('campaign_id','workflow','dataset_id','chunk_id','attempt_id')):
        raise AcceptanceError('ROOT identity/result mismatch')
    checks=evidence.get('dictionary_checks',{})
    if not isinstance(checks,dict) or set(checks)!= {'preflight','output'}:raise AcceptanceError('missing Phase B gate evidence')
    for phase in ('preflight','output'):
        report=checks[phase]
        if report['root_version']!=ex['runtime']['root']:raise AcceptanceError('runtime ROOT mismatch')
        mapped=Path(report['library_maps'][0]).parent
        validate_report(report,mapped,m['files'],p,None,scope_value if phase=='output' else None,ledger if phase=='output' else None,'output.root' if phase=='output' else None)
    from root_output_validation import validate_worker_result
    validate_worker_result({n:observed[n] for n in ('issues','structure','content')},p)
    if observed['issues'] or any(observed['structure'].get(n)!=checks['output']['root_evidence']['structure'].get(n) for n in ('tree','opened','recovered','entries','branch_types')):
        raise AcceptanceError('contradictory schema evidence')
    fingerprint=object_hash(dict(tree=p['tree'],branches=observed['structure']['branch_types']))
    if ex['schema_fingerprint']!=fingerprint:raise AcceptanceError('schema fingerprint mismatch')
    dest=execution_path(identity['campaign_id'],identity['submission_id'],identity['workflow'],identity['dataset_id'],identity['chunk_id'],identity['execution_id'])
    if pub['status']!='published' or pub['acceptance']!='not_evaluated' or pub.get('input_scope')!=scope_value or pub['root_validation']!=evidence or pub['destination']!=dest or result.get('destination')!=dest or any(pub.get(n)!=v for n,v in identity.items()):
        raise AcceptanceError('publication identity/evidence mismatch')
    if pub['checksum_algorithm']!=ca['publication_policy']['checksum_algorithm']:raise AcceptanceError('checksum policy mismatch')
    if observed.get('file',{}).get('size_bytes')!=pub['artifacts']['output.root']['size_bytes'] or Path(observed.get('inspected_path','')).name!='output.root' or Path(observed.get('inspected_path','')).parent.name!=identity['execution_id']:
        raise AcceptanceError('validated output/publication linkage mismatch')
    local=candidate['local_artifacts']
    if set(local)!={'output.root','processing.log','execution.json','publication.json'} or Path(local['execution.json']).resolve()!=paths['execution'].resolve() or Path(local['publication.json']).resolve()!=paths['publication'].resolve():
        raise AcceptanceError('exact local artifacts required')
    observations={}
    for name,path in local.items():
        path=Path(path)
        if path.is_symlink() or not path.is_file():raise AcceptanceError('missing/nonregular artifact')
        before=path.stat();actual=checksums(path,deadline=time.monotonic()+60);after=path.stat()
        if (before.st_size,before.st_mtime_ns,before.st_ctime_ns)!=(after.st_size,after.st_mtime_ns,after.st_ctime_ns):raise AcceptanceError('artifact changed')
        ev=result['publication_marker'] if name=='publication.json' else pub['artifacts'][name]; algorithm=pub['checksum_algorithm']
        if ev.get('locator')!=ca['endpoint']+'/'+dest+'/'+name or any(ev.get(n)!=v for n,v in actual.items()) or ev.get('remote_checksum')!={'algorithm':algorithm,'value':actual[algorithm]}:
            raise AcceptanceError('publication artifact checksum/locator mismatch')
        if name=='output.root' and actual['size_bytes']==0:raise AcceptanceError('zero-byte ROOT output')
        observations[name]=dict(**actual,checked_at=stamp(),source='independent_local_checksum',remote_state='NOT_TESTED')
    if any(digest(p)!=initial_hashes[n] for n,p in paths.items()):raise AcceptanceError('metadata changed during validation')
    return dict(dataset_id=identity['dataset_id'],chunk_id=identity['chunk_id'],index=expected['index'],datatype=expected['datatype'],identity=identity,
                locator=pub['artifacts']['output.root']['locator'],size_bytes=pub['artifacts']['output.root']['size_bytes'],sha256=pub['artifacts']['output.root']['sha256'],
                checksum=pub['artifacts']['output.root']['remote_checksum'],manifest_sha256=ex['manifest_sha256'],schema_fingerprint=fingerprint,input_scope=scope_value,selected_entries=observed['structure']['entries'],processing_schema_id=ex['processing_schema_id'],
                evidence_files={n:dict(path=str(paths[n].resolve()),sha256=digest(paths[n])) for n in paths},local_artifacts={n:str(Path(v).resolve()) for n,v in local.items()},observations=observations,
                technical_acceptance='ACCEPTED',physics_acceptance='NOT_EVALUATED')


def safe_store(path):
    p=Path(path).absolute()
    if any(x.is_symlink() for x in [p,*p.parents]):raise AcceptanceError('symlink index store')
    p=p.resolve()
    if Path('/tmp') not in p.parents or not p.is_dir() or p.stat().st_uid!=os.getuid():
        raise AcceptanceError('persistent activation blocked; owned temporary /tmp store only')
    for name in ['STORE.json','HEAD.json','LOCK','revisions']:
        if (p/name).is_symlink():raise AcceptanceError('unsafe internal store path')
    if p.stat().st_mode & 0o022:raise AcceptanceError('store must not be group/world writable')
    return p


def fsync_dir(root):
    fd=os.open(root,os.O_RDONLY|os.O_DIRECTORY)
    try:os.fsync(fd)
    finally:os.close(fd)


def immutable(path,value):
    content=canonical(value)+b'\n';fd,tmp=tempfile.mkstemp(prefix='.revision-',dir=path.parent)
    try:
        with os.fdopen(fd,'wb') as stream:stream.write(content);stream.flush();os.fsync(stream.fileno())
        try:os.link(tmp,path)
        except FileExistsError:
            if path.is_symlink() or path.read_bytes()!=content:raise AcceptanceError('immutable revision collision')
        fsync_dir(path.parent)
    finally:os.unlink(tmp)


def initialize(path,spec):
    root=safe_store(path);validate_scope(spec)
    if list(root.iterdir()):raise AcceptanceError('store must be empty')
    (root/'revisions').mkdir();immutable(root/'STORE.json',dict(schema_version=1,scope=spec,host=socket.gethostname(),uid=os.getuid(),operator=pwd.getpwuid(os.getuid()).pw_name,test_only=True))
    fsync_dir(root)


@contextmanager
def locked(root,timeout=5):
    fd=os.open(root/'LOCK',os.O_CREAT|os.O_RDWR|os.O_NOFOLLOW,0o600);deadline=time.monotonic()+timeout
    try:
        if os.fstat(fd).st_nlink!=1:raise AcceptanceError('unsafe lock hardlink')
        while True:
            try:fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB);break
            except BlockingIOError:
                if time.monotonic()>=deadline:raise AcceptanceError('lock timeout')
                time.sleep(.02)
        yield
    finally:fcntl.flock(fd,fcntl.LOCK_UN);os.close(fd)


def current(root):
    head=root/'HEAD.json'
    if head.is_symlink():raise AcceptanceError('unsafe HEAD')
    if not head.exists():return None,None
    h=load_json(head)
    if set(h)!={'revision','sha256'}:raise AcceptanceError('invalid HEAD')
    path=root/'revisions'/(h['revision']+'.json');identifier(h['revision'])
    if path.is_symlink() or digest(path)!=h['sha256']:raise AcceptanceError('revision digest mismatch')
    rev=load_json(path);validate_revision(rev)
    if rev['scope']!=load_json(root/'STORE.json')['scope']:raise AcceptanceError('revision differs from authoritative store scope')
    return h,rev


def build_revision(spec,parent,previous,decisions):
    entries={key(x):x for x in (previous['entries'] if previous else [])};history=list(previous['history'] if previous else [])
    for decision,entry in decisions:
        k=key(entry);prior=entries.get(k)
        if decision['action']=='accept' and prior:raise AcceptanceError('already accepted; explicit supersede required')
        if decision['action']=='supersede' and (not prior or decision['supersedes']!=prior['identity']['execution_id']):raise AcceptanceError('superseded reference mismatch')
        if prior and prior['identity']['execution_id']==entry['identity']['execution_id']:raise AcceptanceError('cannot supersede itself')
        entries[k]=entry;history.append(dict(**decision,identity=entry['identity'],prior_identity=prior['identity'] if prior else None,evidence_hash=object_hash(entry)))
    ordered=sorted(entries.values(),key=lambda x:(x['dataset_id'],x['index'],x['chunk_id']))
    missing=[dict(dataset_id=e['dataset_id'],chunk_id=e['chunk_id']) for e in spec['expected'] if key(e) not in entries]
    value=dict(schema_version=1,kind='accepted_output_revision',scope=spec,parent=parent,entries=ordered,history=history,missing=missing,
               status='INCOMPLETE' if missing else 'COMPLETE',physics_acceptance='NOT_EVALUATED',test_only=True)
    value['revision']='r-'+object_hash(value);validate_revision(value);return value


def validate_revision(value):
    fields={'schema_version','kind','scope','parent','entries','history','missing','status','physics_acceptance','test_only','revision'}
    if not isinstance(value,dict) or set(value)!=fields or type(value['schema_version']) is not int or value['schema_version']!=1 or value['kind']!='accepted_output_revision' or value['physics_acceptance']!='NOT_EVALUATED' or value['test_only'] is not True:
        raise AcceptanceError('invalid revision contract')
    validate_scope(value['scope']);unsigned=dict(value);unsigned.pop('revision')
    if value['revision']!='r-'+object_hash(unsigned):raise AcceptanceError('revision content identity mismatch')
    expected={key(e):e for e in value['scope']['expected']};seen=set();selected={}
    for entry in value['entries']:
        k=key(entry)
        if k in seen or k not in expected or entry['index']!=expected[k]['index'] or entry['technical_acceptance']!='ACCEPTED' or entry['physics_acceptance']!='NOT_EVALUATED' or entry['identity']['campaign_id']!=value['scope']['campaign']['campaign_id'] or entry['identity']['workflow']!=value['scope']['workflow'] or key(entry['identity'])!=k:
            raise AcceptanceError('invalid/duplicate accepted chunk')
        from campaign_event_limit import require_complete as complete_scope
        complete_scope(entry['input_scope'])
        ident=entry['identity'];dest=execution_path(ident['campaign_id'],ident['submission_id'],ident['workflow'],ident['dataset_id'],ident['chunk_id'],ident['execution_id'])
        if entry['datatype']!=expected[k]['datatype'] or entry['locator']!=value['scope']['campaign']['endpoint']+'/'+dest+'/output.root' or type(entry['selected_entries']) is not int or not 0<=entry['selected_entries']<=entry['input_scope']['analysis_input_entries']:
            raise AcceptanceError('invalid accepted mode/path/counts')
        seen.add(k)
    last_decisions={}
    for decision in value['history']:
        k=key(decision['identity']);prior=selected.get(k)
        try:
            when=datetime.fromisoformat(decision['timestamp'].replace('Z','+00:00'))
            if when.tzinfo is None:raise ValueError('timezone missing')
        except Exception as exc:raise AcceptanceError('invalid decision timestamp') from exc
        if decision['action']=='accept' and decision['supersedes'] is not None:raise AcceptanceError('accept has supersession reference')
        if not decision.get('operator') or not decision.get('reason') or not decision.get('timestamp') or not decision.get('authorization_reference') or decision['action'] not in ('accept','supersede') or decision['prior_identity']!=prior:
            raise AcceptanceError('invalid decision history')
        if (decision['action']=='accept')!=(prior is None):raise AcceptanceError('invalid supersession history')
        if prior and decision['supersedes']!=prior['execution_id']:raise AcceptanceError('wrong supersession')
        selected[k]=decision['identity'];last_decisions[k]=decision
    if any(last_decisions.get(key(e),{}).get('evidence_hash')!=object_hash(e) for e in value['entries']):raise AcceptanceError('decision/evidence digest mismatch')
    if set(selected)!=seen or any(selected[key(e)]!=e['identity'] for e in value['entries']):raise AcceptanceError('superseded execution selected')
    missing=[dict(dataset_id=e['dataset_id'],chunk_id=e['chunk_id']) for e in value['scope']['expected'] if key(e) not in seen]
    if value['missing']!=missing or value['status']!=('INCOMPLETE' if missing else 'COMPLETE') or value['entries']!=sorted(value['entries'],key=lambda e:(e['dataset_id'],e['index'],e['chunk_id'])):
        raise AcceptanceError('incorrect completeness/order')
    return value


def update(path,expected_parent,candidate,action,reason,authorization_reference,supersedes=None,authorize=False,fault=None):
    if authorize is not True or action not in ('accept','supersede') or not reason.strip() or not authorization_reference.strip():raise AcceptanceError('explicit authorized decision/reason/reference required')
    root=safe_store(path);store=load_json(root/'STORE.json')
    if store['host']!=socket.gethostname() or store['uid']!=os.getuid():raise AcceptanceError('wrong coordinator')
    entry=verify_candidate(store['scope'],candidate)
    if action=='accept' and supersedes is not None:raise AcceptanceError('accept cannot supersede')
    decision=dict(action=action,operator=pwd.getpwuid(os.getuid()).pw_name,timestamp=stamp(),reason=reason,authorization_reference=authorization_reference,supersedes=supersedes)
    with locked(root):
        head,old=current(root);actual=head['revision'] if head else None
        if actual!=expected_parent:raise AcceptanceError('stale expected parent')
        if old and old['scope']!=store['scope']:raise AcceptanceError('scope changed')
        # Revalidate after acquiring the lock; metadata must not change during queueing.
        entry=verify_candidate(store['scope'],candidate)
        rev=build_revision(store['scope'],actual,old,[(decision,entry)]);target=root/'revisions'/(rev['revision']+'.json');immutable(target,rev)
        if fault=='after_revision':raise AcceptanceError('injected interrupted update')
        newhead=dict(revision=rev['revision'],sha256=digest(target));fd,tmp=tempfile.mkstemp(prefix='.head-',dir=root)
        try:
            with os.fdopen(fd,'wb') as stream:stream.write(canonical(newhead)+b'\n');stream.flush();os.fsync(stream.fileno())
            if fault=='before_head':raise AcceptanceError('injected before HEAD visibility')
            os.replace(tmp,root/'HEAD.json');fsync_dir(root)
            if fault=='after_head':raise AcceptanceError('injected after commit; read HEAD before retry')
        finally:
            if os.path.exists(tmp):os.unlink(tmp)
        return rev


def manifest(revision):
    validate_revision(revision)
    if revision['status']!='COMPLETE':raise AcceptanceError('scope incomplete')
    return dict(schema_version=1,kind='accepted_plot_inputs',revision=revision['revision'],revision_sha256=hashlib.sha256(canonical(revision)+b'\n').hexdigest(),scope_id=revision['scope']['scope_id'],
                campaign_id=revision['scope']['campaign']['campaign_id'],workflow=revision['scope']['workflow'],status='COMPLETE',
                campaign_complete_scope=revision['scope']['campaign_complete_scope'],physics_acceptance='NOT_EVALUATED',test_only=True,
                inputs=[dict(dataset_id=e['dataset_id'],chunk_id=e['chunk_id'],index=e['index'],datatype=e['datatype'],locator=e['locator'],tree='outputTree_00000',processing_schema_id=e['processing_schema_id'],schema_fingerprint=e['schema_fingerprint'],input_entries=e['input_scope']['total_input_entries'],processed_entries=e['input_scope']['analysis_input_entries'],selected_entries=e['selected_entries'],execution=e['identity'],checksum=e['checksum'],sha256=e['sha256']) for e in revision['entries']])


def recheck(revision):
    validate_revision(revision)
    for entry in revision['entries']:
        files=entry['evidence_files']
        if any(digest(Path(e['path']))!=e['sha256'] for e in files.values()):raise AcceptanceError('metadata evidence changed')
        candidate={n:v['path'] for n,v in files.items()};candidate['local_artifacts']=entry['local_artifacts']
        fresh=verify_candidate(revision['scope'],candidate)
        if any(fresh[n]!=entry[n] for n in entry if n!='observations'):raise AcceptanceError('accepted reference changed')
    return manifest(revision)


def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__);subs=p.add_subparsers(dest='command',required=True)
    init=subs.add_parser('init');init.add_argument('--store',required=True);init.add_argument('--scope',required=True)
    for name in ['validate','manifest']:
        q=subs.add_parser(name);q.add_argument('--revision',required=True);q.add_argument('--store',required=True)
    q=subs.add_parser('validate-manifest');q.add_argument('--manifest',required=True);q.add_argument('--revision',required=True);q.add_argument('--store',required=True)
    q=subs.add_parser('decide');q.add_argument('--store',required=True);q.add_argument('--candidate',required=True);q.add_argument('--expected-parent',required=True);q.add_argument('--action',choices=['accept','supersede'],required=True);q.add_argument('--supersedes');q.add_argument('--reason',required=True);q.add_argument('--authorization-reference',required=True);q.add_argument('--authorize-user-decision',action='store_true')
    a=p.parse_args(argv)
    try:
        if a.command=='init':initialize(a.store,load_json(a.scope));result=dict(status='PASS',test_only=True)
        elif a.command in ('validate','manifest','validate-manifest'):
            root=safe_store(a.store);head,rev=current(root);selected=load_json(a.revision)
            if head is None or selected['revision']!=head['revision'] or digest(a.revision)!=head['sha256']:raise AcceptanceError('superseded or non-authoritative revision')
            result=recheck(selected)
            if a.command=='validate-manifest' and load_json(a.manifest)!=result:raise AcceptanceError('manifest differs from authoritative revision')
        else:result=update(a.store,None if a.expected_parent=='NONE' else a.expected_parent,load_json(a.candidate),a.action,a.reason,a.authorization_reference,a.supersedes,a.authorize_user_decision)
        print(json.dumps(result,indent=2,allow_nan=False));return 0
    except Exception as exc:print(json.dumps(dict(status='FAIL',error=str(exc))),file=sys.stderr);return 1

if __name__=='__main__':sys.exit(main())
