import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";
import { execSync } from "node:child_process";
import { existsSync } from "node:fs";
import { join, resolve, relative } from "node:path";

// Paths that are human-owned and must never be edited by the agent.
// Modeled after .claude/hooks/pre-tool-use-guard.py and Arches/.claude/settings.json
const GUARDED_PATTERNS = [
  "/.claude/validate.sh",
  "/.claude/lib/",
  "/.claude/settings.json",
  "/.claude/hooks/",
  "/.claude/commands/",
  "/tools/validate/",
];

function isGuarded(filePath: string, cwd: string): boolean {
  const abs = resolve(cwd, filePath);
  const rel = relative(cwd, abs).replace(/\\/g, "/");
  return GUARDED_PATTERNS.some((pattern) => rel.startsWith(pattern.replace(/^\//, "")));
}

function getRepoState(cwd: string): string {
  try {
    const branch = execSync("git rev-parse --abbrev-ref HEAD", {
      cwd, encoding: "utf-8", timeout: 10000,
    }).trim();
    const origin = execSync("git remote get-url origin", {
      cwd, encoding: "utf-8", timeout: 10000,
    }).trim();
    const dirty = execSync("git status --porcelain", {
      cwd, encoding: "utf-8", timeout: 10000,
    }).trim();
    const nDirty = dirty ? dirty.split("\n").filter((l) => l.trim()).length : 0;
    const recent = execSync("git log --oneline -3", {
      cwd, encoding: "utf-8", timeout: 10000,
    }).trim();

    const repo = cwd.split("/").pop() || "unknown";
    const lines = [
      `[${repo}] branch: ${branch} | origin: ${origin} | dirty files: ${nDirty}`,
      "recent commits:",
      ...recent.split("\n").map((l) => `  ${l}`),
    ];
    if (branch.startsWith("feature/")) {
      const slug = branch.split("/", 2)[1];
      lines.push(`NOTE: on a Loop Engine feature branch — commits here must be prefixed [${slug}].`);
    }
    if (branch === "HEAD") {
      lines.push("WARNING: detached HEAD — fix before any branch work.");
    }
    lines.push(
      "gate: ./.claude/validate.sh quick (seconds) | full (build+test). " +
      "Exit 0 pass, 1-76 fail, 77 GATE-BLOCKED (baseline unverified, never a pass)."
    );
    return lines.join("\n");
  } catch {
    return "(unable to determine repo state)";
  }
}

export default function (pi: ExtensionAPI) {
  // ── SessionStart hook equivalent ──────────────────────────────────────
  // Injects repo state as context on the first turn.
  let repoStateInjected = false;

  pi.on("before_agent_start", async (event, ctx) => {
    if (repoStateInjected) return;
    repoStateInjected = true;

    const state = getRepoState(ctx.cwd);
    return {
      systemPrompt: event.systemPrompt + `\n\n## Repo State\n${state}\n`,
    };
  });

  // ── PreToolUse guard equivalent ───────────────────────────────────────
  // Protects human-owned files from casual edits.
  pi.on("tool_call", async (event, ctx) => {
    if (event.toolName !== "edit" && event.toolName !== "write") return;

    // Check all paths in the tool input
    const input = event.input as Record<string, unknown>;
    const pathsToCheck: string[] = [];

    if (typeof input.path === "string") pathsToCheck.push(input.path);
    if (Array.isArray(input.edits)) {
      for (const edit of input.edits) {
        if (typeof (edit as Record<string, unknown>).path === "string") {
          pathsToCheck.push((edit as Record<string, unknown>).path as string);
        }
      }
    }

    for (const p of pathsToCheck) {
      if (isGuarded(p, ctx.cwd)) {
        const ok = await ctx.ui.confirm(
          "Protected file",
          `'${p}' is part of this repo's human-owned validation gate. ` +
          "The agent must never edit these files (a gate the agent can rewrite is not a gate). " +
          "If you are a human deliberately changing the gate, approve this."
        );
        if (!ok) {
          return { block: true, reason: `Blocked: '${p}' is a human-owned gate file.` };
        }
      }
    }
  });

  // ── PostToolUse git diff --check equivalent ───────────────────────────
  // Runs after every Edit or Write to catch whitespace errors.
  pi.on("tool_result", async (event, ctx) => {
    if (event.toolName !== "edit" && event.toolName !== "write") return;

    try {
      const result = execSync("git diff --check", {
        cwd: ctx.cwd,
        encoding: "utf-8",
        timeout: 30000,
      });
      if (result.trim()) {
        return {
          content: [
            ...event.content,
            { type: "text" as const, text: `\n[git diff --check warnings]\n${result}` },
          ],
        };
      }
    } catch {
      // git diff --check exits non-zero when it finds issues
      try {
        const errOutput = execSync("git diff --check 2>&1 || true", {
          cwd: ctx.cwd,
          encoding: "utf-8",
          timeout: 30000,
        });
        if (errOutput.trim()) {
          return {
            content: [
              ...event.content,
              { type: "text" as const, text: `\n[git diff --check issues]\n${errOutput}` },
            ],
          };
        }
      } catch {
        // git not available or not a repo — silently skip
      }
    }
  });

  // ── Stop hook equivalent ──────────────────────────────────────────────
  // Runs the validation gate when the agent settles.
  pi.on("agent_settled", async (_event, ctx) => {
    const validateSh = join(ctx.cwd, ".claude", "validate.sh");
    if (!existsSync(validateSh)) return;

    ctx.ui.setStatus("hal-hooks", "Running validation gate...");

    try {
      const result = execSync(`${validateSh} quick 2>&1`, {
        cwd: ctx.cwd,
        encoding: "utf-8",
        timeout: 180000,
      });
      ctx.ui.notify("Validation gate: PASS", "info");
      console.log(result);
    } catch (e) {
      const output = (e as { stdout?: string; stderr?: string })?.stdout
        || (e as Error).message
        || "unknown error";
      ctx.ui.notify("Validation gate: see output above", "warning");
      console.error(output);
    } finally {
      ctx.ui.setStatus("hal-hooks", undefined);
    }
  });
}
