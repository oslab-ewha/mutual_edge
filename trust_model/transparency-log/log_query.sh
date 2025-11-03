#!/bin/bash

echo "[*] Fetching most recent Rekor entries..."
rekor-cli search --rekor_server http://localhost:3000
