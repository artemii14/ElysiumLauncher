#!/usr/bin/env bash
# Keep the real exit status and expose a bounded error tail via the Checks API.
# Some environments can access api.github.com but not Actions' log storage.
set -uo pipefail
log=$(mktemp)
trap 'rm -f "$log"' EXIT
"$@" 2>&1 | tee "$log"
status=${PIPESTATUS[0]}
if (( status != 0 )); then
    # GitHub truncates annotation messages at 4096 characters. Prioritize
    # actual diagnostics, and leave room for workflow-command escaping.
    errors=$(grep -iE 'error:|fatal error|CMake Error|undefined reference' "$log" || true)
    if [[ -n "$errors" ]]; then
        details=$(printf '%s' "$errors" | tail -c 3000)
    else
        details=$(tail -c 3000 "$log")
    fi
    details=${details//'%'/'%25'}
    details=${details//$'\r'/'%0D'}
    details=${details//$'\n'/'%0A'}
    printf '::error title=CI command failed (exit %s)::%s\n' "$status" "$details"
fi
exit "$status"
