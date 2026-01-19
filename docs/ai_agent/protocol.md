# AI Agent Integration Protocol (qrenderdoc)

This document specifies the on-wire formats used by the qrenderdoc AI integration:

- **stream-json**: newline-delimited JSON events exchanged between qrenderdoc (UI) and the Agent Host
- **JSON-RPC 2.0 tool bridge**: localhost-only tool calls from the Agent Host into qrenderdoc

The goal is to keep these formats stable and versionable so that the UI, Agent Host, and bridge can
evolve independently.

## stream-json (newline-delimited JSON)

Each line is a single JSON object. The UI reads stdout as UTF-8, splits by `\n`, and parses each
line independently.

### Common fields

- `type` (string, required): event discriminator.

### Event: init

Emitted once at the start of a session.

- `type`: `"init"`
- `session_id` (string, optional)
- `model` (string, optional)
- `pid` (number, optional)

### Event: message

Represents conversational output.

- `type`: `"message"`
- `role` (string, required): `"system" | "user" | "assistant"`
- `content` (string, required)

### Event: tool_call

Indicates the agent is requesting a tool execution.

- `type`: `"tool_call"`
- `call_id` (string, required): unique identifier for correlating with `tool_result`
- `name` (string, required): tool name, e.g. `"renderdoc.get_context"`
- `arguments` (object, optional): JSON arguments for the tool (default `{}`)

### Event: tool_result

Returns a tool execution result.

- `type`: `"tool_result"`
- `call_id` (string, required): correlates with `tool_call.call_id`
- `name` (string, required)
- `ok` (boolean, required)
- `result` (any, optional): present when `ok=true`
- `error` (object, optional): present when `ok=false`
  - `code` (string, optional)
  - `message` (string, optional)
  - `details` (any, optional)

### Event: error

Non-tool fatal or recoverable error.

- `type`: `"error"`
- `message` (string, required)
- `code` (string, optional)
- `details` (any, optional)

### Event: result

Emitted once at the end of a session.

- `type`: `"result"`
- `status` (string, required): `"ok" | "error" | "canceled"`
- `elapsed_ms` (number, optional)

## JSON-RPC 2.0 Tool Bridge (localhost-only, HTTP)

The Agent Host calls qrenderdoc via JSON-RPC 2.0 over HTTP.

### Endpoint

- `POST http://127.0.0.1:{port}/rpc`

### Authentication

The bridge must reject requests without a valid bearer token.

- Header: `Authorization: Bearer <token>`

### Request (JSON-RPC 2.0)

Required fields:

- `jsonrpc`: `"2.0"`
- `id`: string or number
- `method`: string
- `params`: object or array (optional)

### Response (JSON-RPC 2.0)

Required fields:

- `jsonrpc`: `"2.0"`
- `id`: string or number
- either `result` or `error`

`error` is an object:

- `code` (number)
- `message` (string)
- `data` (any, optional)

### Tool: renderdoc.get_context (MVP)

JSON-RPC method: `"renderdoc.get_context"`

Params: `{}` (no params)

Result object:

- `capture_path` (string or null)
- `api` (string or number)
- `event_id` (number)
- `event_name` (string or null)

