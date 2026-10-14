"""Explicit sparse approval, frozen bundle identities and full-scope rejection."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest
sys.path[:0] = [str(Path(__file__).parent), str(Path(__file__).resolve().parents[1]/'tools')]
import test_campaign_production as fixtures
import campaign_production as p
import campaign_acceptance as acceptance


class SparsePreparationTests(unittest.TestCase):
    setUp = fixtures.ProductionTests.setUp
    save_config = fixtures.ProductionTests.save_config

    def enable(self, indices=(1,198)):
        self.campaign['max_input_events'] = None
        self.dataset['chunks'] = [dict(schema_version=1,kind='production_chunk',chunk_id='chunk'+str(i),index=i,
                                      ordered_inputs=[f'root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/Synthetic/NANOAODSIM/{i}.root']) for i in indices]
        rows = [dict(dataset_id=self.dataset['dataset_id'],sample_name=self.dataset['sample_name'],das_dataset=self.dataset['das_dataset'],workflow='main',chunk_id=c['chunk_id'],index=c['index'],filelist_sha256=hashlib.sha256(('\n'.join(c['ordered_inputs'])+'\n').encode()).hexdigest()) for c in self.dataset['chunks']]
        approved=dict(mode='sparse_subset',purpose='representative_validation',full_2024_complete=False,approval_reference='explicit test approval',source_inventory_sha256='a'*64,assignments=rows)
        self.approval=self.root/'approved.json';self.approval.write_text(json.dumps(approved))
        self.campaign['representative_subset']=dict(approved,approval_sha256=p.digest(self.approval))
        return self.campaign

    def prepare(self):
        return p.prepare(self.campaign,'submission',self.source,self.bundle,[],approved_assignments=self.approval)

    def test_contiguous_default_unchanged(self):
        self.campaign['max_input_events']=None
        p.validate(self.campaign)
        p.prepare(self.campaign,'submission',self.source,self.bundle,[])
        self.assertNotIn('representative_assignments.json',p.load_json(self.bundle/'manifest.json')['files'])

    def test_sparse_requires_opt_in(self):
        self.enable();self.campaign.pop('representative_subset')
        with self.assertRaisesRegex(ValueError,'contiguous'):p.validate(self.campaign)

    def test_sparse_original_ids_indices_order_hashes(self):
        self.enable();m=self.prepare();p.verify_bundle(self.bundle,p.digest(self.bundle/'manifest.json'))
        self.assertEqual([j['index'] for j in m['jobs']],[1,198])
        self.assertEqual([j['chunk_id'] for j in m['jobs']],['chunk1','chunk198'])
        for j,c in zip(m['jobs'],self.dataset['chunks']):
            self.assertEqual((self.bundle/j['filelist']).read_text().splitlines(),c['ordered_inputs'])
            self.assertEqual(j['filelist_sha256'],self.campaign['representative_subset']['assignments'][j['index']==198]['filelist_sha256'])
        self.assertEqual(len(set(j['attempt_id'] for j in m['jobs'])),2)

    def test_nonzero_single_chunk(self):
        self.enable((3,));self.assertEqual(self.prepare()['jobs'][0]['index'],3)

    def test_large_gap_single_chunk(self):
        self.enable((198,));self.assertEqual(self.prepare()['jobs'][0]['chunk_id'],'chunk198')

    def test_duplicate_approved_assignment(self):
        self.enable();s=self.campaign['representative_subset'];s['assignments'].append(copy.deepcopy(s['assignments'][0]))
        with self.assertRaisesRegex(ValueError,'duplicate'):p.validate(self.campaign)

    def test_duplicate_declared_chunk(self):
        self.enable();self.dataset['chunks'].append(copy.deepcopy(self.dataset['chunks'][0]))
        with self.assertRaisesRegex(ValueError,'duplicate'):p.validate(self.campaign)

    def test_duplicate_dataset(self):
        self.enable();self.campaign['datasets'].append(copy.deepcopy(self.dataset))
        with self.assertRaisesRegex(ValueError,'duplicate'):p.validate(self.campaign)

    def test_missing_assignment(self):
        self.enable();self.dataset['chunks'].pop()
        with self.assertRaisesRegex(ValueError,'approval'):p.validate(self.campaign)

    def test_modified_filelist(self):
        self.enable();self.dataset['chunks'][0]['ordered_inputs'][0]='root://cmsxrootd.fnal.gov//store/mc/RunIII2024Summer24NanoAODv15/Synthetic/NANOAODSIM/changed.root'
        with self.assertRaisesRegex(ValueError,'approval'):p.validate(self.campaign)

    def test_changed_original_index(self):
        self.enable();self.dataset['chunks'][0]['index']=0
        with self.assertRaisesRegex(ValueError,'approval'):p.validate(self.campaign)

    def test_reordered_inputs(self):
        self.enable();c=self.dataset['chunks'][0]
        c['ordered_inputs'].append(c['ordered_inputs'][0].replace('/1.root','/2.root'))
        row=self.campaign['representative_subset']['assignments'][0]
        row['filelist_sha256']=hashlib.sha256(('\n'.join(c['ordered_inputs'])+'\n').encode()).hexdigest()
        c['ordered_inputs'].reverse()
        with self.assertRaisesRegex(ValueError,'approval'):p.validate(self.campaign)

    def test_modified_recorded_hash(self):
        self.enable();self.campaign['representative_subset']['assignments'][0]['filelist_sha256']='b'*64
        with self.assertRaisesRegex(ValueError,'approval'):p.validate(self.campaign)

    def test_unapproved_dataset(self):
        self.enable();self.dataset['dataset_id']='other'
        with self.assertRaisesRegex(ValueError,'approval'):p.validate(self.campaign)

    def test_external_approval_required(self):
        self.enable()
        with self.assertRaisesRegex(ValueError,'explicit approved'):p.prepare(self.campaign,'submission',self.source,self.bundle,[])

    def test_changed_external_approval(self):
        self.enable();self.approval.write_text('{}')
        with self.assertRaisesRegex(ValueError,'contents/hash'):self.prepare()

    def test_frozen_approval_integrity(self):
        self.enable();self.prepare();(self.bundle/'representative_assignments.json').write_text('{}')
        with self.assertRaisesRegex(ValueError,'integrity'):p.verify_bundle(self.bundle,p.digest(self.bundle/'manifest.json'))

    def test_invalid_full_production_purpose(self):
        self.enable();self.campaign['representative_subset']['purpose']='full_production'
        with self.assertRaises(ValueError):p.validate(self.campaign)

    def test_false_full2024_completeness(self):
        self.enable();self.campaign['representative_subset']['full_2024_complete']=True
        with self.assertRaises(ValueError):p.validate(self.campaign)

    def test_PhaseC_requires_explicit_subset_scope(self):
        self.enable();self.prepare();campaign=p.load_json(self.bundle/'campaign.json')
        for selector in ['full','mc','data']:
            with self.assertRaises(acceptance.AcceptanceError):acceptance.scope(campaign,'main',selector,'scope',campaign_record=self.bundle/'campaign.json')
        s=acceptance.scope(campaign,'main','explicit','scope',datasets=['sample'],campaign_record=self.bundle/'campaign.json')
        self.assertFalse(s['campaign_complete_scope'])
        self.assertEqual([x['index'] for x in s['expected']],[1,198])
        acceptance.validate_scope(s)
        s['campaign_complete_scope']=True
        with self.assertRaises(acceptance.AcceptanceError):acceptance.validate_scope(s)


if __name__ == '__main__': unittest.main()
