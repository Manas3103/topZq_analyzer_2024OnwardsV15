import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
import contextlib
import io
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import campaign as c
from campaign_chunks import check_chunks

class ChunkTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name)
        d=c.load_json(c.REPO/'docs/campaign_templates/dataset.json')
        d['ordered_inputs']=['input0','input1']
        d['chunks']=[dict(chunk_id=f'c{i}',index=i,ordered_inputs=[f'input{i}']) for i in range(2)]
        self.expected=dict(schema_version=1,kind='expected_chunks',campaign_id='test',plan_source=dict(path='submission.json',sha256='a'*64),dataset=d)
        self.inventory=dict(schema_version=1,kind='chunk_inventory',campaign_id='test',dataset_id=d['dataset_id'],workflow=d['workflow'],attempts=[])
        for i in range(2):
            a=c.load_json(c.REPO/'docs/campaign_templates/attempt.json')
            a.update(campaign_id='test',workflow=d['workflow'],attempt_id=f'a{i}',status='validated',exit_code=0,ordered_inputs=[f'input{i}'])
            a['runtime_effective']['processing']=c.record('PASS','verified','synthetic log')
            data=f'synthetic {i}'.encode();(self.root/f'{i}.root').write_bytes(data)
            self.inventory['attempts'].append(dict(chunk_id=f'c{i}',index=i,attempt=a,dataset_id=d['dataset_id'],accepted=True,output=dict(path=f'{i}.root',size_bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),validation=dict(state='verified',result='PASS',evidence='synthetic integrity report'))))
    def runcheck(self): return check_chunks(self.expected,self.inventory,self.root)
    def fail(self, code):
        r=self.runcheck();self.assertEqual(r['status'],'FAIL');self.assertIn(code,[i['code'] for i in r['issues']]);self.assertEqual(r['accepted_input_manifest']['inputs'],[])
    def test_complete(self):self.assertEqual(self.runcheck()['status'],'PASS')
    def test_missing(self):self.inventory['attempts'].pop();self.fail('missing_chunk')
    def test_duplicate_index(self):self.inventory['attempts'][1]['index']=0;self.fail('duplicate_or_wrong_index')
    def test_duplicate_plan_index(self):
        self.expected['dataset']['chunks'][1]['index']=0
        with self.assertRaises(ValueError):self.runcheck()
    def test_failed(self):self.inventory['attempts'][0]['attempt'].update(status='failed',exit_code=1);self.fail('failed_attempt')
    def test_retry_success(self):
        a=copy.deepcopy(self.inventory['attempts'][0]);a['accepted']=False;a['attempt'].update(attempt_id='failed',status='failed',exit_code=1)
        self.inventory['attempts'].append(a);self.assertEqual(self.runcheck()['status'],'PASS')
    def test_retry_ambiguous(self):
        a=copy.deepcopy(self.inventory['attempts'][0]);a['attempt']['attempt_id']='retry';self.inventory['attempts'].append(a);self.fail('ambiguous_accepted_attempts')
    def test_campaign(self):self.inventory['campaign_id']='wrong';self.fail('wrong_identity')
    def test_sample(self):self.inventory['attempts'][0]['dataset_id']='wrong';self.fail('wrong_identity')
    def test_workflow(self):self.inventory['workflow']='fakefactor' if self.inventory['workflow']=='main' else 'main';self.fail('wrong_identity')
    def test_empty(self):(self.root/'0.root').write_bytes(b'');self.fail('empty_output')
    def test_absent(self):(self.root/'0.root').unlink();self.fail('missing_or_unreadable_output')
    def test_order(self):
        before=self.runcheck()['accepted_input_manifest'];self.inventory['attempts'].reverse();self.assertEqual(before,self.runcheck()['accepted_input_manifest'])
    def test_unexpected(self):self.inventory['attempts'][0]['chunk_id']='other';self.fail('unexpected_chunk')
    def test_unknown_validation(self):self.inventory['attempts'][0]['output']['validation']['state']='unknown';self.fail('unverified_output')
    def test_wrong_hash(self):self.inventory['attempts'][0]['output']['sha256']='b'*64;self.fail('integrity_mismatch')
    def test_escape(self):self.inventory['attempts'][0]['output']['path']='../out.root';self.fail('unsafe_path')
    def test_duplicate_attempt(self):self.inventory['attempts'].append(copy.deepcopy(self.inventory['attempts'][0]));self.fail('duplicate_attempt')
    def test_cli(self):
        for name,data in [('expected',self.expected),('inventory',self.inventory)]:
            (self.root/f'{name}.json').write_text(json.dumps(data))
        args=['chunks','--expected',str(self.root/'expected.json'),'--inventory',str(self.root/'inventory.json'),'--local-root',str(self.root)]
        for fmt in ('json','summary','accepted'):
            with contextlib.redirect_stdout(io.StringIO()) as out:self.assertEqual(c.main(args+['--format',fmt]),0)
            self.assertIn('PASS',out.getvalue())
        self.inventory['attempts'].pop();(self.root/'inventory.json').write_text(json.dumps(self.inventory))
        with contextlib.redirect_stdout(io.StringIO()):self.assertEqual(c.main(args),1)

if __name__=='__main__':unittest.main()
