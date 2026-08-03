# Arches-HAL — pi agent guide

This file is a router. Open the file that owns the topic.

| If the ask is about… | Read |
|---|---|
| Project overview, layout, Do's/Don'ts, module list | [.claude/CLAUDE.md](.claude/CLAUDE.md) |
| Skills (on-demand capability packages) | `.pi/skills/<name>/SKILL.md` — use `/skill:name` to invoke |
| Prompt templates (slash commands) | `.pi/prompts/<name>.md` — type `/name` to expand |
| Hooks extension (repo context, file guards, validation) | `.pi/extensions/hal-hooks.ts` |
| Validation gate | `./.claude/validate.sh quick\|full` |
| Project settings | `.pi/settings.json` |

## Available skills

| Skill | `/skill:name` | What it does |
|---|---|---|
| coding-standards | `/skill:coding-standards` | clang-format, cppcheck, MISRA C++ compliance |
| proto-contracts | `/skill:proto-contracts` | Protobuf/gRPC contract maintenance |
| remote-test | `/skill:remote-test` | Deploying and running tests on real Jetson hardware |
| add-hal-module | `/skill:add-hal-module` | Adding or restructuring a hardware module |
| native-test | `/skill:native-test` | Running CTest in the local build tree |
| validate | `/skill:validate` | Running the validation gate |
| hal-module-manager | `/skill:hal-module-manager` | Full module lifecycle (add, remove, restructure) |
| project-specs | `/skill:project-specs` | Detailed project layout and architecture reference |

## Available prompt templates

| Command | What it does |
|---|---|
| `/build` | Configure and build HAL (native) |
| `/test` | Run the native CTest suite |
| `/cross-test` | Build and test for ARM64 Jetson target via Docker/QEMU |
| `/proto-contract` | Validate protobuf/gRPC contracts |
| `/validate [quick\|full]` | Run the validation gate |

## Validation

```bash
./.claude/validate.sh quick   # seconds — repo-shape + static checks
./.claude/validate.sh full    # minutes — + cmake configure, native build, full CTest set
```

Exit codes: `0` pass, `1`-`76` fail, `77` GATE-BLOCKED (never a pass).
