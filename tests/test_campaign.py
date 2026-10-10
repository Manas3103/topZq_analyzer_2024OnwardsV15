import contextlib
import copy
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import campaign as c


class CampaignTests(unittest.TestCase):
    def fixture(self, kind='campaign'):
        return c.load_json(c.REPO / 'docs/campaign_templates' / f'{kind}.json')

    def test_all_templates(self):
        for kind in ('campaign', 'dataset', 'attempt', 'stage'):
            c.validate(self.fixture(kind))

    def test_unknown_requires_reason(self):
        m = self.fixture(); m['runtime_effective']['processing']['reason'] = None
        with self.assertRaises(ValueError): c.validate(m)

    def test_unknown_cannot_claim_value(self):
        m = self.fixture(); m['runtime_effective']['processing']['value'] = 'PASS'
        with self.assertRaises(ValueError): c.validate(m)

    def test_verified_requires_evidence(self):
        m = self.fixture(); m['runtime_effective']['processing'] = c.record('PASS', 'verified')
        with self.assertRaises(ValueError): c.validate(m)

    def test_duplicate_dataset(self):
        m = self.fixture(); m['datasets'] *= 2
        with self.assertRaises(ValueError): c.validate(m)

    def test_out_of_scope_era(self):
        m = self.fixture(); m['datasets'][0]['era'] = 'D'
        with self.assertRaises(ValueError): c.validate(m)

    def test_future_era_declared_supported(self):
        m = self.fixture(); m['data_eras'].append('D'); m['datasets'][0]['era'] = 'D'
        c.validate(m)

    def test_mc_with_era_rejected(self):
        m = self.fixture('dataset'); m['datatype'] = 'mc'
        with self.assertRaises(ValueError): c.validate(m)

    def test_duplicate_chunk_indices(self):
        m = self.fixture('dataset'); m['chunks'] = [dict(chunk_id='c1', index=0, ordered_inputs=[]),dict(chunk_id='c2', index=0, ordered_inputs=[])]
        with self.assertRaises(ValueError): c.validate(m)

    def test_chunk_order(self):
        m = self.fixture('dataset'); m['ordered_inputs'] = ['a','b']; m['chunks'] = [dict(chunk_id='c1', index=0, ordered_inputs=['b','a'])]
        with self.assertRaises(ValueError): c.validate(m)

    def test_duplicate_inputs(self):
        m = self.fixture('dataset'); m['ordered_inputs'] = ['a','a']
        with self.assertRaises(ValueError): c.validate(m)

    def test_path_traversal_id(self):
        m = self.fixture(); m['campaign_id'] = '../bad'
        with self.assertRaises(ValueError): c.validate(m)

    def test_invalid_status(self):
        m = self.fixture(); m['status'] = 'done'
        with self.assertRaises(ValueError): c.validate(m)

    def test_invalid_timestamp(self):
        m = self.fixture(); m['created_at'] = 'yesterday'
        with self.assertRaises(ValueError): c.validate(m)

    def test_preview_does_not_create_directory(self):
        text = c.preview(self.fixture())
        self.assertIn('/fakefactor/',text); self.assertIn('/data/C/',text)
        self.assertFalse((c.REPO / 'example-2024-CH').exists())

    def test_config_not_executed(self):
        with tempfile.TemporaryDirectory() as td:
            p = Path(td)/'config.py'; marker=Path(td)/'marker'
            p.write_text(f"raise RuntimeError('must not run')\nconfig={{'year':2024,'datatype':-1,'intreename':'Events','outtreename':'outputTree'}}\nprocflags={{}}\n")
            self.assertEqual(c.read_config(p)['config']['year'],2024)
            self.assertFalse(marker.exists())

    def test_nonliteral_config_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'config.py';p.write_text('config=dict(year=2024)\nprocflags={}\n')
            with self.assertRaises(ValueError):c.read_config(p)

    def test_json_duplicates_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'m.json';p.write_text('{"kind":"dataset","kind":"campaign"}')
            with self.assertRaises(ValueError):c.load_json(p)

    def test_cli_invalid_json_returns_two(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'m.json';p.write_text('{}')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(c.main(['validate',str(p)]),2)

    def test_schema_definitions_valid(self):
        from jsonschema import Draft202012Validator
        for p in c.SCHEMAS.glob('*.json'):Draft202012Validator.check_schema(c.load_json(p))

    def test_external_sum_can_remain_unknown(self):
        m = self.fixture('dataset')
        c.validate(m)
        self.assertIsNone(m['generator_weights']['sumGenWeight']['value'])
        self.assertIsNone(m['normalization']['cross_section_unit']['value'])

    def test_declared_sum_and_units(self):
        m = self.fixture('dataset')
        m['generator_weights']['sumGenWeight'] = c.record(-123.5, 'declared', 'synthetic report', None)
        m['normalization']['cross_section'] = c.record(2.5, 'declared', 'synthetic table', None)
        c.validate(m)

    def test_sum_wrong_type(self):
        m = self.fixture('dataset'); m['generator_weights']['sumGenWeight'] = c.record('123', 'declared')
        with self.assertRaises(ValueError): c.validate(m)

    def test_verified_sum_requires_provenance(self):
        m = self.fixture('dataset'); m['generator_weights']['sumGenWeight'] = c.record(123, 'verified', 'report')
        with self.assertRaises(ValueError): c.validate(m)

    def test_verified_external_sum(self):
        m = self.fixture('dataset'); w = m['generator_weights']
        w['sumGenWeight'] = c.record(123, 'verified', 'synthetic report')
        w['input_dataset'] = c.record(m['dataset_path'], 'verified', 'synthetic dataset')
        for key in ('source_script','input_filelist','calculation_report'):
            w[key] = c.record({'path':'synthetic/'+key,'sha256':'a'*64}, 'verified', 'synthetic evidence')
        for key in ('coverage','provenance'):
            w[key] = c.record({'synthetic':True}, 'verified', 'synthetic evidence')
        c.validate(m)
        w['input_dataset']['value'] = '/wrong/dataset'
        with self.assertRaises(ValueError): c.validate(m)

    def test_no_deletion_enable_flag(self):
        m = self.fixture('stage'); m['chunk_retention']['deletion_enabled'] = True
        with self.assertRaises(ValueError): c.validate(m)

    def test_retention_default(self):
        r = self.fixture('dataset')['chunk_retention']
        self.assertEqual(r['suggested_period_days'],7)
        self.assertFalse(r['deletion_enabled'])
        self.assertEqual(r['cleanup_eligibility'],'unknown')

    def test_eligibility_without_validation_rejected(self):
        m=self.fixture('dataset');m['chunk_retention']['cleanup_eligibility']='eligible_for_review'
        with self.assertRaises(ValueError):c.validate(m)

    def test_duplicate_accepted_attempts(self):
        m=self.fixture('dataset');m['chunks']=[dict(chunk_id='c1',index=0,ordered_inputs=[])]
        a=dict(dataset_id=m['dataset_id'],chunk_id='c1',attempt_id='a1',artifact_path='local.root',sha256=None)
        m['chunk_retention']['accepted_chunks']=[a,dict(a,attempt_id='a2')]
        with self.assertRaises(ValueError):c.validate(m)

    def test_unknown_accepted_chunk(self):
        m=self.fixture('dataset');m['chunk_retention']['accepted_chunks']=[dict(dataset_id=m['dataset_id'],chunk_id='missing',attempt_id='a1',artifact_path='local.root',sha256=None)]
        with self.assertRaises(ValueError):c.validate(m)

    def test_retention_bad_timestamp(self):
        m=self.fixture('stage');m['chunk_retention']['retention_start']=c.record('yesterday','declared')
        with self.assertRaises(ValueError):c.validate(m)

    def test_retention_period_not_elapsed(self):
        m=self.fixture('dataset');r=m['chunk_retention'];r['cleanup_eligibility']='eligible_for_review'
        r['merge_validation']=c.record('PASS','verified','report');r['data_dedup_validation']=c.record('PASS','verified','report')
        r['retention_start']=c.record('2999-01-01T00:00:00Z','verified','report')
        m['chunks']=[dict(chunk_id='c1',index=0,ordered_inputs=[])]
        r['accepted_chunks']=[dict(dataset_id=m['dataset_id'],chunk_id='c1',attempt_id='a1',artifact_path='local.root',sha256=None)]
        r['cleanup_audit_trail']=[dict(timestamp='2026-10-10T00:00:00Z',action='reviewed',actor='tester',evidence='report',note='synthetic')]
        with self.assertRaises(ValueError):c.validate(m)
        r['retention_start']['value']='2000-01-01T00:00:00Z'
        c.validate(m)
        self.assertFalse(r['deletion_enabled'])

    def test_verified_null_rejected(self):
        m=self.fixture('stage');m['validation']['result']=c.record(None,'verified','report')
        with self.assertRaises(ValueError):c.validate(m)

    def test_duplicate_artifact_ids(self):
        m=self.fixture('stage');m['artifacts']*=2
        with self.assertRaises(ValueError):c.validate(m)

    def test_overflow_json_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'m.json';p.write_text('{"value":1e999}')
            with self.assertRaises(ValueError):c.load_json(p)

    def test_runtime_cannot_be_declared(self):
        m=self.fixture('attempt');m['runtime_effective']['processing']=c.record('PASS','declared')
        with self.assertRaises(ValueError):c.validate(m)


if __name__ == '__main__':
    unittest.main()
