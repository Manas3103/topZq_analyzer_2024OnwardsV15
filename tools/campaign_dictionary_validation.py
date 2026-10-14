"""Mode-aware, isolated, read-only campaign dictionary/output gate."""
import hashlib
import json
import math
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))


class GateError(ValueError):
    def __init__(self, code, detail, status='UNKNOWN'):
        self.code, self.status = code, status
        super().__init__(detail)


def expected_tree(profile, maximum):
    from tools.root_output_validation import profile as validate_profile
    validate_profile(profile)
    if maximum is not None and (type(maximum) is not int or not 0 < maximum < 2**63):
        raise GateError('mode', 'positive finite integer limit or explicit null required')
    tree='pilot_outputTree_00000' if maximum is not None else 'outputTree_00000'
    if profile.get('workflow') != 'main' or profile.get('tree') != tree:
        raise GateError('mode', 'requested mode/profile mismatch','FAIL')
    return tree


def context(profile, maximum, scope, ledger):
    from tools.campaign_input_validation import require_complete
    tree=expected_tree(profile,maximum)
    require_complete(ledger, ledger.get('ordered_inputs'), maximum)
    expected=dict(schema_version=1,kind='processing_input_scope',total_input_entries=ledger['expected_total_entries'],
                  requested_max_input_events=maximum,analysis_input_entries=ledger['processed_entries'],
                  generator_weight_bookkeeping_scope='full_input_chain_when_mc',partial=maximum is not None,
                  full_production_eligible=maximum is None,tree_prefix='pilot_outputTree' if maximum is not None else 'outputTree')
    if scope != expected or any(type(scope[k]) is not type(v) for k,v in expected.items()):
        raise GateError('scope','processing scope inconsistent','FAIL')
    return tree


def digest(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as stream:
        for b in iter(lambda:stream.read(1048576),b''):h.update(b)
    return h.hexdigest()


def inspect(bundle, hashes, profile, maximum, scope=None, ledger=None, output=None):
    import ROOT,cppyy
    expected_tree(profile,maximum)
    b=Path(bundle).resolve(strict=True)
    for name in ['libnanoadrdframe.so','rootdict_rdict.pcm','src/Linkdef.h','src/utility.h']:
        p=b/name
        if not p.is_file() or b not in p.resolve().parents or hashes.get(name)!=digest(p):
            raise GateError('artifact_identity','missing/mismatched library, PCM or header: '+name)
    ROOT.gInterpreter.AddIncludePath(str(b));ROOT.gInterpreter.AddIncludePath(str(b/'src'))
    for lib in ['libcorrectionlib.so','libMathMore.so',str(b/'libnanoadrdframe.so')]:cppyy.load_reflection_info(lib)
    maps=sorted({line.split()[-1] for line in Path('/proc/self/maps').read_text().splitlines() if 'libnanoadrdframe.so' in line})
    if maps != [str(b/'libnanoadrdframe.so')]:raise GateError('library','unexpected mapped library')
    dictionaries={n:bool(ROOT.gROOT.GetClass(n) and ROOT.gROOT.GetClass(n).HasDictionary()) for n in ['NanoAODAnalyzerrdframe','BaseAnalyser','OSSF4LInfo']}
    if not all(dictionaries.values()):raise GateError('dictionary','compiled dictionary unavailable')
    if not hasattr(ROOT.BaseAnalyser,'getSnapshotProcessedEntries'):raise GateError('library','Phase A interface absent')
    pcm=ROOT.TFile.Open(str(b/'rootdict_rdict.pcm'),'READ')
    if not pcm or pcm.IsZombie():raise GateError('pcm','PCM metadata unreadable')
    try:keys=[k.GetName() for k in pcm.GetListOfKeys()]
    finally:pcm.Close()
    if not {'__Typedefs','__Enums','__ProtoClasses'}<=set(keys):raise GateError('pcm','incompatible PCM metadata')
    report=dict(status='PASS',phase='preflight' if output is None else 'output',dictionaries=dictionaries,library_maps=maps,
                pcm_keys=keys,artifact_hashes={n:hashes[n] for n in ['libnanoadrdframe.so','rootdict_rdict.pcm','src/Linkdef.h','src/utility.h']},
                mode='pilot' if maximum is not None else 'unlimited',root_version=ROOT.gROOT.GetVersion(),limitations=['PCM integrity/metadata and dictionary registration verified; Cling PCM consumption not traced.'])
    if output is None:return report
    tree_name=context(profile,maximum,scope,ledger)
    from tools.root_output_validation import validate_worker_result
    from tools.root_output_worker import inspect_root
    structure=inspect_root(ROOT,str(output),profile);validate_worker_result(structure,profile)
    if structure['issues']:
        failure='FAIL' if any(i['status']=='FAIL' for i in structure['issues']) else 'UNKNOWN'
        raise GateError('root_validation',json.dumps(structure['issues']),failure)
    f=ROOT.TFile.Open(str(output),'READ')
    if not f or f.IsZombie():raise GateError('root_open','output cannot be opened','FAIL')
    try:
        tree=f.Get(tree_name);other=f.Get('outputTree_00000' if maximum is not None else 'pilot_outputTree_00000')
        if other:raise GateError('mode','opposite-mode tree exists','FAIL')
        marker=f.Get('TZQProcessingProvenance');sidecar=Path(str(output)+'.input_scope.json')
        if maximum is None:
            if marker or sidecar.exists():raise GateError('mode','pilot provenance in unlimited output','FAIL')
        else:
            from tools.campaign_input_validation import strict_json
            if not marker or not marker.InheritsFrom('TNamed') or not sidecar.is_file() or strict_json(marker.GetTitle())!=scope or strict_json(sidecar.read_text())!=scope:
                raise GateError('scope','pilot marker/sidecar mismatch','FAIL')
        if not tree or not tree.InheritsFrom('TTree'):raise GateError('schema','required tree absent','FAIL')
        branch=tree.GetBranch('OSSF4L_info')
        if not branch or branch.GetClassName()!='OSSF4LInfo' or not tree.GetBranch('OSSF4L_category'):
            raise GateError('schema','custom object/category branch missing or wrong type','FAIL')
        n=int(tree.GetEntries())
        if n<0 or n>ledger['processed_entries']:raise GateError('entries','selected exceeds processed','FAIL')
        tree.SetBranchStatus('*',0);tree.SetBranchStatus('OSSF4L_category',1)
        defaults=[];nondefaults=[];scanned=0
        for index in range(min(n,10000)):
            if tree.GetBranch('OSSF4L_category').GetEntry(index)<=0:raise GateError('content','category read failed')
            scanned+=1;target=defaults if int(tree.OSSF4L_category)==0 else nondefaults
            if len(target)<3:target.append(index)
            if len(defaults)==3 and len(nondefaults)==3:break
        # Preserve the proven pilot reader: split child names need not share
        # the parent prefix, so activate all branches for the bounded samples.
        tree.SetBranchStatus('*',1);samples=[]
        for index in sorted(set(defaults+nondefaults)):
            if tree.GetEntry(index)<=0:raise GateError('content','custom object read failed')
            obj=tree.OSSF4L_info
            values={k:int(getattr(obj,k)) for k in ['category','idx1','idx2','idx3','idx4']}
            values.update({k:float(getattr(obj,k)) for k in ['mass1','mass2']})
            if values['category']!=int(tree.OSSF4L_category) or not all(math.isfinite(v) for v in values.values()):raise GateError('content','invalid custom object values','FAIL')
            samples.append(dict(entry=index,values=values))
        report.update(tree=tree_name,provenance_check='pilot_marker_and_sidecar_match' if maximum is not None else 'no_pilot_provenance',opposite_tree_absent=True,entries=n,branch_count=len(list(tree.GetListOfBranches())),scope=scope,
                      category_entries_scanned=scanned,default_indices=defaults,nondefault_indices=nondefaults,samples=samples,root_evidence=structure)
        if not defaults:report['limitations'].append('No default object found in inspected coverage.')
        if not nondefaults:report['limitations'].append('No non-default object found in inspected coverage.')
        return report
    finally:f.Close()


def validate_report(r, bundle, hashes, profile, maximum, scope, ledger, output):
    from tools.root_output_validation import validate_worker_result
    fields={'status','phase','dictionaries','library_maps','pcm_keys','artifact_hashes','mode','root_version','limitations'}
    if output is not None:fields|={'tree','entries','branch_count','scope','category_entries_scanned','default_indices','nondefault_indices','samples','root_evidence','provenance_check','opposite_tree_absent'}
    if not isinstance(r,dict) or set(r)!=fields or r.get('status')!='PASS' or r.get('phase')!=('output' if output is not None else 'preflight') or r.get('mode')!=('pilot' if maximum is not None else 'unlimited'):
        raise GateError('worker_protocol','wrong gate status/mode')
    if r.get('library_maps')!=[str(Path(bundle).resolve()/'libnanoadrdframe.so')] or not isinstance(r.get('pcm_keys'),list) or not {'__Typedefs','__Enums','__ProtoClasses'}<=set(r['pcm_keys']) or r.get('artifact_hashes')!={n:hashes[n] for n in ['libnanoadrdframe.so','rootdict_rdict.pcm','src/Linkdef.h','src/utility.h']} or r.get('dictionaries')!={n:True for n in ['NanoAODAnalyzerrdframe','BaseAnalyser','OSSF4LInfo']} or any(type(v) is not bool for v in r['dictionaries'].values()):
        raise GateError('worker_protocol','missing library/PCM/dictionary evidence')
    if not isinstance(r.get('root_version'),str) or not r['root_version'] or not isinstance(r.get('limitations'),list) or any(not isinstance(v,str) for v in r['limitations']):
        raise GateError('worker_protocol','invalid version/limitations')
    if output is None:return
    if r.get('opposite_tree_absent') is not True or r.get('provenance_check')!=('pilot_marker_and_sidecar_match' if maximum is not None else 'no_pilot_provenance'):
        raise GateError('worker_protocol','missing mode/provenance checks')
    if r.get('tree')!=context(profile,maximum,scope,ledger) or r.get('scope')!=scope:
        raise GateError('worker_protocol','scope/tree mismatch')
    evidence=r.get('root_evidence')
    try:validate_worker_result(evidence,profile)
    except Exception as exc:raise GateError('worker_protocol','invalid ROOT evidence: '+str(exc)) from exc
    n=r.get('entries');scanned=r.get('category_entries_scanned');samples=r.get('samples');defaults=r.get('default_indices');nondefaults=r.get('nondefault_indices')
    if type(n) is not int or not 0<=n<=ledger['processed_entries'] or evidence['issues'] or evidence['structure'].get('entries')!=n or type(r.get('branch_count')) is not int or r['branch_count']!=len(evidence['structure']['branch_types']) or type(scanned) is not int or not 0<=scanned<=min(n,10000) or not all(isinstance(v,list) for v in [samples,defaults,nondefaults]):
        raise GateError('worker_protocol','incomplete structure/coverage')
    indices=defaults+nondefaults
    if len(defaults)>3 or len(nondefaults)>3 or len(indices)!=len(set(indices)) or any(type(i) is not int or not 0<=i<scanned for i in indices) or [x.get('entry') for x in samples]!=sorted(indices):
        raise GateError('worker_protocol','sample index contradiction')
    if scanned!=min(n,10000) and not (len(defaults)==len(nondefaults)==3 and scanned==max(indices)+1):
        raise GateError('worker_protocol','incomplete category search')
    for sample in samples:
        values=sample.get('values')
        if not isinstance(values,dict) or set(values)!={'category','idx1','idx2','idx3','idx4','mass1','mass2'} or any(type(values[k]) is not int for k in ['category','idx1','idx2','idx3','idx4']) or any(type(values[k]) not in (int,float) or not math.isfinite(values[k]) for k in ['mass1','mass2']) or (values['category']==0)!=(sample['entry'] in defaults):
            raise GateError('worker_protocol','object evidence contradiction')
    if n and not indices:raise GateError('worker_protocol','nonempty tree has no inspected objects')


def gate(bundle, hashes, profile, maximum, work, scope=None, ledger=None, output=None):
    from campaign_process import run_process
    from campaign import load_json
    from campaign_production import write_json
    phase='output' if output is not None else 'preflight';request=Path(work)/('dictionary-'+phase+'-request.json');result=Path(work)/('dictionary-'+phase+'.json')
    write_json(request,dict(bundle=str(bundle),hashes=hashes,profile=profile,maximum=maximum,scope=scope,ledger=ledger,output=str(output) if output is not None else None))
    try:
        run_process([sys.executable,str(Path(__file__).resolve()),str(request),str(result)],45,cwd=bundle,log=Path(work)/('dictionary-'+phase+'.log'))
    except Exception as exc:
        try:
            failure=load_json(result)
        except Exception:
            failure=dict(status='UNKNOWN',code='worker_protocol' if result.exists() else getattr(exc,'code','worker_crash'),error=str(exc))
            target=result if not result.exists() else Path(work)/('dictionary-'+phase+'-failure.json')
            write_json(target,failure)
        if not isinstance(failure,dict) or set(failure)!={'status','code','error'} or failure.get('status') not in ('FAIL','UNKNOWN') or not isinstance(failure.get('code'),str) or not isinstance(failure.get('error'),str):
            raise GateError('worker_protocol','nonzero worker with contradictory evidence') from exc
        raise GateError(failure['code'],failure['error'],failure['status']) from exc
    r=load_json(result)
    validate_report(r,bundle,hashes,profile,maximum,scope,ledger,output)
    return r


def verify_publication(published, identity, scope, files, endpoint):
    from campaign_production import validate, execution_path, endpoint as normalize_endpoint
    endpoint=normalize_endpoint(endpoint)
    from campaign_staging import checksums
    receipt=validate(published['receipt']);marker=published['marker']
    dest=execution_path(identity['campaign_id'],identity['submission_id'],identity['workflow'],identity['dataset_id'],identity['chunk_id'],identity['execution_id'])
    if receipt.get('status')!='published' or receipt.get('acceptance')!='not_evaluated' or receipt.get('input_scope')!=scope or receipt.get('destination')!=dest or any(receipt.get(k)!=v for k,v in identity.items()):
        raise GateError('publication','receipt identity/scope mismatch','FAIL')
    for name,path in files.items():
        actual=checksums(path);ev=receipt['artifacts'][name];algorithm=receipt['checksum_algorithm']
        if any(ev.get(k)!=v for k,v in actual.items()) or ev.get('remote_checksum')!=dict(algorithm=algorithm,value=actual[algorithm]) or ev.get('locator')!=endpoint+'/'+dest+'/'+name:
            raise GateError('publication','receipt artifact mismatch','FAIL')
    actual=checksums(Path(files['execution.json']).parent/'publication.json')
    if marker.get('remote_checksum')!=dict(algorithm=receipt['checksum_algorithm'],value=actual[receipt['checksum_algorithm']]) or any(marker.get(k)!=v for k,v in actual.items()) or marker.get('locator')!=endpoint+'/'+dest+'/publication.json':
        raise GateError('publication','receipt marker mismatch','FAIL')


if __name__=='__main__':
    sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
    from tools.campaign import load_json
    import resource
    resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    resource.setrlimit(resource.RLIMIT_CPU,(40,40));resource.setrlimit(resource.RLIMIT_AS,(4*1024**3,4*1024**3))
    request=load_json(sys.argv[1])
    try:result=inspect(**request)
    except Exception as exc:result=dict(status=getattr(exc,'status','UNKNOWN'),code=getattr(exc,'code','inspection'),error=str(exc))
    with Path(sys.argv[2]).open('x') as stream:json.dump(result,stream,indent=2,allow_nan=False)
    print(json.dumps(result,allow_nan=False));sys.exit(0 if result['status']=='PASS' else 1)
