"""Optional pilot input limit and explicit partial-output provenance."""
import json
from pathlib import Path

PROVENANCE_KEY = 'TZQProcessingProvenance'


def positive_events(value):
    n = int(value)
    if n <= 0 or n > 2**63 - 1: raise ValueError('max-input-events must be positive and fit signed 64 bits')
    return n


def configure_limit(analyser, maximum):
    if maximum is not None:
        if type(maximum) is not int: raise ValueError('integer maximum required')
        positive_events(maximum)
        analyser.setMaxInputEvents(maximum)


def record_limit(ROOT, analyser, output, total, maximum, tree_prefix='pilot_outputTree'):
    if maximum is None: return None  # Default output and bookkeeping remain unchanged.
    actual = int(analyser.getPilotInputEntries())
    if actual != min(total, maximum): raise RuntimeError('pilot input entry count inconsistent')
    record = dict(schema_version=1, kind='processing_input_scope', total_input_entries=total,
                  requested_max_input_events=maximum, analysis_input_entries=actual,
                  generator_weight_bookkeeping_scope='full_input_chain_when_mc', partial=True,
                  full_production_eligible=False, tree_prefix=tree_prefix)
    f = ROOT.TFile.Open(str(output), 'UPDATE')
    if not f or f.IsZombie(): raise RuntimeError('cannot record pilot provenance')
    try:
        if f.Get(PROVENANCE_KEY): raise RuntimeError('pilot provenance already exists')
        marker = ROOT.TNamed(PROVENANCE_KEY, json.dumps(record, sort_keys=True, allow_nan=False))
        if marker.Write() <= 0: raise RuntimeError('pilot provenance write failed')
    finally: f.Close()
    path = Path(str(output) + '.input_scope.json')
    with path.open('x') as stream: json.dump(record, stream, indent=2); stream.write('\n')
    return record


def require_complete(scope):
    """Required eligibility gate for production consumers; never grants acceptance."""
    if not isinstance(scope, dict) or scope.get('partial') is not False or scope.get('full_production_eligible') is not True or scope.get('requested_max_input_events') is not None or type(scope.get('total_input_entries')) is not int or scope.get('analysis_input_entries') != scope.get('total_input_entries'):
        raise ValueError('partial or unknown input scope cannot be a complete production chunk')
