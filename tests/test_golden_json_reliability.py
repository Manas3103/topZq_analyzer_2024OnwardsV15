"""Actual compiled certification tests; opt in with an isolated matching runtime."""
import json
import os
from pathlib import Path
import tempfile
import unittest
from array import array


@unittest.skipUnless(os.environ.get('TZQ_GOLDEN_RUNTIME'), 'isolated built runtime required')
class GoldenJSONTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import ROOT
        import cppyy
        cls.ROOT = ROOT
        cls.runtime = Path(os.environ['TZQ_GOLDEN_RUNTIME'])
        ROOT.gInterpreter.AddIncludePath(str(cls.runtime))
        ROOT.gInterpreter.AddIncludePath(str(cls.runtime / 'src'))
        for lib in ['libcorrectionlib.so', 'libMathMore.so', str(cls.runtime / 'libnanoadrdframe.so')]:
            cppyy.load_reflection_info(lib)

    def setUp(self):
        self.work = tempfile.TemporaryDirectory()
        self.addCleanup(self.work.cleanup)
        self.tree = self.ROOT.TTree('Events', 'certification test')
        self.run = array('I', [0]); self.lumi = array('I', [0])
        self.tree.Branch('run', self.run, 'run/i')
        self.tree.Branch('luminosityBlock', self.lumi, 'luminosityBlock/i')
        for r, l in [(100, 1), (100, 2), (100, 3), (100, 4), (100, 0), (101, 2)]:
            self.run[0] = r; self.lumi[0] = l; self.tree.Fill()
        self.analyser = self.ROOT.NanoAODAnalyzerrdframe(self.tree, str(Path(self.work.name) / 'output.root'))

    def path(self, text):
        p = Path(self.work.name) / 'cert.json'
        p.write_text(text)
        return str(p)

    def rejected(self, text):
        with self.assertRaises(Exception):
            self.analyser.readgoodjson(self.path(text))
        self.assertFalse(self.analyser._jsonOK)
        self.assertFalse((Path(self.work.name) / 'output.root').exists())

    def test_inclusive_certified_boundaries_and_rejected_run_lumis(self):
        self.assertTrue(self.analyser.readgoodjson(self.path('{"100": [[1,3]]}')))
        self.assertEqual(int(self.analyser._rlm.Count().GetValue()), 3)
        self.assertTrue(self.analyser._jsonOK)

    def test_disjoint_intervals(self):
        self.assertTrue(self.analyser.readgoodjson(self.path('{"100": [[1,1],[3,3]]}')))
        self.assertEqual(int(self.analyser._rlm.Count().GetValue()), 2)

    def test_missing_file(self):
        with self.assertRaises(Exception):
            self.analyser.readgoodjson(str(Path(self.work.name) / 'missing.json'))
        self.assertFalse(self.analyser._jsonOK)

    def test_unreadable_file(self):
        path = Path(self.path('{"100": [[1,3]]}'))
        path.chmod(0)
        try:
            if os.access(path, os.R_OK):
                self.skipTest('process privileges bypass file read permissions')
            with self.assertRaises(Exception):
                self.analyser.readgoodjson(str(path))
            self.assertFalse(self.analyser._jsonOK)
        finally:
            path.chmod(0o600)

    def test_missing_path(self):
        with self.assertRaises(Exception):
            self.analyser.readgoodjson('')

    def test_malformed_and_trailing_json(self):
        for text in ['{', '{"100":', '{"100":[[1,3]]} garbage']:
            with self.subTest(text=text): self.rejected(text)

    def test_empty_json(self):
        for text in ['', '{}', '[]', 'null']:
            with self.subTest(text=text): self.rejected(text)

    def test_invalid_run_keys(self):
        for key in ['abc','100x',' 100','+100','-100','0100','0','4294967296','9999999999999999999999999999']:
            with self.subTest(key=key): self.rejected(json.dumps({key:[[1,3]]}))

    def test_invalid_interval_shapes(self):
        for value in [[],{},[1,3],[[1]],[[1,2,3]],[[1.5,3]],[[True,3]],[[0,3]],[[-1,3]],[[3,1]],[[1,4294967296]]]:
            with self.subTest(value=value): self.rejected(json.dumps({'100':value}))

    def test_selected_real_json_boundaries(self):
        path = self.runtime / 'data/GoldenJSON/Cert_Collisions2024_378981_386951_Golden.json'
        data = json.loads(path.read_text())
        runs = sorted(map(int, data))
        self.tree.Reset()
        for r,l in [(runs[0],data[str(runs[0])][0][0]), (runs[-1],data[str(runs[-1])][-1][1]), (1,1)]:
            self.run[0]=r; self.lumi[0]=l; self.tree.Fill()
        self.analyser = self.ROOT.NanoAODAnalyzerrdframe(self.tree,str(Path(self.work.name)/'output.root'))
        self.assertTrue(self.analyser.readgoodjson(str(path)))
        self.assertEqual(int(self.analyser._rlm.Count().GetValue()),2)


if __name__ == '__main__': unittest.main()
