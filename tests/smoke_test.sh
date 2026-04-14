#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="$ROOT_DIR/src"
TRACE_FILE="${1:-$SRC_DIR/traces/sample.trace}"

cd "$SRC_DIR"

echo "[smoke] Building project..."
make

echo "[smoke] Build succeeded."
if [[ -f "$TRACE_FILE" ]]; then
  echo "[smoke] Running simulator with trace: $TRACE_FILE"
  ./proj "$TRACE_FILE" 0 100 1
  echo "[smoke] Run succeeded."
else
  echo "[smoke] No trace found at $TRACE_FILE"
  echo "[smoke] Build-only smoke test passed."
fi
