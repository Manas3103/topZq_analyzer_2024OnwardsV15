"""Offline mode/protocol tests; real ROOT runs are captured separately."""
import copy,json,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import campaign_dictionary_validation as g
from tools import campaign_input_validation as i

class GateTests(unittest.TestCase):
    def values(self, maximum=None):
        p=json.loads((Path(__file__).resolve().parents[1]/'docs/root_validation_profiles/main-mc.json').read_text());p['tree']='outputTree_00000' if maximum is None else 'pilot_outputTree_00000'
        l=i.validate_inputs(['/tmp/a.root'],'Events',lambda *a:dict(status='PASS',tree='Events',entries=10,uuid='u'))
        l['files'][0]['chain_participation']='added';l['chain_total_entries']=10;i.complete(l,10 if maximum is None else min(10,maximum),[10 if maximum is None else min(10,maximum)],maximum)
        s=dict(schema_version=1,kind='processing_input_scope',total_input_entries=10,requested_max_input_events=maximum,analysis_input_entries=l['processed_entries'],generator_weight_bookkeeping_scope='full_input_chain_when_mc',partial=maximum is not None,full_production_eligible=maximum is None,tree_prefix='outputTree' if maximum is None else 'pilot_outputTree')
        return p,l,s
    def test_modes(self):
        for maximum in [None,1,1000,10000]:
            p,l,s=self.values(maximum);self.assertEqual(g.context(p,maximum,s,l),p['tree'])
    def test_wrong_tree(self):
        for maximum in [None,1]:
            p,l,s=self.values(maximum);p['tree']='pilot_outputTree_00000' if maximum is None else 'outputTree_00000'
            with self.assertRaises(g.GateError):g.context(p,maximum,s,l)
    def test_invalid_limits(self):
        for n in [0,-1,True,1.2,float('inf')]:
            p,l,s=self.values()
            with self.assertRaises(g.GateError):g.context(p,n,s,l)
    def test_scope_flags(self):
        for maximum in [None,1]:
            for key in ['partial','full_production_eligible']:
                p,l,s=self.values(maximum);s[key]=not s[key]
                with self.subTest(maximum=maximum,key=key),self.assertRaises(g.GateError):g.context(p,maximum,s,l)
    def test_missing_counter(self):
        p,l,s=self.values();del l['snapshot_processed_entries']
        with self.assertRaises(i.InputError):g.context(p,None,s,l)
    def test_wrong_processed(self):
        p,l,s=self.values();l['processed_entries']=9
        with self.assertRaises(i.InputError):g.context(p,None,s,l)
    def test_missing_ledger(self):
        p,l,s=self.values()
        with self.assertRaises(Exception):g.context(p,None,s,{})
    def test_missing_file_evidence(self):
        p,l,s=self.values();l['files']=[]
        with self.assertRaises(i.InputError):g.context(p,None,s,l)
    def test_incomplete_protocol(self):
        p,l,s=self.values()
        with self.assertRaises(g.GateError):g.validate_report({'status':'PASS'},'/tmp/b',{},p,None,s,l,'/tmp/o.root')
    def test_scope_entry_mismatch(self):
        p,l,s=self.values();s['total_input_entries']=11
        with self.assertRaises(g.GateError):g.context(p,None,s,l)
    def test_unsupported_profile_version(self):
        p,l,s=self.values();p['schema_version']=2
        with self.assertRaises(ValueError):g.context(p,None,s,l)
    def test_scope_boolean_integer(self):
        p,l,s=self.values(1);s['requested_max_input_events']=True
        with self.assertRaises(g.GateError):g.context(p,1,s,l)

    def test_worker_crash_unknown(self):
        import tempfile
        from unittest.mock import patch
        from campaign_process import CommandError
        p,l,s=self.values();hashes={n:'0'*64 for n in ['libnanoadrdframe.so','rootdict_rdict.pcm','src/Linkdef.h','src/utility.h']}
        with tempfile.TemporaryDirectory() as d,patch('campaign_process.run_process',side_effect=CommandError('nonzero_exit','worker crash',-11)):
            with self.assertRaises(g.GateError) as caught:g.gate(d,hashes,p,None,d)
            self.assertEqual(caught.exception.status,'UNKNOWN')
            self.assertEqual(json.loads((Path(d)/'dictionary-preflight.json').read_text())['status'],'UNKNOWN')
    def test_valid_worker_failure_preserved(self):
        import tempfile
        from unittest.mock import patch
        from campaign_process import CommandError
        p,l,s=self.values();hashes={n:'0'*64 for n in ['libnanoadrdframe.so','rootdict_rdict.pcm','src/Linkdef.h','src/utility.h']}
        with tempfile.TemporaryDirectory() as d:
            def failing(*args,**kwargs):
                (Path(d)/'dictionary-preflight.json').write_text(json.dumps(dict(status='FAIL',code='schema',error='missing tree')))
                raise CommandError('nonzero_exit','worker failed',1)
            with patch('campaign_process.run_process',side_effect=failing),self.assertRaises(g.GateError) as caught:g.gate(d,hashes,p,None,d)
            self.assertEqual(caught.exception.status,'FAIL')

    def test_publication_not_needed_before_publication(self):
        p,l,s=self.values();self.assertEqual(g.context(p,None,s,l),'outputTree_00000')
    def test_missing_receipt_rejected(self):
        identity=dict(campaign_id='c',submission_id='s',workflow='main',dataset_id='d',chunk_id='c0',execution_id='0'*32,attempt_id='a',cluster_id=1,proc_id=0)
        with self.assertRaises(Exception):g.verify_publication({},identity,{}, {},'root://cmseos.fnal.gov')

if __name__=='__main__':unittest.main()
