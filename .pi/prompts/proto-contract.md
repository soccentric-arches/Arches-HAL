---
description: Validate Arches HAL's canonical protobuf and gRPC contract
---

Run the HAL contract validation gate:

```bash
./.claude/validate.sh quick
```

Report the result using the validation contract: exit 0 is a pass, 1-76 is a failure, and 77 is blocked because a required check could not run.

When the contract changed, confirm that `protos/registry/modules.yaml` matches the affected `protos/modules/*.proto` file. HAL owns generated proto symbols; do not regenerate or duplicate them in Arches-Client or Arches-Server.
