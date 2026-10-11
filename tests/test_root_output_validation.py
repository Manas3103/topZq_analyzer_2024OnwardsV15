import contextlib
import copy
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch, MagicMock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import root_output_validation as v
from root_output_worker import inspect_root


class ValidationTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name); self.path = self.root/'tiny.root'; self.path.write_bytes(b'synthetic')
        self.p = dict(schema_version=1, kind='root_validation_profile', profile_id='test', workflow='main', datatype='data', tree='outputTree_00000', required_branches=['run'], branch_types={'run':'UInt_t'}, required_objects={}, allow_zero_entries=True, content=dict(mode='metadata', branches=[], sample_entries=3), dictionary_branches=[])
        self.inv = dict(schema_version=1, kind='root_output_inventory', outputs=[dict(campaign_id='test', workflow='main', dataset_id='data-C', sample_name='synthetic', datatype='data', chunk_id='c0', index=0, attempt_id='a0', path=str(self.path), profile_id='test')])
        self.response = dict(issues=[], structure={'entries':2}, content={'mode':'metadata'}, diagnostics='')
    def check(self):
        with patch.object(v, 'worker', return_value=self.response):
            return v.validate_outputs(self.inv, {'test':self.p}, self.root)
    def code(self, expected):
        r = self.check(); self.assertIn(expected, [i['code'] for i in r['outputs'][0]['issues']]); return r
    def test_missing(self): self.path.unlink(); self.assertEqual(self.code('missing_file')['status'], 'FAIL')
    def test_empty(self): self.path.write_bytes(b''); self.code('zero_byte_file')
    def test_nonregular(self): self.inv['outputs'][0]['path']=str(self.root); self.assertRaises(ValueError, self.check)
    def test_directory_output(self):
        self.path.unlink(); self.path.mkdir(); self.code('nonregular_file')
    def test_access_failure(self):
        with patch.object(v, 'metadata', side_effect=PermissionError('mock denied')):
            self.assertEqual(self.code('filesystem_access')['status'], 'UNKNOWN')
    def test_changed(self):
        def mutate(*args): self.path.write_bytes(b'changed content'); return self.response
        with patch.object(v, 'worker', side_effect=mutate):
            r=v.validate_outputs(self.inv, {'test':self.p}, self.root)
        self.assertEqual(r['status'],'UNKNOWN'); self.assertEqual(r['outputs'][0]['issues'][0]['code'],'file_changed')
    def test_duplicates(self):
        self.inv['outputs'] *= 2
        self.assertRaises(ValueError, self.check)
    def test_contradictory_attempt(self):
        e=copy.deepcopy(self.inv['outputs'][0]); e.update(chunk_id='c1',index=1,path=str(self.root/'other.root')); self.inv['outputs'].append(e)
        self.assertRaises(ValueError,self.check)
    def test_wrong_profile(self):
        for key,value in [('workflow','fakefactor'),('datatype','mc')]:
            original=self.p[key]; self.p[key]=value; self.assertRaises(ValueError,self.check); self.p[key]=original
    def test_versions(self):
        for obj in (self.p,self.inv):
            obj['schema_version']=2; self.assertRaises(ValueError,self.check); obj['schema_version']=1
    def test_remote_rejected(self):
        for path in ('root://host/file.root','/eos/uscms/file.root','https://host/file.root'):
            self.inv['outputs'][0]['path']=path; self.assertRaises(ValueError,self.check)
    def test_symlink_escape(self):
        self.path.unlink(); self.path.symlink_to('/eos/uscms/file.root'); self.assertRaises(ValueError,self.check)
    def test_no_mutation_provenance(self):
        before=copy.deepcopy(self.inv); profile=copy.deepcopy(self.p); data=self.path.read_bytes()
        r=self.check(); self.assertEqual(r['status'],'PASS'); self.assertEqual(r['outputs'][0]['provenance']['status'],'UNKNOWN')
        self.assertFalse(r['acceptance_changed']); self.assertEqual(self.inv,before); self.assertEqual(self.p,profile); self.assertEqual(self.path.read_bytes(),data)
    def test_worker_timeout(self):
        with patch.object(v.subprocess,'run',side_effect=subprocess.TimeoutExpired('mock',1)):
            r=v.worker(self.path,self.p,1,4096)
        self.assertEqual(r['issues'][0]['code'],'inspection_timeout')
    def test_worker_crash(self):
        with patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([], -11, '', 'mock crash')):
            r=v.worker(self.path,self.p,1,4096)
        self.assertEqual(r['issues'][0]['code'],'inspection_crash')
    def test_cli_statuses_and_preservation(self):
        inp=self.root/'inventory.json'; pro=self.root/'profile.json'; accept=self.root/'acceptance.json'
        inp.write_text(json.dumps(self.inv)); pro.write_text(json.dumps(self.p)); accept.write_text('preserve acceptance')
        before={p:p.read_bytes() for p in self.root.iterdir()}
        args=['--inventory',str(inp),'--profile',str(pro),'--local-root',str(self.root)]
        for status,code in [('PASS',0),('FAIL',1),('UNKNOWN',3)]:
            response=copy.deepcopy(self.response)
            if status!='PASS': response['issues']=[dict(code='mock',status=status,detail='mock')]
            with patch.object(v,'worker',return_value=response),contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(v.main(args),code); self.assertEqual(json.loads(out.getvalue())['status'],status)
        self.assertEqual(before,{p:p.read_bytes() for p in self.root.iterdir()})
        with contextlib.redirect_stderr(io.StringIO()): self.assertEqual(v.main(args+['--timeout','0']),2)
    def test_bad_protocol(self):
        with patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,'garbage','')):
            self.assertEqual(v.worker(self.path,self.p,1,4096)['issues'][0]['code'],'worker_protocol')
    def test_profiles(self):
        for path in (Path(__file__).resolve().parents[1]/'docs/root_validation_profiles').glob('*.json'): v.profile(v.load_json(path))
    def test_dependency_unknown(self):
        self.response['issues']=[dict(code='root_dependency_unavailable',status='UNKNOWN',detail='mock missing ROOT')]
        self.assertEqual(self.code('root_dependency_unavailable')['status'],'UNKNOWN')
    def test_chunk_index_contradiction(self):
        e=copy.deepcopy(self.inv['outputs'][0]); e.update(index=1,attempt_id='a1',path=str(self.root/'other.root')); self.inv['outputs'].append(e)
        self.assertRaises(ValueError,self.check)
    def test_duplicate_path(self):
        e=copy.deepcopy(self.inv['outputs'][0]); e['attempt_id']='a1'; self.inv['outputs'].append(e)
        self.assertRaises(ValueError,self.check)


class MockROOTTests(unittest.TestCase):
    setUp = ValidationTests.setUp
    def root_inspect(self, **changes):
        root=MagicMock(); f=root.TFile.Open.return_value; f.IsZombie.return_value=False; f.TestBit.return_value=False
        tree=f.Get.return_value; tree.InheritsFrom.side_effect=lambda name: name=='TTree'; tree.GetEntries.return_value=5
        branch=MagicMock(); branch.GetName.return_value='run'; branch.GetClassName.return_value=''; leaf=MagicMock(); leaf.GetTypeName.return_value='UInt_t'; branch.GetListOfLeaves.return_value=[leaf]
        branch.GetEntry.return_value=4; branch.GetFileName.return_value=''; tree.GetListOfBranches.return_value=[branch]; tree.LoadTree.return_value=0
        for name,value in changes.items():
            if name=='recovered': f.TestBit.return_value=value
            if name=='zombie': f.IsZombie.return_value=value
            if name=='entries': tree.GetEntries.return_value=value
            if name=='branches': tree.GetListOfBranches.return_value=value
            if name=='read': branch.GetEntry.return_value=value
            if name=='exception': branch.GetEntry.side_effect=value
            if name=='external': branch.GetFileName.return_value=value
            if name=='partial_read': branch.GetEntry.side_effect=value
            if name=='dictionary':
                branch.GetClassName.return_value='MissingClass'; root.TClass.GetClass.return_value.HasDictionary.return_value=value
        return inspect_root(root,'mock.root',self.p)
    def assert_issue(self, code, **changes):
        r=self.root_inspect(**changes); self.assertIn(code,[i['code'] for i in r['issues']]); return r
    def test_zombie_mock(self): self.assert_issue('root_open_failed',zombie=True)
    def test_recovered_mock(self): self.assert_issue('recovered_file',recovered=True)
    def test_missing_branch_mock(self): self.assert_issue('missing_branch',branches=[])
    def test_wrong_type_mock(self): self.p['branch_types']['run']='Float_t'; self.assert_issue('branch_type_mismatch')
    def test_zero_allowed_mock(self): self.assertEqual(self.root_inspect(entries=0)['issues'],[])
    def test_zero_forbidden_mock(self): self.p['allow_zero_entries']=False; self.assert_issue('zero_entries_forbidden',entries=0)
    def test_metadata_mock(self):
        r=self.root_inspect(); self.assertEqual(r['content']['checked_entries'],[]); self.assertFalse(r['content']['complete_configured_read'])
    def test_sample_mock(self):
        self.p['content'].update(mode='sample',branches=['run']); r=self.root_inspect()
        self.assertEqual(r['content']['checked_entries'],[0,2,4]); self.assertFalse(r['content']['complete_configured_read'])
    def test_full_mock(self):
        self.p['content'].update(mode='full',branches=['run']); r=self.root_inspect()
        self.assertEqual(r['content']['entry_range'],[0,5]); self.assertTrue(r['content']['complete_configured_read'])
    def test_read_failure_mock(self): self.p['content'].update(mode='sample',branches=['run']); self.assert_issue('content_unreadable',read=-1)
    def test_exception_mock(self): self.p['content'].update(mode='sample',branches=['run']); self.assert_issue('inspection_exception',exception=RuntimeError('mock'))
    def test_dictionary_mock(self):
        self.p['branch_types']={}; self.p['content'].update(mode='sample',branches=['run']); r=self.assert_issue('dictionary_unavailable',dictionary=False)
        self.assertEqual(r['content']['checked_branches'],[])
    def test_external_baskets_mock(self):
        self.p['content'].update(mode='sample',branches=['run']); self.assert_issue('external_storage',external='root://forbidden/file.root')
    def test_partial_read_coverage_mock(self):
        self.p['content'].update(mode='full',branches=['run']); r=self.assert_issue('content_unreadable',partial_read=[4,0])
        self.assertEqual(r['content']['coverage']['run']['entry_range'],[0,1]); self.assertFalse(r['content']['complete_configured_read'])


class WorkerProtocolTests(unittest.TestCase):
    setUp = ValidationTests.setUp

    def evidence(self, mode='metadata', n=5):
        self.p['content'].update(mode=mode, branches=[] if mode == 'metadata' else ['run'])
        r = MockROOTTests.root_inspect(self, entries=n)
        return r

    def protocol_response(self, payload):
        with patch.object(v.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, payload, 'retained diagnostic')):
            return v.worker(self.path, self.p, 1, 4096)

    def rejected(self, r):
        result = self.protocol_response(json.dumps(r))
        self.assertEqual(result['issues'][0]['code'], 'worker_protocol')
        self.assertEqual(result['issues'][0]['status'], 'UNKNOWN')
        self.assertEqual(result['diagnostics'], 'retained diagnostic')

    def test_empty_evidence(self): self.rejected(dict(issues=[], structure={}, content={}))
    def test_missing_fields(self):
        for field in ('issues', 'structure', 'content'):
            r=self.evidence(); del r[field]; self.rejected(r)
    def test_wrong_types(self):
        for field,value in [('issues',{}),('structure',[]),('content',None)]:
            r=self.evidence(); r[field]=value; self.rejected(r)
        r=self.evidence(); r['structure']['entries']=True; self.rejected(r)
    def test_duplicate_json_keys(self):
        self.assertEqual(self.protocol_response('{"issues": [], "issues": []}')['issues'][0]['code'],'worker_protocol')
    def test_nonfinite_json(self):
        for value in ('NaN','Infinity','-Infinity','1e999'):
            self.assertEqual(self.protocol_response('{"issues": '+value+'}')['issues'][0]['code'],'worker_protocol')
    def test_contradictory_issue_status(self):
        r=self.evidence(); r['issues']=[dict(code='missing_branch',status='UNKNOWN',detail='run')]; self.rejected(r)
    def test_contradictory_evidence(self):
        r=self.evidence(); r['structure']['recovered']=True; self.rejected(r)
        r=self.evidence(); r['issues']=[dict(code='missing_branch',status='FAIL',detail='run')]; self.rejected(r)
    def test_missing_branch_evidence(self):
        r=self.evidence(); del r['structure']['branch_types']['run']; self.rejected(r)
    def test_missing_object_evidence(self):
        r=self.evidence(); self.p['required_objects']={'hist':'TH1'}; self.rejected(r)
    def test_incomplete_sample(self):
        r=self.evidence('sample'); r['content']['coverage']['run']['entries']=[0,2]; self.rejected(r)
    def test_incomplete_full(self):
        r=self.evidence('full'); r['content']['coverage']['run']['entry_range']=[0,4]; self.rejected(r)
    def test_wrong_mode(self):
        r=self.evidence(); r['content']['mode']='full'; self.rejected(r)
    def test_valid_metadata(self):
        r=self.evidence(); self.assertEqual(self.protocol_response(json.dumps(r))['issues'],[])
    def test_valid_zero_full(self):
        r=self.evidence('full',0); self.assertEqual(self.protocol_response(json.dumps(r))['issues'],[])
    def test_valid_missing_tree(self):
        r=self.evidence(); r['structure']={k:r['structure'][k] for k in ('opened','tree','recovered')}
        r['issues']=[dict(code='missing_tree',status='FAIL',detail=self.p['tree'])]
        self.assertEqual(self.protocol_response(json.dumps(r))['issues'],r['issues'])
    def test_valid_missing_dictionary(self):
        self.p['content'].update(mode='sample',branches=['run']); self.p['branch_types']={}
        r=MockROOTTests.root_inspect(self,dictionary=False)
        self.assertEqual(self.protocol_response(json.dumps(r))['issues'],r['issues'])
    def test_valid_missing_branch(self):
        r=self.evidence(); del r['structure']['branch_types']['run']
        r['issues']=[dict(code='missing_branch',status='FAIL',detail='run')]
        self.assertEqual(self.protocol_response(json.dumps(r))['issues'],r['issues'])
    def test_object_match_contradiction(self):
        r=self.evidence(); self.p['required_objects']={'hist':'TH1'}
        r['structure'].update(objects={'hist':None},object_matches={'hist':True})
        r['issues']=[dict(code='missing_object',status='FAIL',detail='hist')]; self.rejected(r)
    def test_completed_read_close_exception(self):
        r=self.evidence('full'); r['issues']=[dict(code='close_exception',status='UNKNOWN',detail='mock')]
        self.assertEqual(self.protocol_response(json.dumps(r))['issues'],r['issues'])
    def object_evidence(self, listed=False, available=True):
        r=self.evidence('full')
        self.p['required_branches']=['object']; self.p['branch_types']={}
        self.p['dictionary_branches']=['object'] if listed else []
        self.p['content']['branches']=['object']
        r['structure'].update(branch_types={'object':'MissingClass'},dictionaries={'object':available},dictionary_dependent_branches=['object'])
        r['content'].update(requested_branches=['object'],checked_branches=['object'],
                            coverage={'object':dict(entries=[],entry_range=[0,5])})
        return r
    def test_unlisted_object_null_dictionary(self):
        self.rejected(self.object_evidence(available=None))
    def test_listed_object_null_dictionary(self):
        self.rejected(self.object_evidence(listed=True,available=None))
    def test_valid_object_boolean_dictionary(self):
        r=self.object_evidence(); self.assertEqual(self.protocol_response(json.dumps(r))['issues'],[])
    def test_absent_null_dictionary_missing_branch(self):
        r=self.evidence(); self.p['dictionary_branches']=['run']
        r['structure'].update(branch_types={},dictionaries={'run':None})
        r['issues']=[dict(code='missing_branch',status='FAIL',detail='run')]
        self.assertEqual(self.protocol_response(json.dumps(r))['issues'],r['issues'])
    def test_valid_unlisted_unavailable_dictionary(self):
        r=self.object_evidence(available=False)
        r['issues']=[dict(code='dictionary_unavailable',status='UNKNOWN',detail='object')]
        r['content']=dict(mode='full',requested_branches=['object'],checked_branches=[],checked_entries=[],entry_range=None,complete_configured_read=False)
        self.assertEqual(self.protocol_response(json.dumps(r))['issues'],r['issues'])
    def test_null_dictionary_controller_cli(self):
        r=self.object_evidence(available=None)
        inp=self.root/'inventory.json'; pro=self.root/'profile.json'
        inp.write_text(json.dumps(self.inv)); pro.write_text(json.dumps(self.p))
        with patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,json.dumps(r),'')), contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(v.main(['--inventory',str(inp),'--profile',str(pro),'--local-root',str(self.root)]),3)
        report=json.loads(out.getvalue()); self.assertEqual(report['status'],'UNKNOWN')
        output=report['outputs'][0]; self.assertEqual(output['issues'][0]['code'],'worker_protocol')
        self.assertEqual(output['checks']['content_readability'],'UNKNOWN')
        self.assertEqual(output['provenance']['status'],'UNKNOWN'); self.assertFalse(report['acceptance_changed'])

    def test_missing_object_dictionary_evidence(self):
        for listed in (False,True):
            r=self.object_evidence(listed=listed); del r['structure']['dictionaries']['object']; self.rejected(r)
    def test_missing_required_object_branch_failure(self):
        r=self.object_evidence(listed=True)
        r['structure'].update(branch_types={},dictionary_dependent_branches=[],dictionaries={'object':None})
        r['issues']=[dict(code=code,status='FAIL',detail='object') for code in ('missing_branch','missing_read_branch')]
        r['content']=dict(mode='full',requested_branches=['object'],checked_branches=[],checked_entries=[],entry_range=None,complete_configured_read=False)
        self.assertEqual(self.protocol_response(json.dumps(r))['issues'],r['issues'])
    def test_missing_dependence_metadata(self):
        r=self.object_evidence(); del r['structure']['dictionary_dependent_branches']; self.rejected(r)
    def test_valid_scalar_no_dictionary(self):
        r=self.evidence('full'); self.assertEqual(r['structure']['dictionary_dependent_branches'],[])
        self.assertEqual(r['structure']['dictionaries'],{})
        self.assertEqual(self.protocol_response(json.dumps(r))['issues'],[])
    def test_missing_dictionary_cli_unknown(self):
        r=self.object_evidence(); del r['structure']['dictionaries']['object']
        inp=self.root/'inventory.json'; pro=self.root/'profile.json'
        inp.write_text(json.dumps(self.inv)); pro.write_text(json.dumps(self.p))
        with patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,json.dumps(r),'')), contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(v.main(['--inventory',str(inp),'--profile',str(pro),'--local-root',str(self.root)]),3)
        report=json.loads(out.getvalue()); self.assertEqual(report['status'],'UNKNOWN')
        self.assertEqual(report['outputs'][0]['issues'][0]['code'],'worker_protocol')

    def test_valid_partial_read(self):
        self.p['content'].update(mode='full',branches=['run'])
        r=MockROOTTests.root_inspect(self,partial_read=[4,0])
        self.assertEqual(self.protocol_response(json.dumps(r))['issues'],r['issues'])
    def test_protocol_cli_unknown(self):
        inp=self.root/'inventory.json'; pro=self.root/'profile.json'
        inp.write_text(json.dumps(self.inv)); pro.write_text(json.dumps(self.p))
        with patch.object(v.subprocess,'run',return_value=subprocess.CompletedProcess([],0,'{"issues": [], "structure": {}, "content": {}}','')), contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(v.main(['--inventory',str(inp),'--profile',str(pro),'--local-root',str(self.root)]),3)
        report=json.loads(out.getvalue()); self.assertEqual(report['status'],'UNKNOWN')
        self.assertEqual(report['outputs'][0]['issues'][0]['code'],'worker_protocol')


class ROOTIntegrationTests(unittest.TestCase):
    def setUp(self):
        ValidationTests.setUp(self)
        try:
            import ROOT
        except ImportError as exc: self.skipTest(f'PyROOT unavailable: {exc}')
        self.ROOT=ROOT
        self.make_file()
    def make_file(self,n=5,tree_name='outputTree_00000',hist=True):
        from array import array
        f=self.ROOT.TFile(str(self.path),'RECREATE'); tree=self.ROOT.TTree(tree_name,'synthetic'); run=array('I',[0]); tree.Branch('run',run,'run/i')
        for i in range(n): run[0]=i; tree.Fill()
        tree.Write()
        if hist: self.ROOT.TH1D('hist','synthetic',2,0,2).Write()
        f.Close()
    def check(self): return v.validate_outputs(self.inv,{'test':self.p},self.root)
    code = ValidationTests.code
    def test_valid_root(self): self.assertEqual(self.check()['status'],'PASS')
    def test_real_zero(self): self.make_file(0); self.assertEqual(self.check()['status'],'PASS')
    def test_real_zero_forbidden(self): self.make_file(0); self.p['allow_zero_entries']=False; self.assertEqual(self.check()['status'],'FAIL')
    def test_real_missing_tree(self): self.make_file(tree_name='wrong'); self.code('missing_tree')
    def test_real_missing_branch(self): self.p['required_branches'].append('absent'); self.code('missing_branch')
    def test_real_wrong_type(self): self.p['branch_types']['run']='Float_t'; self.code('branch_type_mismatch')
    def test_real_missing_object(self): self.p['required_objects']={'absent':'TH1'}; self.code('missing_object')
    def test_real_object(self): self.p['required_objects']={'hist':'TH1'}; self.assertEqual(self.check()['status'],'PASS')
    def test_real_corrupt(self): self.path.write_bytes(b'not ROOT'); self.assertEqual(self.check()['status'],'FAIL')
    def test_real_sample(self):
        self.p['content'].update(mode='sample',branches=['run']); r=self.check()
        self.assertEqual(r['status'],'PASS'); self.assertEqual(r['outputs'][0]['content']['checked_entries'],[0,2,4])
    def test_real_zero_full(self):
        self.make_file(0); self.p['content'].update(mode='full',branches=['run'])
        r=self.check(); self.assertEqual(r['status'],'PASS')
        self.assertTrue(r['outputs'][0]['content']['complete_configured_read'])
    def test_real_vector_dictionary_metadata(self):
        f=self.ROOT.TFile(str(self.path),'RECREATE'); tree=self.ROOT.TTree('outputTree_00000','synthetic')
        values=self.ROOT.std.vector('int')(); tree.Branch('object',values)
        values.push_back(7); tree.Fill(); tree.Write(); f.Close()
        self.p['required_branches']=['object']; self.p['branch_types']={}
        self.p['content'].update(mode='full',branches=['object'])
        r=self.check(); self.assertEqual(r['status'],'PASS')
        structure=r['outputs'][0]['structure']
        self.assertEqual(structure['dictionary_dependent_branches'],['object'])
        self.assertTrue(structure['dictionaries']['object'])
    def test_real_scalar_dictionary_metadata(self):
        r=self.check(); self.assertEqual(r['status'],'PASS')
        self.assertEqual(r['outputs'][0]['structure']['dictionary_dependent_branches'],[])
        self.assertEqual(r['outputs'][0]['structure']['dictionaries'],{})
    def test_real_full(self):
        self.p['content'].update(mode='full',branches=['run']); r=self.check()
        self.assertEqual(r['status'],'PASS'); self.assertEqual(r['outputs'][0]['content']['entry_range'],[0,5])


if __name__=='__main__': unittest.main()
