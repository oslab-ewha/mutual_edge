#!/bin/bash

EIF_PATH="enclave.eif"

ENCLAVE_INFO=$(nitro-cli run-enclave --eif-path $EIF_PATH --cpu-count 2 --memory 1024 --debug-mode)

ENCLAVE_ID=$(echo "$ENCLAVE_INFO" | grep -oP '"EnclaveID":\s*\K"[a-z0-9-]+')

echo "Enclave started with ID: $ENCLAVE_ID"

nitro-cli console --enclave-id $ENCLAVE_ID
