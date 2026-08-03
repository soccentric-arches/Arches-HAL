---
description: Configure and build HAL (native). The hub — Client and Server link this tree.
---

HAL owns the canonical protos and generated gRPC code; Client and Server link this working tree via `add_subdirectory`. A build break here breaks both downstream repos, and a proto change here is a cross-repo breaking change that needs corresponding Client/Server updates (see `/proto-contract`).

Run:

```bash
make build
```

This configures (if needed) and builds the native tree with Ninja.
