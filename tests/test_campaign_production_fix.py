"""Packaging/catalog regressions; mocked processor wiring and synthetic ROOT graphs."""
import ast
import copy
import json
import os
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

import test_campaign_production as baseline_tests
import campaign_production as p
import campaign_catalog as catalog
import campaign_worker as worker
import campaign_event_limit as limits


class PackagingTests(unittest.TestCase):
    setUp = baseline_tests.ProductionTests.setUp
    save_config = baseline_tests.ProductionTests.save_config
    prepare = baseline_tests.ProductionTests.prepare

    def test_wrapper_executable(self):
        self.prepare();self.assertTrue(os.access(self.bundle/'runjob_campaign_2024.sh',os.X_OK))
        self.assertFalse(os.access(self.bundle/'profile.json',os.X_OK))

    def test_source_permissions_and_hashes_unchanged(self):
        files=list(self.source.rglob('*'));before={str(f):(f.stat().st_mode,p.digest(f)) for f in files if f.is_file()}
        self.prepare();self.assertEqual(before,{str(f):(f.stat().st_mode,p.digest(f)) for f in files if f.is_file()})

    def test_safe_pcm_symlink_dereferenced(self):
        pcm=self.source/'rootdict_rdict.pcm';pcm.unlink();target=self.source/'build/dict/rootdict_rdict.pcm';target.parent.mkdir(parents=True);target.write_bytes(b'known active PCM bytes');pcm.symlink_to('build/dict/rootdict_rdict.pcm')
        m=self.prepare();self.assertFalse((self.bundle/'rootdict_rdict.pcm').is_symlink());self.assertEqual((self.bundle/'rootdict_rdict.pcm').read_bytes(),target.read_bytes());self.assertEqual(m['files']['rootdict_rdict.pcm'],p.digest(target))
        other=self.root/'bundle2';self.assertEqual(m,self.prepare(other))

    def test_pcm_escape_rejected(self):
        pcm=self.source/'rootdict_rdict.pcm';pcm.unlink();pcm.symlink_to('/etc/hosts')
        with self.assertRaises(ValueError):self.prepare()

    def test_missing_pcm_target(self):
        pcm=self.source/'rootdict_rdict.pcm';pcm.unlink();pcm.symlink_to('build/dict/rootdict_rdict.pcm')
        with self.assertRaises((ValueError,OSError)):self.prepare()

    def test_local_header_transitive_dependency(self):
        (self.source/'src/utility.h').write_text('#include "extra.h"\n');(self.source/'src/extra.h').write_text('// transitive dependency\n')
        m=self.prepare();self.assertIn('src/extra.h',m['files'])

    def test_header_escape(self):
        (self.source/'src/utility.h').write_text('#include "extra.h"\n');(self.source/'src/extra.h').symlink_to('/etc/hosts')
        with self.assertRaises(ValueError):self.prepare()

    def test_missing_mandatory_header(self):
        (self.source/'src/BaseAnalyser.h').unlink()
        with self.assertRaises(ValueError):self.prepare()

    def test_transferred_nonexecutable_wrapper_rejected(self):
        self.prepare();(self.bundle/'runjob_campaign_2024.sh').chmod(0o644)
        with self.assertRaises(ValueError):p.verify_bundle(self.bundle,p.digest(self.bundle/'manifest.json'))


class CatalogTests(unittest.TestCase):
    setUp = baseline_tests.ProductionTests.setUp
    save_config = baseline_tests.ProductionTests.save_config
    prepare = baseline_tests.ProductionTests.prepare

    def register(self,bundle=None):
        bundle=bundle or self.bundle
        self.catalog=self.root/'catalog';self.catalog.mkdir(exist_ok=True)
        return catalog.register(bundle,p.digest(bundle/'manifest.json'),self.catalog)

    def test_new_and_identical_idempotent(self):
        self.prepare();first=self.register();path=self.catalog/'test/s1.registration.json';before=path.read_bytes();self.assertEqual(first,self.register());self.assertEqual(before,path.read_bytes());self.assertFalse(first['remote_registered']);self.assertEqual(first['acceptance'],'not_evaluated')
        self.assertEqual(p.digest(self.catalog/'test/campaign.json'),first['campaign_sha256'])

    def test_campaign_content_collision(self):
        self.prepare();self.register();before=(self.catalog/'test/campaign.json').read_bytes();self.campaign['processing_timeout']=11;other=self.root/'b2';self.prepare(other)
        with self.assertRaises(ValueError):self.register(other)
        self.assertEqual(before,(self.catalog/'test/campaign.json').read_bytes())

    def test_new_submission_same_campaign(self):
        self.prepare();self.register();other=self.root/'b2';self.prepare(other,'s2');self.register(other)
        self.assertTrue((self.catalog/'test/s2.registration.json').exists())

    def test_registration_links_exact_manifest(self):
        self.prepare();receipt=self.register();self.assertEqual(p.digest(self.catalog/'test/s1.manifest.json'),receipt['manifest_sha256'])

    def test_registration_symlink_collision(self):
        self.prepare();cat=self.root/'catalog';cat.mkdir();(cat/'test').symlink_to(self.source)
        with self.assertRaises(ValueError):catalog.register(self.bundle,p.digest(self.bundle/'manifest.json'),cat)

    def test_remote_catalog_blocked(self):
        self.prepare()
        for path in ['root://cmseos.fnal.gov//store/user/x','/eos/uscms/store/user/x']:
            with self.subTest(path=path),self.assertRaises(ValueError):catalog.register(self.bundle,p.digest(self.bundle/'manifest.json'),path)

    def test_corrupt_existing_receipt_not_overwritten(self):
        self.prepare();self.register();path=self.catalog/'test/s1.registration.json';path.write_bytes(b'partial')
        with self.assertRaises(ValueError):self.register()
        self.assertEqual(path.read_bytes(),b'partial')


class LimitedWorkerTests(unittest.TestCase):
    setUp = baseline_tests.ProductionTests.setUp
    save_config = baseline_tests.ProductionTests.save_config
    prepare = baseline_tests.ProductionTests.prepare
    invoke = baseline_tests.ProductionTests.invoke

    def limited(self):
        self.campaign['max_input_events']=2;self.profile['tree']='pilot_outputTree_00000';self.profile['required_objects']={'TZQProcessingProvenance':'TNamed'};(self.source/'profile.json').write_text(json.dumps(self.profile))

    def processor(self,args,timeout,**kwargs):
        self.assertEqual(args[-2:],['--max-input-events','2']);Path(args[3]).write_bytes(b'synthetic partial root')
        Path(str(args[3])+'.input_scope.json').write_text(json.dumps(dict(schema_version=1,kind='processing_input_scope',total_input_entries=5,requested_max_input_events=2,analysis_input_entries=2,generator_weight_bookkeeping_scope='full_input_chain_when_mc',partial=True,full_production_eligible=False,tree_prefix='pilot_outputTree')))
        Path(kwargs['log']).write_text('Total Number of Entries: 5\n');return __import__('subprocess').CompletedProcess(args,0)

    def test_partial_metadata_and_ineligibility(self):
        self.limited();result,work=self.invoke(processor=self.processor);self.assertEqual(result['status'],'published');scope=p.load_json(work/'execution.json')['input_scope'];self.assertEqual(scope['analysis_input_entries'],2);self.assertFalse(scope['full_production_eligible'])
        with self.assertRaises(ValueError):limits.require_complete(scope)

    def test_limited_requires_distinct_tree(self):
        self.campaign['max_input_events']=2
        with self.assertRaises(ValueError):self.prepare()

    def test_limited_requires_root_marker(self):
        self.limited();self.profile['required_objects']={};(self.source/'profile.json').write_text(json.dumps(self.profile))
        with self.assertRaises(ValueError):self.prepare()

    def test_missing_limit_evidence_cannot_publish(self):
        self.limited()
        def proc(args,t,**kwargs):Path(args[3]).write_bytes(b'partial');Path(kwargs['log']).write_text('Total Number of Entries: 5\n');return __import__('subprocess').CompletedProcess(args,0)
        result,_=self.invoke(processor=proc);self.assertEqual(result['status'],'failed')

    def test_unlimited_eligibility(self):
        result,work=self.invoke();scope=p.load_json(work/'execution.json')['input_scope'];self.assertIsNone(scope['requested_max_input_events']);limits.require_complete(scope)

    def test_forged_complete_flag_rejected(self):
        self.limited();result,work=self.invoke(processor=self.processor);ex=p.load_json(work/'execution.json');ex['input_scope']['full_production_eligible']=True
        with self.assertRaises(ValueError):p.validate(ex)


class ProcessorWiringTests(unittest.TestCase):
    def run_mock(self,workflow,maximum):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);inp=root/'input.txt';inp.write_text('root://fake//store/mc/test.root\n');output=root/'output.root';calls=[];instances=[];markers=[]
            class Analyser:
                def __init__(self,*args):self.maximum=None;instances.append(self)
                def setMaxInputEvents(self,n):calls.append('limit');self.maximum=n
                def setParams(self,*args):calls.append('params')
                def setupCorrections(self,*args):calls.append('corrections')
                def setupObjects(self):calls.append('objects')
                def setupAnalysis(self):calls.append('analysis');self.sum_gen_weight=15
                def run(self,save,tree):calls.append(('run',tree));output.write_bytes(b'synthetic')
                def getPilotInputEntries(self):return min(5,self.maximum)
            class Chain:
                def __init__(self,*args):pass
                def Add(self,*args):pass
                def GetEntries(self):return 5
            fake=types.SimpleNamespace(TChain=Chain,BaseAnalyser=Analyser,FakeFactorAnalyser=Analyser)
            source=ast.parse((p.ROOT/'processnanoaod_v.py').read_text());fn=next(n for n in source.body if isinstance(n,ast.FunctionDef) and n.name=='Nanoaodprocessor_singledir')
            def record(ROOT,analyser,out,total,maxevents,tree_prefix=None):
                if maxevents is not None:markers.append((total,maxevents,analyser.getPilotInputEntries()))
            env=dict(ROOT=fake,os=os,re=__import__('re'),sys=sys,is_valid_das_path=lambda x:False,is_filelist=lambda x:True,configure_limit=limits.configure_limit,record_limit=record)
            exec(compile(ast.Module(body=[fn],type_ignores=[]),'processor-function','exec'),env)
            class Config(dict):
                def __missing__(self,k):return 'synthetic'
            config=Config(intreename='Events',outtreename='outputTree',year=2024,runtype='test',datatype=0,analysertype=workflow)
            with patch('sys.stdout',new=__import__('io').StringIO()):env['Nanoaodprocessor_singledir'](str(inp),str(output),dict(nrootfiles=0,saveallbranches=False,skipcorrections=False),config,maximum)
            return calls,markers,instances[0].sum_gen_weight

    def test_unlimited_both_workflows(self):
        for workflow in ['BaseAnalyser','FakeFactorAnalyser']:
            with self.subTest(workflow=workflow):
                calls,markers,sumw=self.run_mock(workflow,None);self.assertNotIn('limit',calls);self.assertEqual(calls[-1],('run','outputTree'));self.assertFalse(markers);self.assertEqual(sumw,15)

    def test_limit_before_corrections_both_workflows(self):
        for workflow in ['BaseAnalyser','FakeFactorAnalyser']:
            with self.subTest(workflow=workflow):
                calls,markers,sumw=self.run_mock(workflow,2);self.assertLess(calls.index('limit'),calls.index('corrections'));self.assertEqual(calls[-1],('run','pilot_outputTree'));self.assertEqual(markers,[(5,2,2)]);self.assertEqual(sumw,15)

    def test_shared_graph_and_full_sum_static_contract(self):
        source=(p.ROOT/'src/framework/NanoAODAnalyzerrdframe.cpp').read_text();body=source.split('void NanoAODAnalyzerrdframe::setMaxInputEvents',1)[1].split('ULong64_t NanoAODAnalyzerrdframe::getPilotInputEntries',1)[0]
        self.assertIn('_rlm.Range',body);self.assertNotIn('_rd =',body)
        for name in ['src/analysis/BaseAnalyser.cpp','src/fakefactor/FakeFactorAnalyser.cpp']:
            self.assertIn('_rd.Sum("genWeight")',(p.ROOT/name).read_text())

    def test_invalid_limits(self):
        for value in ['0','-1',str(2**64)]:
            with self.subTest(value=value),self.assertRaises(ValueError):limits.positive_events(value)


class SyntheticROOTLimitTests(unittest.TestCase):
    def test_real_range_and_full_bookkeeping(self):
        import ROOT
        from array import array
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'input.root';f=ROOT.TFile(str(path),'RECREATE');t=ROOT.TTree('Events','synthetic');weight=array('f',[0]);t.Branch('genWeight',weight,'genWeight/F')
            for value in [1,2,3,4,5]:weight[0]=value;t.Fill()
            t.Write();f.Close();df=ROOT.RDataFrame('Events',str(path));bounded=df.Range(0,2);count=bounded.Count();sumw=df.Sum('genWeight')
            self.assertEqual(sumw.GetValue(),15);self.assertEqual(count.GetValue(),2);self.assertEqual(bounded.Count().GetValue(),2)

    def test_marker_and_distinct_tree_reject_standard_profile(self):
        import ROOT
        from array import array
        from root_output_validation import validate_outputs
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);path=root/'pilot.root';f=ROOT.TFile(str(path),'RECREATE');t=ROOT.TTree('pilot_outputTree_00000','synthetic');run=array('I',[1]);t.Branch('run',run,'run/i');t.Fill();t.Write();f.Close()
            analyser=types.SimpleNamespace(getPilotInputEntries=lambda:2);scope=limits.record_limit(ROOT,analyser,path,5,2)
            f=ROOT.TFile.Open(str(path),'READ');self.assertFalse(f.Get('outputTree_00000'));self.assertEqual(json.loads(f.Get(limits.PROVENANCE_KEY).GetTitle()),scope);f.Close()
            profile=json.loads((p.ROOT/'docs/root_validation_profiles/main-data.json').read_text());entry=dict(campaign_id='test',workflow='main',dataset_id='data',sample_name='test',datatype='data',chunk_id='c0',index=0,attempt_id='a0',path=str(path),profile_id=profile['profile_id'])
            report=validate_outputs(dict(schema_version=1,kind='root_output_inventory',outputs=[entry]),{profile['profile_id']:profile},root)
            self.assertEqual(report['status'],'FAIL');self.assertIn('missing_tree',[i['code'] for i in report['outputs'][0]['issues']])


if __name__=='__main__':unittest.main()
