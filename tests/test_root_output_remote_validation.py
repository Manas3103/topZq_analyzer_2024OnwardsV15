"""Offline simulations only: never contact XRootD or inspect production files."""
import contextlib
import copy
import io
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import MagicMock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import root_output_remote_validation as r
import root_output_validation as local
from root_output_worker import inspect_root
import test_root_output_validation as offline_tests

PREFIX = 'root://cmseos.fnal.gov//store/user/'
URL = PREFIX + 'synthetic/fixture.root'
STAT = 'Path: /store/user/synthetic/fixture.root\nId: 123\nSize: 1024\nFlags: 16 (IsReadable)\nMTime: 123456\n'
# Format reproduced from saved EOS stat evidence; no live query or production locator.
EOS_STAT = ('Path: /store/user/synthetic/fixture.root\nId: 123\nSize: 1024\n'
            'MTime: 2026-10-15 17:44:12\nCTime: 2026-10-15 17:44:12\n'
            'ATime: 2026-10-15 17:44:12\nFlags: 16 (IsReadable)\n'
            'Mode: 0644\nOwner: synthetic\nGroup: us_cms\n')


class RemoteValidationTests(unittest.TestCase):
    def setUp(self):
        self.p = dict(schema_version=1, kind='root_validation_profile', profile_id='test', workflow='main', datatype='data', tree='outputTree_00000', required_branches=['run'], branch_types={'run': 'UInt_t'}, required_objects={}, allow_zero_entries=True, content=dict(mode='metadata', branches=[], sample_entries=3), dictionary_branches=[])
        self.inv = dict(schema_version=1, kind='remote_root_output_inventory', outputs=[dict(campaign_id='synthetic', workflow='main', dataset_id='data', sample_name='Synthetic', datatype='data', chunk_id='c0', index=0, attempt_id='a0', locator=URL, profile_id='test')])
        self.locator = r.split_locator(URL)
        self.payload = RemoteValidationTests.evidence(self)
        self.calls = []

    def evidence(self, mode='metadata', n=5):
        self.p['content'].update(mode=mode, branches=[] if mode == 'metadata' else ['run'])
        result = offline_tests.WorkerProtocolTests.evidence(self, mode, n)
        result.update(transport='remote'); result['structure']['root_file_size'] = 1024
        return result

    def runner(self, args, timeout, input_text=None):
        self.calls.append((args, timeout, input_text))
        if args[0] == 'xrdfs':
            text = STAT if args[2] == 'stat' else 'adler32 deadbeef\n'
            return subprocess.CompletedProcess(args, 0, text, '')
        request = json.loads(input_text)
        self.assertEqual(request['transport'], 'remote')
        self.assertEqual(request['path'], URL)
        return subprocess.CompletedProcess(args, 0, json.dumps(self.payload), '')

    def validate(self, **kwargs):
        with patch.object(r, 'run_bounded', side_effect=self.runner):
            return r.validate_remote(self.inv, {'test': self.p}, [PREFIX], **kwargs)

    def issues(self, report):
        return {i['code'] for i in report['outputs'][0]['issues']}

    def test_valid_metadata(self):
        result = self.validate(); self.assertEqual(result['status'], 'PASS')
        self.assertEqual(result['outputs'][0]['checks']['content_readability'], 'UNKNOWN')
        self.assertEqual(len(self.calls), 3)

    def test_original_normalized_locator(self):
        self.inv['outputs'][0]['locator'] = URL.replace('cmseos.fnal.gov', 'CMSEOS.FNAL.GOV:1094')
        result = self.validate()['outputs'][0]['locator']
        self.assertEqual(result['original'], self.inv['outputs'][0]['locator'])
        self.assertEqual(result['normalized'], URL)

    def test_rejected_locator_forms(self):
        bad = ['https://host/file', '/eos/uscms/file.root', '/tmp/file.root', 'root://user@cmseos.fnal.gov//store/user/f.root', URL+'?token=secret', URL+'#x', URL+'?', PREFIX+'../file.root', PREFIX+'%2e%2e/file.root', PREFIX+'*/file.root', PREFIX+'a//file.root', PREFIX+'./file.root', PREFIX+'a\\file.root', PREFIX+'file\n.root', PREFIX+'file root', 'root://cmseos.fnal.gov/store/user/file.root', 'root://cmseos.fnal.gov:00001//store/user/f.root', 'root://cmseos.fnal.gov:65536//store/user/f.root']
        for value in bad:
            with self.subTest(value=value): self.assertRaises(ValueError, r.split_locator, value)

    def test_unapproved_endpoint(self):
        self.inv['outputs'][0]['locator'] = URL.replace('cmseos.fnal.gov', 'other.invalid')
        self.assertRaises(ValueError, self.validate)

    def test_prefix_boundary(self):
        self.inv['outputs'][0]['locator'] = 'root://cmseos.fnal.gov//store/users/f.root'
        self.assertRaises(ValueError, self.validate)

    def test_explicit_approval_required(self):
        self.assertRaises(ValueError, r.validate_remote, self.inv, {'test': self.p}, [])
        self.assertRaises(ValueError, r.split_locator, PREFIX[:-1], True)

    def test_duplicate_attempt(self):
        self.inv['outputs'] *= 2; self.assertRaises(ValueError, self.validate)

    def test_duplicate_locator_alias(self):
        e = copy.deepcopy(self.inv['outputs'][0]); e.update(attempt_id='a1', locator=URL.replace('cmseos.fnal.gov', 'cmseos.fnal.gov:1094'))
        self.inv['outputs'].append(e); self.assertRaises(ValueError, self.validate)

    def test_contradictory_chunk_index(self):
        e = copy.deepcopy(self.inv['outputs'][0]); e.update(attempt_id='a1', locator=PREFIX+'synthetic/other.root', index=1)
        self.inv['outputs'].append(e); self.assertRaises(ValueError, self.validate)

    def test_duplicate_job_id(self):
        self.inv['outputs'][0]['job_id'] = '1.0'
        e = copy.deepcopy(self.inv['outputs'][0]); e.update(attempt_id='a1', locator=PREFIX+'synthetic/other.root')
        self.inv['outputs'].append(e); self.assertRaises(ValueError, self.validate)

    def test_wrong_profile(self):
        self.p['datatype'] = 'mc'; self.assertRaises(ValueError, self.validate)

    def test_unsupported_version(self):
        self.inv['schema_version'] = 2; self.assertRaises(ValueError, self.validate)

    def test_record_bounds(self):
        for outputs in ([], self.inv['outputs']*101):
            self.inv['outputs'] = outputs; self.assertRaises(ValueError, self.validate)

    def test_invalid_budgets(self):
        for key, value in [('command_timeout',0),('worker_timeout',61),('output_timeout',True),('inventory_timeout',3601),('memory_mb',128)]:
            with self.subTest(key=key): self.assertRaises(ValueError, self.validate, **{key:value})

    def test_stat_parser(self):
        data = r.parse_stat(STAT, self.locator['namespace'])
        self.assertEqual(data['size_bytes'],1024); self.assertEqual(data['reported_id'],'123'); self.assertEqual(data['flags'],['IsReadable'])

    def test_stat_optional_fields(self):
        data = r.parse_stat('Size: 1\nFlags: 16 (IsReadable)\n',self.locator['namespace'])
        self.assertIsNone(data['reported_id']); self.assertIsNone(data['reported_mtime'])

    def test_saved_eos_stat_format(self):
        data = r.parse_stat(EOS_STAT, self.locator['namespace'])
        self.assertEqual(data['size_bytes'], 1024)
        self.assertEqual(data['reported_mode'], '0644')
        self.assertEqual(data['reported_owner'], 'synthetic')
        self.assertEqual(data['reported_group'], 'us_cms')
        self.assertEqual(data['reported_ctime'], '2026-10-15 17:44:12')
        self.assertEqual(data['reported_atime'], '2026-10-15 17:44:12')

    def test_pipe_separated_stat_flags(self):
        text = EOS_STAT.replace('16 (IsReadable)', '48 (IsReadable|IsWriteable)')
        self.assertEqual(r.parse_stat(text, self.locator['namespace'])['flags'], ['IsReadable', 'IsWriteable'])
        for raw in ('48 (IsReadable||IsWriteable)', '48 (IsReadable|IsReadable)', '16 (IsReadable|IsWriteable)', '48 (IsReadable|)'):
            with self.subTest(flags=raw), self.assertRaises(ValueError):
                r.parse_stat(EOS_STAT.replace('16 (IsReadable)', raw), self.locator['namespace'])

    def test_stat_extended_fields_remain_strict(self):
        for text in (EOS_STAT + 'CTime: again\n', EOS_STAT.replace('Mode: 0644', 'Mode: rw-r--r--'),
                     EOS_STAT.replace('Owner: synthetic', 'Owner: bad name'), EOS_STAT + 'FutureField: x\n'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                r.parse_stat(text, self.locator['namespace'])

    def test_eos_access_time_change_is_not_file_mutation(self):
        normal = self.runner
        count = 0
        def run(args, *extra):
            nonlocal count
            result = normal(args, *extra)
            if args[0] == 'xrdfs' and args[2] == 'stat':
                count += 1
                result.stdout = EOS_STAT if count == 1 else EOS_STAT.replace('ATime: 2026-10-15 17:44:12', 'ATime: 2026-10-15 17:44:15')
            return result
        with patch.object(r, 'run_bounded', side_effect=run):
            report = r.validate_remote(self.inv, {'test': self.p}, [PREFIX])
        self.assertEqual(report['status'], 'PASS')

    def test_eos_identity_permission_changes_are_unknown(self):
        for field in ('CTime: 2026-10-15 17:44:12', 'Mode: 0644', 'Owner: synthetic', 'Group: us_cms'):
            normal = self.runner
            count = 0
            def run(args, *extra):
                nonlocal count
                result = normal(args, *extra)
                if args[0] == 'xrdfs' and args[2] == 'stat':
                    count += 1
                    result.stdout = EOS_STAT
                    if count == 2:
                        key, value = field.split(': ', 1)
                        changed = '0600' if key == 'Mode' else value + '1'
                        result.stdout = result.stdout.replace(field, key + ': ' + changed)
                return result
            with patch.object(r, 'run_bounded', side_effect=run):
                report = r.validate_remote(self.inv, {'test': self.p}, [PREFIX])
            self.assertEqual(report['status'], 'UNKNOWN')
            self.assertIn('remote_changed', self.issues(report))

    def test_malformed_stat(self):
        bad = ['Size: x\nFlags: 16 (IsReadable)', STAT+'Size: 3\n', STAT+'Unknown: x\n', STAT.replace('fixture.root','wrong.root'), STAT.replace('Id: 123','Id: x'), STAT.replace('16 (IsReadable)','2 (IsReadable)'), STAT.replace('16 (IsReadable)','16 (FutureFlag)'), 'Size: 1']
        for text in bad:
            with self.subTest(text=text): self.assertRaises(ValueError,r.parse_stat,text,self.locator['namespace'])

    def test_checksum_algorithms(self):
        for algorithm,digest in [('adler32','deadbeef'),('sha256','a'*64),('md5','a'*32)]:
            result=r.parse_checksum(algorithm+' '+digest)
            self.assertEqual(result['algorithm'],algorithm); self.assertFalse(result['independently_verified'])
        self.assertNotEqual(r.parse_checksum('adler32 deadbeef')['algorithm'],'sha256')

    def test_malformed_checksum(self):
        for text in ('adler32 x','sha256 deadbeef','future deadbeef','adler32 deadbeef trailing'):
            self.assertRaises(ValueError,r.parse_checksum,text)

    def query_failure(self, text, expected, status='UNKNOWN'):
        with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],53,'',text)):
            result=r.query(self.locator,'stat',1,time.monotonic()+10)
        self.assertEqual(result['issues'][0]['code'],expected); self.assertEqual(result['issues'][0]['status'],status)

    def test_confirmed_missing(self): self.query_failure('[3011] No such file','remote_missing','FAIL')
    def test_auth_failure(self): self.query_failure('[3010] denied','remote_authentication')
    def test_proxy_failure(self): self.query_failure('expired proxy','remote_authentication')
    def test_dns_failure(self): self.query_failure('Could not resolve host','remote_network')
    def test_ambiguous_failure(self): self.query_failure('generic failure','remote_query_error')
    def test_conflicting_error_codes(self): self.query_failure('[3011] [3030] failure','remote_authentication')

    def test_notfound_with_network_evidence_is_unknown(self):
        for detail in ('connection timed out', 'network failure', 'socket error', 'DNS failure', 'timeout'):
            with self.subTest(detail=detail):
                self.query_failure('[3011] No such file\n' + detail, 'remote_network')

    def test_notfound_with_unknown_error_code_is_ambiguous(self):
        self.query_failure('[3011] No such file\n[4005] Other error', 'remote_query_error')

    def test_echoed_locator_words_are_not_errors(self):
        locator = r.split_locator(PREFIX + 'synthetic/proxy-timeout/fixture.root')
        with patch.object(r, 'run_bounded', return_value=subprocess.CompletedProcess([], 53, '',
                '[3011] No such file: ' + locator['namespace'])):
            q = r.query(locator, 'stat', 1, time.monotonic() + 10)
        self.assertEqual(q['issues'][0]['code'], 'remote_missing')
        self.assertEqual(q['issues'][0]['status'], 'FAIL')

    def test_mixed_missing_network_cli_is_unknown(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'inventory.json').write_text(json.dumps(self.inv))
            (root / 'profile.json').write_text(json.dumps(self.p))
            args = ['--inventory', str(root / 'inventory.json'), '--profile', str(root / 'profile.json'), '--allow-prefix', PREFIX]
            result = subprocess.CompletedProcess([], 53, '', '[3011] No such file\nconnection timed out')
            with patch.object(r, 'run_bounded', return_value=result), contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(r.main(args), 3)
            self.assertEqual(json.loads(out.getvalue())['status'], 'UNKNOWN')

    def test_command_timeout(self):
        with patch.object(r,'run_bounded',side_effect=subprocess.TimeoutExpired('xrdfs',1,output=b'partial response',stderr=b'network stalled')):
            q=r.query(self.locator,'stat',1,time.monotonic()+10)
        self.assertEqual(q['issues'][0]['code'],'remote_query_timeout')
        self.assertEqual(q['stdout_tail'],'partial response'); self.assertEqual(q['stderr_tail'],'network stalled')

    def test_missing_xrdfs_dependency(self):
        with patch.object(r,'run_bounded',side_effect=FileNotFoundError('xrdfs')):
            q=r.query(self.locator,'stat',1,time.monotonic()+10)
        self.assertEqual(q['issues'][0]['code'],'remote_dependency_or_execution')

    def test_metadata_protocol_unknown(self):
        with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],0,'garbage','')):
            q=r.query(self.locator,'stat',1,time.monotonic()+10)
        self.assertEqual(q['issues'][0]['code'],'remote_metadata_protocol')

    def test_confirmed_missing_skips_worker(self):
        with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],53,'','[3011] not found')) as command:
            report=r.validate_remote(self.inv,{'test':self.p},[PREFIX])
        self.assertEqual(report['status'],'FAIL'); self.assertEqual(command.call_count,1)

    def test_zero_byte(self):
        with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],0,STAT.replace('Size: 1024','Size: 0'),'')):
            report=r.validate_remote(self.inv,{'test':self.p},[PREFIX])
        self.assertIn('remote_zero_byte',self.issues(report)); self.assertEqual(report['status'],'FAIL')

    def test_directory(self):
        with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],0,STAT.replace('16 (IsReadable)','18 (IsDir IsReadable)'),'')):
            report=r.validate_remote(self.inv,{'test':self.p},[PREFIX])
        self.assertIn('remote_nonregular',self.issues(report))

    def test_offline_does_not_recall(self):
        with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],0,STAT.replace('16 (IsReadable)','24 (Offline IsReadable)'),'')) as command:
            report=r.validate_remote(self.inv,{'test':self.p},[PREFIX])
        self.assertEqual(report['status'],'UNKNOWN'); self.assertEqual(command.call_count,1)

    def test_optional_checksum_not_queried(self):
        report=self.validate(); self.assertEqual(report['status'],'PASS')
        self.assertEqual(report['outputs'][0]['checks']['checksum'],'not_requested')
        self.assertFalse(any(args[0]=='xrdfs' and args[2]=='query' for args,_,_ in self.calls))

    def test_required_checksum_pass(self):
        report=self.validate(require_checksum=True); self.assertEqual(report['status'],'PASS')
        self.assertEqual(report['outputs'][0]['checks']['checksum'],'PASS'); self.assertEqual(len(self.calls),5)

    def test_required_checksum_unavailable(self):
        normal=self.runner
        def run(args,*extra):
            if args[0]=='xrdfs' and args[2]=='query': return subprocess.CompletedProcess(args,53,'','[3011] unavailable')
            return normal(args,*extra)
        with patch.object(r,'run_bounded',side_effect=run): report=r.validate_remote(self.inv,{'test':self.p},[PREFIX],require_checksum=True)
        self.assertEqual(report['status'],'UNKNOWN'); self.assertIn('checksum_unavailable',self.issues(report))

    def test_size_changed(self):
        normal=self.runner; count=0
        def run(args,*extra):
            nonlocal count
            result=normal(args,*extra)
            if args[0]=='xrdfs' and args[2]=='stat':
                count+=1
                if count==2: result.stdout=STAT.replace('Size: 1024','Size: 1025')
            return result
        with patch.object(r,'run_bounded',side_effect=run): report=r.validate_remote(self.inv,{'test':self.p},[PREFIX])
        self.assertEqual(report['status'],'UNKNOWN'); self.assertIn('remote_changed',self.issues(report))
        self.assertEqual(report['outputs'][0]['checks']['root_structure'],'UNKNOWN')

    def test_checksum_changed(self):
        normal=self.runner; count=0
        def run(args,*extra):
            nonlocal count
            result=normal(args,*extra)
            if args[0]=='xrdfs' and args[2]=='query':
                count+=1
                if count==2: result.stdout='adler32 cafebabe'
            return result
        with patch.object(r,'run_bounded',side_effect=run): report=r.validate_remote(self.inv,{'test':self.p},[PREFIX],require_checksum=True)
        self.assertEqual(report['status'],'UNKNOWN'); self.assertEqual(report['outputs'][0]['consistency']['checksum'],'changed')

    def test_post_missing_is_unknown(self):
        normal=self.runner; count=0
        def run(args,*extra):
            nonlocal count
            result=normal(args,*extra)
            if args[0]=='xrdfs' and args[2]=='stat':
                count+=1
                if count==2: result=subprocess.CompletedProcess(args,53,'','[3011] gone')
            return result
        with patch.object(r,'run_bounded',side_effect=run): report=r.validate_remote(self.inv,{'test':self.p},[PREFIX])
        self.assertEqual(report['status'],'UNKNOWN'); self.assertIn('remote_changed',self.issues(report))

    def test_identity_and_timestamp_changes(self):
        for field in ('Id: 123','MTime: 123456'):
            normal=self.runner; count=0
            def run(args,*extra):
                nonlocal count
                result=normal(args,*extra)
                if args[0]=='xrdfs' and args[2]=='stat':
                    count+=1
                    if count==2: result.stdout=STAT.replace(field,field+'7')
                return result
            with patch.object(r,'run_bounded',side_effect=run): report=r.validate_remote(self.inv,{'test':self.p},[PREFIX])
            self.assertEqual(report['status'],'UNKNOWN'); self.assertIn('remote_changed',self.issues(report))

    def test_budget_exhaustion_preserves_all_records(self):
        entry=copy.deepcopy(self.inv['outputs'][0]); entry.update(attempt_id='a1',locator=PREFIX+'synthetic/second.root'); self.inv['outputs'].append(entry)
        clock=[100.0]
        def run(args,timeout,input_text=None):
            self.assertLessEqual(timeout,1); self.calls.append(args); clock[0]+=2
            return subprocess.CompletedProcess(args,0,STAT,'')
        with patch.object(r.time,'monotonic',side_effect=lambda:clock[0]),patch.object(r,'run_bounded',side_effect=run):
            report=r.validate_remote(self.inv,{'test':self.p},[PREFIX],inventory_timeout=1)
        self.assertEqual(len(self.calls),1); self.assertEqual(report['summary'],{'UNKNOWN':2})
        self.assertTrue(all(any(i['code']=='validation_deadline' for i in out['issues']) for out in report['outputs']))

    def test_output_deadline_clamps_every_operation(self):
        clock=[100.0]
        def run(args,timeout,input_text=None):
            self.assertLessEqual(timeout,1); clock[0]+=2
            return subprocess.CompletedProcess(args,0,STAT,'')
        with patch.object(r.time,'monotonic',side_effect=lambda:clock[0]),patch.object(r,'run_bounded',side_effect=run):
            report=r.validate_remote(self.inv,{'test':self.p},[PREFIX],output_timeout=1)
        self.assertEqual(report['status'],'UNKNOWN'); self.assertIn('validation_deadline',self.issues(report))

    def test_flags_unreadable(self):
        with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],0,STAT.replace('16 (IsReadable)','0 ()'),'')) as command:
            report=r.validate_remote(self.inv,{'test':self.p},[PREFIX])
        self.assertEqual(report['status'],'UNKNOWN'); self.assertEqual(command.call_count,1)

    def test_conclusive_fail_precedes_checksum_unknown(self):
        self.payload['structure']={k:self.payload['structure'][k] for k in ('opened','tree','recovered','root_file_size')}
        self.payload['issues']=[r.error('missing_tree','FAIL',self.p['tree'])]
        normal=self.runner
        def run(args,*extra):
            if args[0]=='xrdfs' and args[2]=='query': return subprocess.CompletedProcess(args,53,'','authentication failed')
            return normal(args,*extra)
        with patch.object(r,'run_bounded',side_effect=run): report=r.validate_remote(self.inv,{'test':self.p},[PREFIX],require_checksum=True)
        self.assertEqual(report['status'],'FAIL'); self.assertIn('remote_authentication',self.issues(report))

    def test_worker_json_duplicate_and_nonfinite(self):
        for text in ('{"transport":"remote","transport":"remote"}', '{"value":NaN}'):
            with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],0,text,'')):
                result=r.remote_worker(self.locator,self.p,1,4096,time.monotonic()+10)
            self.assertEqual(result['issues'][0]['code'],'worker_protocol')

    def test_root_size_wrong_type(self):
        self.payload['structure']['root_file_size']=True
        self.assertIn('worker_protocol',self.issues(self.validate()))

    def test_worker_query_output_limit(self):
        with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],0,'x'*(r.OUTPUT_LIMIT+1),'')):
            q=r.query(self.locator,'stat',1,time.monotonic()+10)
        self.assertEqual(q['issues'][0]['code'],'remote_metadata_protocol')
        self.assertEqual(len(q['stdout_tail']),r.DIAGNOSTIC_LIMIT)

    def test_streaming_query_output_limit_is_unknown(self):
        with patch.object(r, 'run_bounded', side_effect=r.OutputLimitError(b'prefix', b'diagnostic')):
            q = r.query(self.locator, 'stat', 1, time.monotonic() + 10)
        self.assertEqual(q['issues'][0]['code'], 'remote_metadata_protocol')
        self.assertEqual(q['issues'][0]['status'], 'UNKNOWN')
        self.assertEqual(q['stdout_tail'], 'prefix')
        self.assertEqual(q['stderr_tail'], 'diagnostic')

    def test_streaming_worker_output_limit_is_unknown(self):
        with patch.object(r, 'run_bounded', side_effect=r.OutputLimitError(b'partial JSON', b'diagnostic')):
            result = r.remote_worker(self.locator, self.p, 1, 4096, time.monotonic() + 10)
        self.assertEqual(result['issues'][0]['code'], 'worker_protocol')
        self.assertEqual(result['issues'][0]['status'], 'UNKNOWN')
        self.assertEqual(result['diagnostics'], 'diagnostic')

    def test_root_size_is_diagnostic(self):
        self.payload['structure']['root_file_size']=999
        report=self.validate(); self.assertEqual(report['status'],'PASS')
        self.assertFalse(report['outputs'][0]['root_size_diagnostic']['equality_enforced'])

    def test_sample_coverage(self):
        self.payload=self.evidence('sample')
        report=self.validate(read_mode='sample'); self.assertEqual(report['status'],'PASS')
        self.assertEqual(report['outputs'][0]['root']['content']['checked_entries'],[0,2,4])

    def test_full_opt_in(self):
        self.payload=self.evidence('full')
        self.assertRaises(ValueError,self.validate,read_mode='full')
        self.assertEqual(self.validate(read_mode='full',allow_full=True)['status'],'PASS')

    def test_read_mode_must_be_explicit(self):
        self.payload=self.evidence('sample'); self.assertRaises(ValueError,self.validate)

    def test_partial_content_unknown(self):
        self.payload=self.evidence('full')
        c=self.payload['content']; c.update(entry_range=[0,1],entries_checked=1,complete_configured_read=False)
        c['coverage']['run']['entry_range']=[0,1]
        self.payload['issues']=[r.error('content_unreadable','UNKNOWN','run entry 1')]
        report=self.validate(read_mode='full',allow_full=True)
        self.assertEqual(report['status'],'UNKNOWN'); self.assertIn('content_unreadable',self.issues(report))

    def test_empty_worker_protocol(self):
        self.payload=dict(transport='remote',issues=[],structure={},content={})
        report=self.validate(); self.assertIn('worker_protocol',self.issues(report)); self.assertEqual(report['status'],'UNKNOWN')

    def test_missing_remote_context(self):
        del self.payload['transport']; self.assertIn('worker_protocol',self.issues(self.validate()))

    def test_local_semantics_not_weakened(self):
        self.payload['issues']=[r.error('root_open_failed','FAIL','mock')]; self.payload['structure']={}
        self.assertIn('worker_protocol',self.issues(self.validate()))

    def test_remote_open_unknown(self):
        self.payload['issues']=[r.error('root_open_failed','UNKNOWN','remote refusal')]; self.payload['structure']={}
        report=self.validate(); self.assertEqual(report['status'],'UNKNOWN'); self.assertIn('root_open_failed',self.issues(report))

    def test_missing_tree_fail(self):
        self.payload['structure']={k:self.payload['structure'][k] for k in ('opened','tree','recovered','root_file_size')}
        self.payload['issues']=[r.error('missing_tree','FAIL',self.p['tree'])]
        report=self.validate(); self.assertEqual(report['status'],'FAIL'); self.assertIn('missing_tree',self.issues(report))

    def test_missing_branch_fail(self):
        del self.payload['structure']['branch_types']['run']; self.payload['issues']=[r.error('missing_branch','FAIL','run')]
        self.assertEqual(self.validate()['status'],'FAIL')

    def test_required_type_failure(self):
        self.payload['structure']['branch_types']['run']='Float_t'
        self.payload['issues']=[r.error('branch_type_mismatch','FAIL','run: Float_t != UInt_t')]
        self.assertEqual(self.validate()['status'],'FAIL')

    def test_required_object_failure(self):
        self.p['required_objects']={'hist':'TH1'}
        self.payload['structure'].update(objects={'hist':None},object_matches={'hist':False})
        self.payload['issues']=[r.error('missing_object','FAIL','hist')]
        self.assertEqual(self.validate()['status'],'FAIL')

    def test_remote_coverage_adversarial(self):
        for mode in ('sample','full'):
            original=self.evidence(mode)
            for dimension in ('missing_branch','short_range','bad_indices','missing_dictionary'):
                self.payload=copy.deepcopy(original)
                if dimension=='missing_branch': del self.payload['content']['coverage']['run']
                elif dimension=='short_range':
                    if mode=='full': self.payload['content']['coverage']['run']['entry_range']=[0,4]
                    else: self.payload['content']['coverage']['run']['entries']=[0,2]
                elif dimension=='bad_indices': self.payload['content']['checked_entries']=[0,2,99]
                else:
                    self.payload['structure']['dictionary_dependent_branches']=['run']
                report=self.validate(read_mode=mode,allow_full=mode=='full')
                self.assertEqual(report['status'],'UNKNOWN'); self.assertIn('worker_protocol',self.issues(report))

    def test_dictionary_unknown(self):
        self.payload=self.evidence('sample'); self.p['branch_types']={}
        s=self.payload['structure']; s.update(branch_types={'run':'MissingClass'},dictionary_dependent_branches=['run'],dictionaries={'run':False})
        self.payload['content']=dict(mode='sample',requested_branches=['run'],checked_branches=[],checked_entries=[],entry_range=None,complete_configured_read=False)
        self.payload['issues']=[r.error('dictionary_unavailable','UNKNOWN','run')]
        self.assertEqual(self.validate(read_mode='sample')['status'],'UNKNOWN')

    def test_worker_crash(self):
        with patch.object(r,'run_bounded',return_value=subprocess.CompletedProcess([],-11,'','mock crash')):
            result=r.remote_worker(self.locator,self.p,1,4096,time.monotonic()+10)
        self.assertEqual(result['issues'][0]['code'],'inspection_crash')

    def test_worker_timeout(self):
        with patch.object(r,'run_bounded',side_effect=subprocess.TimeoutExpired('worker',1)):
            result=r.remote_worker(self.locator,self.p,1,4096,time.monotonic()+10)
        self.assertEqual(result['issues'][0]['code'],'inspection_timeout')

    def test_inventory_deadline_no_commands(self):
        with patch.object(r,'run_bounded') as command:
            q=r.query(self.locator,'stat',1,time.monotonic()-1)
        command.assert_not_called(); self.assertEqual(q['issues'][0]['code'],'validation_deadline')

    def test_no_writes_or_acceptance(self):
        before=copy.deepcopy(self.inv); profile=copy.deepcopy(self.p)
        report=self.validate(require_checksum=True)
        self.assertEqual(before,self.inv); self.assertEqual(profile,self.p)
        self.assertFalse(report['acceptance_changed']); self.assertEqual(report['outputs'][0]['provenance']['status'],'UNKNOWN')
        self.assertEqual(report['outputs'][0]['acceptance'],'not_evaluated')
        for args,_,_ in self.calls:
            if args[0]=='xrdfs': self.assertIn(args[2],('stat','query'))
        self.assertTrue(all(timeout<=20 for _,timeout,_ in self.calls))

    def test_cli_cases(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory); inp=root/'inventory.json'; profile=root/'profile.json'; acceptance=root/'acceptance.json'
            inp.write_text(json.dumps(self.inv)); profile.write_text(json.dumps(self.p)); acceptance.write_text('unchanged')
            args=['--inventory',str(inp),'--profile',str(profile),'--allow-prefix',PREFIX]
            before={p.name:p.read_bytes() for p in root.iterdir()}
            for expected, code in [('PASS',0),('UNKNOWN',3),('FAIL',1)]:
                original=copy.deepcopy(self.payload)
                if expected=='UNKNOWN': self.payload=dict(transport='remote',issues=[],structure={},content={})
                if expected=='FAIL':
                    self.payload['structure']={k:original['structure'][k] for k in ('opened','tree','recovered','root_file_size')}
                    self.payload['content']=original['content']; self.payload['issues']=[r.error('missing_tree','FAIL',self.p['tree'])]
                with patch.object(r,'run_bounded',side_effect=self.runner),contextlib.redirect_stdout(io.StringIO()) as out:
                    self.assertEqual(r.main(args),code)
                self.assertEqual(json.loads(out.getvalue())['status'],expected)
                self.payload=original
            self.assertEqual(before,{p.name:p.read_bytes() for p in root.iterdir()})
            with contextlib.redirect_stderr(io.StringIO()): self.assertEqual(r.main(args+['--output-timeout','0']),2)


class RemoteWorkerTests(unittest.TestCase):
    setUp = RemoteValidationTests.setUp

    def fake_root(self):
        root=MagicMock(); f=root.TFile.Open.return_value; f.IsZombie.return_value=False; f.TestBit.return_value=False; f.GetSize.return_value=1024
        tree=f.Get.return_value; tree.InheritsFrom.side_effect=lambda name:name=='TTree'; tree.GetEntries.return_value=5
        b=MagicMock(); b.GetName.return_value='run'; b.GetClassName.return_value=''; b.GetFileName.return_value=''; b.GetListOfBranches.return_value=[]
        leaf=MagicMock(); leaf.GetTypeName.return_value='UInt_t'; b.GetListOfLeaves.return_value=[leaf]; b.GetEntry.return_value=4
        tree.GetListOfBranches.return_value=[b]; tree.LoadTree.return_value=0
        return root,f,tree,b

    def test_remote_open_exception(self):
        root,_,_,_=self.fake_root(); root.TFile.Open.side_effect=OSError('mock network')
        result=inspect_root(root,URL,self.p,'remote'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['issues'][0]['status'],'UNKNOWN')

    def test_remote_zombie(self):
        root,f,_,_=self.fake_root(); f.IsZombie.return_value=True
        result=inspect_root(root,URL,self.p,'remote'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['issues'][0]['status'],'UNKNOWN')

    def test_remote_read_failure(self):
        root,_,_,b=self.fake_root(); self.p['content'].update(mode='full',branches=['run']); b.GetEntry.return_value=-1
        result=inspect_root(root,URL,self.p,'remote'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['issues'][0]['status'],'UNKNOWN')

    def test_remote_unreadable_existing_tree_key(self):
        root,f,_,_=self.fake_root(); f.Get.return_value=None; key=MagicMock(); key.GetName.return_value=self.p['tree']; f.GetListOfKeys.return_value=[key]
        result=inspect_root(root,URL,self.p,'remote'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['issues'][0]['code'],'inspection_exception')

    def test_remote_conclusive_missing_tree(self):
        root,f,_,_=self.fake_root(); f.Get.return_value=None; f.GetListOfKeys.return_value=[]
        result=inspect_root(root,URL,self.p,'remote'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['issues'][0]['code'],'missing_tree'); self.assertEqual(result['issues'][0]['status'],'FAIL')

    def test_remote_unreadable_existing_object_key(self):
        root,f,tree,_=self.fake_root(); self.p['required_objects']={'hist':'TH1'}
        f.Get.side_effect=lambda name:tree if name==self.p['tree'] else None
        key=MagicMock(); key.GetName.return_value='hist'; f.GetListOfKeys.return_value=[key]
        result=inspect_root(root,URL,self.p,'remote'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['issues'][0]['code'],'inspection_exception')

    def test_remote_read_only(self):
        root,_,_,_=self.fake_root(); result=inspect_root(root,URL,self.p,'remote')
        root.TFile.Open.assert_called_once_with(URL,'READ'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['issues'],[])


class ProcessIsolationTests(unittest.TestCase):
    def test_runner_arguments(self):
        with patch.object(r.subprocess,'Popen',wraps=subprocess.Popen) as spawn,patch.object(r.os,'killpg',wraps=os.killpg) as kill:
            result=r.run_bounded([sys.executable, '-c', 'print("ok", end="")'], 5)
        self.assertEqual(result.stdout,'ok'); self.assertTrue(spawn.call_args.kwargs['start_new_session'])
        self.assertNotIn('shell',spawn.call_args.kwargs); self.assertEqual(kill.call_args.args[1],signal.SIGKILL)

    def test_timeout_kills_group(self):
        with patch.object(r.os,'killpg',wraps=os.killpg) as kill:
            self.assertRaises(subprocess.TimeoutExpired,r.run_bounded,[sys.executable,'-c','import time; time.sleep(10)'],0.1)
        self.assertEqual(kill.call_args.args[1],signal.SIGKILL)

    def test_streaming_capture_limit_stdout_stderr_and_combined(self):
        for code in ('import sys; sys.stdout.write("x" * (2 * 1024 * 1024))',
                     'import sys; sys.stderr.write("x" * (2 * 1024 * 1024))',
                     'import sys; sys.stdout.write("x" * 600000); sys.stdout.flush(); sys.stderr.write("y" * 600000)'):
            with self.subTest(code=code), self.assertRaises(r.OutputLimitError) as raised:
                r.run_bounded([sys.executable, '-c', code], 5)
            self.assertLessEqual(len(raised.exception.output) + len(raised.exception.stderr), r.OUTPUT_LIMIT)

    def test_streaming_request_and_both_response_pipes(self):
        code = ('import sys; sys.stdout.write("o" * 400000); sys.stdout.flush(); '
                'data=sys.stdin.read(); sys.stderr.write("e" * 400000); sys.stdout.write(str(len(data)))')
        result = r.run_bounded([sys.executable, '-c', code], 5, 'i' * 400000)
        self.assertEqual(result.stdout, 'o' * 400000 + '400000')
        self.assertEqual(result.stderr, 'e' * 400000)

    def test_oversized_request_refused_before_spawn(self):
        with patch.object(r.subprocess, 'Popen') as spawn:
            self.assertRaises(ValueError, r.run_bounded, ['mock'], 1, 'x' * (r.OUTPUT_LIMIT + 1))
        spawn.assert_not_called()

    def test_streaming_capture_at_exact_limit(self):
        result = r.run_bounded([sys.executable, '-c', 'import sys; sys.stdout.write("x" * 1048576)'], 5)
        self.assertEqual(len(result.stdout), r.OUTPUT_LIMIT)

    def test_actual_timeout_stops_same_group_descendant(self):
        code='import os,time; child=os.fork(); print(child,flush=True) if child else None; time.sleep(10)'
        with self.assertRaises(subprocess.TimeoutExpired) as raised:
            r.run_bounded([sys.executable,'-c',code],0.2)
        output=raised.exception.output
        pid=int(output.decode() if isinstance(output,bytes) else output)
        try: state=Path(f'/proc/{pid}/stat').read_text().split()[2]
        except FileNotFoundError: state=None
        self.assertIn(state,(None,'Z'))  # killed; zombies await OS reaping

    def test_actual_local_timeout(self):
        start=time.monotonic()
        self.assertRaises(subprocess.TimeoutExpired,r.run_bounded,[sys.executable,'-c','import time; time.sleep(10)'],0.1)
        self.assertLess(time.monotonic()-start,3)


class RemoteContextROOTIntegrationTests(unittest.TestCase):
    """Actual local fixtures with remote error policy; no network locator opened."""
    def setUp(self):
        RemoteValidationTests.setUp(self)
        try: import ROOT
        except ImportError as exc: self.skipTest(str(exc))
        self.ROOT=ROOT; self.tmp=tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        from array import array
        self.path=Path(self.tmp.name)/'tiny.root'; f=ROOT.TFile(str(self.path),'RECREATE'); t=ROOT.TTree(self.p['tree'],'synthetic'); run=array('I',[1]); t.Branch('run',run,'run/i')
        for _ in range(5): t.Fill()
        t.Write(); f.Close()

    def test_local_fixture_remote_metadata(self):
        result=inspect_root(self.ROOT,str(self.path),self.p,'remote'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['issues'],[]); self.assertGreater(result['structure']['root_file_size'],0)

    def test_local_fixture_remote_sample(self):
        self.p['content'].update(mode='sample',branches=['run'])
        result=inspect_root(self.ROOT,str(self.path),self.p,'remote'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['content']['checked_entries'],[0,2,4])

    def test_actual_isolated_remote_context_local_fixture(self):
        request=dict(path=str(self.path),profile=self.p,memory_mb=4096,cpu_seconds=10,transport='remote')
        worker=Path(local.__file__).with_name('root_output_worker.py')
        process=r.run_bounded([sys.executable,str(worker)],10,json.dumps(request))
        self.assertEqual(process.returncode,0)
        result=local.strict_worker_json(process.stdout); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['issues'],[]); self.assertTrue(result['structure']['opened'])

    def test_local_fixture_remote_full(self):
        self.p['content'].update(mode='full',branches=['run'])
        result=inspect_root(self.ROOT,str(self.path),self.p,'remote'); local.validate_worker_result(result,self.p,'remote')
        self.assertEqual(result['content']['entry_range'],[0,5]); self.assertTrue(result['content']['complete_configured_read'])


if __name__=='__main__': unittest.main()
