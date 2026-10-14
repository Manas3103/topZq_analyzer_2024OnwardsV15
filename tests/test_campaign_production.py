"""Synthetic local bundles/processes and in-memory endpoint simulation only."""
import copy
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import campaign_production as p
import campaign_worker as w
import campaign_staging as s
from campaign_process import run_process, CommandError


class ProductionTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name); self.source = self.root / 'source'; self.source.mkdir()
        (self.source / 'processnanoaod_v.py').write_text('# synthetic test payload, never run\n')
        (self.source / 'libnanoadrdframe.so').write_bytes(b'synthetic library; never loaded')
        (self.source / 'rootdict_rdict.pcm').write_bytes(b'synthetic PCM')
        (self.source / 'src').mkdir()
        for header in ['NanoAODAnalyzerrdframe.h','BaseAnalyser.h','FakeFactorAnalyser.h','Linkdef.h','utility.h','RNodeTree.h']:
            (self.source / 'src' / header).write_text('// synthetic local header\n')
        self.profile = dict(schema_version=1, kind='root_validation_profile', profile_id='test', workflow='main', datatype='mc',
                            tree='outputTree_00000', required_branches=['run'], branch_types={'run':'UInt_t'}, required_objects={},
                            allow_zero_entries=True, content=dict(mode='metadata', branches=[], sample_entries=3), dictionary_branches=[])
        (self.source / 'profile.json').write_text(json.dumps(self.profile))
        self.config = dict(year=2024, datatype=0, intreename='Events', outtreename='outputTree')
        self.flags = dict(nrootfiles=0)
        self.save_config()
        chunk = dict(schema_version=1, kind='production_chunk', chunk_id='c0', index=0,
                     ordered_inputs=['root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/Synthetic/NANOAODSIM/a.root'])
        self.dataset = dict(schema_version=1, kind='production_dataset', dataset_id='sample', sample_name='Synthetic',
                            year=2024, workflow='main', datatype='mc', era=None, stream=None, das_dataset='/Synthetic/RunIII2024Summer24NanoAODv15-v1/NANOAODSIM',
                            processing_schema_id='main-mc-test', config='config.py', profile='profile.json', chunks=[chunk])
        self.campaign = dict(schema_version=1, kind='production_campaign', campaign_id='test', year=2024,
                             created_at='2026-10-12T00:00:00Z', source=dict(commit='a'*40, repository='synthetic', dirty=False, source_sha256='b'*64),
                             endpoint='root://cmseos.fnal.gov', processing_timeout=10, max_input_events=None,
                             publication_policy=dict(strategy='manifest_last', checksum_algorithm='adler32', no_clobber_confirmed=True, command_timeout=5, overall_timeout=30),
                             datasets=[self.dataset])
        self.bundle = self.root / 'bundle'

    def save_config(self):
        (self.source / 'config.py').write_text('config = ' + repr(self.config) + '\nprocflags = ' + repr(self.flags) + '\n')

    def prepare(self, destination=None, submission='s1'):
        return p.prepare(self.campaign, submission, self.source, destination or self.bundle, [])

    def invoke(self, _omit_ledger=False, **kwargs):
        if not self.bundle.exists(): self.prepare()
        manifest = p.load_json(self.bundle / 'manifest.json'); job = manifest['jobs'][0]
        identity = dict(campaign_id='test', submission_id='s1', **{k:job[k] for k in ('workflow','dataset_id','chunk_id','attempt_id')}, cluster_id=1, proc_id=0)
        def processor(args, timeout, **options):
            Path(args[3]).write_bytes(b'synthetic root output')
            Path(options['log']).write_text('Total Number of Entries: 1\nsynthetic processing completed\n')
            return subprocess.CompletedProcess(args, 0, '', '')
        def validator(output, selected, identity, job, dataset, work):
            return dict(status='PASS', acceptance_changed=False, outputs=[dict(identity={k:identity[k] for k in ('campaign_id','workflow','dataset_id','chunk_id','attempt_id')}, inspected_path=str(output),
                        status='PASS', acceptance='not_evaluated', structure=dict(branch_types={'run':'UInt_t'}, entries=0))])
        def publisher(*args):
            return dict(receipt=dict(status='published'), marker=dict(locator='synthetic', sha256='b'*64))
        actual_processor = kwargs.pop('processor', processor)
        def processor_with_ledger(args, timeout, **options):
            completed = actual_processor(args, timeout, **options)
            output = Path(args[3]); log = Path(options['log'])
            if (not _omit_ledger and completed.returncode == 0 and output.is_file()
                    and log.is_file() and not (output.parent / 'input_validation.json').exists()):
                import re
                from tools.campaign_input_validation import identity as input_identity
                totals = [int(match.group(1)) for line in log.read_text().splitlines()
                          if (match := re.match(r'^Total Number of Entries:\s*([0-9]+)\s*$', line))]
                if len(totals) == 1 and totals[0] > 0:
                    inputs = (self.bundle / job['filelist']).read_text().splitlines()
                    maximum = p.load_json(self.bundle / 'campaign.json').get('max_input_events')
                    processed = totals[0] if maximum is None else min(totals[0], maximum)
                    records = [dict(index=index, locator=locator, identity=input_identity(locator),
                                    status='PASS', tree='Events', entries=totals[0], uuid='synthetic',
                                    empty=False, chain_participation='added')
                               for index, locator in enumerate(inputs)]
                    ledger = dict(schema_version=1, kind='campaign_input_ledger', status='PASS',
                                  completion='PASS', tree='Events', ordered_inputs=inputs, files=records,
                                  expected_total_entries=totals[0], chain_total_entries=totals[0],
                                  processed_entries=processed, snapshot_processed_entries=[processed],
                                  requested_max_input_events=maximum)
                    (output.parent / 'input_validation.json').write_text(json.dumps(ledger))
            return completed
        defaults=dict(processor=processor_with_ledger, validator=validator, publisher=publisher,
                      runtime_probe=lambda:dict(cmssw='CMSSW_13_3_3',architecture='el9_amd64_gcc12',python='3.9',root='synthetic'))
        defaults.update(kwargs)
        # Worker orchestration tests use synthetic non-loadable library bytes;
        # the dictionary gate has its own focused tests and is mocked here.
        with patch('campaign_dictionary_validation.gate', return_value=dict(status='PASS', phase='mocked')), \
             patch('campaign_dictionary_validation.verify_publication'):
            return w.execute(self.bundle, p.digest(self.bundle/'manifest.json'), identity, self.root/'executions', **defaults)

    def test_deterministic_bundle(self):
        first=self.prepare(); second=self.prepare(self.root/'bundle2')
        self.assertEqual(first,second)
        self.assertEqual((self.bundle/'manifest.json').read_bytes(),(self.root/'bundle2/manifest.json').read_bytes())
        self.assertEqual((self.bundle/'filelists/main/sample/c0.txt').read_text(),self.dataset['chunks'][0]['ordered_inputs'][0]+'\n')

    def test_unique_attempts_for_chunks(self):
        c=copy.deepcopy(self.dataset['chunks'][0]);c.update(chunk_id='c1',index=1,ordered_inputs=[c['ordered_inputs'][0].replace('a.root','b.root')]);self.dataset['chunks'].append(c)
        m=self.prepare();self.assertEqual(len({j['attempt_id'] for j in m['jobs']}),2)

    def test_submission_changes_attempt_identity(self):
        a=self.prepare();b=self.prepare(self.root/'b2','s2')
        self.assertNotEqual(a['jobs'][0]['attempt_id'],b['jobs'][0]['attempt_id'])
        self.assertEqual(a['campaign_sha256'],b['campaign_sha256'])

    def test_duplicate_dataset(self):
        self.campaign['datasets'].append(copy.deepcopy(self.dataset))
        with self.assertRaises(ValueError):self.prepare()
        self.assertFalse(self.bundle.exists())

    def test_duplicate_chunk(self):
        self.dataset['chunks'].append(copy.deepcopy(self.dataset['chunks'][0]))
        with self.assertRaises(ValueError):self.prepare()

    def test_duplicate_input(self):
        c=copy.deepcopy(self.dataset['chunks'][0]);c.update(chunk_id='c1',index=1);self.dataset['chunks'].append(c)
        with self.assertRaises(ValueError):self.prepare()

    def test_original_input_restrictions(self):
        for val in ['root://h//store/user/x.root','/tmp/a.root','root://h//store/mc/../a.root','root://u:p@h//store/mc/a.root','root://h//store/mc/a.root?x','root://h//store/mc/a*.root']:
            with self.subTest(val=val),self.assertRaises(ValueError):p.nano_input(val,'mc')

    def test_data_mc_identity(self):
        self.dataset['datatype']='data'
        with self.assertRaises(ValueError):self.prepare()

    def test_unsafe_identifiers_and_paths(self):
        for val in ['../x','a/b','x y','x\n','']:
            with self.subTest(val=val),self.assertRaises(ValueError):p.execution_path(val,'s','main','d','c','e')
        for val in ['/tmp/x','../x','a//b','a/./b','x;touch']:
            with self.subTest(val=val),self.assertRaises(ValueError):p.relative(val)

    def test_main_fakefactor_separation(self):
        ff=copy.deepcopy(self.dataset);ff.update(workflow='fakefactor',config='ff.py',profile='ff.json')
        self.campaign['datasets'].append(ff)
        config=dict(self.config,analysertype='FakeFactorAnalyser');(self.source/'ff.py').write_text('config = '+repr(config)+'\nprocflags = '+repr(self.flags)+'\n')
        (self.source/'ff.json').write_text(json.dumps(dict(self.profile,workflow='fakefactor')))
        m=self.prepare();self.assertEqual({j['workflow'] for j in m['jobs']},{'main','fakefactor'})
        self.assertTrue((self.bundle/'queues/fakefactor.txt').read_text())
        self.assertNotEqual(m['jobs'][0]['attempt_id'],m['jobs'][1]['attempt_id'])

    def test_wrong_profile(self):
        (self.source/'profile.json').write_text(json.dumps(dict(self.profile,datatype='data')))
        with self.assertRaises(ValueError):self.prepare()

    def test_config_truncation(self):
        self.flags['nrootfiles']=1;self.save_config();self.dataset['chunks'][0]['ordered_inputs'].append(self.dataset['chunks'][0]['ordered_inputs'][0].replace('a.root','b.root'))
        with self.assertRaises(ValueError):self.prepare()

    def test_config_workflow_mismatch(self):
        self.config['analysertype']='FakeFactorAnalyser';self.save_config()
        with self.assertRaises(ValueError):self.prepare()

    def test_payload_required(self):
        (self.source/'libnanoadrdframe.so').unlink()
        with self.assertRaises(ValueError):self.prepare()

    def test_payload_symlink(self):
        (self.source/'libnanoadrdframe.so').unlink();(self.source/'libnanoadrdframe.so').symlink_to('/etc/hosts')
        with self.assertRaises(ValueError):self.prepare()

    def test_collision_refused(self):
        self.prepare();before=(self.bundle/'manifest.json').read_bytes()
        with self.assertRaises(ValueError):self.prepare()
        self.assertEqual(before,(self.bundle/'manifest.json').read_bytes())

    def test_hash_tampering(self):
        self.prepare();sha=p.digest(self.bundle/'manifest.json');(self.bundle/'config.py').write_text('tampered')
        with self.assertRaises(ValueError):p.verify_bundle(self.bundle,sha)

    def test_manifest_hash_tampering(self):
        self.prepare()
        with self.assertRaises(ValueError):p.verify_bundle(self.bundle,'0'*64)

    def test_manifest_identity_conflict(self):
        self.prepare();m=p.load_json(self.bundle/'manifest.json');m['jobs'][0]['attempt_id']='wrong';(self.bundle/'manifest.json').write_text(json.dumps(m))
        with self.assertRaises(ValueError):p.verify_bundle(self.bundle,p.digest(self.bundle/'manifest.json'))

    def test_schema_versions(self):
        self.campaign['schema_version']=2
        with self.assertRaises(ValueError):self.prepare()

    def test_schema_unknown_fields(self):
        self.campaign['luminosity']=12
        with self.assertRaises(ValueError):self.prepare()

    def test_success_and_no_acceptance(self):
        result,work=self.invoke();self.assertEqual(result['status'],'published');self.assertEqual(result['acceptance'],'not_evaluated');self.assertEqual(result['staging_exit'],0)
        self.assertTrue((work/'execution.json').is_file());self.assertTrue((work/'result.json').is_file())

    def test_execution_restart_identity(self):
        a,wa=self.invoke();b,wb=self.invoke();self.assertNotEqual(a['identity']['execution_id'],b['identity']['execution_id']);self.assertNotEqual(wa,wb)
        self.assertEqual(a['identity']['attempt_id'],b['identity']['attempt_id'])

    def test_processing_failure(self):
        def processor(*args,**kwargs):raise CommandError('nonzero_exit','test processing failure',7)
        publisher=unittest.mock.Mock();result,work=self.invoke(processor=processor,publisher=publisher)
        self.assertEqual(result['processing_exit'],7);self.assertEqual(result['status'],'failed');publisher.assert_not_called();self.assertTrue((work/'result.json').exists())

    def test_processing_timeout(self):
        result,_=self.invoke(processor=lambda *a,**k:(_ for _ in ()).throw(CommandError('timeout','test timeout')))
        self.assertEqual(result['error']['code'],'timeout');self.assertEqual(result['status'],'failed')

    def test_missing_output(self):
        result,_=self.invoke(processor=lambda *a,**k:subprocess.CompletedProcess([],0))
        self.assertEqual(result['phase'],'processing');self.assertEqual(result['status'],'failed')

    def test_root_validation_fail_unknown(self):
        for status in ['FAIL','UNKNOWN']:
            with self.subTest(status=status):
                publisher=unittest.mock.Mock();result,_=self.invoke(validator=lambda *a:dict(status=status),publisher=publisher)
                self.assertEqual(result['status'],'failed');publisher.assert_not_called()

    def test_incomplete_root_evidence(self):
        result,_=self.invoke(validator=lambda *a:dict(status='PASS',acceptance_changed=False,outputs=[]))
        self.assertEqual(result['status'],'failed')

    def test_transfer_and_publication_failure(self):
        for code in ['timeout','transfer_failure','publication_failure']:
            with self.subTest(code=code):
                def fail(*args):raise s.StagingError(code)
                result,work=self.invoke(publisher=fail);self.assertEqual(result['status'],'failed');self.assertEqual(result['staging_exit'],1);self.assertTrue((work/'processing.log').exists())

    def test_incomplete_publication_response(self):
        result,_=self.invoke(publisher=lambda *a:dict(receipt=dict(status='published')))
        self.assertEqual(result['status'],'failed')

    def test_environment_failure(self):
        processor=unittest.mock.Mock();result,_=self.invoke(runtime_probe=lambda:(_ for _ in ()).throw(ValueError('bad runtime')),processor=processor)
        self.assertEqual(result['phase'],'runtime');processor.assert_not_called()

    def test_wrapper_argument_failure(self):
        r=subprocess.run(['bash',str(p.ROOT/'runjob_campaign_2024.sh')],capture_output=True,text=True)
        self.assertEqual(r.returncode,2)

    def test_real_wrapper_hash_failure(self):
        self.prepare();m=p.load_json(self.bundle/'manifest.json');j=m['jobs'][0]
        r=subprocess.run(['bash',str(p.ROOT/'runjob_campaign_2024.sh'),str(self.bundle),'0'*64,'test','s1','main','sample','c0',j['attempt_id'],'1','0'],cwd=self.root,env=dict(os.environ,_CONDOR_SCRATCH_DIR='',PYTHONDONTWRITEBYTECODE='1'),capture_output=True,text=True)
        self.assertEqual(r.returncode,1);self.assertIn('manifest hash mismatch',r.stdout)

    def test_metadata_contracts_and_attempt_record(self):
        m=self.prepare();j=m['jobs'][0];a=p.validate(p.load_json(self.bundle/f"attempts/main/{j['attempt_id']}.json"))
        self.assertEqual(a['filelist_sha256'],j['filelist_sha256']);p.verify_bundle(self.bundle,p.digest(self.bundle/'manifest.json'))


class StagingTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.root=Path(self.tmp.name)
        self.files={}
        for name in ['output.root','processing.log','execution.json']:
            path=self.root/name;path.write_bytes(('synthetic-'+name).encode());self.files[name]=path
        self.identity=dict(campaign_id='c',submission_id='s',workflow='main',dataset_id='d',chunk_id='c0',attempt_id='a',execution_id='a'*32,cluster_id=1,proc_id=0)
        self.destination=p.execution_path('c','s','main','d','c0','a'*32)
        self.policy=dict(endpoint='root://example.test',strategy='manifest_last',checksum_algorithm='adler32',no_clobber_confirmed=True,command_timeout=1,overall_timeout=10)
        metadata=(self.root/'output.root').stat()
        execution=p.load_json(p.ROOT/'docs/campaign_templates/production/execution.json');execution.update(self.identity);(self.root/'execution.json').write_text(json.dumps(execution))
        self.validation=dict(status='PASS',acceptance_changed=False,outputs=[dict(status='PASS', inspected_path=str(self.root/'output.root'), acceptance='not_evaluated', identity={k:self.identity[k] for k in ('campaign_id','workflow','dataset_id','chunk_id','attempt_id')}, file=dict(size_bytes=metadata.st_size,mtime_ns=metadata.st_mtime_ns,ctime_ns=metadata.st_ctime_ns,inode=metadata.st_ino,device=metadata.st_dev))]);self.remote={};self.calls=[]

    def runner(self,args,timeout):
        self.calls.append(args)
        if args[0]=='xrdcp':
            self.assertNotIn('-f',args);self.remote[args[2]]=Path(args[1]).read_bytes();return subprocess.CompletedProcess(args,0,'','')
        op=args[2]
        if op=='mkdir':return subprocess.CompletedProcess(args,0,'','')
        if op=='stat':
            key=self.policy['endpoint']+'/'+args[3]
            if key not in self.remote:raise CommandError('nonzero_exit','missing',54,'','[3011] No such file')
            return subprocess.CompletedProcess(args,0,f'Path: {args[3]}\nSize: {len(self.remote[key])}\nFlags: 16 (IsReadable)\n','')
        if op=='query':
            data=self.remote[self.policy['endpoint']+'/'+args[4]]
            value=f'{zlib.adler32(data)&0xffffffff:08x}'
            return subprocess.CompletedProcess(args,0,'adler32 '+value,'')
        raise AssertionError(args)

    def stage(self,runner=None):
        return s.stage(self.files,self.root/'publication.json',self.identity,self.destination,self.policy,self.validation,runner or self.runner)

    def test_complete_publication(self):
        result=self.stage();self.assertEqual(result['receipt']['acceptance'],'not_evaluated');self.assertEqual(set(result['receipt']['artifacts']),set(self.files))
        self.assertTrue(self.calls[-1][-1].endswith('publication.json'));self.assertEqual(len([a for a in self.calls if a[0]=='xrdcp']),4)
        self.assertEqual(result['marker']['sha256'],p.digest(self.root/'publication.json'))

    def test_no_overwrite_collision(self):
        def runner(args,timeout):
            if args[2]=='stat':return subprocess.CompletedProcess(args,0,'','')
            return self.runner(args,timeout)
        with self.assertRaises(s.StagingError):self.stage(runner)
        self.assertFalse(self.remote)

    def test_ambiguous_missing_and_network(self):
        for msg in ['[3011] connection timeout','[3010] access denied','network failure','[3011] [102] failed']:
            with self.subTest(msg=msg):
                def runner(*a):raise CommandError('nonzero_exit','test',54,'',msg)
                with self.assertRaises(s.StagingError):self.stage(runner)
        self.assertFalse(self.remote)

    def test_endpoint_capability_unverified(self):
        self.policy['no_clobber_confirmed']=False
        with self.assertRaises(s.StagingError):self.stage()
        self.assertFalse(self.calls)

    def test_validation_required(self):
        self.validation['status']='UNKNOWN'
        with self.assertRaises(s.StagingError):self.stage()
        self.assertFalse(self.calls)

    def test_transfer_timeout(self):
        def runner(args,t):
            if args[0]=='xrdcp':raise CommandError('timeout','timeout')
            return self.runner(args,t)
        with self.assertRaises(CommandError):self.stage(runner)
        self.assertFalse((self.root/'publication.json').exists())

    def test_size_mismatch(self):
        def runner(args,t):
            result=self.runner(args,t)
            if args[2]=='stat' and args[3].endswith('output.root'):result.stdout='Size: 999\nFlags: 16 (IsReadable)\n'
            return result
        with self.assertRaises(s.StagingError):self.stage(runner)

    def test_checksum_unavailable_wrong_algorithm(self):
        for val in ['','sha256 '+'a'*64,'adler32 00000000']:
            with self.subTest(val=val):
                def runner(args,t):
                    if args[2]=='query':return subprocess.CompletedProcess(args,0,val,'')
                    return self.runner(args,t)
                with self.assertRaises(s.StagingError):self.stage(runner)
        self.assertFalse((self.root/'publication.json').exists())

    def test_publication_transfer_failure(self):
        def runner(args,t):
            if args[0]=='xrdcp' and args[2].endswith('publication.json'):raise CommandError('nonzero_exit','publication failed',5)
            return self.runner(args,t)
        with self.assertRaises(CommandError):self.stage(runner)
        self.assertFalse(any(x.endswith('publication.json') for x in self.remote))

    def test_no_rename_cleanup_or_listing(self):
        self.stage()
        self.assertTrue(all(a[0]=='xrdcp' or a[2] in ('stat','mkdir','query') for a in self.calls))
        self.assertTrue(all(not any(x in a for x in ['rm','mv','ls','-f']) for a in self.calls))


class ProcessTests(unittest.TestCase):
    def test_nonzero_exit(self):
        with self.assertRaises(CommandError) as caught:run_process([sys.executable,'-c','raise SystemExit(7)'],2)
        self.assertEqual(caught.exception.returncode,7)

    def test_timeout(self):
        with self.assertRaises(CommandError) as caught:run_process([sys.executable,'-c','import time;time.sleep(10)'],0.1)
        self.assertEqual(caught.exception.code,'timeout')

    def test_streaming_output_limit(self):
        with self.assertRaises(CommandError) as caught:run_process([sys.executable,'-c','import sys;sys.stdout.write("x"*100000)'],2,capture_limit=1000)
        self.assertEqual(caught.exception.code,'output_limit')

    def test_exclusive_log(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'log';path.write_text('protected')
            with self.assertRaises(FileExistsError):run_process([sys.executable,'-c','print(1)'],2,log=path)
            self.assertEqual(path.read_text(),'protected')

    def test_success(self):
        result=run_process([sys.executable,'-c','print("ok")'],2)
        self.assertEqual(result.stdout.strip(),'ok')



class AdversarialProductionTests(unittest.TestCase):
    setUp = ProductionTests.setUp
    save_config = ProductionTests.save_config
    prepare = ProductionTests.prepare
    invoke = ProductionTests.invoke
    def test_data_stream_era_mismatch(self):
        self.dataset.update(datatype='data',era='H',stream='EGamma0',das_dataset='/Muon0/Run2024H-v1/NANOAOD')
        with self.assertRaises(ValueError):self.prepare()

    def test_redirector_alias_input_duplicate(self):
        c=copy.deepcopy(self.dataset['chunks'][0]);c.update(chunk_id='c1',index=1,ordered_inputs=[c['ordered_inputs'][0].replace('cmsxrootd.fnal.gov','another.example')]);self.dataset['chunks'].append(c)
        with self.assertRaises(ValueError):self.prepare()

    def test_campaign_changed_bundle(self):
        self.prepare();m=p.load_json(self.bundle/'manifest.json');campaign=p.load_json(self.bundle/'campaign.json');campaign['campaign_id']='other';(self.bundle/'campaign.json').write_text(json.dumps(campaign));m['files']['campaign.json']=p.digest(self.bundle/'campaign.json');(self.bundle/'manifest.json').write_text(json.dumps(m))
        with self.assertRaises(ValueError):p.verify_bundle(self.bundle,p.digest(self.bundle/'manifest.json'))

    def test_configuration_dependencies_frozen(self):
        (self.source/'payload.json').write_text('{}');self.config['goodjson']='data/payload.json';(self.source/'data').mkdir();(self.source/'data/payload.json').write_text('{}');self.save_config()
        m=self.prepare();self.assertIn('data/payload.json',m['files'])

    def test_preparation_cli(self):
        path=self.root/'campaign.json';path.write_text(json.dumps(self.campaign))
        with patch('sys.stdout',new=io.StringIO()) as output:
            rc=p.main(['--campaign',str(path),'--submission-id','s1','--source-root',str(self.source),'--destination',str(self.bundle)])
        self.assertEqual(rc,0);self.assertEqual(json.loads(output.getvalue())['manifest_sha256'],p.digest(self.bundle/'manifest.json'))




class AdversarialStagingTests(unittest.TestCase):
    setUp = StagingTests.setUp
    runner = StagingTests.runner
    stage = StagingTests.stage
    def test_destination_identity_mismatch(self):
        self.destination=self.destination.replace('/c/main/','/other/main/')
        with self.assertRaises(s.StagingError):self.stage()
        self.assertFalse(self.calls)

    def test_changed_output_after_validation(self):
        self.files['output.root'].write_bytes(b'changed')
        with self.assertRaises(s.StagingError):self.stage()
        self.assertFalse(self.calls)

    def test_malformed_stat_flags(self):
        def runner(args,t):
            result=self.runner(args,t)
            if args[2]=='stat' and args[3].endswith('output.root'):result.stdout='Size: 21\nFlags: nonsense\n'
            return result
        with self.assertRaises(s.StagingError):self.stage(runner)

    def test_root_identity_wrong(self):
        self.validation['outputs'][0]['identity']['attempt_id']='wrong'
        with self.assertRaises(s.StagingError):self.stage()

    def test_publication_verification_failure(self):
        def runner(args,t):
            if args[2]=='query' and args[-1].endswith('publication.json'):return subprocess.CompletedProcess(args,0,'adler32 00000000','')
            return self.runner(args,t)
        with self.assertRaises(s.StagingError):self.stage(runner)




class LocalROOTIntegrationTests(unittest.TestCase):
    setUp = ProductionTests.setUp
    save_config = ProductionTests.save_config
    prepare = ProductionTests.prepare
    invoke = ProductionTests.invoke
    def test_real_local_zero_entry_validation(self):
        def processor(args,timeout,**kwargs):
            import ROOT
            from array import array
            f=ROOT.TFile(str(args[3]),'RECREATE');tree=ROOT.TTree('outputTree_00000','synthetic');run=array('I',[1]);tree.Branch('run',run,'run/i');tree.Write();f.Close();Path(kwargs['log']).write_text('Total Number of Entries: 1\nsynthetic zero-selected-entry ROOT fixture\n');return subprocess.CompletedProcess(args,0,'','')
        result,_=self.invoke(processor=processor,validator=w.validate_local)
        self.assertEqual(result['status'],'published');self.assertEqual(result['validation_status'],'PASS')

    def test_local_root_to_mocked_publication_end_to_end(self):
        def processor(args,timeout,**kwargs):
            import ROOT
            from array import array
            f=ROOT.TFile(str(args[3]),'RECREATE');tree=ROOT.TTree('outputTree_00000','synthetic');run=array('I',[1]);tree.Branch('run',run,'run/i');tree.Fill();tree.Write();f.Close();Path(kwargs['log']).write_text('Total Number of Entries: 1\nsynthetic local ROOT fixture\n');return subprocess.CompletedProcess(args,0,'','')
        remote={};calls=[]
        def runner(args,timeout):
            calls.append(args)
            if args[0]=='xrdcp':
                self.assertNotIn('-f',args);remote[args[2]]=Path(args[1]).read_bytes();return subprocess.CompletedProcess(args,0,'','')
            if args[2]=='mkdir':return subprocess.CompletedProcess(args,0,'','')
            if args[2]=='stat':
                key='root://cmseos.fnal.gov/'+args[-1]
                if key not in remote:raise CommandError('nonzero_exit','absent',54,'','[3011] No such file')
                return subprocess.CompletedProcess(args,0,'Size: '+str(len(remote[key]))+'\nFlags: 16 (IsReadable)\n','')
            value=zlib.adler32(remote['root://cmseos.fnal.gov/'+args[-1]])&0xffffffff
            return subprocess.CompletedProcess(args,0,'adler32 '+format(value,'08x'),'')
        def publisher(*args):return s.stage(*args,runner=runner)
        result,work=self.invoke(processor=processor,validator=w.validate_local,publisher=publisher)
        self.assertEqual(result['status'],'published');self.assertEqual(len(remote),4)
        publication=p.validate(p.load_json(work/'publication.json'));execution=p.validate(p.load_json(work/'execution.json'))
        self.assertEqual(publication['execution_id'],execution['execution_id']);self.assertEqual(publication['acceptance'],'not_evaluated')
        self.assertTrue(calls[-1][-1].endswith('publication.json'))

    def test_real_local_missing_branch_validation(self):
        def processor(args,timeout,**kwargs):
            import ROOT
            f=ROOT.TFile(str(args[3]),'RECREATE');tree=ROOT.TTree('outputTree_00000','synthetic');tree.Write();f.Close();Path(kwargs['log']).write_text('Total Number of Entries: 1\nsynthetic missing-branch fixture\n');return subprocess.CompletedProcess(args,0,'','')
        result,work=self.invoke(processor=processor,validator=w.validate_local)
        self.assertEqual(result['status'],'failed')
        self.assertEqual(p.load_json(work/'validation.json')['status'],'FAIL')



class FinalGateTests(unittest.TestCase):
    setUp = ProductionTests.setUp
    save_config = ProductionTests.save_config
    prepare = ProductionTests.prepare
    invoke = ProductionTests.invoke

    def test_frozen_campaign_rejects_changed_payload(self):
        self.prepare();self.campaign=p.load_json(self.bundle/'campaign.json')
        (self.source/'libnanoadrdframe.so').write_bytes(b'changed build')
        with self.assertRaises(ValueError):self.prepare(self.root/'changed_bundle','s2')
        self.assertFalse((self.root/'changed_bundle').exists())

    def test_processing_cannot_mutate_frozen_bundle(self):
        def processor(args,timeout,**kwargs):
            Path(args[3]).write_bytes(b'synthetic');Path(kwargs['log']).write_text('test')
            (self.bundle/'config.py').write_text('changed configuration')
            return subprocess.CompletedProcess(args,0,'','')
        result,_=self.invoke(processor=processor)
        self.assertEqual(result['status'],'failed');self.assertEqual(result['phase'],'processing')

    def test_seven_example_contracts(self):
        for name in p.KINDS:
            with self.subTest(kind=name):p.validate(p.load_json(p.ROOT/'docs/campaign_templates/production'/(name+'.json')))


class FinalStagingGateTests(unittest.TestCase):
    setUp = StagingTests.setUp
    runner = StagingTests.runner
    stage = StagingTests.stage

    def test_exhausted_deadline_launches_nothing(self):
        with patch('campaign_staging.time.monotonic',side_effect=[0,20]),self.assertRaises(s.StagingError):self.stage()
        self.assertFalse(self.calls)

    def test_inconsistent_flag_mask(self):
        def runner(args,t):
            result=self.runner(args,t)
            if args[2]=='stat' and args[3].endswith('output.root'):
                result.stdout=result.stdout.replace('16 (IsReadable)','16 (IsReadable XBitSet)')
            return result
        with self.assertRaises(s.StagingError):self.stage(runner)


if __name__ == '__main__': unittest.main()
