import { createSdkMcpServer, query, tool, type Message } from "@tencent-ai/agent-sdk";

import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { spawnSync } from "node:child_process";

import { parseArgs } from "./cliArgs";
import { JsonRpcCallError, jsonRpcCall } from "./jsonRpc";
import { readAllStdin } from "./readStdin";
import { writeEvent } from "./streamJson";

type AnyRecord = Record<string, unknown>;

type RenderDocGetContextResult = {
  capture_path: string | null;
  api: string | number | null;
  event_id: number;
  event_name: string | null;
};

function stableNowMs(): number {
  return Date.now();
}

function normalizePrompt(input: string): string {
  return input.trim();
}

function authHelpText(): string {
  return [
    "Authentication required.",
    "Fix:",
    "- Run CodeBuddy Code in a terminal, then execute /login to sign in.",
    "- Or set CODEBUDDY_API_KEY for non-interactive usage.",
  ].join("\n");
}

function isAuthErrorText(text: string): boolean {
  const t = text.toLowerCase();
  return t.includes("authentication required") || t.includes("unauthorized") || t.includes("/login");
}

function fileExistsNonEmpty(p: string): boolean {
  try {
    const st = fs.statSync(p);
    return st.isFile() && st.size > 0;
  } catch {
    return false;
  }
}

function isLikelyLoggedIn(): boolean {
  if (process.env.CODEBUDDY_API_KEY) return true;
  if (process.env.CODEBUDDY_AUTH_TOKEN) return true;

  const home = os.homedir();
  return fileExistsNonEmpty(path.join(home, ".codebuddy-cli", "oauth_creds.json"));
}

function resolveBundledCodebuddyPath(): string | null {
  if (process.env.CODEBUDDY_CODE_PATH && fs.existsSync(process.env.CODEBUDDY_CODE_PATH)) {
    return process.env.CODEBUDDY_CODE_PATH;
  }

  const filename = process.platform === "win32" ? "codebuddy.cmd" : "codebuddy";
  const candidate = path.join(__dirname, "..", "bin", filename);
  if (fs.existsSync(candidate)) return candidate;

  return null;
}

function detectAuthRequiredViaCli(codebuddyPath: string, model: string | undefined, timeoutMs: number): boolean {
  const cliArgs: string[] = ["--print", "--output-format", "json", "--setting-sources", "user"];
  if (model) cliArgs.push("--model", model);
  cliArgs.push("ping");

  const r = spawnSync(codebuddyPath, cliArgs, {
    encoding: "utf8",
    windowsHide: true,
    timeout: timeoutMs,
    maxBuffer: 1024 * 1024,
  });

  const out = `${r.stdout ?? ""}\n${r.stderr ?? ""}`.trim();
  if (!out) return false;
  return isAuthErrorText(out);
}

async function run(): Promise<number> {
  const start = stableNowMs();
  const args = parseArgs(process.argv.slice(2));

  const prompt = normalizePrompt(await readAllStdin());
  if (!prompt) {
    writeEvent({ type: "result", status: "error" });
    return 2;
  }

  const pid = typeof process.pid === "number" ? process.pid : undefined;
  writeEvent({ type: "init", pid, model: args.model });
  writeEvent({ type: "message", role: "system", content: "Starting agent..." });

  const bundledCodebuddy = resolveBundledCodebuddyPath();
  if (!bundledCodebuddy) {
    writeEvent({
      type: "error",
      code: "codebuddy_not_found",
      message: "CodeBuddy CLI not found for Agent SDK. Set CODEBUDDY_CODE_PATH or reinstall dependencies.",
    });
    writeEvent({ type: "result", status: "error", elapsed_ms: stableNowMs() - start });
    return 1;
  }

  // Best-effort auth precheck. If credentials are missing, exit early with clear guidance.
  if (!isLikelyLoggedIn() && !process.env.CODEBUDDY_API_KEY && !process.env.CODEBUDDY_AUTH_TOKEN) {
    writeEvent({ type: "error", code: "auth_required", message: authHelpText() });
    writeEvent({ type: "result", status: "error", elapsed_ms: stableNowMs() - start });
    return 3;
  }

  const toolNameById = new Map<string, string>();

  const renderdocServer = createSdkMcpServer({
    name: "renderdoc",
    version: "0.1.0",
    tools: [
      tool(
        "renderdoc.get_context",
        "Get current qrenderdoc capture context (MVP).",
        {},
        async () => {
          const rpcUrl = process.env.RENDERDOC_AI_BRIDGE_URL;
          const token = process.env.RENDERDOC_AI_BRIDGE_TOKEN;

          if (!rpcUrl || !token) {
            throw new Error(
              "Tool bridge not configured. Set RENDERDOC_AI_BRIDGE_URL and RENDERDOC_AI_BRIDGE_TOKEN.",
            );
          }

          try {
            const { requestId, result } = await jsonRpcCall<RenderDocGetContextResult>({
              rpcUrl,
              bearerToken: token,
              method: "renderdoc.get_context",
              timeoutMs: 2000,
            });

            return {
              content: [
                {
                  type: "text",
                  text: JSON.stringify({ request_id: requestId, result }),
                },
              ],
            };
          } catch (e) {
            if (e instanceof JsonRpcCallError) {
              throw new Error(`Tool bridge RPC failed (request_id=${e.requestId}): ${e.message}`);
            }
            throw e;
          }
        },
      ),
    ],
  });

  const allowTool = async (
    toolName: string,
    input: Record<string, unknown>,
  ): Promise<{ behavior: "allow"; updatedInput: Record<string, unknown> } | { behavior: "deny"; message: string }> => {
    if (toolName === "renderdoc.get_context")
      return { behavior: "allow", updatedInput: input };
    return { behavior: "deny", message: "Tool not allowed by host policy." };
  };

  const abortController = new AbortController();

  let canceled = false;
  let cancelReason: "signal" | "timeout" | null = null;

  const timeout = setTimeout(() => {
    canceled = true;
    cancelReason = "timeout";
    abortController.abort();
  }, args.timeoutMs);

  try {
    const q = query({
      prompt,
      options: {
        abortController,
        model: args.model,
        permissionMode: "dontAsk",
        // Reuse existing CLI credentials/settings from user profile by default.
        settingSources: ["user"],
        env: {
          CODEBUDDY_CODE_PATH: bundledCodebuddy,
        },
        canUseTool: async (toolName, input) => allowTool(toolName, input),
        mcpServers: {
          renderdoc: renderdocServer,
        },
      },
    });

    const cancel = async () => {
      if (canceled) return;
      canceled = true;
      cancelReason = "signal";
      abortController.abort();
      try {
        await q.interrupt();
      } catch {
        // ignore
      }
    };
    process.on("SIGINT", () => void cancel());
    process.on("SIGTERM", () => void cancel());

    for await (const msg of q as AsyncIterable<Message>) {
      if (canceled) break;

      if (msg.type === "system") {
        // Best-effort: surface session metadata as a system message (init was already emitted).
        if (msg.subtype === "init") {
          writeEvent({
            type: "message",
            role: "system",
            content: `Session: ${msg.session_id}  Model: ${msg.model}`,
          });
        }
        continue;
      }

      if (msg.type === "assistant") {
        for (const block of msg.message.content) {
          if (block.type === "text") {
            const text = block.text.trimEnd();
            if (text) writeEvent({ type: "message", role: "assistant", content: text });
            continue;
          }

          if (block.type === "tool_use") {
            toolNameById.set(block.id, block.name);
            writeEvent({
              type: "tool_call",
              call_id: block.id,
              name: block.name,
              arguments: block.input,
            });
            continue;
          }

          if (block.type === "tool_result") {
            const name = toolNameById.get(block.tool_use_id) ?? "unknown";
            if (block.is_error) {
              writeEvent({
                type: "tool_result",
                call_id: block.tool_use_id,
                name,
                ok: false,
                error: {
                  code: "tool_error",
                  message: typeof block.content === "string" ? block.content : "tool error",
                },
              });
            } else {
              writeEvent({
                type: "tool_result",
                call_id: block.tool_use_id,
                name,
                ok: true,
                result: block.content ?? "",
              });
            }
            continue;
          }
        }

        continue;
      }

      if (msg.type === "user") {
        const content = msg.message.content;
        if (typeof content === "string") {
          writeEvent({ type: "message", role: "user", content });
        } else {
          writeEvent({ type: "message", role: "user", content: JSON.stringify(content) });
        }
        continue;
      }

      if (msg.type === "error") {
        writeEvent({ type: "error", message: msg.error });
        continue;
      }

      if (msg.type === "result") {
        // The SDK result indicates the session finished.
        const status = msg.is_error ? "error" : "ok";
        writeEvent({ type: "result", status, elapsed_ms: stableNowMs() - start });
        clearTimeout(timeout);
        return msg.is_error ? 1 : 0;
      }

      // Keep deterministic output even for unhandled message types.
      writeEvent({ type: "error", message: "unhandled sdk message", details: msg as unknown as AnyRecord });
    }

    clearTimeout(timeout);

    if (canceled) {
      writeEvent({ type: "result", status: "canceled", elapsed_ms: stableNowMs() - start });
      return cancelReason === "timeout" ? 124 : 130;
    }

    writeEvent({ type: "result", status: "ok", elapsed_ms: stableNowMs() - start });
    return 0;
  } catch (e) {
    clearTimeout(timeout);
    const errText = String(e);
    if (isAuthErrorText(errText) || (errText.includes("Transport closed") && detectAuthRequiredViaCli(bundledCodebuddy, args.model, 15000))) {
      writeEvent({ type: "error", code: "auth_required", message: authHelpText() });
      writeEvent({ type: "result", status: "error", elapsed_ms: stableNowMs() - start });
      return 3;
    }

    writeEvent({ type: "error", message: "agent host failed", details: errText });
    writeEvent({ type: "result", status: "error", elapsed_ms: stableNowMs() - start });
    return 1;
  }
}

run().then(
  (code) => process.exit(code),
  () => process.exit(1),
);
