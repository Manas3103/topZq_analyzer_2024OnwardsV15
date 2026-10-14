"""Deterministic offline protocol/chain tests; ROOT runtime is not invoked."""
import copy
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import Mock
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import campaign_input_validation as v


class InputTests(unittest.TestCase):
    def ledger(self, counts=(2,0,3)):
        inputs=[f'/tmp/input-{i}.root' for i in range(len(counts))]
        return v.validate_inputs(inputs,'Events',lambda loc,t,timeout:dict(status='PASS',tree=t,entries=counts[inputs.index(loc)],uuid='test'))
    def chain(self, ledger):
        records=[r for r in ledger['files'] if r['entries']]
        c=Mock();c.AddFile.return_value=1;c.GetEntries.return_value=sum(r['entries'] for r in records)
        c.GetListOfFiles.return_value=[Mock(**{'GetTitle.return_value':r['locator'],'GetName.return_value':'Events','GetEntries.return_value':r['entries']}) for r in records]
        root=Mock();root.TChain.return_value=c
        return root,c
    def test_valid_ordered_ledger(self):
        l=self.ledger();self.assertEqual(l['expected_total_entries'],5);self.assertTrue(l['files'][1]['empty'])
    def test_empty_chunk(self):
        l=self.ledger((0,0));self.assertEqual(l['issue'],'unsupported_empty_chunk');self.assertEqual(l['completion'],'UNKNOWN')
    def test_chain_with_empty(self):
        l=self.ledger();r,c=self.chain(l);v.checked_chain(r,l);self.assertEqual(c.AddFile.call_count,2);self.assertEqual(l['files'][1]['chain_participation'],'validated_empty')
    def test_chain_add_failure(self):
        l=self.ledger();r,c=self.chain(l);c.AddFile.return_value=0
        with self.assertRaises(v.InputError):v.checked_chain(r,l)
    def test_chain_total_failure(self):
        l=self.ledger();r,c=self.chain(l);c.GetEntries.return_value=4
        with self.assertRaises(v.InputError):v.checked_chain(r,l)
    def test_chain_missing_element(self):
        l=self.ledger();r,c=self.chain(l);c.GetListOfFiles.return_value=[]
        with self.assertRaises(v.InputError):v.checked_chain(r,l)
    def test_chain_wrong_entries(self):
        l=self.ledger();r,c=self.chain(l);c.GetListOfFiles.return_value[0].GetEntries.return_value=3
        with self.assertRaises(v.InputError):v.checked_chain(r,l)
    def test_chain_wrong_order(self):
        l=self.ledger();r,c=self.chain(l);c.GetListOfFiles.return_value.reverse()
        with self.assertRaises(v.InputError):v.checked_chain(r,l)
    def test_missing_file(self):
        with tempfile.TemporaryDirectory() as d:
            l=v.validate_inputs([d+'/missing.root'],'Events',lambda *a:(_ for _ in ()).throw(FileNotFoundError()))
            self.assertEqual(l['files'][0]['issue'],'missing_input');self.assertNotEqual(l['status'],'PASS')
    def test_inspection_failures(self):
        for message in ['invalid ROOT','missing tree','permission denied','timeout','missing ROOT dependency']:
            with self.subTest(message=message):
                l=v.validate_inputs(['root://host//store/a.root'],'Events',lambda *a:(_ for _ in ()).throw(RuntimeError(message)))
                self.assertEqual(l['status'],'UNKNOWN');self.assertEqual(l['files'][0]['issue'],message)
    def test_invalid_worker_evidence(self):
        for field,value in [('entries',True),('entries',-1),('entries',2.0),('status','UNKNOWN'),('tree','wrong'),('uuid',None)]:
            with self.subTest(field=field,value=value):
                result=dict(status='PASS',entries=2,tree='Events',uuid='u');result[field]=value
                l=v.validate_inputs(['root://host//store/a.root'],'Events',lambda *a:result);self.assertNotEqual(l['status'],'PASS')
    def test_missing_worker_field(self):
        l=v.validate_inputs(['root://host//store/a.root'],'Events',lambda *a:dict(status='PASS'));self.assertNotEqual(l['status'],'PASS')
    def test_duplicates(self):
        with self.assertRaises(v.InputError):v.validate_inputs(['root://a//store/a.root','root://b//store/a.root'],'Events')
    def test_unsafe_locators(self):
        for x in ['root://host//store/../a.root','root://host//store/*.root','https://host/a.root','root://u:p@host//store/a.root']:
            with self.subTest(x=x),self.assertRaises(v.InputError):v.identity(x)
    def test_deadline(self):
        l=v.validate_inputs(['root://host//store/a.root'],'Events',overall_timeout=0);self.assertNotEqual(l['status'],'PASS')
    def test_completion(self):
        l=self.ledger();r,c=self.chain(l);v.checked_chain(r,l);v.complete(l,5,[5],None);v.require_complete(l,l['ordered_inputs'],None)
    def test_pilot_completion(self):
        l=self.ledger();r,c=self.chain(l);v.checked_chain(r,l);v.complete(l,2,[2],2);v.require_complete(l,l['ordered_inputs'],2)
    def test_incomplete_processing(self):
        for count, snapshots in [(4,[4]),(5,[]),(5,[5,4]),(True,[True])]:
            with self.subTest(count=count),self.assertRaises(v.InputError):v.complete(self.ledger(),count,snapshots,None)
    def test_worker_refuses_bad_ledger(self):
        l=self.ledger();r,c=self.chain(l);v.checked_chain(r,l);v.complete(l,5,[5],None)
        for field,value in [('completion','UNKNOWN'),('chain_total_entries',4),('ordered_inputs',[]),('status','UNKNOWN')]:
            bad=copy.deepcopy(l);bad[field]=value
            with self.subTest(field=field),self.assertRaises(v.InputError):v.require_complete(bad,l['ordered_inputs'],None)
    def test_atomic_no_overwrite(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'ledger.json';v.save(p,{'a':1})
            with self.assertRaises(FileExistsError):v.save(p,{'a':2})
            self.assertEqual(v.strict_json(p.read_text()),{'a':1})
    def test_json(self):
        for s in ['{"a":1,"a":2}','{"a":NaN}','{"a":Infinity}']:
            with self.subTest(s=s),self.assertRaises(v.InputError):v.strict_json(s)

    def test_real_inspector_contract_with_fake_root(self):
        from unittest.mock import patch
        for n in [0,4]:
            tree=Mock();tree.InheritsFrom.return_value=True;tree.GetEntries.return_value=n
            f=Mock();f.IsZombie.return_value=False;f.TestBit.return_value=False;f.Get.return_value=tree;f.GetUUID.return_value.AsString.return_value='u'
            root=Mock();root.TFile.Open.return_value=f
            with patch.dict(sys.modules,ROOT=root):
                self.assertEqual(v.inspect('file.root','Events')['entries'],n)
            f.Close.assert_called_once()
    def test_root_open_zombie_and_missing_tree(self):
        from unittest.mock import patch
        for kind in ['null','zombie','tree','recovered']:
            root=Mock();f=Mock();f.IsZombie.return_value=kind=='zombie';f.TestBit.return_value=kind=='recovered';f.Get.return_value=None
            root.TFile.Open.return_value=None if kind=='null' else f
            with self.subTest(kind=kind),patch.dict(sys.modules,ROOT=root),self.assertRaises(v.InputError):
                v.inspect('file.root','Events')
    def test_worker_missing_ledger_blocks_publication(self):
        sys.path.insert(0,str(Path(__file__).parent))
        from test_campaign_production import ProductionTests
        case=ProductionTests('test_deterministic_bundle');case.setUp();self.addCleanup(case.doCleanups)
        publisher=Mock();result,work=case.invoke(_omit_ledger=True,publisher=publisher)
        self.assertEqual(result['status'],'failed');publisher.assert_not_called()
        self.assertTrue((work/'execution.failure.json').is_file())
    def test_cpp_counter_guard_order(self):
        # Static guard check only; not a claim of compiled/runtime equivalence.
        source=(Path(__file__).resolve().parents[1]/'src/framework/NanoAODAnalyzerrdframe.cpp').read_text()
        section=source[source.index('void NanoAODAnalyzerrdframe::run(bool'):]
        self.assertLess(section.index('completionCount = inputNode.Count()'),section.index('arnode->Snapshot('))
        self.assertLess(section.index('!completionCount.IsReady()'),section.index('completionCount.GetValue()'))
        self.assertIn('_rd.GetNRuns() != runsBefore + 1',section)

if __name__=='__main__':unittest.main()
