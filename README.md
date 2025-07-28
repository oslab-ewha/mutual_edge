# Mutual Edge for IIoT(Industrial IoT)

Mutual Edge is a cooperative edge-computing model for Industrial IoT (IIoT) environments. Instead of depending on overprovisioned private servers or costly public edge rentals, enterprises temporarily share idle computing capacity by leveraging differences in peak-load periods across industries. Under this model, each enterprise executes part of its workload locally and offloads additional tasks to a partner’s edge node when necessary.
To ensure practical deployability, Mutual Edge incorporates a security model based on Trusted Execution Environments (TEEs). When external tasks run on a local private edge, standard OS-level mechanisms—such as access control, memory isolation, and namespace separation—prevent them from interfering with local workloads. When tasks are offloaded to another enterprise’s edge node, hardware-assisted security becomes essential, since a remote administrator controls the host OS. Technologies such as Intel SGX, AMD SEV, ARM TrustZone, and emerging TDX/CCA architectures enable enclave-based execution that keeps user data confidential even from privileged system software.
A prototype implementation built with Intel SGX–style attestation workflows demonstrates how secure offloading can be achieved in practice. Tasks run inside enclave-protected execution environments, and remote attestation is used to verify enclave integrity before exchanging encrypted data. End-to-end encryption and replay-attack protections further ensure that sensitive information remains confidential throughout the offloading lifecycle.
This repository provides the reference implementation, cost-model formulation, and scheduling framework that together enable a more flexible, sustainable, and secure IIoT edge-computing environment.


## Build
To build `gastask` and `gasgen`, use CMake:
```
$ mkdir -p build && cd build
$ cmake ..
$ make
```

## Run
- Create a new configuration file. Refer to `gastask.conf.tmpl`.
- run `gasgen`
```
# ./gasgen gastask.conf
```
- Tasks list will be generated into <code>task_generated.txt</code> <code>network_generated.txt</code> <code>network_commander_generated.txt</code>according to gastask.conf
- paste <code>task_generated.txt</code> into the task section of gastask.conf 
- paste <code>network_generated.txt</code> into the network section of gastask.conf
- paste <code>network_commander_generated.txt</code> into the net_commander_ section of gastask.conf
- run gastask
```
# ./gastask gastask.conf
```
- scheduling information is generated in <code>task.txt</code>, which can be used as an input to simrts.

## Experimental Configuration
This section describes the configuration parameters used for evaluating different offloading strategies in the Mutual Edge scheduling framework.
- Experimental Configuration (Mutual / Hybrid / Private / Public Strategies)
  
### [1] Mutual / Hybrid  - Local

**Workload range:** `0.05 → 1.0` (step = `0.05`)

| File | Line | Parameter | Change |
|------|------|------------|--------|
| `run_extend.sh` | 52 | `*mem max_capacity` | `45000000` |
| `run_extend.sh` | 60 | `*offloadingratio` | `0` |
| `gen_task.c` | 17 | `offloading_bool[i]` | `0` |
| `task.c` | 34 | `double real_offloading` | `0.0` |
| `report.c` | 104 | `// gene->util = gene->util / 2.0;` | (keep commented out) |
| `GA.c` | 271 | `double total_memory` | `45000.0` |
| `GA.c` | 323–324 | `total_cost += base_cost;`<br>`gene->cost_base = base_cost;` |  |
| `GA.c` | 327 | `private_depreciation_cost` | `(HW_COST / DEP_PERIOD_SEC) * ALL_Period` |

---

### [2] Private

**Workload range:** `0.05 → 1.5` (step = `0.05`)

| File | Line | Parameter | Change |
|------|------|------------|--------|
| `run_extend.sh` | 52 | `*mem max_capacity` | `90000000` |
| `run_extend.sh` | 60 | `*offloadingratio` | `0` |
| `gen_task.c` | 17 | `offloading_bool[i]` | `0` |
| `task.c` | 34 | `double real_offloading` | `0.0` |
| `report.c` | 104 | `gene->util = gene->util / 2.0;` | (enable division) |
| `GA.c` | 271 | `double total_memory` | `90000.0` |
| `GA.c` | 323–324 | `total_cost += 2 * base_cost;`<br>`gene->cost_base = 2 * base_cost;` |  |
| `GA.c` | 327 | `private_depreciation_cost` | `((2 * HW_COST) / DEP_PERIOD_SEC) * ALL_Period` |

---

### [3] Public

**Workload range:** `0.05 → 1.5` (step = `0.05`)

| File | Line | Parameter | Change |
|------|------|------------|--------|
| `run_extend.sh` | 52 | `*mem max_capacity` | `45000000` |
| `run_extend.sh` | 60 | `*offloadingratio` | `1` |
| `gen_task.c` | 17 | `offloading_bool[i]` | `1` |
| `task.c` | 34 | `double real_offloading` | `1.0` |
| `report.c` | 104 | `// gene->util = gene->util / 2.0;` | (keep commented out) |
| `GA.c` | 271 | `double total_memory` |  `45000.0` |
| `GA.c` | 311–329 |  | Comment out entire block (`/* ... */`) |

---

Using these strategy configurations, the system reproduces the rental and utilization scenarios presented in the paper by tuning resource and workload parameters.

## IIoT Application Offloading with Intel SGX and Remote Attestation

### Secure Offloading Framework for Private Industrial Edge Environments

This repo demonstrates how Industrial IoT (IIoT) application tasks can be securely offloaded to
a Trusted Execution Environment (TEE) on a privately owned edge server using Intel SGX.
The system provides a complete workflow for enclave creation, remote attestation, secure channel establishment, and protected execution of offloaded tasks.

### System Overview

- **IIoT Device**: A constrained device that generates computation tasks. Tasks can execute locally or be offloaded to a secure enclave running on a remote edge server.
- **Gateway (Local Edge Node)**: Receives IIoT tasks and forwards them to a remote SGX enclave. Performs attestation, key exchange, and secure channel setup.
- **Intel SGX Enclave (Remote Edge Server)**: Executes sensitive or cooperative offloaded tasks inside an SGX enclave. The enclave protects memory regions (EPC) from access by the host OS or privileged administrators.

Modify or extend the logic depending on your offloading needs.

### Build System Components
```aiignore
$ mkdir build
$ cd build
$ cmake ..
$ make
```

This produces the following binaries:
- iiot_device – IIoT task generator
- gateway – Performs attestation + secure forwarding
- enclave.signed – SGX enclave with embedded measurement & signature

**Note**: enclave.signed is produced after enclave signing (Step 3).


### Build & Sign the SGX Enclave
#### Build the enclave shared object

```aiignore
$ make enclave
```

#### Sign the enclave (required for attestation)
```aiignore
$ sgx_sign sign -key enclave_private.pem \
                 -enclave enclave.so \
                 -out enclave.signed \
                 -config enclave.config.xml
```

The signing step produces:
* MRENCLAVE: measurement of the enclave code
* MRSIGNER: fingerprint of signing key

These values are used by the gateway to validate remote attestation.


### Launch the Gateway and SGX Enclave

#### Terminal 1 — Launch Gateway
```aiignore
$ ./gateway
```

#### Terminal 2 — Launch SGX Enclave Host Loader
```aiignore
$ ./enclave_host ./enclave.signed
```


The host loader creates the enclave and exposes its RA (Remote Attestation) interface.

During startup:

- The enclave creates a quote using Intel DCAP/EPID (depending on platform).
- The gateway requests and verifies the quote (via Intel Attestation Service or local DCAP verifier).
- After verification, a secure symmetric key is established.

When attestation completes, the gateway will print:

```aiignore
[OK] Remote enclave verified.
[OK] Secure channel established.
```

### Run the IIoT Emulator
```aiignore
$ ./iiot_device -g <gateway_ip>
```
If successful:

- Tasks are encrypted at the gateway
- Decrypted only inside SGX enclave memory
- Results are re-encrypted and sent back securely

Terminal output will confirm:

```aiignore
Task offloaded securely.
Enclave execution successful.
Returned result: <value>
```
