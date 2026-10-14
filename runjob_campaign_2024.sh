#!/bin/bash
# New campaign-only worker. Never cleans worker diagnostics or touches legacy outputs.
set -euo pipefail
if [ "$#" -ne 10 ]; then
    echo 'Usage: wrapper bundle manifest-sha campaign submission workflow dataset chunk attempt ClusterId ProcId' >&2
    exit 2
fi
bundle=$1
manifest_sha=$2
campaign_id=$3
submission_id=$4
workflow=$5
dataset_id=$6
chunk_id=$7
attempt_id=$8
cluster_id=$9
proc_id=${10}
worker_root=${_CONDOR_SCRATCH_DIR:-$PWD}
execution_id=$(python3 -c 'import uuid; print(uuid.uuid4().hex)')
mkdir -p "$worker_root/campaign_executions"
bootstrap="$worker_root/campaign_executions/$execution_id.bootstrap.json"
python3 - "$bootstrap" "$execution_id" "$campaign_id" "$submission_id" <<'PYBOOT'
import json, sys
from datetime import datetime, timezone
with open(sys.argv[1], 'x') as f:
    json.dump(dict(execution_id=sys.argv[2], campaign_id=sys.argv[3], submission_id=sys.argv[4], phase='runtime_bootstrap', started_at=datetime.now(timezone.utc).isoformat()), f)
PYBOOT
if [ -n "${_CONDOR_SCRATCH_DIR:-}" ]; then
    cd "${_CONDOR_SCRATCH_DIR}"
    worker_root=$PWD
    source /cvmfs/cms.cern.ch/cmsset_default.sh
    export SCRAM_ARCH=el9_amd64_gcc12
    if [ ! -d CMSSW_13_3_3 ]; then scramv1 project CMSSW CMSSW_13_3_3; fi
    cd CMSSW_13_3_3/src
    test "$(scramv1 arch)" = el9_amd64_gcc12
    runtime_commands=$(scramv1 runtime -sh)
    eval "$runtime_commands"
    test "$CMSSW_VERSION" = CMSSW_13_3_3
    test "$CMSSW_BASE" = "$worker_root/CMSSW_13_3_3"
    cd "$worker_root"
fi
export PYTHONDONTWRITEBYTECODE=1
exec python3 "$bundle/tools/campaign_worker.py" --bundle "$bundle" --manifest-sha256 "$manifest_sha" \
    --campaign-id "$campaign_id" --submission-id "$submission_id" --workflow "$workflow" \
    --dataset-id "$dataset_id" --chunk-id "$chunk_id" --attempt-id "$attempt_id" \
    --execution-id "$execution_id" --cluster-id "$cluster_id" --proc-id "$proc_id" --work-root "$worker_root/campaign_executions"
