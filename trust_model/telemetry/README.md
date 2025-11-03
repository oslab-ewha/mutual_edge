# OpenTelemetry Resource Usage Logger (Mutual Edge Demo)

This directory provides a minimal OpenTelemetry setup for logging
resource-segment (RS) usage events as described in our Mutual Edge trust model.

## Components

- **OTEL Collector** (Docker)  
  Receives OTLP traces and exports them to console and file output.

- **demo_producer.py**  
  Generates synthetic RS usage logs. These logs represent the amount of
  offloaded computation consumed by an enterprise.

## Run

```bash
docker-compose up -d
python demo_producer.py
```
