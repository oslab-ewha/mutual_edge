docker build -t ne -f Dockerfile.ne .
nitro-cli build-enclave --docker-uri ne:latest --output-file enclave.eif
