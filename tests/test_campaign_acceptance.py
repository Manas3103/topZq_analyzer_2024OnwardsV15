"""Synthetic metadata with saved ROOT validation evidence; no ROOT copies or EOS."""
import copy
import hashlib
import json
import multiprocessing
import os
from pathlib import Path
import sys
import tempfile
import unittest
from urllib.parse import urlsplit
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import campaign_acceptance as a

BASE=Path('/uscms_data/d3/msahoo/Project_tzq')
PB=BASE/'production_records/2024/phase_b_local_validation/validation-ecc704f9d54a48cb8e589fc658da7e8e'
PA=BASE/'production_records/2024/phase_a_local_validation/validation-626ebeb8c587465e838ae5176c058e69'
OLD=BASE/'production_bundles/2024/production-validation-main-tzq-2024-1de3bd3adb6d437a9bf9cdf779ffc676/submission-retry-08f12d8dacef41918cdf9a40a9e7708e/bundle'
OLD_EX=BASE/'production_records/2024/production-validation-main-tzq-2024-1de3bd3adb6d437a9bf9cdf779ffc676/submission-retry-08f12d8dacef41918cdf9a40a9e7708e/campaign_executions/fa3842004865455996fce345935f922e'

def write(p,v):p.write_text(json.dumps(v,indent=2,allow_nan=False))
def race(store,candidate,queue):
 try:r=a.update(store,None,candidate,'accept','manual test','test authorization',authorize=True);queue.put(('PASS',r['revision']))
 except Exception as e:queue.put(('FAIL',str(e)))

def kill_after_revision(store,candidate):
    import signal
    original=a.immutable
    def interrupted(path,value):
        original(path,value)
        if path.parent.name=='revisions':os.kill(os.getpid(),signal.SIGKILL)
    a.immutable=interrupted
    a.update(store,None,candidate,'accept','synthetic kill test','unit-test authorization',authorize=True)


class IndexTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='phasec-test-');self.addCleanup(self.tmp.cleanup);self.root=Path(self.tmp.name)
        self.c=a.load_json(OLD/'campaign.json');self.c['campaign_id']='synthetic-phasec';self.c['max_input_events']=None
        results=a.load_json(PB/'logs/real_results.json');self.g=copy.deepcopy(next(x['report'] for x in results if x['case']=='unlimited'))
        pre=a.load_json(PB/'logs/cli-final-unlimited/dictionary-preflight.json');self.pre=pre
        self.profile=a.load_json(Path(__file__).resolve().parents[1]/'docs/root_validation_profiles/main-mc.json');self.profile_path=self.root/'profile.json';write(self.profile_path,self.profile)
        self.c['datasets'][0]['profile']='production-profile.json'
        self.c['processing_files'].update(self.g['artifact_hashes'])
        if 'production-profile.json' in self.c['processing_files']:self.c['processing_files']['production-profile.json']=a.digest(self.profile_path)
        self.campaign_path=self.root/'campaign.json';write(self.campaign_path,self.c)
        self.spec=a.scope(self.c,'main','full','two-chunk-main',campaign_record=self.campaign_path)
        self.store=self.root/'store';self.store.mkdir();a.initialize(self.store,self.spec)
    def candidate(self,chunk='chunk0',submission='s1',execution='1'*32):
        d=self.root/(chunk+'-'+submission+'-'+execution);d.mkdir()
        ex=a.load_json(OLD_EX/'execution.json');m=a.load_json(OLD/'manifest.json');pub=a.load_json(OLD_EX/'publication.json')
        ident=dict(campaign_id=self.c['campaign_id'],submission_id=submission,workflow='main',dataset_id='tzqb-zto2l',chunk_id=chunk,attempt_id=a.attempt_id(self.c['campaign_id'],submission,'main','tzqb-zto2l',chunk),execution_id=execution,cluster_id=1,proc_id=0)
        ex.update(ident);ex['campaign_sha256']=a.digest(self.campaign_path);m.update(campaign_id=ident['campaign_id'],submission_id=submission,campaign_sha256=ex['campaign_sha256'])
        target=next(x for x in self.c['datasets'][0]['chunks'] if x['chunk_id']==chunk);inputs=target['ordered_inputs'];job=copy.deepcopy(next(x for x in m['jobs'] if x['chunk_id']==chunk))
        job.update(attempt_id=ident['attempt_id'],profile_sha256=a.digest(self.profile_path));m['jobs']=[job];m['files'].update(self.g['artifact_hashes']);m['files']['production-profile.json']=a.digest(self.profile_path);m['files']['campaign.json']=ex['campaign_sha256']
        ex['payload_hashes']=m['files'];ex['profile_sha256']=job['profile_sha256'];ex['ordered_inputs']=inputs;ex['filelist_sha256']=job['filelist_sha256'];ex['source']=self.c['source'];ex['processing_schema_id']=self.c['datasets'][0]['processing_schema_id']
        at=dict(schema_version=1,kind='production_attempt',**{n:ident[n] for n in ('campaign_id','submission_id','workflow','dataset_id','chunk_id','attempt_id')},filelist_sha256=job['filelist_sha256'],config_sha256=job['config_sha256'],created_at=m['created_at']);write(d/'attempt.json',at);m['files']['attempts/main/'+ident['attempt_id']+'.json']=a.digest(d/'attempt.json')
        mp=d/'manifest.json';write(mp,m);ex['manifest_sha256']=a.digest(mp)
        scope=copy.deepcopy(self.g['scope']);ex['input_scope']=scope
        ledger=dict(schema_version=1,kind='campaign_input_ledger',status='PASS',completion='PASS',tree='Events',ordered_inputs=inputs,expected_total_entries=141000,chain_total_entries=141000,processed_entries=141000,snapshot_processed_entries=[141000],requested_max_input_events=None,files=[])
        counts=[141000//len(inputs)]*len(inputs);counts[-1]+=141000-sum(counts)
        for i,(loc,n) in enumerate(zip(inputs,counts)):ledger['files'].append(dict(index=i,locator=loc,identity='/'+urlsplit(loc).path.lstrip('/'),tree='Events',status='PASS',entries=n,empty=False,chain_participation='added',uuid='synthetic-'+str(i)))
        evidence=ex['root_validation'];obs=evidence['outputs'][0];obs['identity'].update({n:ident[n] for n in ('campaign_id','workflow','dataset_id','chunk_id','attempt_id')});obs['profile']=dict(profile_id=self.profile['profile_id'],schema_version=1)
        obs['inspected_path']='/worker/campaign_executions/'+execution+'/output.root';obs['file']['size_bytes']=(PA/'outputs/source-unlimited.root').stat().st_size
        obs['structure']=copy.deepcopy(self.g['root_evidence']['structure']);obs['content']=copy.deepcopy(self.g['root_evidence']['content']);obs['issues']=[]
        evidence.update(input_completeness=ledger,dictionary_checks=dict(preflight=copy.deepcopy(self.pre),output=copy.deepcopy(self.g)))
        ex['schema_fingerprint']=a.object_hash(dict(tree=self.profile['tree'],branches=obs['structure']['branch_types']))
        ep=d/'execution.json';write(ep,ex);log=d/'processing.log';log.write_text('synthetic execution metadata fixture; no analyzer run\n')
        dest=a.execution_path(ident['campaign_id'],submission,'main','tzqb-zto2l',chunk,execution);root=PA/'outputs/source-unlimited.root';local={'output.root':str(root),'processing.log':str(log),'execution.json':str(ep),'publication.json':str(d/'publication.json')}
        pub.update(ident);pub.update(destination=dest,input_scope=scope,root_validation=evidence,artifacts={})
        for name in ['output.root','processing.log','execution.json']:
            sums=a.checksums(local[name]);pub['artifacts'][name]=dict(locator=self.c['endpoint']+'/'+dest+'/'+name,**sums,remote_checksum=dict(algorithm='adler32',value=sums['adler32']))
        write(d/'publication.json',pub);marker=a.checksums(d/'publication.json');result=dict(status='published',processing_exit=0,validation_status='PASS',staging_exit=0,identity=ident,input_scope=scope,destination=dest,publication_marker=dict(locator=self.c['endpoint']+'/'+dest+'/publication.json',**marker,remote_checksum=dict(algorithm='adler32',value=marker['adler32'])))
        write(d/'result.json',result)
        return dict(execution=str(ep),publication=str(d/'publication.json'),result=str(d/'result.json'),manifest=str(mp),profile=str(self.profile_path),attempt=str(d/'attempt.json'),local_artifacts=local)
    def decide(self,c,parent=None,action='accept',supersedes=None):return a.update(self.store,parent,c,action,'explicit synthetic test decision','unit-test authorization',supersedes,True)
    def test_single_is_incomplete_for_two_chunks(self):
        r=self.decide(self.candidate());self.assertEqual(r['status'],'INCOMPLETE')
        with self.assertRaises(a.AcceptanceError):a.manifest(r)
    def test_complete_declared_scope(self):
        r=self.decide(self.candidate());r=self.decide(self.candidate('chunk1'),r['revision']);m=a.recheck(r);self.assertEqual(m['status'],'COMPLETE');self.assertEqual(len(m['inputs']),2)
    def test_duplicate_success_requires_choice(self):
        r=self.decide(self.candidate());c=self.candidate(submission='s2',execution='2'*32)
        with self.assertRaises(a.AcceptanceError):self.decide(c,r['revision'])
        self.assertEqual(a.current(self.store)[1]['revision'],r['revision'])
    def test_explicit_supersession(self):
        r=self.decide(self.candidate());r=self.decide(self.candidate(submission='s2',execution='2'*32),r['revision'],'supersede','1'*32);self.assertEqual(r['entries'][0]['identity']['execution_id'],'2'*32);self.assertEqual(r['history'][-1]['prior_identity']['execution_id'],'1'*32)
    def test_stale_parent(self):
        self.decide(self.candidate())
        with self.assertRaises(a.AcceptanceError):self.decide(self.candidate('chunk1'),None)
    def test_failure_and_partial(self):
        for field,value in [('status','failed'),('processing_exit',1),('input_scope',{'partial':True})]:
            c=self.candidate(submission='s-'+field);x=a.load_json(c['execution']);x[field]=value;write(Path(c['execution']),x)
            with self.subTest(field=field),self.assertRaises(Exception):self.decide(c)
        self.assertFalse((self.store/'HEAD.json').exists())
    def test_missing_phase_evidence(self):
        for name in ['input_completeness','dictionary_checks']:
            c=self.candidate(submission='s-'+name);x=a.load_json(c['execution']);x['root_validation'].pop(name);write(Path(c['execution']),x)
            with self.subTest(name=name),self.assertRaises(Exception):self.decide(c)
    def test_bad_receipt(self):
        c=self.candidate();p=a.load_json(c['publication']);p['destination']+='-wrong';write(Path(c['publication']),p)
        with self.assertRaises(Exception):self.decide(c)
    def test_missing_root(self):
        c=self.candidate();c['local_artifacts']['output.root']=str(self.root/'missing.root')
        with self.assertRaises(Exception):self.decide(c)
    def test_checksum(self):
        c=self.candidate();f=self.root/'wrong.root';f.write_bytes(b'wrong bytes');c['local_artifacts']['output.root']=str(f)
        with self.assertRaises(Exception):self.decide(c)
    def test_concurrent_writers(self):
        c=self.candidate();q=multiprocessing.Queue();ps=[multiprocessing.Process(target=race,args=(str(self.store),c,q)) for _ in range(2)]
        for p in ps:p.start()
        for p in ps:p.join(20);self.assertFalse(p.is_alive())
        rows=[q.get(timeout=2) for _ in ps];self.assertEqual(sorted(x[0] for x in rows),['FAIL','PASS']);self.assertEqual(len(a.current(self.store)[1]['entries']),1)
    def test_interrupted_update(self):
        with self.assertRaises(a.AcceptanceError):a.update(self.store,None,self.candidate(),'accept','reason','test',authorize=True,fault='after_revision')
        self.assertIsNone(a.current(self.store)[0]);self.assertEqual(len(list((self.store/'revisions').glob('*.json'))),1)
    def test_revision_corruption(self):
        r=self.decide(self.candidate());path=self.store/'revisions'/(r['revision']+'.json');path.write_text('{}')
        with self.assertRaises(Exception):a.current(self.store)
    def test_deterministic_regeneration(self):
        c=self.candidate();e=a.verify_candidate(self.spec,c);d=dict(action='accept',operator='msahoo',timestamp='2026-10-13T00:00:00+00:00',reason='test',authorization_reference='test',supersedes=None)
        self.assertEqual(a.build_revision(self.spec,None,None,[(d,e)]),a.build_revision(self.spec,None,None,[(d,e)]))
    def test_persistent_activation_blocked(self):
        with self.assertRaises(a.AcceptanceError):a.safe_store(a.PROSPECTIVE_STORE)
    def test_manual_authorization_required(self):
        with self.assertRaises(a.AcceptanceError):a.update(self.store,None,self.candidate(),'accept','reason','test')
    def test_superseded_reference_wrong(self):
        r=self.decide(self.candidate())
        with self.assertRaises(a.AcceptanceError):self.decide(self.candidate(submission='s2',execution='2'*32),r['revision'],'supersede','3'*32)
    def test_symlink_store(self):
        link=self.root/'alias';link.symlink_to(self.store)
        with self.assertRaises(a.AcceptanceError):a.safe_store(link)
    def test_history_tamper(self):
        r=self.decide(self.candidate());r['history'][0]['operator']='';unsigned=dict(r);unsigned.pop('revision');r['revision']='r-'+a.object_hash(unsigned)
        with self.assertRaises(a.AcceptanceError):a.validate_revision(r)
    def test_metadata_changed_after_acceptance(self):
        c=self.candidate();r=self.decide(c);Path(c['result']).write_text('{}')
        with self.assertRaises(Exception):a.recheck(r)

    def test_scoped_mc_subset_not_full_campaign(self):
        c=copy.deepcopy(self.c);data=copy.deepcopy(c['datasets'][0]);data.update(dataset_id='muoneg-c',datatype='data',era='C',stream='MuonEG',das_dataset='/MuonEG/Run2024C-MINIv6NANOv15-v1/NANOAOD',sample_name='MuonEG-C');data['chunks']=[dict(schema_version=1,kind='production_chunk',chunk_id='chunk0',index=0,ordered_inputs=['root://cmsxrootd.fnal.gov//store/data/Run2024C/MuonEG/NANOAOD/MINIv6NANOv15-v1/0000/test.root'])];c['datasets'].append(data)
        path=self.root/'mixed-campaign.json';write(path,c)
        mc=a.scope(c,'main','mc','mc-scope',campaign_record=path);dt=a.scope(c,'main','data','data-scope',campaign_record=path);full=a.scope(c,'main','full','full-scope',campaign_record=path)
        self.assertEqual(len(mc['expected']),2);self.assertEqual(len(dt['expected']),1);self.assertEqual(len(full['expected']),3);self.assertFalse(mc['campaign_complete_scope']);self.assertFalse(dt['campaign_complete_scope']);self.assertTrue(full['campaign_complete_scope'])
    def test_invalid_scope(self):
        bad=copy.deepcopy(self.spec);bad['expected']=bad['expected'][:1]
        with self.assertRaises(a.AcceptanceError):a.validate_scope(bad)
    def test_duplicate_chunk_revision(self):
        r=self.decide(self.candidate());r['entries'].append(copy.deepcopy(r['entries'][0]));u=dict(r);u.pop('revision');r['revision']='r-'+a.object_hash(u)
        with self.assertRaises(a.AcceptanceError):a.validate_revision(r)
    def test_bad_manifest_identity(self):
        c=self.candidate();m=a.load_json(c['manifest']);m['submission_id']='other';write(Path(c['manifest']),m)
        with self.assertRaises(Exception):self.decide(c)
    def test_lock_timeout(self):
        with a.locked(self.store):
            with self.assertRaises(a.AcceptanceError):
                with a.locked(self.store,timeout=.03):pass
    def test_wrong_coordinator(self):
        store=a.load_json(self.store/'STORE.json');store['host']='another-host';write(self.store/'STORE.json',store)
        with self.assertRaises(a.AcceptanceError):self.decide(self.candidate())
    def test_before_head_interruption(self):
        with self.assertRaises(a.AcceptanceError):a.update(self.store,None,self.candidate(),'accept','reason','test',authorize=True,fault='before_head')
        self.assertIsNone(a.current(self.store)[0])
    def test_after_head_error_recovery(self):
        c=self.candidate()
        with self.assertRaises(a.AcceptanceError):a.update(self.store,None,c,'accept','reason','test',authorize=True,fault='after_head')
        self.assertIsNotNone(a.current(self.store)[0])
        with self.assertRaises(a.AcceptanceError):self.decide(c,None)
    def test_internal_symlink(self):
        (self.store/'HEAD.json').symlink_to(self.root/'elsewhere')
        with self.assertRaises(a.AcceptanceError):a.safe_store(self.store)
    def test_bool_version(self):
        spec=copy.deepcopy(self.spec);spec['schema_version']=True
        with self.assertRaises(a.AcceptanceError):a.validate_scope(spec)

    def test_cli_validation(self):
        import subprocess
        c=self.candidate();r=self.decide(c);r=self.decide(self.candidate('chunk1'),r['revision']);path=self.store/'revisions'/(r['revision']+'.json')
        x=subprocess.run([sys.executable,str(Path(a.__file__)),'validate','--store',str(self.store),'--revision',str(path)],capture_output=True,text=True)
        self.assertEqual(x.returncode,0,x.stderr);m=json.loads(x.stdout);self.assertEqual(m['revision_sha256'],a.digest(path))
        mf=self.root/'manifest.json';write(mf,m);m['inputs'].append(copy.deepcopy(m['inputs'][0]));write(mf,m)
        x=subprocess.run([sys.executable,str(Path(a.__file__)),'validate-manifest','--store',str(self.store),'--revision',str(path),'--manifest',str(mf)],capture_output=True,text=True)
        self.assertNotEqual(x.returncode,0)
    def test_cli_rejects_superseded_revision(self):
        import subprocess
        r=self.decide(self.candidate());old=self.store/'revisions'/(r['revision']+'.json');self.decide(self.candidate(submission='s2',execution='2'*32),r['revision'],'supersede','1'*32)
        x=subprocess.run([sys.executable,str(Path(a.__file__)),'validate','--store',str(self.store),'--revision',str(old)],capture_output=True,text=True)
        self.assertNotEqual(x.returncode,0);self.assertIn('non-authoritative',x.stderr)
    def test_schemas(self):
        from jsonschema import Draft202012Validator
        r=self.decide(self.candidate());r=self.decide(self.candidate('chunk1'),r['revision'])
        root=Path(__file__).resolve().parents[1]/'schemas/production'
        for name,value in [('accepted_scope.schema.json',self.spec),('accepted_revision.schema.json',r),('accepted_plot_inputs.schema.json',a.manifest(r))]:
            schema=a.load_json(root/name);Draft202012Validator.check_schema(schema);Draft202012Validator(schema).validate(value)
    def test_bad_attempt_record(self):
        c=self.candidate();record=a.load_json(c['attempt']);record['submission_id']='other';write(Path(c['attempt']),record)
        with self.assertRaises(Exception):self.decide(c)
    def test_schema_evidence_failure(self):
        c=self.candidate();ex=a.load_json(c['execution']);ex['root_validation']['dictionary_checks']['output']['status']='UNKNOWN';write(Path(c['execution']),ex)
        with self.assertRaises(Exception):self.decide(c)
    def test_invalid_json_metadata(self):
        c=self.candidate();Path(c['execution']).write_text('{"status":"validated","status":"validated"}')
        with self.assertRaises(Exception):self.decide(c)
    def test_scope_unsupported_version(self):
        s=copy.deepcopy(self.spec);s['schema_version']=2
        with self.assertRaises(a.AcceptanceError):a.validate_scope(s)

    def test_concurrent_different_chunks_no_lost_update(self):
        candidates=[self.candidate(),self.candidate('chunk1')];q=multiprocessing.Queue();ps=[multiprocessing.Process(target=race,args=(str(self.store),c,q)) for c in candidates]
        for p in ps:p.start()
        for p in ps:p.join(20);self.assertFalse(p.is_alive())
        rows=[q.get(timeout=2) for _ in ps];self.assertEqual(sorted(r[0] for r in rows),['FAIL','PASS']);self.assertEqual(a.current(self.store)[1]['status'],'INCOMPLETE')

    def test_actual_killed_writer(self):
        import signal
        p=multiprocessing.Process(target=kill_after_revision,args=(str(self.store),self.candidate()));p.start();p.join(10)
        if p.is_alive():p.kill();p.join(2);self.fail('writer did not reach bounded kill point')
        self.assertEqual(p.exitcode,-signal.SIGKILL);self.assertIsNone(a.current(self.store)[0]);self.assertEqual(len(list((self.store/'revisions').glob('*.json'))),1)
        with a.locked(self.store,timeout=.2):pass

    def test_historical_success_without_phases_not_accepted(self):
        campaign=a.load_json(OLD/'campaign.json');spec=a.scope(campaign,'main','full','historical-review',campaign_record=OLD/'campaign.json');m=a.load_json(OLD/'manifest.json');job=next(j for j in m['jobs'] if j['chunk_id']=='chunk0')
        candidate=dict(execution=str(OLD_EX/'execution.json'),publication=str(OLD_EX/'publication.json'),result=str(OLD_EX/'result.json'),manifest=str(OLD/'manifest.json'),profile=str(OLD/'production-profile.json'),attempt=str(OLD/'attempts/main'/(job['attempt_id']+'.json')),local_artifacts={name:str(OLD_EX/name) for name in ['output.root','processing.log','execution.json','publication.json']})
        with self.assertRaises(a.AcceptanceError) as caught:a.verify_candidate(spec,candidate)
        self.assertIn('Phase A',str(caught.exception))

    def test_root_validation_wrong_artifact(self):
        c=self.candidate();ex=a.load_json(c['execution']);ex['root_validation']['outputs'][0]['file']['size_bytes']+=1;write(Path(c['execution']),ex)
        with self.assertRaises(Exception):self.decide(c)

if __name__=='__main__':unittest.main()
