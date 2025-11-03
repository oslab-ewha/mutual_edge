# Mutual Edge Trust Model — Open-source Prototype (PoC)

## *Hyperledger Fabric + OpenTelemetry + Sigstore Rekor*

This repository provides a minimal, fully reproducible proof-of-concept that demonstrates how the Mutual Edge trust and incentive framework (as described in our paper) can be instantiated using open-source technologies.

The goal is not to build a production-level system, but to show that the proposed trust model—mileage-based accounting, transparent usage logging, and tamper-evident auditing—is practically achievable with existing tools.

## Overview
The prototype implements three core components of the Mutual Edge trust model:

1. **Mileage-based Accounting (Hyperledger Fabric)**

- A permissioned blockchain ledger stores mileage credits associated with enterprises.
- Credits increase when an enterprise supplies idle edge resources and decrease when it borrows resources.
- Implemented using Fabric chaincode (mileage_cc.go).

2. **Usage Logging (OpenTelemetry)**

- Resource usage events (e.g., RS segment compute consumption) are exported to an OTEL Collector.
- Logs are standardized, machine-readable, and stored via console/file exporters.

3. **Tamper-evident Auditing (Sigstore Rekor)**

- Usage logs are hashed and published to a Rekor transparency log.
- This provides immutable, publicly verifiable evidence of reported resource usage.
- Together, these components provide a lightweight trust and incentive alignment layer that supports inter-enterprise Mutual Edge cooperation without requiring a full monetary payment system.

## Running the Prototype Demo

### 1. Start Fabric network
```
$ cd fabric-network/scripts
$ ./generate.sh
$ ./start.sh
$ ./deploy_cc.sh
```

### 2. Start OpenTelemetry Collector
```
$ cd telemetry
$ docker-compose up -d
```

### 3. Generate usage logs
```
$ python demo_producer.py
```
Check output:
```
$ docker logs otel-collector
$ cat telemetry/output/usage.log
```

### 4. Start Rekor transparency log
```
$ cd transparency-log
$ docker-compose up -d
```

### 5. Submit usage logs to Rekor
```
$./log_submit.sh
```

### 6. Query Rekor
```
$ ./log_query.sh
```