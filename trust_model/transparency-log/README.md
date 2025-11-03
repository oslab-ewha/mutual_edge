# Sigstore Rekor Transparency Log (Mutual Edge Demo)

This directory contains a minimal setup for Sigstore Rekor, used as a
tamper-evident transparency log for auditing resource usage reports in the
Mutual Edge trust model.

## Components

- **Rekor Server**  
  Runs using a lightweight docker-compose configuration.

- **log_submit.sh**  
  Submits a usage log (generated via OpenTelemetry) to the Rekor transparency
  log. The log is hashed and stored immutably.

- **log_query.sh**  
  Queries entries from the Rekor log for verification.

## Usage

### 1. Start Rekor:
```bash
docker-compose up -d
```

### 2. Submit a usage log:
```bash
./log_submit.sh
```

### 3. Query the log:
```bash
./log_query.sh
```

This setup forms a complete PoC for tamper-evident resource usage auditing in Mutual Edge.
