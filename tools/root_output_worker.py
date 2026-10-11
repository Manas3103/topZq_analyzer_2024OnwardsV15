"""Isolated ROOT reader; private subprocess protocol for root_output_validation."""
import json
import os
import resource
import sys


def inspect_root(ROOT, path, p):
    r = dict(issues=[], structure={}, content=dict(mode=p['content']['mode'], requested_branches=p['content']['branches'], checked_branches=[], checked_entries=[], entry_range=None, complete_configured_read=False))
    def issue(code, status, detail): r['issues'].append(dict(code=code, status=status, detail=detail))
    f = None
    try:
        try:
            f = ROOT.TFile.Open(path, 'READ')
        except OSError as exc:
            issue('root_open_failed', 'FAIL', str(exc)); return r
        if not f or f.IsZombie():
            issue('root_open_failed', 'FAIL', 'null or zombie ROOT file'); return r
        r['structure']['opened'] = True
        r['structure']['tree'] = p['tree']
        r['structure']['recovered'] = bool(f.TestBit(ROOT.TFile.kRecovered))
        if r['structure']['recovered']: issue('recovered_file', 'FAIL', 'ROOT recovery flag set')
        tree = f.Get(p['tree'])
        if not tree or not tree.InheritsFrom('TTree'):
            issue('missing_tree', 'FAIL', p['tree']); return r
        if tree.InheritsFrom('TChain'):
            issue('external_storage', 'UNKNOWN', 'TChain is not a self-contained output tree'); return r
        n = int(tree.GetEntries()); r['structure']['entries'] = n
        if n < 0: issue('invalid_entry_count', 'FAIL', str(n)); return r
        if n == 0 and not p['allow_zero_entries']: issue('zero_entries_forbidden', 'FAIL', 'profile requires selected events')
        types = {}
        branches = {b.GetName(): b for b in tree.GetListOfBranches()}
        for name, b in branches.items():
            leaves = list(b.GetListOfLeaves())
            types[name] = b.GetClassName() or (leaves[0].GetTypeName() if len(leaves) == 1 else None)
        r['structure']['branch_types'] = types
        # GetClassName is ROOT's object/container branch metadata; leaves alone
        # do not establish whether a branch needs a class dictionary.
        dependent = [name for name, b in branches.items() if b.GetClassName()]
        r['structure']['dictionary_dependent_branches'] = dependent
        for name in p['required_branches']:
            if name not in branches: issue('missing_branch', 'FAIL', name)
        for name, expected in p['branch_types'].items():
            if name in branches and types[name] != expected: issue('branch_type_mismatch', 'FAIL', f'{name}: {types[name]} != {expected}')
        objects = {}; object_matches = {}
        for name, cls in p['required_objects'].items():
            obj = f.Get(name); objects[name] = obj.ClassName() if obj else None
            object_matches[name] = bool(obj and obj.InheritsFrom(cls))
            if not obj: issue('missing_object', 'FAIL', name)
            elif not obj.InheritsFrom(cls): issue('object_type_mismatch', 'FAIL', name)
        r['structure']['objects'] = objects
        r['structure']['object_matches'] = object_matches
        dictionaries = {}
        for name in set(p['dictionary_branches']) | set(dependent):
            if name not in branches:
                dictionaries[name] = None  # configured dictionary-sensitive branch absent
                continue
            cls = branches[name].GetClassName()
            tc = ROOT.TClass.GetClass(cls) if cls else None
            dictionaries[name] = bool(tc and tc.HasDictionary()) if cls else False
        r['structure']['dictionaries'] = dictionaries
        if p['content']['mode'] == 'metadata': return r
        requested = p['content']['branches']
        # A local ROOT header can reference separate (possibly remote) basket files.
        # Refuse those rather than following them during content inspection.
        def external_storage(branch):
            return bool(branch.GetFileName()) or any(external_storage(child) for child in branch.GetListOfBranches())
        for name in requested:
            if name in branches and external_storage(branches[name]):
                issue('external_storage', 'UNKNOWN', f'{name}: external basket storage unsupported')
        if any(i['code'] == 'external_storage' for i in r['issues']): return r
        for name in requested:
            if name not in branches: issue('missing_read_branch', 'FAIL', name)
            elif name in dictionaries and not dictionaries[name]: issue('dictionary_unavailable', 'UNKNOWN', name)
        if any(name not in branches or name in dictionaries and not dictionaries[name] for name in requested): return r
        if p['content']['mode'] == 'full':
            indices = range(n)
        else:
            count = min(n, p['content']['sample_entries'])
            indices = sorted({i * (n - 1) // (count - 1) for i in range(count)}) if count > 1 else ([0] if count else [])
        # Read each configured branch independently so another branch's byte count
        # cannot hide a failed branch. Split children are explicitly enabled.
        tree.SetBranchStatus('*', 0)
        for name in requested: tree.SetBranchStatus(name + '*', 1)
        done = 0
        r['content']['coverage'] = {name: dict(entries=[], entry_range=None) for name in requested}
        for index in indices:
            if tree.LoadTree(index) < 0: issue('content_unreadable', 'FAIL', f'LoadTree {index}'); return r
            for name in requested:
                if branches[name].GetEntry(index) <= 0:
                    issue('content_unreadable', 'FAIL', f'{name} entry {index}'); return r
                coverage = r['content']['coverage'][name]
                if p['content']['mode'] == 'sample': coverage['entries'].append(index)
                else: coverage['entry_range'] = [0, index + 1]
                if name not in r['content']['checked_branches']: r['content']['checked_branches'].append(name)
            done += 1
            r['content']['entries_checked'] = done
            if p['content']['mode'] == 'sample': r['content']['checked_entries'].append(index)
            else: r['content']['entry_range'] = [0, done]  # half-open, completed entries
        r['content']['checked_branches'] = requested if done else []
        r['content']['complete_configured_read'] = p['content']['mode'] == 'full'
        r['content']['entries_checked'] = done
    except Exception as exc:
        issue('inspection_exception', 'UNKNOWN', str(exc))
    finally:
        if f:
            try: f.Close()
            except Exception as exc: issue('close_exception', 'UNKNOWN', str(exc))
    return r


def main():
    request = json.load(sys.stdin)
    # Preserve a clean protocol channel; ROOT/PyROOT diagnostics go to stderr.
    protocol = os.fdopen(os.dup(1), 'w')
    os.dup2(2, 1)
    try:
        resource.setrlimit(resource.RLIMIT_AS, (request['memory_mb'] * 1024**2,) * 2)
        resource.setrlimit(resource.RLIMIT_CPU, (request['cpu_seconds'], request['cpu_seconds'] + 1))
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        import ROOT
    except Exception as exc:
        result = dict(issues=[dict(code='root_dependency_unavailable', status='UNKNOWN', detail=str(exc))], structure={}, content={})
    else:
        result = inspect_root(ROOT, request['path'], request['profile'])
    protocol.write(json.dumps(result, allow_nan=False)); protocol.flush()


if __name__ == '__main__': main()
