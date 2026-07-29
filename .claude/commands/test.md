---
description: Run HAL's full native CTest suite (~36 executables — correct in HAL's own tree)
allowed-tools: Bash(make test)
---

!`make test`

The full unfiltered set is correct HERE — the label filters exist for Client and
Server, which link HAL and would otherwise trip over its unbuilt
EXCLUDE_FROM_ALL targets. Report failures by test name with the output lines
that justify the verdict. For the ARM64/QEMU cross run, use /cross-test instead.
