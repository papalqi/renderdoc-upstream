export type StreamJsonEvent =
  | StreamJsonInitEvent
  | StreamJsonMessageEvent
  | StreamJsonToolCallEvent
  | StreamJsonToolResultEvent
  | StreamJsonErrorEvent
  | StreamJsonResultEvent;

export type StreamJsonInitEvent = {
  type: "init";
  session_id?: string;
  model?: string;
  pid?: number;
};

export type StreamJsonMessageEvent = {
  type: "message";
  role: "system" | "user" | "assistant";
  content: string;
};

export type StreamJsonToolCallEvent = {
  type: "tool_call";
  call_id: string;
  name: string;
  arguments?: Record<string, unknown>;
};

export type StreamJsonToolResultEvent = {
  type: "tool_result";
  call_id: string;
  name: string;
  ok: boolean;
  result?: unknown;
  error?: {
    code?: string;
    message?: string;
    details?: unknown;
  };
};

export type StreamJsonErrorEvent = {
  type: "error";
  message: string;
  code?: string;
  details?: unknown;
};

export type StreamJsonResultEvent = {
  type: "result";
  status: "ok" | "error" | "canceled";
  elapsed_ms?: number;
};

export function writeEvent(event: StreamJsonEvent): void {
  process.stdout.write(JSON.stringify(event) + "\n");
}

