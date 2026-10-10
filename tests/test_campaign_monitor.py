import contextlib
import copy
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import campaign as c
import campaign_monitor as m


class MonitorTests(unittest.TestCase):
    def setUp(self):
        d = c.load_json(c.REPO / 'docs/campaign_templates/dataset.json')
        d.update(ordered_inputs=['input'], chunks=[dict(chunk_id='c0', index=0, ordered_inputs=['input'])])
        a = c.load_json(c.REPO / 'docs/campaign_templates/attempt.json')
        a.update(campaign_id='campaign', attempt_id='a0', workflow=d['workflow'], ordered_inputs=['input'])
        self.data = dict(schema_version=1, kind='condor_monitor_input', expected=[dict(schema_version=1, kind='expected_chunks', campaign_id='campaign', plan_source=dict(path='submit', sha256='a'*64), dataset=d)], attempts=[dict(dataset_id=d['dataset_id'], chunk_id='c0', attempt=a, job_id='10.0', mapping_evidence='reviewed submission receipt')])
        self.snap = dict(schema_version=1, kind='condor_snapshot', source='synthetic', observed_at='2026-10-10T00:00:00Z', queue=[], history=[])
        self.ad = dict(ClusterId=10, ProcId=0, JobStatus=1)

    def report(self):
        with patch.object(m.subprocess, 'check_output', side_effect=AssertionError('no live access')):
            return m.reconcile(self.data, self.snap)

    def state(self, code, expected):
        self.ad['JobStatus'] = code
        self.snap['queue'] = [self.ad]
        self.assertEqual(self.report()['attempts'][0]['state'], expected)

    def test_idle(self): self.state(1, 'idle')
    def test_running(self): self.state(2, 'running')
    def test_removed(self): self.state(3, 'removed')
    def test_held(self):
        self.ad.update(HoldReason='test hold', HoldReasonCode=12)
        self.state(5, 'held')
        self.assertEqual(self.report()['attempts'][0]['current']['ad']['HoldReason'], 'test hold')
    def test_transferring(self): self.state(6, 'transferring_output')
    def test_suspended(self): self.state(7, 'suspended')
    def completed(self, **fields):
        self.ad.update(JobStatus=4, **fields)
        self.snap['history'] = [self.ad]
        return self.report()['attempts'][0]['exit']
    def test_zero(self): self.assertEqual(self.completed(ExitBySignal=False, ExitCode=0), 'normal_zero')
    def test_nonzero(self): self.assertEqual(self.completed(ExitBySignal=False, ExitCode=9), 'normal_nonzero')
    def test_signal(self): self.assertEqual(self.completed(ExitBySignal=True, ExitSignal=9), 'signal')
    def test_missing_exit(self):
        self.completed(ExitCode=0)
        attempt = self.report()['attempts'][0]
        self.assertEqual(attempt['state'], 'completed')
        self.assertEqual(attempt['exit'], 'unknown')
        self.assertEqual(attempt['reconciliation'], 'unresolved')
        self.assertIn('missing_exit_evidence', attempt['issues'])
        self.assertEqual(self.cli_report()[0], 1)
    def test_absent(self): self.assertEqual(self.report()['attempts'][0]['state'], 'unobserved')
    def test_unknown_status(self): self.state(99, 'unknown')
    def test_missing_status(self):
        del self.ad['JobStatus']; self.snap['queue'] = [self.ad]
        self.assertEqual(self.report()['attempts'][0]['state'], 'unknown')
    def test_retries(self):
        entry = copy.deepcopy(self.data['attempts'][0]); entry['attempt']['attempt_id'] = 'retry'; entry['job_id'] = '11.0'
        self.data['attempts'].append(entry)
        self.snap['queue'] = [self.ad, dict(ClusterId=11, ProcId=0, JobStatus=2)]
        self.assertEqual([a['state'] for a in self.report()['attempts']], ['idle', 'running'])
    def explicit(self):
        identity = m.registry(self.data)[0]['identity']
        self.ad.update({attr: identity[k] for k, attr in zip(m.IDENTITY, m.ATTRS)})
        self.data['attempts'][0].update(job_id=None, mapping_evidence=None)
        self.snap['queue'] = [self.ad]
    def test_explicit(self):
        self.explicit(); self.assertEqual(self.report()['attempts'][0]['current']['matching'], 'explicit_attributes')
    def test_campaign_overlap(self):
        self.explicit(); self.ad['TZQCampaignId'] = 'other'
        self.assertEqual(self.report()['summary']['unresolved_observations'], 1)
    def test_two_campaigns_same_chunk(self):
        self.explicit()
        plan = copy.deepcopy(self.data['expected'][0]); plan['campaign_id'] = 'other'
        entry = copy.deepcopy(self.data['attempts'][0]); entry['attempt']['campaign_id'] = 'other'
        self.data['expected'].append(plan); self.data['attempts'].append(entry)
        self.snap['queue'].append(dict(self.ad, ClusterId=11, TZQCampaignId='other', JobStatus=2))
        self.assertEqual([a['state'] for a in self.report()['attempts']], ['idle', 'running'])
    def test_partial_identity(self):
        self.explicit(); del self.ad['TZQAttemptId']
        self.assertEqual(self.report()['summary']['unresolved_observations'], 1)
    def test_mapping_conflict(self):
        self.explicit(); self.data['attempts'][0].update(job_id='10.0', mapping_evidence='receipt'); self.ad['TZQCampaignId'] = 'wrong'
        self.assertIn('identity_conflict', self.report()['attempts'][0]['issues'])
    def cli_report(self):
        with tempfile.TemporaryDirectory() as td:
            inp = Path(td)/'input.json'; snap = Path(td)/'snapshot.json'
            inp.write_text(json.dumps(self.data)); snap.write_text(json.dumps(self.snap))
            with patch.object(m.subprocess, 'check_output', side_effect=AssertionError('no live access')):
                with contextlib.redirect_stdout(io.StringIO()) as out:
                    code = m.main(['--input', str(inp), '--snapshot', str(snap)])
            return code, json.loads(out.getvalue())
    def conflicting_attempts(self, sources, other_campaign=False, mapped=False):
        self.explicit()
        entry = copy.deepcopy(self.data['attempts'][0])
        entry['attempt']['attempt_id'] = 'a1'
        if other_campaign:
            entry['attempt']['campaign_id'] = 'other'
            plan = copy.deepcopy(self.data['expected'][0]); plan['campaign_id'] = 'other'
            self.data['expected'].append(plan)
        self.data['attempts'].append(entry)
        other = dict(self.ad, TZQAttemptId='a1', TZQCampaignId=entry['attempt']['campaign_id'])
        if mapped:
            self.data['attempts'][0].update(job_id='10.0', mapping_evidence='receipt')
        self.snap.update(queue=[], history=[])
        for source, ad in zip(sources, (self.ad, other)):
            self.snap[source].append(ad)
        return [self.ad, other]
    def assert_job_conflict(self, originals):
        code, report = self.cli_report()
        self.assertEqual(code, 1)
        self.assertEqual(report['summary']['unresolved_attempts'], 2)
        self.assertEqual(report['summary']['unresolved_observations'], 2)
        for attempt in report['attempts']:
            self.assertEqual(attempt['reconciliation'], 'unresolved')
            self.assertIn('job_identity_conflict', attempt['issues'])
            self.assertIsNone(attempt['current'])
            self.assertEqual([o['ad'] for o in attempt['observations']], originals)
            self.assertTrue(all(not o['resolved'] for o in attempt['observations']))
    def test_job_identity_queue_history(self):
        self.assert_job_conflict(self.conflicting_attempts(('queue', 'history')))
    def test_job_identity_queue_queue(self):
        self.assert_job_conflict(self.conflicting_attempts(('queue', 'queue')))
    def test_job_identity_history_history(self):
        self.assert_job_conflict(self.conflicting_attempts(('history', 'history')))
    def test_job_identity_mapping_attributes(self):
        self.assert_job_conflict(self.conflicting_attempts(('queue', 'history'), mapped=True))
    def test_job_identity_campaign_overlap(self):
        self.assert_job_conflict(self.conflicting_attempts(('queue', 'history'), other_campaign=True))
    def test_consistent_identity_state_conflict(self):
        self.explicit()
        self.snap['history'] = [dict(self.ad, JobStatus=2)]
        code, report = self.cli_report()
        attempt = report['attempts'][0]
        self.assertEqual(code, 1)
        self.assertIn('queue_history_conflict', attempt['issues'])
        self.assertNotIn('job_identity_conflict', attempt['issues'])
        self.assertEqual(len(attempt['observations']), 2)
    def test_consistent_identity_overlap(self):
        self.explicit(); self.snap['history'] = [copy.deepcopy(self.ad)]
        code, report = self.cli_report()
        self.assertEqual(code, 0)
        self.assertEqual(report['attempts'][0]['reconciliation'], 'observed')
        self.assertEqual(len(report['attempts'][0]['observations']), 2)
    def test_job_identity_unknown_contradiction(self):
        self.explicit(); self.snap['history'] = [dict(self.ad, TZQAttemptId='unknown')]
        code, report = self.cli_report()
        self.assertEqual(code, 1)
        self.assertIn('job_identity_conflict', report['attempts'][0]['issues'])
        self.assertEqual(len(report['attempts'][0]['observations']), 2)
    def test_multiple_jobs(self):
        self.explicit(); other = dict(self.ad, ClusterId=11); self.snap['queue'].append(other)
        self.assertIn('multiple_jobs', self.report()['attempts'][0]['issues'])
    def test_overlap_conflict(self):
        self.snap.update(queue=[self.ad], history=[dict(self.ad, JobStatus=4)])
        r = self.report()['attempts'][0]
        self.assertIn('queue_history_conflict', r['issues']); self.assertEqual(len(r['observations']), 2)
    def test_identical_overlap(self):
        self.snap.update(queue=[self.ad], history=[copy.deepcopy(self.ad)])
        self.assertEqual(self.report()['attempts'][0]['state'], 'idle')
    def test_duplicate_queue(self):
        self.snap['queue'] = [self.ad, copy.deepcopy(self.ad)]
        self.assertIn('duplicate_observations', self.report()['attempts'][0]['issues'])
    def test_duplicate_history(self):
        self.snap['history'] = [self.ad, copy.deepcopy(self.ad)]
        self.assertIn('duplicate_observations', self.report()['attempts'][0]['issues'])
    def test_duplicate_attempt(self):
        self.data['attempts'] *= 2
        with self.assertRaises(ValueError): self.report()
    def test_duplicate_mapping(self):
        entry = copy.deepcopy(self.data['attempts'][0]); entry['attempt']['attempt_id'] = 'retry'; self.data['attempts'].append(entry)
        with self.assertRaises(ValueError): self.report()
    def test_wrong_inputs(self):
        self.data['attempts'][0]['attempt']['ordered_inputs'] = ['wrong']
        with self.assertRaises(ValueError): self.report()
    def test_missing_mapping_evidence(self):
        self.data['attempts'][0]['mapping_evidence'] = None
        with self.assertRaises(ValueError): self.report()
    def test_bad_snapshot(self):
        self.snap['queue'] = [{}]
        with self.assertRaises(ValueError): self.report()
    def test_bad_field_type(self):
        self.snap['queue'] = [dict(self.ad, ExitBySignal=0)]
        with self.assertRaises(ValueError): self.report()
    def test_versions(self):
        for target in (self.data, self.snap):
            target['schema_version'] = 2
            with self.assertRaises(ValueError): self.report()
            target['schema_version'] = 1
    def test_no_acceptance_mutation(self):
        before = copy.deepcopy(self.data); self.snap['queue'] = [self.ad]
        self.assertFalse(self.report()['acceptance_changed']); self.assertEqual(before, self.data)
    def test_cli_stdout_only(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td); inp = root/'input.json'; snap = root/'snapshot.json'; report = root/'report.json'
            inp.write_text(json.dumps(self.data)); snap.write_text(json.dumps(self.snap)); report.write_text('preserve')
            before = {p: p.read_bytes() for p in root.iterdir()}
            args = ['--input', str(inp), '--snapshot', str(snap)]
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(m.main(args), 1)
            self.assertEqual(json.loads(out.getvalue())['output_acceptance'], 'not_evaluated')
            self.assertEqual(before, {p: p.read_bytes() for p in root.iterdir()})
            with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
                m.main(args + ['--output', str(report)])
            self.assertEqual(report.read_text(), 'preserve')
    def test_live_bounds_and_commands(self):
        with patch.object(m.subprocess, 'check_output', return_value='[]') as run:
            snap, commands = m.collect(self.data)
            self.assertEqual(run.call_count, 2); self.assertIn('-scanlimit', commands['history'])
            self.assertIn('-match', commands['history']); self.assertIn('-constraint', commands['queue'])
            self.assertEqual(snap['history'], [])
        with self.assertRaises(ValueError): m.collect(self.data, timeout=61)
    def test_live_failure(self):
        with patch.object(m.subprocess, 'check_output', side_effect=m.subprocess.TimeoutExpired('condor_q', 20)):
            with self.assertRaises(m.subprocess.TimeoutExpired): m.collect(self.data)
    def test_cli_invalid_json(self):
        with tempfile.TemporaryDirectory() as td:
            inp = Path(td)/'input.json'; snap = Path(td)/'snapshot.json'
            inp.write_text(json.dumps(self.data)); snap.write_text('{"kind":1,"kind":2}')
            with contextlib.redirect_stderr(io.StringIO()), contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(m.main(['--input', str(inp), '--snapshot', str(snap)]), 2)
            self.assertEqual(out.getvalue(), '')


if __name__ == '__main__':
    unittest.main()
