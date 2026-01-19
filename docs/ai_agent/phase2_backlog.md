# AI Agent Integration - Phase 2 Backlog (Tools + Safe Modify Actions)

This document captures post-MVP work for extending the qrenderdoc AI integration tool surface.
The intent is to keep Phase 1 formats stable while adding new capabilities behind an explicit
allow-list and (for any modify action) explicit user confirmation.

## Goals

- Add high-value **read-only** tools for analysis (pipeline, shaders, statistics).
- Define a safe pattern for **modify** actions (replay-only, confirmation gated).
- Keep Phase 1 protocols stable:
  - `stream-json` event envelope
  - JSON-RPC 2.0 bridge (`http://127.0.0.1:{port}/rpc`, bearer token)

## Non-goals (for Phase 2)

- No silent capture modification on disk.
- No remote tool bridge (localhost-only remains the default).
- No unrestricted file/tool execution from the Agent Host.

## Proposed tools (read-only)

Each tool is a JSON-RPC 2.0 method on the in-app bridge. The Agent Host must allow-list each tool
name before it can be called.

### Tool: renderdoc.get_capture_summary

- Method: `"renderdoc.get_capture_summary"`
- Params: `{}`
- Result (MVP+):
  - `capture_path` (string|null)
  - `api` (string|number|null)
  - `drawcall_count` (number)
  - `event_id` (number)
  - `event_name` (string|null)

Acceptance:
- Works with and without an open capture (null/empty fields where applicable).
- Runs fast enough for interactive use (no UI stalls).

### Tool: renderdoc.get_pipeline_summary

- Method: `"renderdoc.get_pipeline_summary"`
- Params:
  - `event_id` (number|null) - default to current event when null
- Result (example shape):
  - `event_id` (number)
  - `stages` (array) - per-stage shader name/id and key resource counts
  - `resources` (array) - high-level bound resource summary

Acceptance:
- Stable field names suitable for long-term prompting and tests.
- Clearly reports "not available" when the API has no pipeline state for the current context.

### Tool: renderdoc.get_shader_reflection

- Method: `"renderdoc.get_shader_reflection"`
- Params:
  - `shader_id` (string) - stable identifier (hash or resource id string)
- Result (example shape):
  - `entry_point` (string|null)
  - `stage` (string)
  - `inputs` / `outputs` (array)
  - `resources` (array)
  - `uniforms` (array)

Acceptance:
- Enables analysis prompts like "count uniforms and branch hints" without needing raw disassembly.
- Returns deterministic identifiers for resources/bindings to support follow-up tool calls.

### Tool: renderdoc.get_event_statistics

- Method: `"renderdoc.get_event_statistics"`
- Params:
  - `scope` (string) - `"current_event" | "frame" | "range"`
  - `start_event_id` (number|null)
  - `end_event_id` (number|null)
- Result (example shape):
  - `counters` (object) - named counters and values

Acceptance:
- Can be used to answer prompts like "top cost events" or "drawcall distribution".

## Proposed tools (modify, confirmation gated)

Modify actions must be opt-in and confirmation gated. The default policy is deny.

### Confirmation model (recommended)

Use a two-step protocol:

1. `renderdoc.modify.preview_*` returns a **proposed change summary** and an `action_id`.
2. UI prompts the user and returns a `confirm_token`.
3. `renderdoc.modify.apply_*` executes the change only if `confirm_token` is valid for `action_id`.

Minimum requirements for any modify tool:

- Must be replay-only (session scoped), not writing to the capture on disk.
- Must emit a clear audit log entry (what changed, when, user-confirmed).
- Must be explicitly allow-listed on both sides:
  - Agent Host allow-list (tool name)
  - qrenderdoc bridge policy (JSON-RPC method)

### Candidate modify actions (examples)

- `renderdoc.modify.preview_shader_replacement` / `renderdoc.modify.apply_shader_replacement`
  - Use case: experiment with shader edits to reduce branches or uniforms.
- `renderdoc.modify.preview_pipeline_override` / `renderdoc.modify.apply_pipeline_override`
  - Use case: toggle states for diagnosis (depth test, blending) in a safe replay-only way.

Acceptance (common):
- No action executes without explicit user confirmation in the UI.
- Cancel/retry does not leave the replay in a broken state.
- Clear error messages when an action is denied by policy.

## Implementation checklist for adding a new tool

1. Define JSON-RPC schema (params/result) and add test fixtures.
2. Implement method handler in the in-app bridge (qrenderdoc).
3. Add allow-list entry in the Agent Host (`canUseTool` policy).
4. Add/extend unit tests (schema marshalling, error mapping, deny-by-default).
5. Extend the manual E2E checklist (docs/ai_agent/protocol.md).

