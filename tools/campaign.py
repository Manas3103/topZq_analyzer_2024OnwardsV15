#!/usr/bin/env python3
"""Read-only campaign metadata planning. Never imports ROOT or analysis configs."""
import argparse
import ast
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys

REPO = Path(__file__).resolve().parents[1]
SCHEMAS = REPO / 'schemas' / 'campaign'


def record(value=None, state='unknown', evidence=None, reason='Not measured'):
    return dict(state=state, value=value, evidence=evidence, reason=reason)


def load_json(path):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError(f'duplicate JSON key: {key}')
            result[key] = value
        return result
    def finite_float(text):
        value = float(text)
        if not math.isfinite(value):
            raise ValueError('non-finite JSON number')
        return value
    return json.loads(Path(path).read_text(), object_pairs_hook=pairs, parse_float=finite_float,
                      parse_constant=lambda x: (_ for _ in ()).throw(ValueError(f'invalid JSON number: {x}')))


def read_config(path):
    path = Path(path)
    tree = ast.parse(path.read_text(), filename=str(path))
    values = {}
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id in ('config', 'procflags'):
                    if target.id in values:
                        raise ValueError(f'{path}: repeated {target.id} assignment')
                    values[target.id] = ast.literal_eval(node.value)
    if set(values) != {'config', 'procflags'}:
        raise ValueError(f'{path}: literal config and procflags dictionaries required')
    c, flags = values['config'], values['procflags']
    if not isinstance(c, dict) or not isinstance(flags, dict):
        raise ValueError('config/procflags must be dictionaries')
    if c.get('year') != 2024 or c.get('datatype') not in (-1, 0, 1):
        raise ValueError('requires 2024 configuration and datatype -1/0/1')
    if not all(isinstance(c.get(k), str) and c[k] for k in ('intreename', 'outtreename')):
        raise ValueError('input/output tree names required')
    if c['intreename'] == c['outtreename']:
        raise ValueError('input and output tree names must differ')
    return dict(path=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest(), **values)


def validate(manifest):
    try:
        from jsonschema import Draft202012Validator, FormatChecker
    except ImportError as exc:
        raise ValueError('validation requires jsonschema; no packages installed automatically') from exc
    kind = manifest.get('kind') if isinstance(manifest, dict) else None
    if kind not in ('campaign', 'dataset', 'attempt', 'stage'):
        raise ValueError('unknown manifest kind')
    schema = load_json(SCHEMAS / f'{kind}.schema.json')
    if kind == 'campaign':
        schema['properties']['datasets']['items'] = load_json(SCHEMAS / 'dataset.schema.json')
    errors = sorted(Draft202012Validator(schema, format_checker=FormatChecker()).iter_errors(manifest), key=lambda e: str(list(e.path)))
    if errors:
        raise ValueError('; '.join(f'{list(e.path)}: {e.message}' for e in errors))
    datasets = manifest.get('datasets', [manifest] if kind == 'dataset' else [])
    ids = [d['dataset_id'] for d in datasets]
    if len(ids) != len(set(ids)):
        raise ValueError('duplicate dataset_id')
    for d in datasets:
        if (d['datatype'] == 'data') != (d['era'] is not None):
            raise ValueError('data requires era; MC era must be null')
        if kind == 'campaign' and d['era'] and d['era'] not in manifest['data_eras']:
            raise ValueError('dataset era outside campaign scope')
        for key in ('index', 'chunk_id'):
            vals = [c[key] for c in d['chunks']]
            if len(vals) != len(set(vals)):
                raise ValueError(f'duplicate chunk {key}')
        ordered = [i for c in d['chunks'] for i in c['ordered_inputs']]
        if d['chunks'] and ordered != d['ordered_inputs']:
            raise ValueError('chunk input ordering differs from dataset ordered_inputs')
        if len(d['ordered_inputs']) != len(set(d['ordered_inputs'])):
            raise ValueError('duplicate input path in dataset')
    retentions = [(d['chunk_retention'], d) for d in datasets]
    if kind == 'stage':
        retentions.append((manifest['chunk_retention'], None))
        artifact_ids = [a['artifact_id'] for a in manifest['artifacts']]
        if len(artifact_ids) != len(set(artifact_ids)):
            raise ValueError('duplicate artifact_id')
    for retention, dataset in retentions:
        keys = [(c['dataset_id'], c['chunk_id']) for c in retention['accepted_chunks']]
        if len(keys) != len(set(keys)):
            raise ValueError('multiple accepted attempts for one chunk')
        if dataset:
            expected = {c['chunk_id'] for c in dataset['chunks']}
            if any(c['dataset_id'] != dataset['dataset_id'] or c['chunk_id'] not in expected
                   for c in retention['accepted_chunks']):
                raise ValueError('accepted chunk outside dataset chunk definition')
            weights = dataset['generator_weights']
            if weights['sumGenWeight']['state'] == 'verified':
                required = ['source_script', 'input_dataset', 'input_filelist', 'calculation_report', 'coverage', 'provenance']
                if any(weights[k]['state'] != 'verified' for k in required):
                    raise ValueError('verified sumGenWeight requires verified source/input/coverage/provenance')
                if weights['input_dataset']['value'] != dataset['dataset_path']:
                    raise ValueError('generator-weight dataset identity mismatch')
                if any(not weights[k]['value']['sha256'] for k in ('source_script', 'input_filelist', 'calculation_report')):
                    raise ValueError('verified generator-weight identities require checksums')
        start = retention['retention_start']
        if start['value'] is not None:
            try:
                stamp = datetime.datetime.fromisoformat(start['value'].replace('Z', '+00:00'))
                if stamp.tzinfo is None:
                    raise ValueError('timezone required')
            except (TypeError, ValueError) as exc:
                raise ValueError('retention_start must be a timezone-aware ISO timestamp') from exc
        if retention['cleanup_eligibility'] == 'eligible_for_review':
            validations = ['merge_validation']
            if dataset is None or dataset['datatype'] == 'data':
                validations.append('data_dedup_validation')
            if any(retention[k]['state'] != 'verified' or retention[k]['value'] != 'PASS'
                   for k in validations):
                raise ValueError('cleanup review eligibility requires verified validation PASS')
            if start['state'] != 'verified' or not retention['accepted_chunks'] or not retention['cleanup_audit_trail']:
                raise ValueError('cleanup review eligibility requires retention start, accepted chunks and audit evidence')
            deadline = stamp + datetime.timedelta(days=retention['suggested_period_days'])
            if deadline > datetime.datetime.now(datetime.timezone.utc):
                raise ValueError('suggested retention period has not elapsed')
    return manifest


def git_source(repo):
    def git(*args):
        return subprocess.check_output(['git', '-C', str(repo), *args], text=True, timeout=10).strip()
    changes = git('status', '--porcelain', '--untracked-files=all').splitlines()
    return dict(repository=git('config', '--get', 'remote.origin.url'),
                branch=git('branch', '--show-current'), commit=git('rev-parse', 'HEAD'),
                dirty=bool(changes), changes=changes)


def plan(args):
    main = read_config(args.main_config)
    ff = read_config(args.ff_config)
    if main['config'].get('analysertype', 'BaseAnalyser') != 'BaseAnalyser' or ff['config'].get('analysertype') != 'FakeFactorAnalyser':
        raise ValueError('main/FF analyser configuration mismatch')
    def reference(path):
        p = REPO / path
        return record({'path':path, 'sha256':hashlib.sha256(p.read_bytes()).hexdigest()},
                      'declared', str(p), None)
    payloads = {}
    for workflow, config in [('main', main), ('fakefactor', ff)]:
        payloads[workflow] = {}
        for key, value in config['config'].items():
            if isinstance(value, str) and value.endswith(('.json', '.json.gz', '.root')):
                p = Path(value)
                if not p.is_absolute():
                    p = REPO / p
                resolved = p.resolve()
                local = REPO == resolved or REPO in resolved.parents
                if not local:
                    payloads[workflow][key] = {'path':value, 'sha256':None, 'availability':'not inspected: outside repository'}
                else:
                    payloads[workflow][key] = {'path':value, 'sha256':hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None,
                                               'availability':'present' if p.is_file() else 'unavailable'}
    unknown_norm = {name: record() for name in ('formula', 'luminosity', 'luminosity_unit', 'cross_sections', 'cross_section_unit', 'sum_weight_denominators', 'data_policy', 'applied_terms', 'disabled_terms')}
    unknown_norm.update({
        'formula': record('luminosity * cross_section / sumw; multiplied by genWeight when available', 'declared', 'normalized_hist/Python_rdf_norm/create_hist_rdf.py', None),
        'data_policy': record('unit weight 1.0', 'declared', 'normalized_hist/Python_rdf_norm/create_hist_rdf.py', None),
        'disabled_terms': record(['pileup', 'lepton SF', 'btag SF'], 'declared', 'normalized_hist/Python_rdf_norm/create_hist_rdf.py', None),
        'recipe_reference': reference('normalized_hist/Python_rdf_norm/create_hist_rdf.py'),
        'wrapper_reference': reference('normalized_hist/Python_rdf_norm/runjob_hist_py.sh')})
    result = dict(schema_version=1, kind='campaign', campaign_id=args.id, year=2024,
                  data_eras=args.eras.split(','), status='planned', approval='experimental',
                  created_at=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  source=git_source(REPO), plotting_source=dict(repository=args.plot_repo, branch=None,
                  commit=args.plot_commit, dirty=None, changes=[]),
                  configurations=dict(main=main, fakefactor=ff), datasets=[load_json(p) for p in args.dataset],
                  declared={'selection_reference':reference('src/analysis/BaseAnalyser.cpp'),
                            'trigger_reference':reference('src/framework/NanoAODAnalyzerrdframe.cpp'),
                            'fakefactor_reference':reference('src/fakefactor/FakeFactorAnalyser.cpp'),
                            'dictionary_reference':reference('src/Linkdef.h'),
                            'payloads':record(payloads, 'declared', 'configuration literal paths; hashes from local bytes', None)},
                  runtime_effective={'processing':record(reason='No processing executed')},
                  environment={key:record(os.environ[key], 'declared', 'current shell environment', None) if os.environ.get(key) else record(reason='Environment value unavailable') for key in ('CMSSW_VERSION','SCRAM_ARCH','ROOTSYS','ROOT_VERSION','CORRECTIONLIB_VERSION')},
                  normalization={'main':unknown_norm, 'fakefactor':unknown_norm}, artifact_manifests=[])
    return validate(result)


def preview(manifest):
    validate(manifest)
    if manifest['kind'] != 'campaign':
        raise ValueError('tree/summary requires campaign manifest')
    root = f"<campaign-root>/{manifest['campaign_id']}"
    lines = [root + '/metadata/']
    for workflow in ('main', 'fakefactor'):
        lines += [f'{root}/{workflow}/attempts/<attempt-id>/chunks/mc/<sample-id>/',
                  f'{root}/{workflow}/attempts/<attempt-id>/logs/',
                  f'{root}/{workflow}/stages/<stage-id>/merged/mc/',
                  f'{root}/{workflow}/stages/<stage-id>/histograms/',
                  f'{root}/{workflow}/stages/<stage-id>/validation/']
        for era in manifest['data_eras']:
            lines += [f'{root}/{workflow}/attempts/<attempt-id>/chunks/data/{era}/<dataset-id>/',
                      f'{root}/{workflow}/stages/<stage-id>/merged/data/{era}/',
                      f'{root}/{workflow}/stages/<stage-id>/data-dedup/{era}/']
    return '\n'.join(lines + [root + '/plots/<revision>/', root + '/validation/'])


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest='command', required=True)
    p = subs.add_parser('plan', help='emit a draft JSON manifest to stdout; creates no directories')
    p.add_argument('--id', required=True)
    p.add_argument('--eras', default='C,H')
    p.add_argument('--main-config', default=str(REPO / 'jobconfiganalysis_2024.py'))
    p.add_argument('--ff-config', default=str(REPO / 'jobconfiganalysis_2024_ff.py'))
    p.add_argument('--dataset', action='append', default=[], help='dataset manifest; repeat for each sample')
    p.add_argument('--plot-repo', default='https://github.com/Manas3103/stack_histogram')
    p.add_argument('--plot-commit', default=None, help='declared full SHA; not remotely verified')
    subs.add_parser('config').add_argument('path')
    for command in ('validate','summary','tree'):
        subs.add_parser(command).add_argument('manifest')
    args = parser.parse_args(argv)
    try:
        if args.command == 'plan':
            print(json.dumps(plan(args), indent=2, allow_nan=False))
        elif args.command == 'config':
            print(json.dumps(read_config(args.path), indent=2, allow_nan=False))
        else:
            m = validate(load_json(args.manifest))
            if args.command == 'validate':
                print(f"VALID: {m['kind']} structure; no runtime/physics certification")
            elif args.command == 'tree':
                print(preview(m))
            else:
                preview(m)
                print(f"Campaign {m['campaign_id']} | 2024 eras {','.join(m['data_eras'])}\n"
                      f"Source {m['source']['commit']} | dirty={m['source']['dirty']}\n"
                      f"Status {m['status']} | approval {m['approval']} | datasets {len(m['datasets'])}\n"
                      'Runtime and normalization evidence must be reviewed separately.')
        return 0
    except (ValueError, OSError, SyntaxError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
