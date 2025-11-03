#!/bin/bash

LOG_FILE="../telemetry/output/usage.log"

if [ ! -f "$LOG_FILE" ]; then
    echo "usage.log not found. Run OpenTelemetry example first."
    exit 1
fi

echo "[*] Hashing usage.log..."
HASH=$(sha256sum $LOG_FILE | awk '{print $1}')
echo "Log hash: $HASH"

echo "[*] Submitting hash to Rekor..."
rekor-cli upload \
  --artifact $LOG_FILE \
  --rekor_server http://localhost:3000

echo "[*] Done. Entry available in Rekor transparency log."
