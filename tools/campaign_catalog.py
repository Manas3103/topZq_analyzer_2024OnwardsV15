#!/usr/bin/env python3
"""Collision-safe LOCAL registration; remote registration is deliberately blocked."""
import argparse
import os
from pathlib import Path
import sys
from campaign_production import canonical, digest, identifier, verify_bundle


def exclusive_bytes(path, content):
    """O_EXCL on local filesystem; identical existing content alone is idempotent."""
    try:
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    except FileExistsError:
        path = Path(path)
        if path.is_symlink() or not path.is_file() or path.read_bytes() != content:
            raise ValueError('catalog collision or incomplete record: ' + str(path))
        return
    with os.fdopen(fd, 'wb') as stream:
        stream.write(content); stream.flush(); os.fsync(stream.fileno())


def register(bundle, manifest_sha256, catalog_root):
    m, c = verify_bundle(bundle, manifest_sha256)
    if ':' in str(catalog_root): raise ValueError('remote registration is blocked')
    root = Path(catalog_root)
    if '://' in str(root) or root.is_symlink(): raise ValueError('explicit local catalog required')
    root = root.resolve()
    if root == Path('/eos') or Path('/eos') in root.parents: raise ValueError('remote registration is blocked')
    if not root.is_dir(): raise ValueError('catalog root must already exist')
    ca, su = identifier(c['campaign_id']), identifier(m['submission_id'])
    target = root / ca
    target.mkdir(exist_ok=True)
    if target.is_symlink() or target.resolve().parent != root: raise ValueError('unsafe campaign catalog path')
    frozen = (Path(bundle) / 'campaign.json').read_bytes()
    exclusive_bytes(target / 'campaign.json', frozen)
    receipt = dict(schema_version=1, kind='local_campaign_registration', campaign_id=ca, submission_id=su,
                   campaign_sha256=m['campaign_sha256'], manifest_sha256=manifest_sha256,
                   processing_files=c['processing_files'], created_at=m['created_at'],
                   location='local', remote_registered=False, acceptance='not_evaluated',
                   event_limited=c.get('max_input_events') is not None, full_production_eligible=c.get('max_input_events') is None)
    from campaign import load_json
    from campaign_production import ROOT
    from jsonschema import Draft202012Validator, FormatChecker
    Draft202012Validator(load_json(ROOT / 'schemas/production/catalog_registration.schema.json'), format_checker=FormatChecker()).validate(receipt)
    # Submission manifest must exist and match before its receipt is visible.
    exclusive_bytes(target / (su + '.manifest.json'), (Path(bundle) / 'manifest.json').read_bytes())
    exclusive_bytes(target / (su + '.registration.json'), canonical(receipt) + b'\n')
    return receipt


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bundle', required=True); p.add_argument('--manifest-sha256', required=True)
    p.add_argument('--local-catalog', required=True); a = p.parse_args(argv)
    try:
        import json
        print(json.dumps(register(a.bundle, a.manifest_sha256, a.local_catalog), indent=2)); return 0
    except (OSError, ValueError) as exc: print(str(exc), file=sys.stderr); return 2


if __name__ == '__main__': raise SystemExit(main())
