---
name: proto-contracts
description: "Change Arches HAL protobuf files, gRPC services, module registry metadata, generated proto symbols, or Client/Server integration contracts."
---

# Maintain HAL Proto Contracts

This repository is the single source of truth for the generated protobuf and
gRPC symbols consumed by Arches-Client and Arches-Server. Consumers link HAL as
a sibling CMake subproject; they must never regenerate these contracts.

## Contract Rules

- Preserve package `arches.stream` in `protos/client_service.proto` unless a
  coordinated breaking change across Client and Server is explicitly intended.
- For a module contract, keep `protos/modules/<id>.proto` and the corresponding
  `protos/registry/modules.yaml` entry synchronized.
- Module ids are wire identifiers: make them unique, permanent, and never
  reuse a retired id.
- Increment the registry version for a contract change. Keep the configured
  fully-qualified package, Config type, Result type, and source path accurate.
- New top-level .proto files must be explicitly named in
  `protos/CMakeLists.txt`; module proto files are globbed.
- Do not hand-edit generated protobuf sources or add regenerated copies in a
  sibling Arches repository.

## Validation

Run the contract-aware gate first:

```bash
./.claude/validate.sh quick
```

Run `./.claude/validate.sh full` when generated targets, service signatures, or
shared CMake configuration changed. Treat exit 77 as blocked, not successful.
