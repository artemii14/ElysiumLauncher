#!/usr/bin/env bash
# Keep the real exit status and expose a bounded error tail via the Checks API.
# Some environments can access api.github.com but not Actions' log storage.
set -uo pipefail
log=$(mktemp)
trap 'rm -f "$log"' EXIT
"$@" 2>&1 | tee "$log"
status=${PIPESTATUS[0]}
if (( status != 0 )); then
    details=$(tail -c 14000 "$log")
    details=${details//'%'/'%25'}
    details=${details//$'\r'/'%0D'}
    details=${details//$'\n'/'%0A'}
    printf '::error title=CI command failed (exit %s)::%s\n' "$status" "$details"
fi
exit "$status"
