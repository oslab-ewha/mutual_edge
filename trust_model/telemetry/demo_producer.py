import time
from random import randint

from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
from opentelemetry.sdk.trace.export import BatchSpanProcessor

# Configure tracer
trace.set_tracer_provider(TracerProvider())
tracer = trace.get_tracer("mutual_edge.usage")

exporter = OTLPSpanExporter(endpoint="http://localhost:4318/v1/traces")
processor = BatchSpanProcessor(exporter)
trace.get_tracer_provider().add_span_processor(processor)

def report_usage(rs_id: str, usage_amount: int):
    with tracer.start_as_current_span("resource_usage") as span:
        span.set_attribute("rs.id", rs_id)
        span.set_attribute("usage.amount", usage_amount)

if __name__ == "__main__":
    print("[*] Sending RS usage logs to OTEL collector...")
    for _ in range(5):
        usage = randint(3, 15)
        report_usage("RS-1031", usage)
        time.sleep(1)
    print("[*] Done.")
